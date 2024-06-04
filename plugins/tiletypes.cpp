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
#include <queue>
#include <set>
#include <sstream>
#include <vector>

using std::vector;
using std::string;
using std::endl;
using std::set;

#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "DataIdentity.h"
#include "Export.h"
#include "LuaTools.h"
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

struct tiletypes_options {
    // whether to display help
    bool help = false;
    bool quiet = false;

    // if set, then use this position instead of the active game cursor
    df::coord cursor;

    static struct_identity _identity;
};
static const struct_field_info tiletypes_options_fields[] = {
    { struct_field_info::PRIMITIVE, "help",   offsetof(tiletypes_options, help),   &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "quiet",  offsetof(tiletypes_options, quiet),  &df::identity_traits<bool>::identity, 0, 0 },
    { struct_field_info::SUBSTRUCT, "cursor", offsetof(tiletypes_options, cursor), &df::coord::_identity,                0, 0 },
    { struct_field_info::END }
};
struct_identity tiletypes_options::_identity(sizeof(tiletypes_options), &df::allocator_fn<tiletypes_options>, NULL, "tiletypes_options", NULL, tiletypes_options_fields);
// pair<bottomShape, topShape> -> newTopShape
static const std::map<std::pair<df::tiletype_shape, df::tiletype_shape>, df::tiletype_shape> surroundingsMap = {
    { { df::tiletype_shape::WALL, df::tiletype_shape::EMPTY    }, df::tiletype_shape::FLOOR    },
    { { df::tiletype_shape::RAMP, df::tiletype_shape::EMPTY    }, df::tiletype_shape::RAMP_TOP },
};

static const char * HISTORY_FILE = "dfhack-config/tiletypes.history";
CommandHistory tiletypes_hist;

command_result df_tiletypes (color_ostream &out, vector <string> & parameters);
command_result df_tiletypes_command (color_ostream &out, vector <string> & parameters);
command_result df_tiletypes_here (color_ostream &out, vector <string> & parameters);
command_result df_tiletypes_here_point (color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    tiletypes_hist.load(HISTORY_FILE);
    commands.push_back(PluginCommand("tiletypes", "Paints tiles of specified types onto the map.", df_tiletypes, true));
    commands.push_back(PluginCommand("tiletypes-command", "Run tiletypes commands (seperated by ' ; ')", df_tiletypes_command));
    commands.push_back(PluginCommand("tiletypes-here", "Repeat tiletypes command at cursor (with brush)", df_tiletypes_here));
    commands.push_back(PluginCommand("tiletypes-here-point", "Repeat tiletypes command at cursor (with single tile brush)", df_tiletypes_here_point));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    tiletypes_hist.save(HISTORY_FILE);
    return CR_OK;
}

void help( color_ostream & out, std::vector<std::string> &commands, int start, int end)
{
    std::string option = commands.size() > size_t(start) ? commands[start] : "";
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
            << " Designated / d: set designated flag (Only for filters)" << std::endl
            << " Hidden / h: set hidden flag" << std::endl
            << " Light / l: set light flag" << std::endl
            << " Subterranean / st: set subterranean flag" << std::endl
            << " Skyview / sv: set skyview flag" << std::endl
            << " Aquifer / aqua: set aquifer flag" << std::endl
            << " Surroundings / surr: set surroundings flag" << std::endl
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
        out << "Available designated flags (Only for filters):" << std::endl
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
            << " ANY, 0, 1, 2" << std::endl;
    }
    else if (option == "surroundings" || option == "surrounding" || option == "surr")
    {
        out << "Available surroundings flags:" << std::endl
            << " 0, 1" << std::endl;
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
    int surroundings;
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
        surroundings = 0;
        stone_material = -1;
        vein_type = inclusion_type::CLUSTER;
    }

    bool empty()
    {
        return shape == -1 && material == -1 && special == -1 && variant == -1
            && dig == -1 && hidden == -1 && light == -1 && subterranean == -1
            && skyview == -1 && aquifer == -1 && surroundings == 0
            && stone_material == -1;
    }

    inline bool matches(const df::tiletype source,
                        const df::tile_designation des,
                        const df::tile_occupancy occ,
                        const t_matpair mat) const
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
        rv &= (aquifer == -1 || (aquifer == 2) == occ.bits.heavy_aquifer);
        return rv;
    }
};

struct PaintResult {
    int paintCount = 0;
    std::function<void(MapExtras::MapCache&)> postWrite = [](MapExtras::MapCache& map) {};
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

        stream << (paint.dig ? "DESIGNATED" : "UNDESIGNATED");
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

        stream << (paint.aquifer ? (paint.aquifer == 1 ? "LIGHT AQUIFER" : "HEAVY AQUIFER") : "NO AQUIFER");
        used = true;
        needSpace = true;
    }

    if (paint.surroundings == 1)
    {
        if (needSpace)
        {
            stream << " ";
            needSpace = false;
        }

        stream << "UPDATE SURROUNDINGS";
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

bool processTileType(color_ostream & out, TileType &paint, std::vector<std::string> &params, int start, int end, bool isFilter)
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
        if (isFilter)
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
        else
        {
            out << "Option only available in filters: '" << option << "'" << std::endl;
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
        if (valInt >= -1 && valInt < 3)
        {
            paint.aquifer = valInt;
            found = true;
        }
        else
        {
            out << "Unknown aquifer flag: " << value << std::endl;
        }
    }
    else if (option == "surroundings" || option == "surrounding" || option == "surr")
    {
        if (valInt >= 0 && valInt < 2)
        {
            paint.surroundings = valInt;
            found = true;
        }
        else
        {
            out << "Unknown surroundings flag: " << value << std::endl;
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

static bool paintTileProcessing(MapExtras::Block* block, const df::coord2d& blockPos, const TileType& target) {
    df::tiletype source = block->tiletypeAt(blockPos);
    df::tile_designation des = block->DesignationAt(blockPos);

    // Stone painting operates on the base layer
    if (target.stone_material >= 0)
        source = block->baseTiletypeAt(blockPos);

    df::tiletype_shape shape = target.shape;
    if (shape == tiletype_shape::NONE)
        shape = tileShape(source);

    df::tiletype_material material = target.material;
    if (material == tiletype_material::NONE)
        material = tileMaterial(source);

    df::tiletype_special special = target.special;
    if (special == tiletype_special::NONE)
        special = tileSpecial(source);

    df::tiletype_variant variant = target.variant;
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
    if (!(material == tiletype_material::RIVER || shape == tiletype_shape::BROOK_BED || special == tiletype_special::TRACK || (shape == tiletype_shape::WALL && (material == tiletype_material::CONSTRUCTION || special == tiletype_special::SMOOTH))))
        direction.whole = 0;

    df::tiletype type = DFHack::findTileType(shape, material, variant, special, direction);
    // hack for empty space
    if (shape == tiletype_shape::EMPTY && material == tiletype_material::AIR && variant == tiletype_variant::VAR_1 && special == tiletype_special::NORMAL && direction.whole == 0)
        type = tiletype::OpenSpace;

    // make sure it's not invalid
    if (type != tiletype::Void) {
        if (target.stone_material >= 0) {
            if (!block->setStoneAt(blockPos, type, target.stone_material, target.vein_type, true, true))
                return false;
        }
        else
            block->setTiletypeAt(blockPos, type);
    }

    if (target.hidden > -1)
        des.bits.hidden = target.hidden;

    if (target.light > -1)
        des.bits.light = target.light;

    if (target.subterranean > -1)
        des.bits.subterranean = target.subterranean;

    if (target.skyview > -1)
        des.bits.outside = target.skyview;

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

    return block->setDesignationAt(blockPos, des);
}

static bool updateSurroundings(MapExtras::Block* botBlock, MapExtras::Block* topBlock, const df::coord2d& blockPos, const TileType& target) {
    if (!botBlock || !topBlock)
        return false;
    tiletype::tiletype botTiletype = botBlock->tiletypeAt(blockPos);
    tiletype::tiletype topTiletype = topBlock->tiletypeAt(blockPos);

    auto iter = surroundingsMap.find(std::make_pair(tileShape(botTiletype), tileShape(topTiletype)));
    if (iter == surroundingsMap.end())
        return false;

    df::tiletype_shape newTopShape = iter->second;
    TileType tiletype = TileType(target);
    tiletype.shape = newTopShape;
    tiletype.material = tiletype_material::NONE;
    tiletype.variant = tiletype_variant::NONE;
    tiletype.special = tiletype_special::NONE;
    tiletype.stone_material = -1;
    tiletype.aquifer = -1;
    tiletype.dig = -1;
    tiletype.surroundings = 0;
    if (paintTileProcessing(topBlock, blockPos, tiletype) && tileShape(topBlock->tiletypeAt(blockPos)) == newTopShape)
        return true;

    tiletype_material::tiletype_material topMat = tileMaterial(topTiletype);
    tiletype_material::tiletype_material botMat = tileMaterial(botTiletype);
    topMat = topMat == tiletype_material::NONE ? botMat : topMat;

    tiletype_variant::tiletype_variant topVariant = tileVariant(topTiletype);
    tiletype_variant::tiletype_variant botVariant = tileVariant(botTiletype);
    topVariant = topVariant == tiletype_variant::NONE ? botVariant : topVariant;

    tiletype_special::tiletype_special topSpecial = tileSpecial(topTiletype);
    tiletype_special::tiletype_special botSpecial = tileSpecial(botTiletype);
    topSpecial = topSpecial == tiletype_special::NONE ? botSpecial : topSpecial;
    botSpecial = botSpecial == tiletype_special::NONE ? tiletype_special::NORMAL : botSpecial;

    int topLikeness = 0;

    for (df::tiletype tt = (df::enum_traits<df::tiletype>::first_item); DFHack::is_valid_enum_item(tt); tt = DFHack::next_enum_item(tt, false))
    {
        if (newTopShape != tileShape(tt))
            continue;

        int tempLikeness = 0;
        tiletype_material::tiletype_material mat = tileMaterial(tt);
        if (mat == topMat)
            tempLikeness += 16;
        else if (mat == botMat)
            tempLikeness += 8;
        else continue;

        tiletype_variant::tiletype_variant variant = tileVariant(tt);
        if (variant == topVariant)
            tempLikeness += 2;
        else if (variant == botVariant)
            tempLikeness += 1;

        tiletype_special::tiletype_special special = tileSpecial(tt);
        if (special == topSpecial)
            tempLikeness += 8;
        else if (special == botSpecial)
            tempLikeness += 4;

        if (tempLikeness > topLikeness) {
            topLikeness = tempLikeness;
            tiletype.material = mat;
            tiletype.variant = variant;
            tiletype.special = special;
        }
    }
    return paintTileProcessing(topBlock, blockPos, tiletype);
}

static bool updateAreaSurroundings(MapExtras::MapCache& map, const df::coord& pos1, const df::coord& pos2, const TileType& botType) {
    df::coord minPos = df::coord(std::min(pos1.x, pos2.x), std::min(pos1.y, pos2.y), std::min(pos1.z, pos2.z));
    df::coord maxPos = df::coord(std::max(pos1.x, pos2.x), std::max(pos1.y, pos2.y), std::max(pos1.z, pos2.z));

    bool updated = false;
    // Loop through the affected blocks
    for (int16_t blockX = (minPos.x >> 4) << 4; blockX <= maxPos.x; blockX += 16) {
        for (int16_t blockY = (minPos.y >> 4) << 4; blockY <= maxPos.y; blockY += 16) {
            std::queue<MapExtras::Block*> blockQueue = std::queue<MapExtras::Block*>();
            blockQueue.push(map.BlockAtTile(df::coord(blockX, blockY, minPos.z - 1)));
            blockQueue.push(map.BlockAtTile(df::coord(blockX, blockY, minPos.z)));
            for (int16_t z = minPos.z; z <= maxPos.z; z++) {
                blockQueue.push(map.BlockAtTile(df::coord(blockX, blockY, minPos.z + 1)));
                MapExtras::Block* belowBlock = blockQueue.front();
                blockQueue.pop();

                int16_t startX = std::max(minPos.x - blockX, 0);
                int16_t startY = std::max(minPos.y - blockY, 0);
                int16_t endX = std::min(maxPos.x - blockX, 15);
                int16_t endY = std::min(maxPos.y - blockY, 15);
                // Loop through all tiles in the block
                for (int16_t xOffset = startX; xOffset <= endX; xOffset++) {
                    for (int16_t yOffset = startY; yOffset <= endY; yOffset++) {
                        updated |= updateSurroundings(blockQueue.front(), blockQueue.back(), df::coord2d(xOffset, yOffset), botType);
                        updated |= updateSurroundings(belowBlock, blockQueue.front(), df::coord2d(xOffset, yOffset), botType);
                    }
                }
                blockQueue.front()->enableBlockUpdates(true, true);
            }
        }
    }
    return updated;
}

static PaintResult paintArea(MapExtras::MapCache& map, const df::coord& pos1, const df::coord& pos2,
    const TileType& target, const TileType& match = TileType()) {
    df::coord minPos = df::coord(std::min(pos1.x, pos2.x), std::min(pos1.y, pos2.y), std::min(pos1.z, pos2.z));
    df::coord maxPos = df::coord(std::max(pos1.x, pos2.x), std::max(pos1.y, pos2.y), std::max(pos1.z, pos2.z));

    int totalAffectedCount = 0;
    int totalFilteredCount = 0;
    coord_vec skipList = coord_vec();

    // Loop through the affected blocks
    for (int16_t z = minPos.z; z <= maxPos.z; z++) {
        for (int16_t blockX = (minPos.x >> 4) << 4; blockX <= maxPos.x; blockX += 16) {
            for (int16_t blockY = (minPos.y >> 4) << 4; blockY <= maxPos.y; blockY += 16) {
                MapExtras::Block* block = map.BlockAtTile(df::coord(blockX, blockY, z));
                if (!block)
                    continue;

                int16_t startX = std::max(minPos.x - blockX, 0);
                int16_t startY = std::max(minPos.y - blockY, 0);
                int16_t endX = std::min(maxPos.x - blockX, 15);
                int16_t endY = std::min(maxPos.y - blockY, 15);
                // Loop through the affected tiles in the block
                for (int16_t xOffset = startX; xOffset <= endX; xOffset++) {
                    for (int16_t yOffset = startY; yOffset <= endY; yOffset++) {
                        df::coord2d blockOffset = df::coord2d(xOffset, yOffset);

                        df::tiletype source = block->tiletypeAt(blockOffset);
                        df::tile_designation des = block->DesignationAt(blockOffset);
                        df::tile_occupancy occ = block->OccupancyAt(blockOffset);

                        // Stone painting operates on the base layer
                        if (target.stone_material >= 0)
                            source = block->baseTiletypeAt(blockOffset);

                        t_matpair basemat = block->baseMaterialAt(blockOffset);

                        if (!match.matches(source, des, occ, basemat)) {
                            totalFilteredCount++;
                            skipList.push_back(df::coord(blockX + xOffset, blockY + yOffset, z));
                            continue;
                        }

                        if (paintTileProcessing(block, blockOffset, target)) {
                            totalAffectedCount++;
                        }
                        else {
                            skipList.push_back(df::coord(blockX + xOffset, blockY + yOffset, z));
                            continue;
                        }
                    }
                }
            }
        }
    }

    return PaintResult{
        .paintCount = totalAffectedCount + totalFilteredCount,
        .postWrite = [totalAffectedCount, skipList, target, pos1, pos2](MapExtras::MapCache& map) {
            if (totalAffectedCount > 0) {
                auto filter = [skipList](df::coord pos, df::map_block* block) -> bool {
                    // Returns true if 'pos' is in 'skipList'
                    return skipList.end() == std::find_if(skipList.begin(), skipList.end(), [pos](df::coord p) { return p.x == pos.x && p.y == pos.y && p.z == pos.z; });
                };

                if (target.aquifer == 0)
                    Maps::removeAreaAquifer(pos1, pos2, filter);
                else if (target.aquifer > 0)
                    Maps::setAreaAquifer(pos1, pos2, target.aquifer == 2, filter);

                if (target.surroundings > 0 && updateAreaSurroundings(map, pos1, pos2, target))
                    map.WriteAll();
            }
        }
    };
}

static PaintResult paintTile(MapExtras::MapCache &map, const df::coord &pos,
                      const TileType &target, const TileType &match = TileType()) {
    MapExtras::Block *blk = map.BlockAtTile(pos);
    if (!blk)
        return PaintResult();

    df::tiletype source = map.tiletypeAt(pos);
    df::tile_designation des = map.designationAt(pos);
    df::tile_occupancy occ = map.occupancyAt(pos);

    // Stone painting operates on the base layer
    if (target.stone_material >= 0)
        source = blk->baseTiletypeAt(pos);

    t_matpair basemat = blk->baseMaterialAt(pos);

    if (!match.matches(source, des, occ, basemat))
        return PaintResult();

    if (paintTileProcessing(blk, df::coord2d(pos.x&15, pos.y&15), target)) {
        return PaintResult{
            .paintCount = 1,
            .postWrite = [target, pos](MapExtras::MapCache& map) {
                if (target.aquifer == 0)
                    Maps::removeTileAquifer(pos);
                else if (target.aquifer > 0)
                    Maps::setTileAquifer(pos, target.aquifer == 2);

                if (target.surroundings > 0) {
                    MapExtras::Block* block = map.BlockAtTile(pos);
                    MapExtras::Block* topBlock = map.BlockAtTile(df::coord(pos.x, pos.y, pos.z + 1));
                    MapExtras::Block* belowBlock = map.BlockAtTile(df::coord(pos.x, pos.y, pos.z - 1));
                    bool updated = updateSurroundings(block, topBlock, df::coord2d(pos.x & 15, pos.y & 15), target);
                    updated |= updateSurroundings(belowBlock, block, df::coord2d(pos.x & 15, pos.y & 15), target);
                    block->enableBlockUpdates(true, true);
                    if (updated)
                        map.WriteAll();
                }
            }
        };
    }
    return PaintResult();
}

command_result executePaintJob(color_ostream &out,
                               const tiletypes_options &opts)
{
    if (paint.empty())
    {
        out.printerr("Set the paint first.\n");
        return CR_OK;
    }

    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    uint32_t x_max = 0, y_max = 0, z_max = 0;
    Maps::getSize(x_max, y_max, z_max);

    df::coord cursor;
    if (Maps::isValidTilePos(opts.cursor))
    {
        cursor = opts.cursor;
    }
    else
    {
        cursor = Gui::getCursorPos();
        if (!cursor.isValid())
        {
            out.printerr("Can't get cursor coords! Make sure you have a cursor active in DF or specify the --cursor option.\n");
            return CR_FAILURE;
        }
    }

    if (!opts.quiet)
        out.print("Cursor coords: (%d, %d, %d)\n",
                  cursor.x, cursor.y, cursor.z);

    MapExtras::MapCache map;
    coord_vec all_tiles = brush->points(map, cursor);
    if (!opts.quiet)
        out.print("working...\n");

    int failures = 0;
    std::vector<PaintResult> paintResults = std::vector<PaintResult>();

    if (all_tiles.size() > 0) {
        // Force the game to recompute its walkability cache
        world->reindex_pathfinding = true;

        if (dynamic_cast<RectangleBrush*>(brush) != nullptr || dynamic_cast<BlockBrush*>(brush) != nullptr || dynamic_cast<ColumnBrush*>(brush) != nullptr) {
            df::coord minPos = all_tiles[0];
            df::coord maxPos = all_tiles[0];
            for (df::coord& tile : all_tiles) {
                minPos.x = std::min(minPos.x, tile.x);
                minPos.y = std::min(minPos.y, tile.y);
                minPos.z = std::min(minPos.z, tile.z);
                maxPos.x = std::max(maxPos.x, tile.x);
                maxPos.y = std::max(maxPos.y, tile.y);
                maxPos.z = std::max(maxPos.z, tile.z);
            }
            PaintResult result = paintArea(map, minPos, maxPos, paint, filter);
            paintResults.push_back(result);
            failures = all_tiles.size() - result.paintCount;
        }
        else {
            for (coord_vec::iterator iter = all_tiles.begin(); iter != all_tiles.end(); ++iter)
            {
                PaintResult result = paintTile(map, *iter, paint, filter);
                if (result.paintCount == 0)
                    ++failures;
                else
                    paintResults.push_back(result);
            }
        }
    }

    if (failures > 0)
        out.printerr("Could not update %d tiles of %zu.\n", failures, all_tiles.size());
    else if (!opts.quiet)
        out.print("Processed %zu tiles.\n", all_tiles.size());

    if (map.WriteAll())
    {
        for (PaintResult& result : paintResults) {
            result.postWrite(map);
        }
        if (!opts.quiet)
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
    tiletypes_options opts;
    if (commands.size() == size_t(start))
    {
        return executePaintJob(out, opts);
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
        processTileType(out, filter, commands, loc, end, true);
    }
    else if (command == "paint" || (command == "p" && commands.size() > 1))
    {
        processTileType(out, paint, commands, loc, end, false);
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
        executePaintJob(out, opts);
    }

    return CR_OK;
}

command_result df_tiletypes (color_ostream &out_, vector <string> & parameters)
{
    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            return CR_WRONG_USAGE;
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
        int rv = 0;

        while ((rv = out.lineedit("tiletypes> ",input,tiletypes_hist))
                == Console::RETRY);
        if (rv <= Console::FAILURE)
            return rv == Console::FAILURE ? CR_FAILURE : CR_OK;
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
    CoreSuspender suspend;

    tiletypes_options opts;
    if (!Lua::CallLuaModuleFunction(out, "plugins.tiletypes", "parse_commandline", std::make_tuple(&opts, parameters))
            || opts.help)
    {
        out << "This command is supposed to be mapped to a hotkey." << endl;
        out << "It will use the current/last parameters set in tiletypes (including brush settings!)." << endl;
        return opts.help ? CR_OK : CR_FAILURE;
    }

    if (!opts.quiet)
    {
        out.print("Run tiletypes-here with these parameters: ");
        printState(out);
    }

    return executePaintJob(out, opts);
}

command_result df_tiletypes_here_point (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    tiletypes_options opts;
    if (!Lua::CallLuaModuleFunction(out, "plugins.tiletypes", "parse_commandline", std::make_tuple(&opts, parameters))
            || opts.help)
    {
        out << "This command is supposed to be mapped to a hotkey." << endl;
        out << "It will use the current/last parameters set in tiletypes (except with a point brush)." << endl;
        return opts.help ? CR_OK : CR_FAILURE;
    }

    Brush *old = brush;
    brush = new RectangleBrush(1, 1);

    if (!opts.quiet)
    {
        out.print("Run tiletypes-here-point with these parameters: ");
        printState(out);
    }

    command_result rv = executePaintJob(out, opts);

    delete brush;
    brush = old;
    return rv;
}

static bool setTile(color_ostream& out, df::coord pos, TileType target) {
    if (!Maps::isValidTilePos(pos)) {
        out.printerr("Invalid map position: %d, %d, %d\n", pos.x, pos.y, pos.z);
        return false;
    }

    if (!is_valid_enum_item(target.shape)) {
        out.printerr("Invalid shape type: %d\n", target.shape);
        return false;
    }
    if (!is_valid_enum_item(target.material)) {
        out.printerr("Invalid material type: %d\n", target.material);
        return false;
    }
    if (!is_valid_enum_item(target.special)) {
        out.printerr("Invalid special type: %d\n", target.special);
        return false;
    }
    if (!is_valid_enum_item(target.variant)) {
        out.printerr("Invalid variant type: %d\n", target.variant);
        return false;
    }
    if (target.hidden < -1 || target.hidden > 1) {
        out.printerr("Invalid hidden value: %d\n", target.hidden);
        return false;
    }
    if (target.light < -1 || target.light > 1) {
        out.printerr("Invalid light value: %d\n", target.light);
        return false;
    }
    if (target.subterranean < -1 || target.subterranean > 1) {
        out.printerr("Invalid subterranean value: %d\n", target.subterranean);
        return false;
    }
    if (target.skyview < -1 || target.skyview > 1) {
        out.printerr("Invalid skyview value: %d\n", target.skyview);
        return false;
    }
    if (target.aquifer < -1 || target.aquifer > 2) {
        out.printerr("Invalid aquifer value: %d\n", target.aquifer);
        return false;
    }
    if (target.surroundings < 0 || target.surroundings > 1) {
        out.printerr("Invalid surroundings value: %d\n", target.surroundings);
        return false;
    }
    if (target.material == df::tiletype_material::STONE) {
        if (target.stone_material != -1 && !isStoneInorganic(target.stone_material)) {
            out.printerr("Invalid stone material: %d\n", target.stone_material);
            return false;
        }
        if (!is_valid_enum_item(target.vein_type)) {
            out.printerr("Invalid vein type: %d\n", target.vein_type);
            return false;
        }
    }
    else {
        target.stone_material = -1;
        target.vein_type = df::inclusion_type::CLUSTER;
    }

    MapExtras::MapCache map;
    PaintResult result = paintTile(map, pos, target);
    if (result.paintCount > 0 && map.WriteAll()) {
        result.postWrite(map);
        return true;
    }
    return false;
}

static int tiletypes_setTile(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();

    TileType target = TileType();

    df::coord pos;
    Lua::CheckDFAssign(L, &pos, 1);
    if (lua_istable(L, 2)) {
        auto lua_getintfield = [L](const char* fieldName, int defaultValue) -> int {
            lua_getfield(L, 2, fieldName);
            int output = defaultValue;
            if (!lua_isnil(L, -1)) {
                output = lua_tointeger(L, -1);
            }
            lua_pop(L, 1);
            return output;
        };

        target.shape     = (df::tiletype_shape)lua_getintfield("shape", target.shape);
        target.material  = (df::tiletype_material)lua_getintfield("material", target.material);
        target.special   = (df::tiletype_special)lua_getintfield("special", target.special);
        target.variant   = (df::tiletype_variant)lua_getintfield("variant", target.variant);
        target.hidden         = lua_getintfield("hidden", target.hidden);
        target.light          = lua_getintfield("light", target.light);
        target.subterranean   = lua_getintfield("subterranean", target.subterranean);
        target.skyview        = lua_getintfield("skyview", target.skyview);
        target.aquifer        = lua_getintfield("aquifer", target.aquifer);
        target.surroundings   = lua_getintfield("surroundings", target.surroundings);
        if (target.material == df::tiletype_material::STONE) {
            target.stone_material = lua_getintfield("stone_material", target.stone_material);
            target.vein_type = (df::inclusion_type)lua_getintfield("vein_type", target.vein_type);
        }
    }
    else {
        target.shape = (df::tiletype_shape)lua_tointeger(L, 2);
        target.material = (df::tiletype_material)lua_tointeger(L, 3);
        target.special = (df::tiletype_special)lua_tointeger(L, 4);
        target.variant = (df::tiletype_variant)lua_tointeger(L, 5);
    }

    Lua::Push(L, setTile(*out, pos, target));
    return 1;
}

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(tiletypes_setTile),
    DFHACK_LUA_END
};
