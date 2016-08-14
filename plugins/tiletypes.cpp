// Plugin tiletypes
//
// This plugin allows fine editing of individual game tiles (expect for
// changing material subtypes).
//
// Commands:
// tiletypes            - runs the interractive interpreter
// tiletypes-command    - run the given command
//                        (intended to be mapped to a hotkey or used from dfhack-run)
// tiletypes-here       - runs the execute method with the last settings from
//                        tiletypes(-command), including brush!
//                        (intended to be mapped to a hotkey)
// tiletypes-here-point - runs the execute method with the last settings from
//                        tiletypes(-command), except with a point brush!
//                        (intended to be mapped to a hotkey)
// Options (everything but tiletypes-command):
// ?, help        - print some help
//
// Options (tiletypes-command):
// (anything) - run the given command

#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>

using std::vector;
using std::string;
using std::endl;
using std::set;

#include "Console.h"
#include "Core.h"
#include "Export.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Gui.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"

#include "df/tile_dig_designation.h"
#include "df/world.h"

#include "Brushes.h"

using namespace MapExtras;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("tiletypes");
REQUIRE_GLOBAL(world);

CommandHistory tiletypes_hist;

command_result df_tiletypes (color_ostream &out, vector <string> & parameters);
command_result df_tiletypes_command (color_ostream &out, vector <string> & parameters);
command_result df_tiletypes_here (color_ostream &out, vector <string> & parameters);
command_result df_tiletypes_here_point (color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    tiletypes_hist.load("tiletypes.history");
    commands.push_back(PluginCommand("tiletypes", "Paint map tiles freely, similar to liquids.", df_tiletypes, true));
    commands.push_back(PluginCommand("tiletypes-command", "Run tiletypes commands (seperated by ' ; ')", df_tiletypes_command));
    commands.push_back(PluginCommand("tiletypes-here", "Repeat tiletypes command at cursor (with brush)", df_tiletypes_here));
    commands.push_back(PluginCommand("tiletypes-here-point", "Repeat tiletypes command at cursor (without brush)", df_tiletypes_here_point));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    tiletypes_hist.save("tiletypes.history");
    return CR_OK;
}

void help( color_ostream & out, std::vector<std::string> &commands, int start, int end)
{
    std::string option = commands.size() > start ? commands[start] : "";
    if (option.empty())
    {
        out << "Commands:" << std::endl
            << " quit / q              : quit" << std::endl
            << " filter / f [options]  : change filter options" << std::endl
            << " paint / p [options]   : change paint options" << std::endl
            << " point / p             : set point brush" << std::endl
            << " range / r [w] [h] [z] : set range brush" << std::endl
            << " block                 : set block brush" << std::endl
            << " column                : set column brush" << std::endl
            << " run / (empty)         : paint!" << std::endl
            << std::endl
            << "Filter/paint options:" << std::endl
            << " Any: reset to default (no filter/paint)" << std::endl
            << " Shape / sh / s: set tile shape information" << std::endl
            << " Material / mat / m: set tile material information" << std::endl
            << " Special / sp: set special tile information" << std::endl
            << " Variant / var / v: set variant tile information" << std::endl
            << " All / a: set the four above at the same time (no ANY support)" << std::endl
            << " Designated / d: set designated flag" << std::endl
            << " Hidden / h: set hidden flag" << std::endl
            << " Light / l: set light flag" << std::endl
            << " Subterranean / st: set subterranean flag" << std::endl
            << " Skyview / sv: set skyview flag" << std::endl
            << " Aquifer / aqua: set aquifer flag" << std::endl
            << " Stone: paint specific stone material" << std::endl
            << " Veintype: use specific vein type for stone" << std::endl
            << "See help [option] for more information" << std::endl;
    }
    else if (option == "shape" || option == "s" ||option == "sh")
    {
        out << "Available shapes:" << std::endl
            << " ANY" << std::endl;
        FOR_ENUM_ITEMS(tiletype_shape,i)
        {
            out << " " << ENUM_KEY_STR(tiletype_shape,i) << std::endl;
        }
    }
    else if (option == "material"|| option == "mat" ||option == "m")
    {
        out << "Available materials:" << std::endl
            << " ANY" << std::endl;
        FOR_ENUM_ITEMS(tiletype_material,i)
        {
            out << " " << ENUM_KEY_STR(tiletype_material,i) << std::endl;
        }
    }
    else if (option == "special" || option == "sp")
    {
        out << "Available specials:" << std::endl
            << " ANY" << std::endl;
        FOR_ENUM_ITEMS(tiletype_special,i)
        {
            out << " " << ENUM_KEY_STR(tiletype_special,i) << std::endl;
        }
    }
    else if (option == "variant" || option == "var" || option == "v")
    {
        out << "Available variants:" << std::endl
            << " ANY" << std::endl;
        FOR_ENUM_ITEMS(tiletype_variant,i)
        {
            out << " " << ENUM_KEY_STR(tiletype_variant,i) << std::endl;
        }
    }
    else if (option == "designated" || option == "d")
    {
        out << "Available designated flags:" << std::endl
            << " ANY, 0, 1" << std::endl;
    }
    else if (option == "hidden" || option == "h")
    {
        out << "Available hidden flags:" << std::endl
            << " ANY, 0, 1" << std::endl;
    }
    else if (option == "light" || option == "l")
    {
        out << "Available light flags:" << std::endl
            << " ANY, 0, 1" << std::endl;
    }
    else if (option == "subterranean" || option == "st")
    {
        out << "Available subterranean flags:" << std::endl
            << " ANY, 0, 1" << std::endl;
    }
    else if (option == "skyview" || option == "sv")
    {
        out << "Available skyview flags:" << std::endl
            << " ANY, 0, 1" << std::endl;
    }
    else if (option == "aquifer" || option == "aqua")
    {
        out << "Available aquifer flags:" << std::endl
            << " ANY, 0, 1" << std::endl;
    }
    else if (option == "stone")
    {
        out << "The stone option allows painting any specific stone material." << std::endl
            << "The normal 'material' option is forced to STONE, and cannot" << std::endl
            << "be changed without cancelling the specific stone selection." << std::endl
            << "Note: this feature paints under ice and constructions," << std::endl
            << "instead of replacing them with brute force." << std::endl;
    }
    else if (option == "veintype")
    {
        out << "Specifies which vein type to use when painting specific stone." << std::endl
            << "The vein type determines stone drop rate. Available types:" << std::endl;
        FOR_ENUM_ITEMS(inclusion_type,i)
            out << " " << ENUM_KEY_STR(inclusion_type,i) << std::endl;
        out << "Vein type other than CLUSTER forces creation of a vein." << std::endl;
    }
}

struct TileType
{
    df::tiletype_shape shape;
    df::tiletype_material material;
    df::tiletype_special special;
    df::tiletype_variant variant;
    int dig;
    int hidden;
    int light;
    int subterranean;
    int skyview;
    int aquifer;
    int stone_material;
    df::inclusion_type vein_type;

    TileType()
    {
        clear();
    }

    void clear()
    {
        shape = tiletype_shape::NONE;
        material = tiletype_material::NONE;
        special = tiletype_special::NONE;
        variant = tiletype_variant::NONE;
        dig = -1;
        hidden = -1;
        light = -1;
        subterranean = -1;
        skyview = -1;
        aquifer = -1;
        stone_material = -1;
        vein_type = inclusion_type::CLUSTER;
    }

    bool empty()
    {
        return shape == -1 && material == -1 && special == -1 && variant == -1
            && dig == -1 && hidden == -1 && light == -1 && subterranean == -1
            && skyview == -1 && aquifer == -1 && stone_material == -1;
    }

    inline bool matches(const df::tiletype source,
                        const df::tile_designation des,
                        const t_matpair mat)
    {
        bool rv = true;
        rv &= (shape == -1 || shape == tileShape(source));
        if (stone_material >= 0)
            rv &= isStoneMaterial(source) && mat.mat_type == 0 && mat.mat_index == stone_material;
        else
            rv &= (material == -1 || material == tileMaterial(source));
        rv &= (special == -1 || special == tileSpecial(source));
        rv &= (variant == -1 || variant == tileVariant(source));
        rv &= (dig == -1 || (dig != 0) == (des.bits.dig != tile_dig_designation::No));
        rv &= (hidden == -1 || (hidden != 0) == des.bits.hidden);
        rv &= (light == -1 || (light != 0) == des.bits.light);
        rv &= (subterranean == -1 || (subterranean != 0) == des.bits.subterranean);
        rv &= (skyview == -1 || (skyview != 0) == des.bits.outside);
        rv &= (aquifer == -1 || (aquifer != 0) == des.bits.water_table);
        return rv;
    }
};

std::ostream &operator<<(std::ostream &stream, const TileType &paint)
{
    bool used = false;
    bool needSpace = false;

    if (paint.special >= 0)
    {
        stream << ENUM_KEY_STR(tiletype_special,paint.special);
        used = true;
        needSpace = true;
    }

    if (paint.material >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << ENUM_KEY_STR(tiletype_material,paint.material);
        used = true;
        needSpace = true;
    }

    if (paint.shape >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << ENUM_KEY_STR(tiletype_shape,paint.shape);
        used = true;
        needSpace = true;
    }

    if (paint.variant >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << ENUM_KEY_STR(tiletype_variant,paint.variant);
        used = true;
        needSpace = true;
    }

    if (paint.dig >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << (paint.dig ? "DESIGNATED" : "UNDESIGATNED");
        used = true;
        needSpace = true;
    }

    if (paint.hidden >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << (paint.hidden ? "HIDDEN" : "VISIBLE");
        used = true;
        needSpace = true;
    }

    if (paint.light >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << (paint.light ? "LIGHT" : "DARK");
        used = true;
        needSpace = true;
    }

    if (paint.subterranean >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << (paint.subterranean ? "SUBTERRANEAN" : "ABOVE GROUND");
        used = true;
        needSpace = true;
    }

    if (paint.skyview >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << (paint.skyview ? "OUTSIDE" : "INSIDE");
        used = true;
        needSpace = true;
    }

    if (paint.aquifer >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << (paint.aquifer ? "AQUIFER" : "NO AQUIFER");
        used = true;
        needSpace = true;
    }

    if (paint.stone_material >= 0)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << MaterialInfo(0,paint.stone_material).getToken()
               << " " << ENUM_KEY_STR(inclusion_type, paint.vein_type);
        used = true;
        needSpace = true;
    }

    if (!used)
    {
        stream << "any";
    }

    return stream;
}

static TileType filter, paint;
static Brush *brush = new RectangleBrush(1,1);

void printState(color_ostream &out)
{
    out << "Filter: " << filter << std::endl
        << "Paint: "  << paint  << std::endl
        << "Brush: "  << brush << std::endl;
}

//zilpin: These two functions were giving me compile errors in VS2008, so I cheated with the C style loop below, just to get it to build.
//Original code is commented out.
void tolower(std::string &str)
{
    //The C++ way...
    //std::transform(str.begin(), str.end(), str.begin(), std::bind2nd(std::ptr_fun(&std::tolower<char> ), std::locale("")));

    //The C way...
    for(char *c=(char *)str.c_str(); *c; ++c)
    {
        *c = tolower(*c);
    }
}

void toupper(std::string &str)
{
    //std::transform(str.begin(), str.end(), str.begin(), std::bind2nd(std::ptr_fun(&std::toupper<char>), std::locale("")));
    for(char *c=(char *)str.c_str(); *c; ++c)
    {
        *c = toupper(*c);
    }
}

int toint(const std::string &str, int failValue = 0)
{
    std::istringstream ss(str);
    int valInt;
    ss >> valInt;
    if (ss.fail())
    {
        return failValue;
    }
    return valInt;
}

bool tryShape(std::string value, TileType &paint)
{
    FOR_ENUM_ITEMS(tiletype_shape,i)
    {
        if (value == ENUM_KEY_STR(tiletype_shape,i))
        {
            paint.shape = i;
            return true;
        }
    }
    return false;
}

bool tryMaterial(std::string value, TileType &paint)
{
    FOR_ENUM_ITEMS(tiletype_material, i)
    {
        if (value == ENUM_KEY_STR(tiletype_material,i))
        {
            paint.material = i;
            return true;
        }
    }
    return false;
}

bool trySpecial(std::string value, TileType &paint)
{
    FOR_ENUM_ITEMS(tiletype_special, i)
    {
        if (value == ENUM_KEY_STR(tiletype_special,i))
        {
            paint.special = i;
            return true;
        }
    }
    return false;
}

bool tryVariant(std::string value, TileType &paint)
{
    FOR_ENUM_ITEMS(tiletype_variant, i)
    {
        if (value == ENUM_KEY_STR(tiletype_variant,i))
        {
            paint.variant = i;
            return true;
        }
    }
    return false;
}

bool processTileType(color_ostream & out, TileType &paint, std::vector<std::string> &params, int start, int end)
{
    if (start == end)
    {
        out << "Missing argument." << std::endl;
        return false;
    }

    int loc = start;
    std::string option = params[loc++];
    std::string value = end <= loc ? "" : params[loc++];
    tolower(option);
    toupper(value);
    int valInt;
    if (value == "ANY")
    {
        valInt = -1;
    }
    else
    {
        valInt = toint(value, -2);
    }
    bool found = false;

    if (option == "any")
    {
        paint.clear();
    }
    else if (option == "shape" || option == "sh" || option == "s")
    {
        if (is_valid_enum_item((df::tiletype_shape)valInt))
        {
            paint.shape = (df::tiletype_shape)valInt;
            found = true;
        }
        else
        {
            if (!tryShape(value, paint))
            {
                out << "Unknown tile shape: " << value << std::endl;
            }
        }
    }
    else if (option == "material" || option == "mat" || option == "m")
    {
        // Setting the material conflicts with stone_material
        paint.stone_material = -1;

        if (is_valid_enum_item((df::tiletype_material)valInt))
        {
            paint.material = (df::tiletype_material)valInt;
            found = true;
        }
        else
        {
            if (!tryMaterial(value, paint))
            {
                out << "Unknown tile material: " << value << std::endl;
            }
        }
    }
    else if (option == "special" || option == "sp")
    {
        if (is_valid_enum_item((df::tiletype_special)valInt))
        {
            paint.special = (df::tiletype_special)valInt;
            found = true;
        }
        else
        {
            if (!trySpecial(value, paint))
            {
                out << "Unknown tile special: " << value << std::endl;
            }
        }
    }
    else if (option == "variant" || option == "var" || option == "v")
    {
        if (is_valid_enum_item((df::tiletype_variant)valInt))
        {
            paint.variant = (df::tiletype_variant)valInt;
            found = true;
        }
        else
        {
            if (!tryVariant(value, paint))
            {
                out << "Unknown tile variant: " << value << std::endl;
            }
        }
    }
    else if (option == "designated" || option == "d")
    {
        if (valInt >= -1 && valInt < 2)
        {
            paint.dig = valInt;
            found = true;
        }
        else
        {
            out << "Unknown designation flag: " << value << std::endl;
        }
    }
    else if (option == "hidden" || option == "h")
    {
        if (valInt >= -1 && valInt < 2)
        {
            paint.hidden = valInt;
            found = true;
        }
        else
        {
            out << "Unknown hidden flag: " << value << std::endl;
        }
    }
    else if (option == "light" || option == "l")
    {
        if (valInt >= -1 && valInt < 2)
        {
            paint.light = valInt;
            found = true;
        }
        else
        {
            out << "Unknown light flag: " << value << std::endl;
        }
    }
    else if (option == "subterranean" || option == "st")
    {
        if (valInt >= -1 && valInt < 2)
        {
            paint.subterranean = valInt;
            found = true;
        }
        else
        {
            out << "Unknown subterranean flag: " << value << std::endl;
        }
    }
    else if (option == "skyview" || option == "sv")
    {
        if (valInt >= -1 && valInt < 2)
        {
            paint.skyview = valInt;
            found = true;
        }
        else
        {
            out << "Unknown skyview flag: " << value << std::endl;
        }
    }
    else if (option == "aquifer" || option == "aqua")
    {
        if (valInt >= -1 && valInt < 2)
        {
            paint.aquifer = valInt;
            found = true;
        }
        else
        {
            out << "Unknown aquifer flag: " << value << std::endl;
        }
    }
    else if (option == "all" || option == "a")
    {
        loc--;
        for (; loc < end; loc++)
        {
            std::string param = params[loc];
            toupper(param);

            if (!(tryShape(param, paint) || tryMaterial(param, paint) ||
                         trySpecial(param, paint) || tryVariant(param, paint)))
            {
                out << "Unknown description: '" << param << "'" << std::endl;
                break;
            }
        }

        found = true;
    }
    else if (option == "stone")
    {
        MaterialInfo mat;

        if (!mat.findInorganic(value))
            out << "Unknown inorganic material: " << value << std::endl;
        else if (!isStoneInorganic(mat.index))
            out << "Not a stone material: " << value << std::endl;
        else
        {
            paint.material = tiletype_material::STONE;
            paint.stone_material = mat.index;
        }
    }
    else if (option == "veintype")
    {
        if (!find_enum_item(&paint.vein_type, value))
            out << "Unknown vein type: " << value << std::endl;
    }
    else
    {
        out << "Unknown option: '" << option << "'" << std::endl;
    }

    return found;
}

command_result executePaintJob(color_ostream &out)
{
    if (paint.empty())
    {
        out.printerr("Set the paint first.\n");
        return CR_OK;
    }

    CoreSuspender suspend;
    uint32_t x_max = 0, y_max = 0, z_max = 0;
    int32_t x = 0, y = 0, z = 0;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    Maps::getSize(x_max, y_max, z_max);

    if (!Gui::getCursorCoords(x,y,z))
    {
        out.printerr("Can't get cursor coords! Make sure you have a cursor active in DF.\n");
        return CR_FAILURE;
    }
    out.print("Cursor coords: (%d, %d, %d)\n", x, y, z);

    DFHack::DFCoord cursor(x,y,z);
    MapExtras::MapCache map;
    coord_vec all_tiles = brush->points(map, cursor);
    out.print("working...\n");

    // Force the game to recompute its walkability cache
    world->reindex_pathfinding = true;

    int failures = 0;

    for (coord_vec::iterator iter = all_tiles.begin(); iter != all_tiles.end(); ++iter)
    {
        MapExtras::Block *blk = map.BlockAtTile(*iter);
        if (!blk)
            continue;

        df::tiletype source = map.tiletypeAt(*iter);
        df::tile_designation des = map.designationAt(*iter);

        // Stone painting operates on the base layer
        if (paint.stone_material >= 0)
            source = blk->baseTiletypeAt(*iter);

        t_matpair basemat = blk->baseMaterialAt(*iter);

        if (!filter.matches(source, des, basemat))
        {
            continue;
        }

        df::tiletype_shape shape = paint.shape;
        if (shape == tiletype_shape::NONE)
        {
            shape = tileShape(source);
        }

        df::tiletype_material material = paint.material;
        if (material == tiletype_material::NONE)
        {
            material = tileMaterial(source);
        }

        df::tiletype_special special = paint.special;
        if (special == tiletype_special::NONE)
        {
            special = tileSpecial(source);
        }
        df::tiletype_variant variant = paint.variant;
        /*
         * FIXME: variant should be:
         * 1. If user variant:
         * 2.   If user variant \belongs target variants
         * 3.     use user variant
         * 4.   Else
         * 5.     use variant 0
         * 6. If the source variant \belongs target variants
         * 7    use source variant
         * 8  ElseIf num target shape/material variants > 1
         * 9.   pick one randomly
         * 10.Else
         * 11.  use variant 0
         *
         * The following variant check has been disabled because it's severely limiting
         * the usefullness of the tool.
         */
        /*
        if (variant == tiletype_variant::NONE)
        {
            variant = tileVariant(source);
        }
        */
        // Remove direction from directionless tiles
        DFHack::TileDirection direction = tileDirection(source);
        if (!(material == tiletype_material::RIVER || shape == tiletype_shape::BROOK_BED || special == tiletype_special::TRACK || shape == tiletype_shape::WALL && (material == tiletype_material::CONSTRUCTION || special == tiletype_special::SMOOTH)))
        {
            direction.whole = 0;
        }

        df::tiletype type = DFHack::findTileType(shape, material, variant, special, direction);
        // hack for empty space
        if (shape == tiletype_shape::EMPTY && material == tiletype_material::AIR && variant == tiletype_variant::VAR_1 && special == tiletype_special::NORMAL && direction.whole == 0)
        {
            type = tiletype::OpenSpace;
        }
        // make sure it's not invalid
        if(type != tiletype::Void)
        {
            if (paint.stone_material >= 0)
            {
                if (!blk->setStoneAt(*iter, type, paint.stone_material, paint.vein_type, true, true))
                    failures++;
            }
            else
                map.setTiletypeAt(*iter, type);
        }

        if (paint.hidden > -1)
        {
            des.bits.hidden = paint.hidden;
        }

        if (paint.light > -1)
        {
            des.bits.light = paint.light;
        }

        if (paint.subterranean > -1)
        {
            des.bits.subterranean = paint.subterranean;
        }

        if (paint.skyview > -1)
        {
            des.bits.outside = paint.skyview;
        }

        if (paint.aquifer > -1)
        {
            des.bits.water_table = paint.aquifer;
        }

        // Remove liquid from walls, etc
        if (type != (df::tiletype)-1 && !DFHack::FlowPassable(type))
        {
            des.bits.flow_size = 0;
            //des.bits.liquid_type = DFHack::liquid_water;
            //des.bits.water_table = 0;
            des.bits.flow_forbid = 0;
            //des.bits.liquid_static = 0;
            //des.bits.water_stagnant = 0;
            //des.bits.water_salt = 0;
        }

        map.setDesignationAt(*iter, des);
    }

    if (failures > 0)
        out.printerr("Could not update %d tiles of %d.\n", failures, all_tiles.size());
    else
        out.print("Processed %d tiles.\n", all_tiles.size());

    if (map.WriteAll())
    {
        out.print("OK\n");
        return CR_OK;
    }
    else
    {
        out.printerr("Something failed horribly! RUN!\n");
        return CR_FAILURE;
    }
}

command_result processCommand(color_ostream &out, std::vector<std::string> &commands, int start, int end, bool & endLoop, bool hasConsole = false)
{
    if (commands.size() == start)
    {
        return executePaintJob(out);
    }

    int loc = start;

    std::string command = commands[loc++];
    tolower(command);

    if (command == "help" || command == "?")
    {
        help(out, commands, loc, end);
    }
    else if (command == "quit" || command == "q")
    {
        endLoop = true;
    }
    else if (command == "filter" || command == "f")
    {
        processTileType(out, filter, commands, loc, end);
    }
    else if (command == "paint" || (command == "p" && commands.size() > 1))
    {
        processTileType(out, paint, commands, loc, end);
    }
    else if (command == "point" || command == "p")
    {
        delete brush;
        brush = new RectangleBrush(1,1);
    }
    else if (command == "range" || command == "r")
    {
        int width = 1, height = 1, zLevels = 1;

        command_result res = parseRectangle(out, commands, loc, end,
                                            width, height, zLevels, hasConsole);
        if (res != CR_OK)
        {
            return res;
        }

        delete brush;
        brush = new RectangleBrush(width, height, zLevels, 0, 0, 0);
    }
    else if (command == "block")
    {
        delete brush;
        brush = new BlockBrush();
    }
    else if (command == "column")
    {
        delete brush;
        brush = new ColumnBrush();
    }
    else if (command == "run" || command.empty())
    {
        executePaintJob(out);
    }

    return CR_OK;
}

command_result df_tiletypes (color_ostream &out_, vector <string> & parameters)
{
    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out_.print("This tool allows painting tiles types with a brush, using an optional filter.\n"
                       "The tool is interactive, similarly to the liquids tool.\n"
                       "Further help is available inside.\n"
            );
            return CR_OK;
        }
    }

    if(!out_.is_console())
        return CR_FAILURE;
    Console &out = static_cast<Console&>(out_);

    std::vector<std::string> commands;
    bool end = false;
    out << "Welcome to the tiletype tool.\nType 'help' or '?' for a list of available commands, 'q' to quit.\nPress return after a command to confirm." << std::endl;
    out.printerr("THIS TOOL CAN BE DANGEROUS. YOU'VE BEEN WARNED.\n");
    while (!end)
    {
        printState(out);

        std::string input = "";

        if (out.lineedit("tiletypes> ",input,tiletypes_hist) == -1)
            return CR_FAILURE;
        tiletypes_hist.add(input);

        commands.clear();
        Core::cheap_tokenise(input, commands);

        command_result ret = processCommand(out, commands, 0, commands.size(), end, true);

        if (ret != CR_OK)
        {
            return ret;
        }
    }
    return CR_OK;
}

command_result df_tiletypes_command (color_ostream &out, vector <string> & parameters)
{
    bool dummy;
    int start = 0, end = 0;

    parameters.push_back(";");
    for (size_t i = 0; i < parameters.size();i++)
    {
        if (parameters[i] == ";") {
            command_result rv = processCommand(out, parameters, start, end, dummy);
            if (rv != CR_OK) {
                return rv;
            }
            end++;
            start = end;
        } else {
            end++;
        }
    }

    return CR_OK;
}

command_result df_tiletypes_here (color_ostream &out, vector <string> & parameters)
{
    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out << "This command is supposed to be mapped to a hotkey." << endl;
            out << "It will use the current/last parameters set in tiletypes (including brush settings!)." << endl;
            return CR_OK;
        }
    }

    out.print("Run tiletypes-here with these parameters: ");
    printState(out);

    return executePaintJob(out);
}

command_result df_tiletypes_here_point (color_ostream &out, vector <string> & parameters)
{
    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out << "This command is supposed to be mapped to a hotkey." << endl;
            out << "It will use the current/last parameters set in tiletypes (except with a point brush)." << endl;
            return CR_OK;
        }
    }

    Brush *old = brush;
    brush = new RectangleBrush(1, 1);

    out.print("Run tiletypes-here with these parameters: ");
    printState(out);

    command_result rv = executePaintJob(out);

    delete brush;
    brush = old;
    return rv;
}
