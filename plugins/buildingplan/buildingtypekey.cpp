#include "buildingplan.h"
#include "buildingtypekey.h"

#include "Debug.h"
#include "MiscUtils.h"

using std::string;
using std::vector;

namespace DFHack {
    DBG_EXTERN(buildingplan, status);
}

using namespace DFHack;

// building type, subtype, custom
BuildingTypeKey::BuildingTypeKey(df::building_type type, int16_t subtype, int32_t custom)
        : tuple(type, subtype, custom) { }

static BuildingTypeKey deserialize(color_ostream &out, const std::string &serialized) {
    vector<string> key_parts;
    split_string(&key_parts, serialized, ",");
    if (key_parts.size() != 3) {
        WARN(status,out).print("invalid key_str: '%s'\n", serialized.c_str());
        return BuildingTypeKey(df::building_type::NONE, -1, -1);
    }
    return BuildingTypeKey((df::building_type)string_to_int(key_parts[0]),
                        string_to_int(key_parts[1]), string_to_int(key_parts[2]));
}

BuildingTypeKey::BuildingTypeKey(color_ostream &out, const std::string &serialized)
        :tuple(deserialize(out, serialized)) { }

string BuildingTypeKey::serialize() const {
    std::ostringstream ser;
    ser << std::get<0>(*this) << ",";
    ser << std::get<1>(*this) << ",";
    ser << std::get<2>(*this);
    return ser.str();
}

// rotates a size_t value left by count bits
// assumes count is not 0 or >= size_t_bits
// replace this with std::rotl when we move to C++20
static std::size_t rotl_size_t(size_t val, uint32_t count)
{
    static const int size_t_bits = CHAR_BIT * sizeof(std::size_t);
    return val << count | val >> (size_t_bits - count);
}

std::size_t BuildingTypeKeyHash::operator() (const BuildingTypeKey & key) const {
    // cast first param to appease gcc-4.8, which is missing the enum
    // specializations for std::hash
    std::size_t h1 = std::hash<int32_t>()(static_cast<int32_t>(std::get<0>(key)));
    std::size_t h2 = std::hash<int16_t>()(std::get<1>(key));
    std::size_t h3 = std::hash<int32_t>()(std::get<2>(key));

    return h1 ^ rotl_size_t(h2, 8) ^ rotl_size_t(h3, 16);
}
