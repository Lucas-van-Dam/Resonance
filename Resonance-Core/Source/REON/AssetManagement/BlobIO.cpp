#include "reonpch.h"
#include "BlobIO.h"

namespace REON
{
DiskBlobReader::DiskBlobReader(std::filesystem::path root) : root_(std::move(root)) {}

bool DiskBlobReader::ReadRange(const std::string_view uri, std::uint64_t offset, std::uint64_t size,
                               std::vector<std::byte>& out)
{
    const std::filesystem::path path = root_ / uri;
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;

    f.seekg((std::streamoff)offset, std::ios::beg);
    if (!f)
        return false;

    out.resize(size);
    f.read(reinterpret_cast<char*>(out.data()), (std::streamsize)size);
    return (bool)f;
}
} // namespace REON