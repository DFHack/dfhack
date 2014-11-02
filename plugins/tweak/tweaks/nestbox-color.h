#include "df/building_flags.h"
#include "df/building_drawbuffer.h"
#include "df/building_nest_boxst.h"

using namespace df::enums;

struct nestbox_color_hook : df::building_nest_boxst {
    typedef df::building_nest_boxst interpose_base;
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

IMPLEMENT_VMETHOD_INTERPOSE(nestbox_color_hook, drawBuilding);
