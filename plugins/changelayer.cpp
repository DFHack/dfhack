// changelayer plugin
// allows changing the material type of geological layers

#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Gui.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/Materials.h"

// DF data structure definition headers
#include "df/region_map_entry.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_geo_biome.h"
#include "df/world_geo_layer.h"

using namespace DFHack;
using namespace df::enums;
using namespace std;

using std::vector;
using std::string;

DFHACK_PLUGIN("changelayer");
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(cursor);

const string changelayer_help =
    "  Allows to change the material of whole geology layers.\n"
    "  Can have impact on all surrounding regions, not only your embark!\n"
    "  By default changing stone to soil and vice versa is not allowed.\n"
    "  By default changes only the layer at the cursor position.\n"
    "  Note that one layer can stretch across lots of z levels.\n"
    "  By default changes only the geology which is linked to the biome under the\n"
    "  cursor. That geology might be linked to other biomes as well, though.\n"
    "  Mineral veins and gem clusters will stay on the map.\n"
    "  Use 'changevein' for them.\n\n"
    "  tl;dr: You will end up with changing quite big areas in one go.\n\n"
    "Options (first parameter MUST be the material id):\n"
    "  all_biomes - Change layer for all biomes on your map.\n"
    "               Result may be undesirable since the same layer\n"
    "               can AND WILL be on different z-levels for different biomes.\n"
    "               Use the tool 'probe' to get an idea how layers and biomes\n"
    "               are distributed on your map.\n"
    "  all_layers - Change all layers on your map.\n"
    "               Candy mountain, anyone?\n"
    "               Will make your map quite boring, but tidy.\n"
    "  force      - Allow changing stone to soil and vice versa.\n"
    "               !!THIS CAN HAVE WEIRD EFFECTS, USE WITH CARE!!\n"
    "               Note that soil will not be magically replaced with stone.\n"
    "                 You will, however, get a stone floor after digging so it\n"
    "                 will allow the floor to be engraved.\n"
    "               Note that stone will not be magically replaced with soil.\n"
    "                 You will, however, get a soil floor after digging so it\n"
    "                 could be helpful for creating farm plots on maps with no soil.\n"
    "  verbose    - Give some more details about what is being changed.\n"
    "  trouble    - Give some advice for known problems.\n"
    "Example:\n"
    "  changelayer GRANITE\n"
    "    Convert layer at cursor position into granite.\n"
    "  changelayer SILTY_CLAY force\n"
    "    Convert layer at cursor position into clay even if it's stone.\n"
    "  changelayer MARBLE allbiomes alllayers\n"
    "    Convert all layers of all biomes into marble.\n";

const string changelayer_trouble =
    "Known problems with changelayer:\n\n"
    "  Nothing happens, the material stays the old.\n"
    "    Pause/unpause the game and/or move the cursor a bit. Then retry.\n"
    "    Try changing another layer, undo the changes and try again.\n"
    "    Try saving and loading the game.\n\n"
    "  Weird stuff happening after using the 'force' option.\n"
    "    Change former stone layers back to stone, soil back to soil.\n"
    "    If in doubt, use the 'probe' tool to find tiles with soil walls\n"
    "    and stone layer type or the other way round.\n";


command_result changelayer (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "changelayer", "Change a whole geology layer.",
        changelayer, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        changelayer_help.c_str()
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

bool conversionAllowed(color_ostream &out, MaterialInfo mi, MaterialInfo ml, bool force);

// no need to spam the "stone to soil" warning more than once
// in case multiple biomes and/or layers are going to be changed
static bool warned = false;

command_result changelayer (color_ostream &out, std::vector <std::string> & parameters)
{
    CoreSuspender suspend;

    string material;
    bool force = false;
    bool all_biomes = false;
    bool all_layers = false;
    bool verbose = false;
    warned = false;

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out.print(changelayer_help.c_str());
            return CR_OK;
        }
        if(parameters[i] == "trouble")
        {
            out.print(changelayer_trouble.c_str());
            return CR_OK;
        }
        if(parameters[i] == "force")
            force = true;
        if(parameters[i] == "all_biomes")
            all_biomes = true;
        if(parameters[i] == "all_layers")
            all_layers = true;
        if(parameters[i] == "verbose")
            verbose = true;
    }

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    if (parameters.empty())
    {
        out.printerr("You need to specify a material!\n");
        return CR_WRONG_USAGE;
    }

    material = parameters[0];

    MaterialInfo mat_new;
    if (!mat_new.findInorganic(material))
    {
        out.printerr("No such material!\n");
        return CR_FAILURE;
    }

    // check if specified material is stone or gem or soil
    if (mat_new.inorganic->material.flags.is_set(material_flags::IS_METAL) ||
        mat_new.inorganic->material.flags.is_set(material_flags::NO_STONE_STOCKPILE))
    {
        out.printerr("Invalid material - you must select a type of stone or gem or soil.\n");
        return CR_FAILURE;
    }


    MapExtras::MapCache mc;

    int32_t regionX, regionY, regionZ;
    Maps::getPosition(regionX,regionY,regionZ);

    int32_t cursorX, cursorY, cursorZ;
    Gui::getCursorCoords(cursorX,cursorY,cursorZ);
    if(cursorX == -30000)
    {
        out.printerr("No cursor; place cursor over tile.\n");
        return CR_FAILURE;
    }
    DFCoord cursor (cursorX,cursorY,cursorZ);

    uint32_t blockX = cursorX / 16;
    uint32_t tileX = cursorX % 16;
    uint32_t blockY = cursorY / 16;
    uint32_t tileY = cursorY % 16;

    MapExtras::Block * b = mc.BlockAt(cursor/16);
    if(!b || !b->is_valid())
    {
        out.printerr("No data.\n");
        return CR_OK;
    }

    df::tile_designation des = b->DesignationAt(cursor%16);

    // get biome and geolayer at cursor position
    uint32_t biome = des.bits.biome;
    uint32_t layer = des.bits.geolayer_index;
    if(verbose)
    {
        out << "biome: " << biome << endl
            << "geolayer: " << layer << endl;
    }


    // there is no Maps::WriteGeology or whatever, and I didn't want to mess with the library and add it
    // so I copied the stuff which reads the geology information and modified it to be able to change it
    //
    // a more elegant solution would probably look like this:
    // 1) modify Maps::ReadGeology to accept and fill one more optional vector
    //    where the geolayer ids of the 9 biomes are stored
    // 2) call ReadGeology here, modify the data in the vectors without having to do all that map stuff
    // 3) write Maps::WriteGeology, pass the vectors, let it do it's work
    // Step 1) is optional, but it would make implementing 3) easier.
    // Otherwise that "check which geo_index is used by biome X" loop would need to be done again.

    // no need to touch the same geology more than once
    // though it wouldn't matter much since there is not much data to be processed
    vector<uint16_t> v_geoprocessed;
    v_geoprocessed.clear();

    // iterate over 8 surrounding regions + local region
    for (int i = eNorthWest; i < eBiomeCount; i++)
    {
        if(verbose)
            out << "---Biome: " << i;
        if(!all_biomes && i!=biome)
        {
            if(verbose)
                out << "-skipping" << endl;
            continue;
        }
        else
        {
            if(verbose)
                out << "-checking" << endl;
        }

        // check against worldmap boundaries, fix if needed
        // regionX is in embark squares
        // regionX/16 is in 16x16 embark square regions
        // i provides -1 .. +1 offset from the current region
        int bioRX = world->map.region_x / 16 + ((i % 3) - 1);
        if (bioRX < 0) bioRX = 0;
        if (bioRX >= world->world_data->world_width) bioRX = world->world_data->world_width - 1;
        int bioRY = world->map.region_y / 16 + ((i / 3) - 1);
        if (bioRY < 0) bioRY = 0;
        if (bioRY >= world->world_data->world_height) bioRY = world->world_data->world_height - 1;

        // get index into geoblock vector
        uint16_t geoindex = world->world_data->region_map[bioRX][bioRY].geo_index;

        if(verbose)
            out << "geoindex: " << geoindex << endl;

        bool skip = false;
        for(int g=0; g<v_geoprocessed.size(); g++)
        {
            if(v_geoprocessed.at(g)==geoindex)
            {
                if(verbose)
                    out << "already processed" << endl;
                skip = true;
                break;
            }
        }
        if(skip)
            continue;

        v_geoprocessed.push_back(geoindex);

        /// geology blocks have a vector of layer descriptors
        // get the vector with pointer to layers
        df::world_geo_biome *geo_biome = df::world_geo_biome::find(geoindex);
        if (!geo_biome)
        {
            if(verbose)
                out << "no geology found here." << endl;
            continue;
        }

        vector <df::world_geo_layer*> &geolayers = geo_biome->layers;

        // complain if layer is out of range
        // geology has up to 16 layers currently, but can have less!
        if(layer >= geolayers.size() || layer < 0)
        {
            if(verbose)
                out << "layer out of range!";
            continue;
        }

        // now let's actually write the new mat id to the layer(s)
        if(all_layers)
        {
            for (size_t j = 0; j < geolayers.size(); j++)
            {
                MaterialInfo mat_old;
                mat_old.decode(0, geolayers[j]->mat_index);
                if(conversionAllowed(out, mat_new, mat_old, force))
                {
                    if(verbose)
                        out << "changing geolayer " << j
                            << " from " << mat_old.getToken()
                            << " to " << mat_new.getToken()
                            << endl;
                    geolayers[j]->mat_index = mat_new.index;
                }
            }
        }
        else
        {
            MaterialInfo mat_old;
            mat_old.decode(0, geolayers[layer]->mat_index);
            if(conversionAllowed(out, mat_new, mat_old, force))
            {
                if(verbose)
                    out << "changing geolayer " << layer
                        << " from " << mat_old.getToken()
                        << " to " << mat_new.getToken()
                        << endl;
                geolayers[layer]->mat_index = mat_new.index;
            }
        }
    }

    out.print("Done.\n");

    // Give control back to DF.
    return CR_OK;
}

// check if user tries to convert soil <-> stone
// throw some warning if he does
bool conversionAllowed(color_ostream &out, MaterialInfo mat_new, MaterialInfo mat_old, bool force)
{
    // check if current layer mat is stone or soil:
    // by default don't allow to turn stone to soil and vice versa
    bool unsafe = false;
    bool allowed = true;

    // throw warning if user wants to change soil to stone or vice versa
    // while it does work it might be unsafe and can have weird results
    // you can't simply convert stone to soil, the rock walls will remain (same the other way round)
    // the floor will turn into soil, though, so it might be useful for creating farm plots
    // therefore it's not completely forbidden and can be enabled by the 'force' option

    if (    mat_new.inorganic->flags.is_set(inorganic_flags::SOIL_ANY)
        && !mat_old.inorganic->flags.is_set(inorganic_flags::SOIL_ANY))
    {
        if(!warned)
        {
            out << "Changing a stone layer into soil is probably not wise." << endl
                << "The stone will remain and you get a soil floor after digging." << endl;
        }
        unsafe = true;
    }
    else if (  !mat_new.inorganic->flags.is_set(inorganic_flags::SOIL_ANY)
             && mat_old.inorganic->flags.is_set(inorganic_flags::SOIL_ANY))
    {
        if(!warned)
        {
            out << "Changing a soil layer into stone is probably not wise." << endl
                << "The soil will remain and you only get a stone floor after digging." << endl;
        }
        unsafe = true;
    }

    if(unsafe)
    {
        if(force)
        {
            if(!warned)
            {
                out << "You've been warned, good luck." << endl;
            }
            allowed = true;
        }
        else
        {
            if(!warned)
            {
                out << "Use the option 'force' if you REALLY want to do that." << endl
                    << "Weird things can happen with your map, so save your game before trying!" << endl
                    << "Example: 'changelayer GRANITE force'" << endl;
            }
            allowed = false;
        }
        // avoid multiple warnings for the same stuff
        warned = true;
    }
    return allowed;
}
