#include "itemfilter.h"

#include "Debug.h"

#include "df/item.h"

using namespace DFHack;

namespace DFHack {
    DBG_EXTERN(buildingplan, status);
}

ItemFilter::ItemFilter() {
    clear();
}

void ItemFilter::clear() {
    min_quality = df::item_quality::Ordinary;
    max_quality = df::item_quality::Masterful;
    decorated_only = false;
    mat_mask.whole = 0;
    materials.clear();
}

bool ItemFilter::isEmpty() {
    return min_quality == df::item_quality::Ordinary
            && max_quality == df::item_quality::Masterful
            && !decorated_only
            && !mat_mask.whole
            && materials.empty();
}

static bool deserializeMaterialMask(std::string ser, df::dfhack_material_category mat_mask) {
    if (ser.empty())
        return true;

    if (!parseJobMaterialCategory(&mat_mask, ser)) {
        DEBUG(status).print("invalid job material category serialization: '%s'", ser.c_str());
        return false;
    }
    return true;
}

static bool deserializeMaterials(std::string ser, std::vector<DFHack::MaterialInfo> &materials) {
    if (ser.empty())
        return true;

    std::vector<std::string> mat_names;
    split_string(&mat_names, ser, ",");
    for (auto m = mat_names.begin(); m != mat_names.end(); m++) {
        DFHack::MaterialInfo material;
        if (!material.find(*m) || !material.isValid()) {
            DEBUG(status).print("invalid material name serialization: '%s'", ser.c_str());
            return false;
        }
        materials.push_back(material);
    }
    return true;
}

ItemFilter::ItemFilter(std::string serialized) {
    clear();

    std::vector<std::string> tokens;
    split_string(&tokens, serialized, "/");
    if (tokens.size() != 5) {
        DEBUG(status).print("invalid ItemFilter serialization: '%s'", serialized.c_str());
        return;
    }

    if (!deserializeMaterialMask(tokens[0], mat_mask) || !deserializeMaterials(tokens[1], materials))
        return;

    setMinQuality(atoi(tokens[2].c_str()));
    setMaxQuality(atoi(tokens[3].c_str()));
    decorated_only = static_cast<bool>(atoi(tokens[4].c_str()));
}

// format: mat,mask,elements/materials,list/minq/maxq/decorated
std::string ItemFilter::serialize() const {
    std::ostringstream ser;
    ser << bitfield_to_string(mat_mask, ",") << "/";
    if (!materials.empty()) {
        ser << materials[0].getToken();
        for (size_t i = 1; i < materials.size(); ++i)
            ser << "," << materials[i].getToken();
    }
    ser << "/" << static_cast<int>(min_quality);
    ser << "/" << static_cast<int>(max_quality);
    ser << "/" << static_cast<int>(decorated_only);
    return ser.str();
}

static void clampItemQuality(df::item_quality *quality) {
    if (*quality > df::item_quality::Artifact) {
        DEBUG(status).print("clamping quality to Artifact");
        *quality = df::item_quality::Artifact;
    }
    if (*quality < df::item_quality::Ordinary) {
        DEBUG(status).print("clamping quality to Ordinary");
        *quality = df::item_quality::Ordinary;
    }
}

void ItemFilter::setMinQuality(int quality) {
    min_quality = static_cast<df::item_quality>(quality);
    clampItemQuality(&min_quality);
    if (max_quality < min_quality)
        max_quality = min_quality;
}

void ItemFilter::setMaxQuality(int quality) {
    max_quality = static_cast<df::item_quality>(quality);
    clampItemQuality(&max_quality);
    if (max_quality < min_quality)
        min_quality = max_quality;
}

void ItemFilter::setDecoratedOnly(bool decorated) {
    decorated_only = decorated;
}

void ItemFilter::setMaterialMask(uint32_t mask) {
    mat_mask.whole = mask;
}

void ItemFilter::setMaterials(const std::vector<DFHack::MaterialInfo> &materials) {
    this->materials = materials;
}

std::string ItemFilter::getMinQuality() const {
    return ENUM_KEY_STR(item_quality, min_quality);
}

std::string ItemFilter::getMaxQuality() const {
    return ENUM_KEY_STR(item_quality, max_quality);
}

bool ItemFilter::getDecoratedOnly() const {
    return decorated_only;
}

uint32_t ItemFilter::getMaterialMask() const {
    return mat_mask.whole;
}

static std::string material_to_string_fn(const MaterialInfo &m) { return m.toString(); }

std::vector<std::string> ItemFilter::getMaterials() const {
    std::vector<std::string> descriptions;
    transform_(materials, descriptions, material_to_string_fn);

    if (descriptions.size() == 0)
        bitfield_to_string(&descriptions, mat_mask);

    if (descriptions.size() == 0)
        descriptions.push_back("any");

    return descriptions;
}

static bool matchesMask(DFHack::MaterialInfo &mat, df::dfhack_material_category mat_mask) {
    return mat_mask.whole ? mat.matches(mat_mask) : true;
}

bool ItemFilter::matches(df::dfhack_material_category mask) const {
    return mask.whole & mat_mask.whole;
}

bool ItemFilter::matches(DFHack::MaterialInfo &material) const {
    for (auto it = materials.begin(); it != materials.end(); ++it)
        if (material.matches(*it))
            return true;
    return false;
}

bool ItemFilter::matches(df::item *item) const {
    if (item->getQuality() < min_quality || item->getQuality() > max_quality)
        return false;

    if (decorated_only && !item->hasImprovements())
        return false;

    auto imattype = item->getActualMaterial();
    auto imatindex = item->getActualMaterialIndex();
    auto item_mat = DFHack::MaterialInfo(imattype, imatindex);

    return (materials.size() == 0) ? matchesMask(item_mat, mat_mask) : matches(item_mat);
}
