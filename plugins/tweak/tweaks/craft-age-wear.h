#include "df/item_crafted.h"

struct craft_age_wear_hook : df::item_crafted {
    typedef df::item_crafted interpose_base;

    DEFINE_VMETHOD_INTERPOSE(bool, ageItem, (int amount)) {
        uint32_t orig_age = age;
        age += amount;
        if (age > 200000000)
            age = 200000000;
        if (age == orig_age)
            return false;

        MaterialInfo mat(mat_type, mat_index);
        if (!mat.isValid())
            return false;
        int wear = 0;

        if (mat.material->flags.is_set(df::material_flags::WOOD))
            wear = 5;
        else if (mat.material->flags.is_set(df::material_flags::LEATHER) ||
            mat.material->flags.is_set(df::material_flags::THREAD_PLANT) ||
            mat.material->flags.is_set(df::material_flags::SILK) ||
            mat.material->flags.is_set(df::material_flags::YARN))
            wear = 1;
        else
            return false;
        wear = ((orig_age % wear) + int32_t(age - orig_age)) / wear;
        if (wear > 0)
            return incWearTimer(wear);
        else
            return false;
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(craft_age_wear_hook, ageItem);
