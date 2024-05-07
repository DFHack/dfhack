// When displaying the names of partially-consumed items, show the percentage remaining
// Potentially useful for revealing why random pieces of cloth or thread aren't suitable for jobs

#include "df/item_barst.h"
#include "df/item_clothst.h"
#include "df/item_drinkst.h"
#include "df/item_globst.h"
#include "df/item_liquid_miscst.h"
#include "df/item_powder_miscst.h"
#include "df/item_sheetst.h"
#include "df/item_threadst.h"

#define DEFINE_PARTIAL_ITEM_TWEAK(TYPE, DIM) \
struct partial_items_hook_##TYPE : df::item_##TYPE##st { \
    typedef df::item_##TYPE##st interpose_base; \
    DEFINE_VMETHOD_INTERPOSE(void, getItemDescription, (std::string *str, int8_t plurality)) \
    { \
        INTERPOSE_NEXT(getItemDescription)(str, plurality); \
        if (dimension != DIM) \
            str->append(stl_sprintf(" (%i%%)", std::max(1, dimension * 100 / DIM))); \
    } \
}; \
IMPLEMENT_VMETHOD_INTERPOSE(partial_items_hook_##TYPE, getItemDescription);

DEFINE_PARTIAL_ITEM_TWEAK(bar, 150)
DEFINE_PARTIAL_ITEM_TWEAK(drink, 150)
DEFINE_PARTIAL_ITEM_TWEAK(glob, 150)
DEFINE_PARTIAL_ITEM_TWEAK(liquid_misc, 150)
DEFINE_PARTIAL_ITEM_TWEAK(powder_misc, 150)
DEFINE_PARTIAL_ITEM_TWEAK(cloth, 10000)
DEFINE_PARTIAL_ITEM_TWEAK(sheet, 10000)
DEFINE_PARTIAL_ITEM_TWEAK(thread, 15000)
