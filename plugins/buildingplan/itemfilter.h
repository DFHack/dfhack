#pragma once

#include "ColorText.h"

#include "modules/Materials.h"

#include "df/dfhack_material_category.h"
#include "df/item_quality.h"

#include <set>

class ItemFilter {
public:
    ItemFilter();
    ItemFilter(DFHack::color_ostream &out, const std::string& serialized);

    void clear();
    bool isEmpty() const;
    std::string serialize() const;

    void setMinQuality(int quality);
    void setMaxQuality(int quality, bool is_default = false);
    void setDecoratedOnly(bool decorated);
    void setMaterialMask(uint32_t mask);
    void setMaterials(const std::set<DFHack::MaterialInfo> &materials);

    df::item_quality getMinQuality() const { return min_quality; }
    df::item_quality getMaxQuality() const {return max_quality; }
    bool getDecoratedOnly() const { return decorated_only; }
    df::dfhack_material_category getMaterialMask() const { return mat_mask; }
    std::set<DFHack::MaterialInfo> getMaterials() const { return materials; }

    bool matches(df::dfhack_material_category mask) const;
    bool matches(DFHack::MaterialInfo &material) const;
    bool matches(df::item *item) const;

private:
    df::item_quality min_quality;
    df::item_quality max_quality;
    df::item_quality default_max_quality;
    bool decorated_only;
    df::dfhack_material_category mat_mask;
    std::set<DFHack::MaterialInfo> materials;
};

std::vector<ItemFilter> deserialize_item_filters(DFHack::color_ostream &out, const std::string &serialized);
std::string serialize_item_filters(const std::vector<ItemFilter> &filters);
