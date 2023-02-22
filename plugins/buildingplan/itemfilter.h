#pragma once

#include "modules/Materials.h"

#include "df/dfhack_material_category.h"
#include "df/item_quality.h"

class ItemFilter {
public:
    ItemFilter();
    ItemFilter(std::string serialized);

    void clear();
    bool isEmpty();
    std::string serialize() const;

    void setMinQuality(int quality);
    void setMaxQuality(int quality);
    void setDecoratedOnly(bool decorated);
    void setMaterialMask(uint32_t mask);
    void setMaterials(const std::vector<DFHack::MaterialInfo> &materials);

    std::string getMinQuality() const;
    std::string getMaxQuality() const;
    bool getDecoratedOnly() const;
    uint32_t getMaterialMask() const;
    std::vector<std::string> getMaterials() const;

    bool matches(df::dfhack_material_category mask) const;
    bool matches(DFHack::MaterialInfo &material) const;
    bool matches(df::item *item) const;

private:
    df::item_quality min_quality;
    df::item_quality max_quality;
    bool decorated_only;
    df::dfhack_material_category mat_mask;
    std::vector<DFHack::MaterialInfo> materials;
};
