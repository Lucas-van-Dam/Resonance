#pragma once

#include "CookOutput.h"
#include "REON/AssetManagement/Artifact.h"
#include "REON/AssetManagement/Asset.h"
#include "REON/AssetManagement/ManifestFormat.h"
#include "REON/Application.h"
#include <filesystem>

namespace REON_EDITOR
{
class ManifestWriter
{
  public:
    void Upsert(const std::unordered_map<REON::AssetKey, REON::ArtifactRef, REON::AssetKeyHash>& refs)
    {
        for (const auto& [k, incoming] : refs)
        {
            auto it = entries_.find(k);

            REON::ArtifactRef updated = incoming;

            if (it != entries_.end())
            {
                updated.revision = it->second.revision + 1;

                it->second = updated;

                REON::Application::Get().GetEngineServices().resources.NotifyResourceChanged(k, updated);
            }
            else
            {
                updated.revision = 1;
                entries_.emplace(k, updated);
            }
        }
    }

    void Remove(const REON::AssetKey& k)
    {
        entries_.erase(k);
    }

    void Save(const std::filesystem::path& manifestPath)
    {
        std::filesystem::create_directories(manifestPath.parent_path());

        // Sort for deterministic output + easy binary search runtime
        std::vector<std::pair<REON::AssetKey, REON::ArtifactRef>> sorted;
        sorted.reserve(entries_.size());
        for (auto& kv : entries_)
            sorted.push_back(kv);

        std::sort(sorted.begin(), sorted.end(),
                  [](auto& a, auto& b)
                  {
                      if (a.first.type != b.first.type)
                          return a.first.type < b.first.type;
                      return a.first.id < b.first.id;
                  });

        // Build a string table (concatenate unique URIs)
        std::vector<std::string> uniqueUris;
        uniqueUris.reserve(sorted.size());
        {
            std::unordered_map<std::string, uint32_t> seen;
            for (auto& [k, r] : sorted)
            {
                if (seen.find(r.uri) == seen.end())
                {
                    seen.emplace(r.uri, (uint32_t)uniqueUris.size());
                    uniqueUris.push_back(r.uri);
                }
            }
        }

        // Map uri->offset/len in final table
        std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> uriToSpan;
        uriToSpan.reserve(uniqueUris.size());
        uint32_t stringBytes = 0;
        for (auto& s : uniqueUris)
        {
            uriToSpan[s] = {stringBytes, (uint32_t)s.size()};
            stringBytes += (uint32_t)s.size();
        }

        std::vector<REON::ManifestEntry> outEntries;
        outEntries.reserve(sorted.size());

        for (auto& [k, r] : sorted)
        {
            auto span = uriToSpan.at(r.uri);

            REON::ManifestEntry e{};
            e.key.type = k.type;
            std::memcpy(e.key.id, k.id.bytes.data(), 16);
            e.ref.uriOffset = span.first;
            e.ref.uriLength = span.second;
            e.ref.offset = r.offset;
            e.ref.size = r.size;
            e.ref.format = r.format;
            e.ref.flags = r.flags;
            outEntries.push_back(e);
        }

        REON::ManifestHeader hdr{};
        hdr.headerSize = (uint16_t)sizeof(REON::ManifestHeader);
        hdr.entryCount = (uint32_t)outEntries.size();
        hdr.entriesOffset = sizeof(REON::ManifestHeader);

        const uint64_t entriesBytes = (uint64_t)outEntries.size() * sizeof(REON::ManifestEntry);
        const uint64_t stringsOffset = hdr.entriesOffset + entriesBytes;

        // Write: header | entries | string table (raw concatenated bytes)
        std::ofstream out(manifestPath, std::ios::binary | std::ios::trunc);
        if (!out)
            REON_ERROR("ManifestWriter: failed to open " + manifestPath.string());

        out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        out.write(reinterpret_cast<const char*>(outEntries.data()), (std::streamsize)entriesBytes);

        for (auto& s : uniqueUris)
            out.write(s.data(), (std::streamsize)s.size());

        out.flush();
        if (!out)
            REON_ERROR("ManifestWriter: write failed " + manifestPath.string());
    }

  private:
    std::unordered_map<REON::AssetKey, REON::ArtifactRef, REON::AssetKeyHash> entries_;
};
} // namespace REON::EDITOR