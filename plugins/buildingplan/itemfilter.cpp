#include "itemfilter.h"

#include "Debug.h"

#include "df/item.h"

namespace DFHack {
    DBG_EXTERN(buildingplan, control);
    DBG_EXTERN(buildingplan, cycle);
}

using std::set;
using std::string;
using std::vector;

using namespace DFHack;

ItemFilter::ItemFilter() : default_max_quality(df::item_quality::Masterful) {
    clear();
}

void ItemFilter::clear() {
    min_quality = df::item_quality::Ordinary;
    max_quality = default_max_quality;
    decorated_only = false;
    mat_mask.whole = 0;
    materials.clear();
}

bool ItemFilter::isEmpty() const {
    return min_quality == df::item_quality::Ordinary
            && max_quality == default_max_quality
            && !decorated_only
            && !mat_mask.whole
            && materials.empty();
}

static bool deserializeMaterialMask(const string& ser, df::dfhack_material_category& mat_mask) {
    if (ser.empty())
        return true;

    if (!parseJobMaterialCategory(&mat_mask, ser)) {
        DEBUG(control).print("invalid job material category serialization: '%s'", ser.c_str());
        return false;
    }
    return true;
}

static bool deserializeMaterials(const string& ser, set<DFHack::MaterialInfo> &materials) {
    if (ser.empty())
        return true;

    vector<string> mat_names;
    split_string(&mat_names, ser, ",");
    for (auto m = mat_names.begin(); m != mat_names.end(); m++) {
        DFHack::MaterialInfo material;
        if (!material.find(*m) || !material.isValid()) {
            DEBUG(control).print("invalid material name serialization: '%s'", ser.c_str());
            return false;
        }
        materials.emplace(material);
    }
    return true;
}

ItemFilter::ItemFilter(color_ostream &out, const string& serialized) : ItemFilter() {
    vector<string> tokens;
    split_string(&tokens, serialized, "/");
    if (tokens.size() < 5) {
        DEBUG(control,out).print("invalid ItemFilter serialization: '%s'", serialized.c_str());
        return;
    }

    if (!deserializeMaterialMask(tokens[0], mat_mask) || !deserializeMaterials(tokens[1], materials))
        return;

    setMinQuality(atoi(tokens[2].c_str()));
    setMaxQuality(atoi(tokens[3].c_str()));
    decorated_only = static_cast<bool>(atoi(tokens[4].c_str()));

    if (tokens.size() >= 6)
        default_max_quality = static_cast<df::item_quality>(atoi(tokens[5].c_str()));
}

// format: mat,mask,elements/materials,list/minq/maxq/decorated
string ItemFilter::serialize() const {
    std::ostringstream ser;
    ser << bitfield_to_string(mat_mask, ",") << "/";
    vector<string> matstrs;
    for (auto &mat : materials)
        matstrs.emplace_back(mat.getToken());
    ser << join_strings(",", matstrs);
    ser << "/" << static_cast<int>(min_quality);
    ser << "/" << static_cast<int>(max_quality);
    ser << "/" << static_cast<int>(decorated_only);
    ser << "/" << static_cast<int>(default_max_quality);
    return ser.str();
}

static void clampItemQuality(df::item_quality *quality) {
    if (*quality > df::item_quality::Artifact) {
        DEBUG(control).print("clamping quality to Artifact");
        *quality = df::item_quality::Artifact;
    }
    if (*quality < df::item_quality::Ordinary) {
        DEBUG(control).print("clamping quality to Ordinary");
        *quality = df::item_quality::Ordinary;
    }
}

void ItemFilter::setMinQuality(int quality) {
    min_quality = static_cast<df::item_quality>(quality);
    clampItemQuality(&min_quality);
    if (max_quality < min_quality)
        max_quality = min_quality;
}

void ItemFilter::setMaxQuality(int quality, bool is_default) {
    max_quality = static_cast<df::item_quality>(quality);
    clampItemQuality(&max_quality);
    if (max_quality < min_quality)
        min_quality = max_quality;
    if (is_default)
        default_max_quality = max_quality;
}

void ItemFilter::setDecoratedOnly(bool decorated) {
    decorated_only = decorated;
}

void ItemFilter::setMaterialMask(uint32_t mask) {
    mat_mask.whole = mask;
}

void ItemFilter::setMaterials(const set<DFHack::MaterialInfo> &materials) {
    this->materials = materials;
}

static bool matchesMask(DFHack::MaterialInfo &mat, df::dfhack_material_category mat_mask) {
    return mat_mask.whole ? mat.matches(mat_mask) : true;
}

bool ItemFilter::matches(df::dfhack_material_category mask) const {
    return mask.whole & mat_mask.whole;
}

bool ItemFilter::matches(DFHack::MaterialInfo &material) const {
    for (auto &mat : materials)
        if (material.matches(mat))
            return true;
    return false;
}

bool ItemFilter::matches(df::item *item) const {
    int16_t quality = (item->flags.bits.artifact ? df::item_quality::Artifact : item->getQuality());
    if (quality < min_quality || quality > max_quality) {
        TRACE(cycle).print("item outside of quality range (%d not between %d and %d)\n",
                quality, min_quality, max_quality);
        return false;
    }

    if (decorated_only && !item->hasImprovements()) {
        TRACE(cycle).print("item needs improvements and doesn't have any\n");
        return false;
    }

    if (!mat_mask.whole)
        return true;

    auto imattype = item->getActualMaterial();
    auto imatindex = item->getActualMaterialIndex();
    auto item_mat = DFHack::MaterialInfo(imattype, imatindex);

    return (materials.size() == 0) ? matchesMask(item_mat, mat_mask) : matches(item_mat);
}

vector<ItemFilter> deserialize_item_filters(color_ostream &out, const string &serialized) {
    vector<ItemFilter> filters;

    vector<string> filter_strs;
    split_string(&filter_strs, serialized, ";");
    for (auto &str : filter_strs) {
        filters.emplace_back(out, str);
    }

    return filters;
}

string serialize_item_filters(const vector<ItemFilter> &filters) {
    vector<string> strs;
    for (auto &filter : filters) {
        strs.emplace_back(filter.serialize());
    }
    return join_strings(";", strs);
}
