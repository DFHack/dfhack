// Make flask item names indicate what's inside them (e.g. "liquid fire vial (green glass)")

#include "df/item_flaskst.h"
#include "df/builtin_mats.h"
#include "modules/Items.h"

struct flask_contents_hook : df::item_flaskst {
    typedef df::item_flaskst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, getItemDescription, (std::string *str, int8_t plurality)) {
        std::vector<df::item*> contents;
        Items::getContainedItems(this, &contents);

        if (contents.size() == 0)
        {
            INTERPOSE_NEXT(getItemDescription)(str, plurality);
            return;
        }
        MaterialInfo mat(mat_type, mat_index);
        if (contents.size() == 1)
        {
            if ((mat.material->flags.is_set(df::material_flags::LEATHER)) && (contents[0]->getMaterial() == df::builtin_mats::WATER))
                str->append("Filled");
            else
                contents[0]->getItemDescription(str, 1);
        }
        else
            str->append("Mixed");

        if (mat.material->flags.is_set(df::material_flags::LEATHER))
            str->append(" waterskin (");
        else if (mat.material->flags.is_set(df::material_flags::IS_GLASS))
            str->append(" vial (");
        else
            str->append(" flask (");
        str->append(mat.toString());
        str->append(")");
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(flask_contents_hook, getItemDescription);
