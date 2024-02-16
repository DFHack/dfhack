#include "df/item_eggst.h"

struct eggs_fertile_hook : df::item_eggst {
    typedef df::item_eggst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, getItemDescription, (std::string *str, int8_t plurality)) {
        INTERPOSE_NEXT(getItemDescription)(str, plurality);
        if (this->egg_flags.bits.fertile)
            str->append(stl_sprintf(" (Fertile)"));
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(eggs_fertile_hook, getItemDescription);
