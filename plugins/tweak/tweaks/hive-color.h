#include "df/building_flags.h"
#include "df/building_drawbuffer.h"
#include "df/building_hivest.h"

using namespace df::enums;

struct hive_color_hook : df::building_hivest {
    typedef df::building_hivest interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, drawBuilding, (df::building_drawbuffer* db, int16_t unk))
    {
        INTERPOSE_NEXT(drawBuilding)(db, unk);
        if (flags.bits.exists)
        {
            MaterialInfo mat(mat_type, mat_index);
            db->fore[0][0] = mat.material->build_color[0];
            db->back[0][0] = mat.material->build_color[1];
            db->bright[0][0] = mat.material->build_color[2];
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(hive_color_hook, drawBuilding);
