#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

namespace REON
{

struct IBlobReader
{
    virtual ~IBlobReader() = default;
    virtual bool ReadRange(std::string_view uri, std::uint64_t offset, std::uint64_t size,
                           std::vector<std::byte>& out) = 0;
};

class DiskBlobReader final : public IBlobReader
{
  public:
    explicit DiskBlobReader(std::filesystem::path root);
    bool ReadRange(std::string_view uri, std::uint64_t offset, std::uint64_t size,
                   std::vector<std::byte>& out) override;

  private:
    std::filesystem::path root_;
};
} // namespace REON