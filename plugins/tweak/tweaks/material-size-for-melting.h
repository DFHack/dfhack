#include <cmath>

#include "modules/Materials.h"
#include "modules/Random.h"

#include "df/inorganic_raw.h"
#include "df/item_armorst.h"
#include "df/item_constructed.h"
#include "df/item_glovesst.h"
#include "df/item_helmst.h"
#include "df/item_pantsst.h"
#include "df/item_shieldst.h"
#include "df/item_shoesst.h"
#include "df/item_toolst.h"
#include "df/item_trapcompst.h"
#include "df/item_weaponst.h"

static Random::MersenneRNG rng;
static float get_random() {
    return static_cast <float> (rng.drandom1());
}

static int32_t get_material_size_for_melting(df::item_constructed *item, int32_t base_material_size, float production_stack_size) {
    const float melt_return_per_material_size = 0.3f, base_melt_recovery = 0.95f, loss_per_wear_level = 0.1f;

    if (item->mat_type == 0) // INORGANIC only
    {
        float calculated_size, forging_cost_per_item;

        auto inorganic = df::inorganic_raw::find(item->mat_index);
        if (inorganic && inorganic->flags.is_set(df::inorganic_flags::DEEP_SPECIAL)){
            //adamantine items
            forging_cost_per_item = static_cast<float>(base_material_size) / production_stack_size;
        }
        else {
            // non adamantine items
            forging_cost_per_item = std::max(std::floor(static_cast<float>(base_material_size) / 3.0f), 1.0f);
            forging_cost_per_item /= production_stack_size;
        }
        calculated_size = forging_cost_per_item / melt_return_per_material_size;
        float melt_recovery = base_melt_recovery - static_cast<float>(item->wear) * loss_per_wear_level;
        calculated_size *= melt_recovery;
        int32_t random_part = ((modff(calculated_size, &calculated_size) > get_random()) ? 1 : 0);
        return  static_cast<int32_t>(calculated_size) + random_part;
    }
    return base_material_size;
}

#define DEFINE_MATERIAL_SIZE_FOR_MELTING_TWEAK(TYPE, PRODUCTION_STACK_SIZE) \
struct material_size_for_melting_##TYPE##_hook : df::item_##TYPE##st {\
    typedef df::item_##TYPE##st interpose_base;\
    DEFINE_VMETHOD_INTERPOSE(int32_t, getMaterialSizeForMelting, ()) {\
        return get_material_size_for_melting(this, INTERPOSE_NEXT(getMaterialSizeForMelting)(),PRODUCTION_STACK_SIZE);\
    }\
};\
IMPLEMENT_VMETHOD_INTERPOSE(material_size_for_melting_##TYPE##_hook, getMaterialSizeForMelting);

DEFINE_MATERIAL_SIZE_FOR_MELTING_TWEAK(armor, 1.0f)
DEFINE_MATERIAL_SIZE_FOR_MELTING_TWEAK(gloves, 2.0f)
DEFINE_MATERIAL_SIZE_FOR_MELTING_TWEAK(shoes, 2.0f)
DEFINE_MATERIAL_SIZE_FOR_MELTING_TWEAK(helm, 1.0f)
DEFINE_MATERIAL_SIZE_FOR_MELTING_TWEAK(pants, 1.0f)
DEFINE_MATERIAL_SIZE_FOR_MELTING_TWEAK(weapon, 1.0f)
DEFINE_MATERIAL_SIZE_FOR_MELTING_TWEAK(trapcomp, 1.0f)
DEFINE_MATERIAL_SIZE_FOR_MELTING_TWEAK(tool, 1.0f)
