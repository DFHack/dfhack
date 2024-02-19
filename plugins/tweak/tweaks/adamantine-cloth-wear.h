#include "modules/Materials.h"

#include "df/item_armorst.h"
#include "df/item_constructed.h"
#include "df/item_glovesst.h"
#include "df/item_helmst.h"
#include "df/item_shoesst.h"
#include "df/item_pantsst.h"

static bool inc_wear_timer (df::item_constructed *item, int amount) {
    if (item->flags.bits.artifact)
        return false;

    MaterialInfo mat(item->mat_type, item->mat_index);
    if (mat.isInorganic() && mat.inorganic->flags.is_set(df::inorganic_flags::DEEP_SPECIAL))
        return false;

    item->wear_timer += amount;
    return (item->wear_timer > 806400);
}

struct adamantine_cloth_wear_armor_hook : df::item_armorst {
    typedef df::item_armorst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, incWearTimer, (int amount)) {
        return inc_wear_timer(this, amount);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(adamantine_cloth_wear_armor_hook, incWearTimer);

struct adamantine_cloth_wear_helm_hook : df::item_helmst {
    typedef df::item_helmst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, incWearTimer, (int amount)) {
        return inc_wear_timer(this, amount);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(adamantine_cloth_wear_helm_hook, incWearTimer);

struct adamantine_cloth_wear_gloves_hook : df::item_glovesst {
    typedef df::item_glovesst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, incWearTimer, (int amount)) {
        return inc_wear_timer(this, amount);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(adamantine_cloth_wear_gloves_hook, incWearTimer);

struct adamantine_cloth_wear_shoes_hook : df::item_shoesst {
    typedef df::item_shoesst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, incWearTimer, (int amount)) {
        return inc_wear_timer(this, amount);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(adamantine_cloth_wear_shoes_hook, incWearTimer);

struct adamantine_cloth_wear_pants_hook : df::item_pantsst {
    typedef df::item_pantsst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, incWearTimer, (int amount)) {
        return inc_wear_timer(this, amount);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(adamantine_cloth_wear_pants_hook, incWearTimer);
