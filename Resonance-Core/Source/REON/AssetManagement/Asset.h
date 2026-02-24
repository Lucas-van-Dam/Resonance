#pragma once

#include "REON/Object.h"

#include <any>
#include <array>
#include <string>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <random>
#include <compare>

#include "nlohmann/json.hpp"

namespace REON
{
struct AssetId
{
    std::array<std::uint8_t, 16> bytes{};

    auto& operator[](size_t i)
    {
        return bytes[i];
    }
    auto operator[](size_t i) const
    {
        return bytes[i];
    }

    auto begin()
    {
        return bytes.begin();
    }
    auto end()
    {
        return bytes.end();
    }
    auto begin() const
    {
        return bytes.begin();
    }
    auto end() const
    {
        return bytes.end();
    }

    operator const std::array<std::uint8_t, 16>&() const
    {
        return bytes;
    }
    operator std::array<std::uint8_t, 16>&()
    {
        return bytes;
    }



    std::string to_string() const{
        static constexpr char hex[] = "0123456789abcdef";

        std::string s;
        s.reserve(36);

        for (size_t i = 0; i < 16; ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                s.push_back('-');

            std::uint8_t b = bytes[i];
            s.push_back(hex[b >> 4]);
            s.push_back(hex[b & 0x0F]);
        }

        return std::move(s);
    }

    static AssetId from_string(const std::string& s) {

        AssetId id{};

        if (s.size() != 36 || s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-')
        {
            REON_ERROR("AssetId: invalid format, expected: 00112233-4455-6677-8899-aabbccddeeff, got: {}", s);
            return id;
        }

        auto hexval = [](char c) -> std::uint8_t
        {
            if ('0' <= c && c <= '9')
                return static_cast<std::uint8_t>(c - '0');
            if ('a' <= c && c <= 'f')
                return static_cast<std::uint8_t>(c - 'a' + 10);
            if ('A' <= c && c <= 'F')
                return static_cast<std::uint8_t>(c - 'A' + 10);
            REON_ERROR("AssetId: invalid hex character '{}'", c);
            return 0;
        };

        size_t si = 0;
        for (size_t i = 0; i < 16; ++i)
        {
            if (si == 8 || si == 13 || si == 18 || si == 23)
                ++si; // skip '-'

            id[i] = static_cast<std::uint8_t>((hexval(s[si]) << 4) | hexval(s[si + 1]));

            si += 2;
        }
        return id;
    }

    bool operator==(const AssetId&) const = default;

    bool operator<(const AssetId& other) const noexcept
    {
        return std::lexicographical_compare(begin(), end(), other.begin(), other.end());
    }
};

constexpr AssetId NullAssetId{};

inline bool IsNull(const AssetId& id) {
	return id == NullAssetId;
}

inline void to_json(nlohmann::json& j, const AssetId& id)
{
    static constexpr char hex[] = "0123456789abcdef";

    std::string s;
    s.reserve(36);

    for (size_t i = 0; i < 16; ++i)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            s.push_back('-');

        std::uint8_t b = id[i];
        s.push_back(hex[b >> 4]);
        s.push_back(hex[b & 0x0F]);
    }

    j = std::move(s);
}

inline void from_json(const nlohmann::json& j, AssetId& id)
{
    const std::string s = j.get<std::string>();

    if (s.size() != 36 || s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-')
    {
        REON_ERROR("AssetId: invalid format, expected: 00112233-4455-6677-8899-aabbccddeeff, got: {}", s);
        id = NullAssetId;
        return;
    }

    auto hexval = [](char c) -> std::uint8_t
    {
        if ('0' <= c && c <= '9')
            return static_cast<std::uint8_t>(c - '0');
        if ('a' <= c && c <= 'f')
            return static_cast<std::uint8_t>(c - 'a' + 10);
        if ('A' <= c && c <= 'F')
            return static_cast<std::uint8_t>(c - 'A' + 10);
        REON_ERROR("AssetId: invalid hex character '{}'", c);
        return 0;
    };

    size_t si = 0;
    for (size_t i = 0; i < 16; ++i)
    {
        if (si == 8 || si == 13 || si == 18 || si == 23)
            ++si; // skip '-'

        id[i] = static_cast<std::uint8_t>((hexval(s[si]) << 4) | hexval(s[si + 1]));

        si += 2;
    }
}


inline AssetId MakeRandomAssetId()
{
    std::random_device rd;
    std::mt19937_64 rng(rd());

    AssetId id{};
    for (size_t i = 0; i < 16; ++i)
    {
        id[i] = static_cast<std::uint8_t>(rng() & 0xFF);
    }

    // If you want it to be a real UUID v4, set version/variant bits:
    id[6] = (id[6] & 0x0F) | 0x40; // version 4
    id[8] = (id[8] & 0x3F) | 0x80; // variant 10xxxxxx

    return id;
}

using AssetTypeId = uint32_t;
enum : AssetTypeId
{
    ASSET_MESH = 1,
    ASSET_TEXTURE = 2,
    ASSET_MATERIAL = 3,
    ASSET_MODEL = 4,
    ASSET_SKELETON = 5,
};

struct AssetKey
{
    AssetTypeId type;
    AssetId id;
    friend bool operator==(const AssetKey& lhs, const AssetKey& rhs)
    {
        return lhs.type == rhs.type && lhs.id == rhs.id;
    }
};

struct AssetKeyHash
{
    size_t operator()(const AssetKey& key) const noexcept {
        // Simple hash combining type and id, probably change later to something more robust if needed
        return std::hash<AssetTypeId>()(key.type) ^ std::hash<std::string>()(std::string(key.id.begin(), key.id.end()));
    }
};
} // namespace REON

namespace std
{
template <> struct hash<REON::AssetId>
{
    size_t operator()(const REON::AssetId& id) const noexcept
    {
        // FNV-1a 64-bit (simple + decent)
        size_t h = 1469598103934665603ull;
        for (uint8_t b : id)
        {
            h ^= static_cast<size_t>(b);
            h *= 1099511628211ull;
        }
        return h;
    }
};
} // namespace std