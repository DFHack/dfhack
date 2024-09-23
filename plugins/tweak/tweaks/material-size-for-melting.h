#include "modules/Materials.h"

#include "df/inorganic_raw.h"
#include "df/item_constructed.h"
#include "df/item_armorst.h"
#include "df/item_glovesst.h"
#include "df/item_helmst.h"
#include "df/item_shoesst.h"
#include "df/item_pantsst.h"
#include "df/item_weaponst.h"
#include "df/item_shieldst.h"
#include "df/item_toolst.h"
#include "df/item_trapcompst.h"
#include <cmath>

static int32_t get_material_size_for_melting(df::item_constructed *item, int32_t base_material_size, float production_stack_size) {

    if (item->mat_type == 0) // INORGANIC only
    {
        const float melt_return_per_material_size = 0.3f;
        auto inorganic = df::inorganic_raw::find(item->mat_index);
        if (inorganic && inorganic->flags.is_set(df::inorganic_flags::DEEP_SPECIAL)){
            return static_cast<int32_t>(std::round(static_cast<float>(base_material_size) / production_stack_size / melt_return_per_material_size));
            // get size for melting to minimize diff between forging cost and melting return, for adamantine forging cost == base_material_size, divided by amount of items created in batch
        }
            return static_cast<int32_t>(std::round(std::max(std::floor(static_cast<float>(base_material_size) / 3.0f), 1.0f) / production_stack_size / melt_return_per_material_size));
            // (std::max(std::floor(static_cast<float>(base_material_size) / 3.0f), 1.0f) / production_stack_size) - forging cost for non adamantine item
    }
    return base_material_size;
}

static int32_t get_material_size_for_melting(df::item_constructed* item, int32_t base_size) {
    return get_material_size_for_melting(item, base_size, 1.0f);
}

struct material_size_for_melting_armor_hook : df::item_armorst {
    typedef df::item_armorst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(int32_t, getMaterialSizeForMelting, ()) {
        return get_material_size_for_melting(this, INTERPOSE_NEXT(getMaterialSizeForMelting)());
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(material_size_for_melting_armor_hook, getMaterialSizeForMelting);

struct material_size_for_melting_gloves_hook : df::item_glovesst {
    typedef df::item_glovesst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(int32_t, getMaterialSizeForMelting, ()) {
        return get_material_size_for_melting(this, INTERPOSE_NEXT(getMaterialSizeForMelting)(), 2.0f);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(material_size_for_melting_gloves_hook, getMaterialSizeForMelting);

struct material_size_for_melting_shoes_hook : df::item_shoesst {
    typedef df::item_shoesst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(int32_t, getMaterialSizeForMelting, ()) {
        return get_material_size_for_melting(this, INTERPOSE_NEXT(getMaterialSizeForMelting)(), 2.0f);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(material_size_for_melting_shoes_hook, getMaterialSizeForMelting);

struct material_size_for_melting_helm_hook : df::item_helmst {
    typedef df::item_helmst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(int32_t, getMaterialSizeForMelting, ()) {
        return get_material_size_for_melting(this, INTERPOSE_NEXT(getMaterialSizeForMelting)());
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(material_size_for_melting_helm_hook, getMaterialSizeForMelting);

struct material_size_for_melting_pants_hook : df::item_pantsst {
    typedef df::item_pantsst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(int32_t, getMaterialSizeForMelting, ()) {
        return get_material_size_for_melting(this, INTERPOSE_NEXT(getMaterialSizeForMelting)());
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(material_size_for_melting_pants_hook, getMaterialSizeForMelting);

struct material_size_for_melting_weapon_hook : df::item_weaponst {
    typedef df::item_weaponst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(int32_t, getMaterialSizeForMelting, ()) {
        return get_material_size_for_melting(this, INTERPOSE_NEXT(getMaterialSizeForMelting)());
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(material_size_for_melting_weapon_hook, getMaterialSizeForMelting);

struct material_size_for_melting_trapcomp_hook : df::item_trapcompst {
    typedef df::item_trapcompst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(int32_t, getMaterialSizeForMelting, ()) {
        return get_material_size_for_melting(this, INTERPOSE_NEXT(getMaterialSizeForMelting)());
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(material_size_for_melting_trapcomp_hook, getMaterialSizeForMelting);

struct material_size_for_melting_tool_hook : df::item_toolst {
    typedef df::item_toolst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(int32_t, getMaterialSizeForMelting, ()) {
        return get_material_size_for_melting(this, INTERPOSE_NEXT(getMaterialSizeForMelting)());
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(material_size_for_melting_tool_hook, getMaterialSizeForMelting);
