// changeitem plugin
// allows to change the material type and quality of selected items
#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
using namespace std;

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"

#include "DataDefs.h"
#include "df/item.h"
#include "df/itemdef.h"
#include "df/world.h"
#include "df/general_ref.h"

using namespace DFHack;
using namespace df::enums;

using MapExtras::Block;
using MapExtras::MapCache;

DFHACK_PLUGIN("changeitem");
REQUIRE_GLOBAL(world);

command_result df_changeitem(color_ostream &out, vector <string> & parameters);

const string changeitem_help =
    "Changeitem allows to change some item attributes.\n"
    "By default the item currently selected in the UI will be changed\n"
    "(you can select items in the 'k' list or inside containers/inventory).\n"
    "By default change is only allowed if materials is of the same subtype\n"
    "(for example wood<->wood, stone<->stone etc). But since some transformations\n"
    "work pretty well and may be desired you can override this with 'force'.\n"
    "Note that some attributes will not be touched, possibly resulting in weirdness.\n"
    "To get an idea how the RAW id should look like, check some items with 'info'.\n"
    "Using 'force' might create items which are not touched by crafters/haulers.\n"
    "Options:\n"
    "  info         - don't change anything, print some item info instead\n"
    "  here         - change all items at cursor position\n"
    "  material, m  - change material. must be followed by material RAW id\n"
    "  subtype, s   - change subtype. must be followed by correct RAW id\n"
    "  quality, q   - change base quality. must be followed by number (0-5)\n"
    "  force        - ignore subtypes, force change to new material.\n"
    "Example:\n"
    "  changeitem m INORGANIC:GRANITE here\n"
    "    change material of all items under the cursor to granite\n"
    "  changeitem q 5\n"
    "    change currently selected item to masterpiece quality\n";


DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "changeitem", "Change item attributes (material, quality).",
        df_changeitem, false,
        changeitem_help.c_str()
    ));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

// probably there is some method in the library which does the same
// todo: look for it :)
string describeQuality(int q)
{
    switch(q)
    {
    case 0:
        return "Basic";
    case 1:
        return "-Well-crafted-";
    case 2:
        return "+Finely-crafted+";
    case 3:
        return "*Superior quality*";
    case 4:
        return "#Exceptional#";
    case 5:
        return "$Masterful$";
    default:
        return "!INVALID!";
    }
}

command_result changeitem_execute(
    color_ostream &out, df::item * item,
    bool info, bool force,
    bool change_material, string new_material,
    bool change_quality, int new_quality,
    bool change_subtype, string new_subtype);

command_result df_changeitem(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    bool here = false;
    bool info = false;
    bool force = false;

    bool change_material = false;
    string new_material = "none";

    bool change_subtype = false;
    string new_subtype = "NONE";

    bool change_quality = false;
    int new_quality = 0;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        string & p = parameters[i];

        if (p == "help" || p == "?")
        {
            out << changeitem_help << endl;
            return CR_OK;
        }
        else if (p == "here")
        {
            here = true;
        }
        else if (p == "info")
        {
            info = true;
        }
        else if (p == "force")
        {
            force = true;
        }
        else if (p == "material" || p == "m" )
        {
            // must be followed by material RAW id
            // (string like 'INORGANIC:GRANITE', 'PLANT:MAPLE:WOOD', ...)
            if(i == parameters.size()-1)
            {
                out.printerr("no material specified!\n");
                return CR_WRONG_USAGE;
            }
            change_material = true;
            new_material = parameters[i+1];
            i++;
        }
        else if (p == "quality" || p == "q")
        {
            // must be followed by numeric quality (allowed: 0-5)
            if(i == parameters.size()-1)
            {
                out.printerr("no quality specified!\n");
                return CR_WRONG_USAGE;
            }
            string & q = parameters[i+1];
            // meh. should use a stringstream instead. but it's only 6 numbers
            if(q == "0")
                new_quality = 0;
            else if(q == "1")
                new_quality = 1;
            else if(q == "2")
                new_quality = 2;
            else if(q == "3")
                new_quality = 3;
            else if(q == "4")
                new_quality = 4;
            else if(q == "5")
                new_quality = 5;
            else
            {
                out << "Invalid quality specified!" << endl;
                return CR_WRONG_USAGE;
            }
            out << "change to quality: " << describeQuality(new_quality) << endl;
            change_quality = true;
            i++;
        }
        else if (p == "subtype" || p == "s" )
        {
            // must be followed by subtype RAW id
            // (string like 'ITEM_GLOVES_GAUNTLETS', 'ITEM_WEAPON_DAGGER_LARGE', 'ITEM_TOOL_KNIFE_MEAT_CLEAVER', ...)
            if(i == parameters.size()-1)
            {
                out.printerr("no subtype specified!\n");
                return CR_WRONG_USAGE;
            }
            change_subtype = true;
            new_subtype = parameters[i+1];
            i++;
        }
        else
        {
            out << p << ": Unknown command!" << endl;
            return CR_WRONG_USAGE;
        }
    }

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    MaterialInfo mat_new;
    if (change_material && !mat_new.find(new_material))
    {
        out.printerr("No such material!\n");
        return CR_FAILURE;
    }

    if (here)
    {
        int processed_total = 0;
        int cx, cy, cz;
        DFCoord pos_cursor;

        // needs a cursor
        if (!Gui::getCursorCoords(cx,cy,cz))
        {
            out.printerr("Cursor position not found. Please enable the cursor.\n");
            return CR_FAILURE;
        }
        pos_cursor = DFCoord(cx,cy,cz);

        // uh. is this check necessary?
        // changeitem doesn't do stuff with map blocks...
        {
            MapCache MC;
            Block * b = MC.BlockAt(pos_cursor / 16);
            if(!b)
            {
                out.printerr("Cursor is in an invalid/uninitialized area. Place it over a floor.\n");
                return CR_FAILURE;
            }
            // when only changing material it doesn't matter if cursor is over a tile
            //df::tiletype ttype = MC.tiletypeAt(pos_cursor);
            //if(!DFHack::isFloorTerrain(ttype))
            //{
            //    out.printerr("Cursor should be placed over a floor.\n");
            //    return CR_FAILURE;
            //}
        }

        // iterate over all items, process those where pos = pos_cursor
        size_t numItems = world->items.all.size();
        for(size_t i=0; i< numItems; i++)
        {
            df::item * item = world->items.all[i];
            DFCoord pos_item(item->pos.x, item->pos.y, item->pos.z);

            if (pos_item != pos_cursor)
                continue;

            changeitem_execute(out, item, info, force, change_material, new_material, change_quality, new_quality, change_subtype, new_subtype);
            processed_total++;
        }
        out.print("Done. %d items processed.\n", processed_total);
    }
    else
    {
        // needs a selected item
        df::item *item = Gui::getSelectedItem(out);
        if (!item)
        {
            out.printerr("No item selected.\n");
            return CR_FAILURE;
        }
        changeitem_execute(out, item, info, force, change_material, new_material, change_quality, new_quality, change_subtype, new_subtype);
    }
    return CR_OK;
}

command_result changeitem_execute(
    color_ostream &out, df::item * item,
    bool info, bool force,
    bool change_material, string new_material,
    bool change_quality, int new_quality,
    bool change_subtype, string new_subtype )
{
    MaterialInfo mat_new;
    MaterialInfo mat_old;

    ItemTypeInfo sub_old;
    ItemTypeInfo sub_new;
    int new_subtype_id = -1;

    if(change_material)
        mat_new.find(new_material);
    if(change_material || info)
        mat_old.decode(item);

    if(change_subtype || info)
        sub_old.decode(item);
    if(change_subtype)
    {
        string new_type = ENUM_KEY_STR(item_type, item->getType()) + ":" + new_subtype;
        if (new_subtype == "NONE")
            new_subtype_id = -1;
        else if (sub_new.find(new_type))
            new_subtype_id = sub_new.subtype;
        else
        {
//          out.printerr("Invalid subtype for selected item, skipping\n");
            return CR_FAILURE;
        }
    }

    // print some info, don't change stuff
    if(info)
    {
        out << "Item info: " << endl;
        out << "  type:    " << ENUM_KEY_STR(item_type, item->getType()) << endl;
        out << "  subtype: " << (sub_old.custom ? sub_old.custom->id : "NONE") << endl;
        out << "  quality: " << describeQuality(item->getQuality()) << endl;
        //if(item->isImproved())
        //    out << "  imp.quality: " << describeQuality(item->getImprovementQuality()) << endl;
        out << "  material: " << mat_old.getToken() << endl;
        return CR_OK;
    }

    if(change_quality)
    {
        item->setQuality(new_quality);
        // it would be nice to be able to change the improved quality, too
        // (only allowed if the item is already improved)
        // but there is no method in item.h which supports that
        // ok: hints from _Q/angavrilov: improvent is a vector, an item can have more than one improvement
        // -> virtual_cast to item_constructedst
    }

    if(change_material)
    {
        // subtype and mode should match to avoid doing dumb stuff like changing boulders into meat whatever
        // changing a stone cabinet to wood is fine, though. as well as lots of other combinations.
        // still, it's better to make the user activate 'force' if he really wants to.

        // fixme: changing material of cloth items needs more work...
        // <_Q> cloth items have a "CLOTH" improvement which tells you about the cloth that was used to make it

        if(force||(mat_old.subtype == mat_new.subtype && mat_old.mode==mat_new.mode))
        {
            item->setMaterial(mat_new.type);
            item->setMaterialIndex(mat_new.index);
        }
        else
        {
            out.printerr("change denied: subtype doesn't match. use 'force' to override.\n");
        }

        item->flags.bits.temps_computed = 0;              // recalc temperatures next time touched
        item->flags.bits.weight_computed = 0;   // recalc weight next time touched
    }

    if(change_subtype)
    {
        item->setSubtype(new_subtype_id);
        item->flags.bits.weight_computed = 0;   // recalc weight next time touched
    }
    return CR_OK;
}
