#include "reonpch.h"

#include "AssetResolver.h"

namespace REON
{
bool ManifestAssetResolver::StartWatchingFile(std::filesystem::path p)
{
    manifestPath_ = std::move(p);

    std::chrono::milliseconds interval = std::chrono::milliseconds(250);

    fileWatcherThread_ = std::jthread(
        [this, interval](std::stop_token token)
        {
            while (!token.stop_requested())
            {
                ReloadManifestIfChanged();
                std::this_thread::sleep_for(interval);
            }
        });

    return true;
}

bool ManifestAssetResolver::Resolve(const AssetKey& k, ArtifactRef& out) const
{
    auto it = std::lower_bound(entries_.begin(), entries_.end(), k,
                               [](const ManifestEntry& e, const AssetKey& key)
                               {
                                   if (e.key.type != key.type)
                                       return e.key.type < key.type;
                                   auto eId = reinterpret_cast<const std::array<std::uint8_t, 16>&>(e.key.id);
                                   return eId < key.id.bytes;
                               });

    if (it == entries_.end())
        return false;
    if (it->key.type != k.type || reinterpret_cast<const std::array<std::uint8_t, 16>&>(it->key.id) != k.id.bytes)
        return false;

    if ((uint64_t)it->ref.uriOffset + (uint64_t)it->ref.uriLength > strings_.size())
        return false;

    out.uri.assign(reinterpret_cast<const char*>(strings_.data() + it->ref.uriOffset), (size_t)it->ref.uriLength);
    out.offset = it->ref.offset;
    out.size = it->ref.size;
    out.format = it->ref.format;
    out.flags = it->ref.flags;
    return true;
}

bool ManifestAssetResolver::ReloadManifestIfChanged()
{
    std::error_code ec;
    auto currentWriteTime = std::filesystem::last_write_time(manifestPath_, ec);
    if (ec)
    {
        return false;
    }

    if (lastWriteTime_ && *lastWriteTime_ == currentWriteTime)
    {
        return false;
    }

    std::vector<uint8_t> bytes;
    {
        std::ifstream f(manifestPath_, std::ios::binary);
        if (!f)
            REON_CORE_ERROR("ManifestAssetResolver: failed to open manifest file: {}", manifestPath_.string());
        f.seekg(0, std::ios::end);
        size_t sz = (size_t)f.tellg();
        f.seekg(0, std::ios::beg);
        bytes.resize(sz);
        f.read((char*)bytes.data(), (std::streamsize)sz);
        if (!f)
            REON_CORE_ERROR("ManifestAssetResolver: failed to read manifest file: {}", manifestPath_.string());
    }

    if (bytes.size() < sizeof(ManifestHeader))
        REON_CORE_ERROR("Manifest too small");
    const auto* hdr = reinterpret_cast<const ManifestHeader*>(bytes.data());
    if (hdr->magic != MANIFEST_MAGIC || hdr->version != MANIFEST_VERSION)
        REON_CORE_ERROR("Manifest bad magic/version");
    if (hdr->entriesOffset + (uint64_t)hdr->entryCount * sizeof(ManifestEntry) > bytes.size())
        REON_CORE_ERROR("Manifest entries out of range");

    const auto* entries = reinterpret_cast<const ManifestEntry*>(bytes.data() + hdr->entriesOffset);
    entries_.assign(entries, entries + hdr->entryCount);

    // Strings are whatever remains after entries
    const uint64_t stringsOffset = hdr->entriesOffset + (uint64_t)hdr->entryCount * sizeof(ManifestEntry);
    if (stringsOffset > bytes.size())
        REON_CORE_ERROR("Manifest strings offset out of range");
    strings_.assign(bytes.begin() + (ptrdiff_t)stringsOffset, bytes.end());

    lastWriteTime_ = currentWriteTime;

    return true;
}

} // namespace REON
