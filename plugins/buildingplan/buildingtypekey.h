#pragma once

#include "df/building_type.h"

#include <tuple>
#include <string>

namespace DFHack {
    class color_ostream;
}

// building type, subtype, custom
struct BuildingTypeKey : public std::tuple<df::building_type, int16_t, int32_t> {
    BuildingTypeKey(df::building_type type, int16_t subtype, int32_t custom);
    BuildingTypeKey(DFHack::color_ostream &out, const std::string & serialized);

    std::string serialize() const;
};

template <>
struct fmt::formatter<BuildingTypeKey> : fmt::formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const BuildingTypeKey& key, FormatContext& ctx) const
    {
        return fmt::formatter<std::string_view>::format(
            fmt::format("({}, {}, {})", ENUM_AS_STR(std::get<0>(key)), std::get<1>(key), std::get<2>(key)), ctx);
    }
};

struct BuildingTypeKeyHash {
    std::size_t operator() (const BuildingTypeKey & key) const;
};
