#include <vector>
#include <cstdio>
#include <cstdlib>
#include <stack>
#include <string>
#include <cmath>

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "uicommon.h"

#include "modules/Gui.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/Materials.h"

#include "df/ui_sidebar_menus.h"

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
using namespace df::enums;

command_result digv (color_ostream &out, vector <string> & parameters);
command_result digvx (color_ostream &out, vector <string> & parameters);
command_result digl (color_ostream &out, vector <string> & parameters);
command_result diglx (color_ostream &out, vector <string> & parameters);
command_result digauto (color_ostream &out, vector <string> & parameters);
command_result digexp (color_ostream &out, vector <string> & parameters);
command_result digcircle (color_ostream &out, vector <string> & parameters);
command_result digtype (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("dig");
REQUIRE_GLOBAL(ui_sidebar_menus);
REQUIRE_GLOBAL(world);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "digv","Dig a whole vein.",digv,Gui::cursor_hotkey,
        "  Designates a whole vein under the cursor for digging.\n"
        "Options:\n"
        "  x - follow veins through z-levels with stairs.\n"
        ));
    commands.push_back(PluginCommand(
        "digvx","Dig a whole vein, following through z-levels.",digvx,Gui::cursor_hotkey,
        "  Designates a whole vein under the cursor for digging.\n"
        "  Also follows the vein between z-levels with stairs, like 'digv x' would.\n"
        ));
   commands.push_back(PluginCommand(
        "digl","Dig layerstone.",digl,Gui::cursor_hotkey,
        "  Designates layerstone under the cursor for digging.\n"
        "  Veins will not be touched.\n"
        "Options:\n"
        "  x    - follow layer through z-levels with stairs.\n"
        "  undo - clear designation (can be used together with 'x').\n"
        ));
    commands.push_back(PluginCommand(
        "diglx","Dig layerstone, following through z-levels.",diglx,Gui::cursor_hotkey,
        "  Designates layerstone under the cursor for digging.\n"
        "  Also follows the stone between z-levels with stairs, like 'digl x' would.\n"
        ));
    commands.push_back(PluginCommand("digexp","Select or designate an exploratory pattern.",digexp));
    commands.push_back(PluginCommand("digcircle","Dig designate a circle (filled or hollow)",digcircle));
    //commands.push_back(PluginCommand("digauto","Mark a tile for continuous digging.",autodig));
    commands.push_back(PluginCommand("digtype", "Dig all veins of a given type.", digtype,Gui::cursor_hotkey,
        "For every tile on the map of the same vein type as the selected tile, this command designates it to have the same designation as the selected tile. If the selected tile has no designation, they will be dig designated.\n"
        "If an argument is given, the designation of the selected tile is ignored, and all appropriate tiles are set to the specified designation.\n"
        "Options:\n"
        "  dig\n"
        "  channel\n"
        "  ramp\n"
        "  updown - up/down stairs\n"
        "  up     - up stairs\n"
        "  down   - down stairs\n"
        "  clear  - clear designation\n"
        ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

template <class T>
bool from_string(T& t,
    const std::string& s,
    std::ios_base& (*f)(std::ios_base&))
{
    std::istringstream iss(s);
    return !(iss >> f >> t).fail();
}

enum circle_what
{
    circle_set,
    circle_unset,
    circle_invert,
};

bool dig (MapExtras::MapCache & MCache,
    circle_what what,
    df::tile_dig_designation type,
    int32_t priority,
    int32_t x, int32_t y, int32_t z,
    int x_max, int y_max
    )
{
    DFCoord at (x,y,z);
    auto b = MCache.BlockAt(at/16);
    if(!b || !b->is_valid())
        return false;
    if(x == 0 || x == x_max * 16 - 1)
    {
        //out.print("not digging map border\n");
        return false;
    }
    if(y == 0 || y == y_max * 16 - 1)
    {
        //out.print("not digging map border\n");
        return false;
    }
    df::tiletype tt = MCache.tiletypeAt(at);
    df::tile_designation des = MCache.designationAt(at);
    // could be potentially used to locate hidden constructions?
    if(tileMaterial(tt) == tiletype_material::CONSTRUCTION && !des.bits.hidden)
        return false;
    df::tiletype_shape ts = tileShape(tt);
    if (ts == tiletype_shape::EMPTY && !des.bits.hidden)
        return false;
    if(!des.bits.hidden)
    {
        do
        {
            df::tiletype_shape_basic tsb = ENUM_ATTR(tiletype_shape, basic_shape, ts);
            if(tsb == tiletype_shape_basic::Wall)
            {
                std::cerr << "allowing tt" << (int)tt << ", is wall\n";
                break;
            }
            if (tsb == tiletype_shape_basic::Floor
                && (type == tile_dig_designation::DownStair || type == tile_dig_designation::Channel)
                && ts != tiletype_shape::BRANCH
                && ts != tiletype_shape::TRUNK_BRANCH
                && ts != tiletype_shape::TWIG
                )
            {
                std::cerr << "allowing tt" << (int)tt << ", is floor\n";
                break;
            }
            if (tsb == tiletype_shape_basic::Stair && type == tile_dig_designation::Channel )
                break;
            return false;
        }
        while(0);
    }
    switch(what)
    {
    case circle_set:
        if(des.bits.dig == tile_dig_designation::No)
        {
            des.bits.dig = type;
        }
        break;
    case circle_unset:
        if (des.bits.dig != tile_dig_designation::No)
        {
            des.bits.dig = tile_dig_designation::No;
        }
        break;
    case circle_invert:
        if(des.bits.dig == tile_dig_designation::No)
        {
            des.bits.dig = type;
        }
        else
        {
            des.bits.dig = tile_dig_designation::No;
        }
        break;
    }
    std::cerr << "allowing tt" << (int)tt << "\n";
    MCache.setDesignationAt(at,des,priority);
    return true;
};

bool lineX (MapExtras::MapCache & MCache,
    circle_what what,
    df::tile_dig_designation type,
    int32_t priority,
    int32_t y1, int32_t y2, int32_t x, int32_t z,
    int x_max, int y_max
    )
{
    for(int32_t y = y1; y <= y2; y++)
    {
        dig(MCache, what, type, priority, x,y,z, x_max, y_max);
    }
    return true;
};

bool lineY (MapExtras::MapCache & MCache,
    circle_what what,
    df::tile_dig_designation type,
    int32_t priority,
    int32_t x1, int32_t x2, int32_t y, int32_t z,
    int x_max, int y_max
    )
{
    for(int32_t x = x1; x <= x2; x++)
    {
        dig(MCache, what, type, priority, x,y,z, x_max, y_max);
    }
    return true;
};

int32_t parse_priority(color_ostream &out, vector<string> &parameters)
{
    int32_t default_priority = ui_sidebar_menus->designation.priority;

    for (auto it = parameters.begin(); it != parameters.end(); ++it)
    {
        const string &s = *it;
        if (s.substr(0, 2) == "p=" || s.substr(0, 2) == "-p")
        {
            if (s.size() >= 3)
            {
                auto priority = int32_t(1000 * atof(s.c_str() + 2));
                parameters.erase(it);
                return priority;
            }
            else if (it + 1 != parameters.end())
            {
                auto priority = int32_t(1000 * atof((*(it + 1)).c_str()));
                parameters.erase(it, it + 2);
                return priority;
            }
            else
            {
                out.printerr("invalid priority specified; reverting to %i\n", default_priority);
                break;
            }
        }
    }

    return default_priority;
}

string forward_priority(color_ostream &out, vector<string> &parameters)
{
    return string("-p") + int_to_string(parse_priority(out, parameters) / 1000);
}

command_result digcircle (color_ostream &out, vector <string> & parameters)
{
    static bool filled = false;
    static circle_what what = circle_set;
    static df::tile_dig_designation type = tile_dig_designation::Default;
    static int diameter = 0;
    auto saved_d = diameter;
    bool force_help = false;
    int32_t priority = parse_priority(out, parameters);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            force_help = true;
        }
        else if(parameters[i] == "hollow")
        {
            filled = false;
        }
        else if(parameters[i] == "filled")
        {
            filled = true;
        }
        else if(parameters[i] == "set")
        {
            what = circle_set;
        }
        else if(parameters[i] == "unset")
        {
            what = circle_unset;
        }
        else if(parameters[i] == "invert")
        {
            what = circle_invert;
        }
        else if(parameters[i] == "dig")
        {
            type = tile_dig_designation::Default;
        }
        else if(parameters[i] == "ramp")
        {
            type = tile_dig_designation::Ramp;
        }
        else if(parameters[i] == "dstair")
        {
            type = tile_dig_designation::DownStair;
        }
        else if(parameters[i] == "ustair")
        {
            type = tile_dig_designation::UpStair;
        }
        else if(parameters[i] == "xstair")
        {
            type = tile_dig_designation::UpDownStair;
        }
        else if(parameters[i] == "chan")
        {
            type = tile_dig_designation::Channel;
        }
        else if (!from_string(diameter,parameters[i], std::dec))
        {
            diameter = saved_d;
        }
    }
    if(diameter < 0)
        diameter = -diameter;
    if(force_help || diameter == 0)
    {
        out.print(
            "A command for easy designation of filled and hollow circles.\n"
            "\n"
            "Options:\n"
            " hollow = Set the circle to hollow (default)\n"
            " filled = Set the circle to filled\n"
            "\n"
            "    set = set designation\n"
            "  unset = unset current designation\n"
            " invert = invert current designation\n"
            "\n"
            "    dig = normal digging\n"
            "   ramp = ramp digging\n"
            " ustair = staircase up\n"
            " dstair = staircase down\n"
            " xstair = staircase up/down\n"
            "   chan = dig channel\n"
            "\n"
            "      # = diameter in tiles (default = 0)\n"
            "   -p # = designation priority (default = 4)\n"
            "\n"
            "After you have set the options, the command called with no options\n"
            "repeats with the last selected parameters:\n"
            "'digcircle filled 3' = Dig a filled circle with diameter = 3.\n"
            "'digcircle' = Do it again.\n"
            );
        return CR_OK;
    }
    int32_t cx, cy, cz;
    CoreSuspender suspend;
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    uint32_t x_max, y_max, z_max;
    Maps::getSize(x_max,y_max,z_max);

    MapExtras::MapCache MCache;
    if(!Gui::getCursorCoords(cx,cy,cz) || cx == -30000)
    {
        out.printerr("Can't get the cursor coords...\n");
        return CR_FAILURE;
    }
    int r = diameter / 2;
    int iter;
    bool adjust;
    if(diameter % 2)
    {
        // paint center
        if(filled)
        {
            lineY(MCache, what, type, priority, cx - r, cx + r, cy, cz, x_max, y_max);
        }
        else
        {
            dig(MCache, what, type, priority, cx - r, cy, cz, x_max, y_max);
            dig(MCache, what, type, priority, cx + r, cy, cz, x_max, y_max);
        }
        adjust = false;
        iter = 2;
    }
    else
    {
        adjust = true;
        iter = 1;
    }
    int lastwhole = r;
    for(; iter <= diameter - 1; iter +=2)
    {
        // top, bottom coords
        int top = cy - ((iter + 1) / 2) + adjust;
        int bottom = cy + ((iter + 1) / 2);
        // see where the current 'line' intersects the circle
        double val = std::sqrt(double(diameter*diameter - iter*iter));
        // adjust for circles with odd diameter
        if(!adjust)
            val -= 1;
        // map the found value to the DF grid
        double whole;
        double fraction = std::modf(val / 2.0, & whole);
        if (fraction > 0.5)
            whole += 1.0;
        int right = cx + whole;
        int left = cx - whole + adjust;
        int diff = lastwhole - whole;
        // paint
        if(filled || iter == diameter - 1)
        {
            lineY(MCache, what, type, priority, left, right, top, cz, x_max, y_max);
            lineY(MCache, what, type, priority, left, right, bottom, cz, x_max, y_max);
        }
        else
        {
            dig(MCache, what, type, priority, left, top, cz, x_max, y_max);
            dig(MCache, what, type, priority, left, bottom, cz, x_max, y_max);
            dig(MCache, what, type, priority, right, top, cz, x_max, y_max);
            dig(MCache, what, type, priority, right, bottom, cz, x_max, y_max);
        }
        if(!filled && diff > 1)
        {
            int lright = cx + lastwhole;
            int lleft = cx - lastwhole + adjust;
            lineY(MCache, what, type, priority, lleft + 1, left - 1, top + 1 , cz, x_max, y_max);
            lineY(MCache, what, type, priority, right + 1, lright - 1, top + 1 , cz, x_max, y_max);
            lineY(MCache, what, type, priority, lleft + 1, left - 1, bottom - 1 , cz, x_max, y_max);
            lineY(MCache, what, type, priority, right + 1, lright - 1, bottom - 1 , cz, x_max, y_max);
        }
        lastwhole = whole;
    }
    MCache.WriteAll();
    return CR_OK;
}
typedef char digmask[16][16];

static digmask diag5[5] =
{
    {
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
    },
    {
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
    },
    {
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
    },
    {
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
    },
    {
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
    },
};

static digmask diag5r[5] =
{
    {
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
    },
    {
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
    },
    {
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
    },
    {
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
    },
    {
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
    },
};

static digmask ladder[3] =
{
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0},
        {1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1},
        {0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0},
        {1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1},
        {0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0},
        {1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1},
        {0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0},
        {1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1},
        {0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0},
    },
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0},
        {0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,0},
        {0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0},
        {0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,0},
        {0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0},
        {0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,0},
        {0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0},
        {0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,0},
        {0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1},
    },
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0},
        {1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1},
        {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0},
        {1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1},
        {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0},
        {1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1},
        {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0},
        {1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1},
        {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0},
    },
};

static digmask ladderr[3] =
{
    {
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
    },
    {
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
    },
    {
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
    },
};

static digmask all_tiles =
{
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

static digmask cross =
{
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,1,0,0,0,1,1,0,0,0,1,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0},
    {0,0,0,1,0,0,0,1,1,0,0,0,1,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};
enum explo_how
{
    EXPLO_NOTHING,
    EXPLO_DIAG5,
    EXPLO_DIAG5R,
    EXPLO_LADDER,
    EXPLO_LADDERR,
    EXPLO_CLEAR,
    EXPLO_CROSS,
};

enum explo_what
{
    EXPLO_ALL,
    EXPLO_HIDDEN,
    EXPLO_DESIGNATED,
};

bool stamp_pattern (uint32_t bx, uint32_t by, int z_level,
    digmask & dm, explo_how how, explo_what what,
    int x_max, int y_max
    )
{
    df::map_block * bl = Maps::getBlock(bx,by,z_level);
    if(!bl)
        return false;
    int x = 0,mx = 16;
    if(bx == 0)
        x = 1;
    if(bx == x_max - 1)
        mx = 15;
    for(; x < mx; x++)
    {
        int y = 0,my = 16;
        if(by == 0)
            y = 1;
        if(by == y_max - 1)
            my = 15;
        for(; y < my; y++)
        {
            df::tile_designation & des = bl->designation[x][y];
            df::tiletype tt = bl->tiletype[x][y];
            // could be potentially used to locate hidden constructions?
            if(tileMaterial(tt) == tiletype_material::CONSTRUCTION && !des.bits.hidden)
                continue;
            if(!isWallTerrain(tt) && !des.bits.hidden)
                continue;
            if(how == EXPLO_CLEAR)
            {
                des.bits.dig = tile_dig_designation::No;
                continue;
            }
            if(dm[y][x])
            {
                if(what == EXPLO_ALL
                    || des.bits.dig == tile_dig_designation::Default && what == EXPLO_DESIGNATED
                    || des.bits.hidden && what == EXPLO_HIDDEN)
                {
                    des.bits.dig = tile_dig_designation::Default;
                }
            }
            else if(what == EXPLO_DESIGNATED)
            {
                des.bits.dig = tile_dig_designation::No;
            }
        }
    }
    bl->flags.bits.designated = true;
    return true;
};

command_result digexp (color_ostream &out, vector <string> & parameters)
{
    bool force_help = false;
    static explo_how how = EXPLO_NOTHING;
    static explo_what what = EXPLO_HIDDEN;
    int32_t priority = parse_priority(out, parameters);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            force_help = true;
        }
        else if(parameters[i] == "all")
        {
            what = EXPLO_ALL;
        }
        else if(parameters[i] == "hidden")
        {
            what = EXPLO_HIDDEN;
        }
        else if(parameters[i] == "designated")
        {
            what = EXPLO_DESIGNATED;
        }
        else if(parameters[i] == "diag5")
        {
            how = EXPLO_DIAG5;
        }
        else if(parameters[i] == "diag5r")
        {
            how = EXPLO_DIAG5R;
        }
        else if(parameters[i] == "clear")
        {
            how = EXPLO_CLEAR;
        }
        else if(parameters[i] == "ladder")
        {
            how = EXPLO_LADDER;
        }
        else if(parameters[i] == "ladderr")
        {
            how = EXPLO_LADDERR;
        }
        else if(parameters[i] == "cross")
        {
            how = EXPLO_CROSS;
        }
    }
    if(force_help || how == EXPLO_NOTHING)
    {
        out.print(
            "This command can be used for exploratory mining.\n"
            "http://dwarffortresswiki.org/Exploratory_mining\n"
            "\n"
            "There are two variables that can be set: pattern and filter.\n"
            "Patterns:\n"
            "  diag5 = diagonals separated by 5 tiles\n"
            " diag5r = diag5 rotated 90 degrees\n"
            " ladder = A 'ladder' pattern\n"
            "ladderr = ladder rotated 90 degrees\n"
            "  clear = Just remove all dig designations\n"
            "  cross = A cross, exactly in the middle of the map.\n"
            "Filters:\n"
            " all        = designate whole z-level\n"
            " hidden     = designate only hidden tiles of z-level (default)\n"
            " designated = Take current designation and apply pattern to it.\n"
            "\n"
            "After you have a pattern set, you can use 'expdig' to apply it:\n"
            "'digexp diag5 hidden' = set filter to hidden, pattern to diag5.\n"
            "'digexp' = apply the pattern with filter.\n"
            );
        return CR_OK;
    }
    CoreSuspender suspend;
    uint32_t x_max, y_max, z_max;
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    Maps::getSize(x_max,y_max,z_max);
    int32_t xzzz,yzzz,z_level;
    if(!Gui::getViewCoords(xzzz,yzzz,z_level))
    {
        out.printerr("Can't get view coords...\n");
        return CR_FAILURE;
    }
    if(how == EXPLO_DIAG5)
    {
        int which;
        for(uint32_t x = 0; x < x_max; x++)
        {
            for(int32_t y = 0 ; y < y_max; y++)
            {
                which = (4*x + y) % 5;
                stamp_pattern(x,y_max - 1 - y, z_level, diag5[which],
                    how, what, x_max, y_max);
            }
        }
    }
    else if(how == EXPLO_DIAG5R)
    {
        int which;
        for(uint32_t x = 0; x < x_max; x++)
        {
            for(int32_t y = 0 ; y < y_max; y++)
            {
                which = (4*x + 1000-y) % 5;
                stamp_pattern(x,y_max - 1 - y, z_level, diag5r[which],
                    how, what, x_max, y_max);
            }
        }
    }
    else if(how == EXPLO_LADDER)
    {
        int which;
        for(uint32_t x = 0; x < x_max; x++)
        {
            which = x % 3;
            for(int32_t y = 0 ; y < y_max; y++)
            {
                stamp_pattern(x, y, z_level, ladder[which],
                    how, what, x_max, y_max);
            }
        }
    }
    else if(how == EXPLO_LADDERR)
    {
        int which;
        for(int32_t y = 0 ; y < y_max; y++)
        {
            which = y % 3;
            for(uint32_t x = 0; x < x_max; x++)
            {
                stamp_pattern(x, y, z_level, ladderr[which],
                    how, what, x_max, y_max);
            }
        }
    }
    else if(how == EXPLO_CROSS)
    {
        // middle + recentering for the image
        int xmid = x_max * 8 - 8;
        int ymid = y_max * 8 - 8;
        MapExtras::MapCache mx;
        for(int x = 0; x < 16; x++)
            for(int y = 0; y < 16; y++)
            {
                DFCoord pos(xmid+x,ymid+y,z_level);
                df::tiletype tt = mx.tiletypeAt(pos);
                if(tt == tiletype::Void)
                    continue;
                df::tile_designation des = mx.designationAt(pos);
                if(tileMaterial(tt) == tiletype_material::CONSTRUCTION && !des.bits.hidden)
                    continue;
                if(!isWallTerrain(tt) && !des.bits.hidden)
                    continue;
                if(cross[y][x])
                {
                    des.bits.dig = tile_dig_designation::Default;
                    mx.setDesignationAt(pos,des,priority);
                }
            }
        mx.WriteAll();
    }
    else for(uint32_t x = 0; x < x_max; x++)
    {
        for(int32_t y = 0 ; y < y_max; y++)
        {
            stamp_pattern(x, y, z_level, all_tiles,
                how, what, x_max, y_max);
        }
    }
    return CR_OK;
}

command_result digvx (color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    vector <string> lol;
    lol.push_back("x");
    lol.push_back(forward_priority(out, parameters));
    return digv(out,lol);
}

command_result digv (color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    uint32_t x_max,y_max,z_max;
    bool updown = false;
    int32_t priority = parse_priority(out, parameters);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters.size() && parameters[0]=="x")
            updown = true;
        else
            return CR_WRONG_USAGE;
    }

    auto &con = out;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int32_t cx, cy, cz;
    Maps::getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;
    Gui::getCursorCoords(cx,cy,cz);
    while(cx == -30000)
    {
        con.printerr("Cursor is not active. Point the cursor at a vein.\n");
        return CR_FAILURE;
    }
    DFHack::DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    if(xy.x == 0 || xy.x == tx_max - 1 || xy.y == 0 || xy.y == ty_max - 1)
    {
        con.printerr("I won't dig the borders. That would be cheating!\n");
        return CR_FAILURE;
    }
    MapExtras::MapCache * MCache = new MapExtras::MapCache;
    df::tile_designation des = MCache->designationAt(xy);
    df::tiletype tt = MCache->tiletypeAt(xy);
    int16_t veinmat = MCache->veinMaterialAt(xy);
    if( veinmat == -1 )
    {
        con.printerr("This tile is not a vein.\n");
        delete MCache;
        return CR_FAILURE;
    }
    con.print("%d/%d/%d tiletype: %d, veinmat: %d, designation: 0x%x ... DIGGING!\n", cx,cy,cz, tt, veinmat, des.whole);
    stack <DFHack::DFCoord> flood;
    flood.push(xy);

    while( !flood.empty() )
    {
        DFHack::DFCoord current = flood.top();
        flood.pop();
        if (MCache->tagAt(current))
            continue;
        int16_t vmat2 = MCache->veinMaterialAt(current);
        tt = MCache->tiletypeAt(current);
        if(!DFHack::isWallTerrain(tt))
            continue;
        if(vmat2!=veinmat)
            continue;

        // found a good tile, dig+unset material
        df::tile_designation des = MCache->designationAt(current);
        df::tile_designation des_minus;
        df::tile_designation des_plus;
        des_plus.whole = des_minus.whole = 0;
        int16_t vmat_minus = -1;
        int16_t vmat_plus = -1;
        bool below = 0;
        bool above = 0;
        if(updown)
        {
            if(MCache->testCoord(current-1))
            {
                below = 1;
                des_minus = MCache->designationAt(current-1);
                vmat_minus = MCache->veinMaterialAt(current-1);
            }

            if(MCache->testCoord(current+1))
            {
                above = 1;
                des_plus = MCache->designationAt(current+1);
                vmat_plus = MCache->veinMaterialAt(current+1);
            }
        }
        if(MCache->testCoord(current))
        {
            MCache->setTagAt(current, 1);

            if(current.x < tx_max - 2)
            {
                flood.push(DFHack::DFCoord(current.x + 1, current.y, current.z));
                if(current.y < ty_max - 2)
                {
                    flood.push(DFHack::DFCoord(current.x + 1, current.y + 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y + 1,current.z));
                }
                if(current.y > 1)
                {
                    flood.push(DFHack::DFCoord(current.x + 1, current.y - 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y - 1,current.z));
                }
            }
            if(current.x > 1)
            {
                flood.push(DFHack::DFCoord(current.x - 1, current.y,current.z));
                if(current.y < ty_max - 2)
                {
                    flood.push(DFHack::DFCoord(current.x - 1, current.y + 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y + 1,current.z));
                }
                if(current.y > 1)
                {
                    flood.push(DFHack::DFCoord(current.x - 1, current.y - 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y - 1,current.z));
                }
            }
            if(updown)
            {
                if(current.z > 0 && below && vmat_minus == vmat2)
                {
                    flood.push(current-1);

                    if(des_minus.bits.dig == tile_dig_designation::DownStair)
                        des_minus.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des_minus.bits.dig = tile_dig_designation::UpStair;
                    MCache->setDesignationAt(current-1,des_minus,priority);

                    des.bits.dig = tile_dig_designation::DownStair;
                }
                if(current.z < z_max - 1 && above && vmat_plus == vmat2)
                {
                    flood.push(current+ 1);

                    if(des_plus.bits.dig == tile_dig_designation::UpStair)
                        des_plus.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des_plus.bits.dig = tile_dig_designation::DownStair;
                    MCache->setDesignationAt(current+1,des_plus,priority);

                    if(des.bits.dig == tile_dig_designation::DownStair)
                        des.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des.bits.dig = tile_dig_designation::UpStair;
                }
            }
            if(des.bits.dig == tile_dig_designation::No)
                des.bits.dig = tile_dig_designation::Default;
            MCache->setDesignationAt(current,des,priority);
        }
    }
    MCache->WriteAll();
    delete MCache;
    return CR_OK;
}

command_result diglx (color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    vector <string> lol;
    lol.push_back("x");
    lol.push_back(forward_priority(out, parameters));
    return digl(out,lol);
}

// TODO:
// digl and digv share the longish floodfill code and only use different conditions
// to check if a tile should be marked for digging or not.
// to make the plugin a bit smaller and cleaner a main execute method would be nice
// (doing the floodfill stuff and do the checks dependin on whether called in
// "vein" or "layer" mode)
command_result digl (color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    uint32_t x_max,y_max,z_max;
    bool updown = false;
    bool undo = false;
    int32_t priority = parse_priority(out, parameters);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i]=="x")
        {
            out << "This might take a while for huge layers..." << std::endl;
            updown = true;
        }
        else if(parameters[i]=="undo")
        {
            out << "Removing dig designation." << std::endl;
            undo = true;
        }
        else
            return CR_WRONG_USAGE;
    }

    auto &con = out;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int32_t cx, cy, cz;
    Maps::getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;
    Gui::getCursorCoords(cx,cy,cz);
    while(cx == -30000)
    {
        con.printerr("Cursor is not active. Point the cursor at a vein.\n");
        return CR_FAILURE;
    }
    DFHack::DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    if(xy.x == 0 || xy.x == tx_max - 1 || xy.y == 0 || xy.y == ty_max - 1)
    {
        con.printerr("I won't dig the borders. That would be cheating!\n");
        return CR_FAILURE;
    }
    MapExtras::MapCache * MCache = new MapExtras::MapCache;
    df::tile_designation des = MCache->designationAt(xy);
    df::tiletype tt = MCache->tiletypeAt(xy);
    int16_t veinmat = MCache->veinMaterialAt(xy);
    int16_t basemat = MCache->layerMaterialAt(xy);
    if( veinmat != -1 )
    {
        con.printerr("This is a vein. Use vdig instead!\n");
        delete MCache;
        return CR_FAILURE;
    }
    con.print("%d/%d/%d tiletype: %d, basemat: %d, designation: 0x%x ... DIGGING!\n", cx,cy,cz, tt, basemat, des.whole);
    stack <DFHack::DFCoord> flood;
    flood.push(xy);

    int i = 0;
    while( !flood.empty() )
    {
        DFHack::DFCoord current = flood.top();
        flood.pop();
        if (MCache->tagAt(current))
            continue;
        int16_t vmat2 = MCache->veinMaterialAt(current);
        int16_t bmat2 = MCache->layerMaterialAt(current);
        tt = MCache->tiletypeAt(current);

        if(!DFHack::isWallTerrain(tt))
            continue;
        if(vmat2!=-1)
            continue;
        if(bmat2!=basemat)
            continue;

        // don't dig out LAVA_STONE or MAGMA (semi-molten rock) accidentally
        if(    tileMaterial(tt)!=tiletype_material::STONE
            && tileMaterial(tt)!=tiletype_material::SOIL)
            continue;

        // found a good tile, dig+unset material
        df::tile_designation des = MCache->designationAt(current);
        df::tile_designation des_minus;
        df::tile_designation des_plus;
        des_plus.whole = des_minus.whole = 0;
        int16_t vmat_minus = -1;
        int16_t vmat_plus = -1;
        int16_t bmat_minus = -1;
        int16_t bmat_plus = -1;
        df::tiletype tt_minus;
        df::tiletype tt_plus;

        if(MCache->testCoord(current))
        {
            MCache->setTagAt(current, 1);
            if(current.x < tx_max - 2)
            {
                flood.push(DFHack::DFCoord(current.x + 1, current.y, current.z));
                if(current.y < ty_max - 2)
                {
                    flood.push(DFHack::DFCoord(current.x + 1, current.y + 1, current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y + 1, current.z));
                }
                if(current.y > 1)
                {
                    flood.push(DFHack::DFCoord(current.x + 1, current.y - 1, current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y - 1, current.z));
                }
            }
            if(current.x > 1)
            {
                flood.push(DFHack::DFCoord(current.x - 1, current.y, current.z));
                if(current.y < ty_max - 2)
                {
                    flood.push(DFHack::DFCoord(current.x - 1, current.y + 1, current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y + 1, current.z));
                }
                if(current.y > 1)
                {
                    flood.push(DFHack::DFCoord(current.x - 1, current.y - 1, current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y - 1, current.z));
                }
            }
            if(updown)
            {
                bool below = 0;
                bool above = 0;
                if(MCache->testCoord(current-1))
                {
                    //below = 1;
                    des_minus = MCache->designationAt(current-1);
                    vmat_minus = MCache->veinMaterialAt(current-1);
                    bmat_minus = MCache->layerMaterialAt(current-1);
                    tt_minus = MCache->tiletypeAt(current-1);
                    if (   tileMaterial(tt_minus)==tiletype_material::STONE
                        || tileMaterial(tt_minus)==tiletype_material::SOIL)
                        below = 1;
                }
                if(MCache->testCoord(current+1))
                {
                    //above = 1;
                    des_plus = MCache->designationAt(current+1);
                    vmat_plus = MCache->veinMaterialAt(current+1);
                    bmat_plus = MCache->layerMaterialAt(current+1);
                    tt_plus = MCache->tiletypeAt(current+1);
                    if (   tileMaterial(tt_plus)==tiletype_material::STONE
                        || tileMaterial(tt_plus)==tiletype_material::SOIL)
                        above = 1;
                }
                if(current.z > 0 && below && vmat_minus == -1 && bmat_minus == basemat)
                {
                    flood.push(current-1);

                    if(des_minus.bits.dig == tile_dig_designation::DownStair)
                        des_minus.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des_minus.bits.dig = tile_dig_designation::UpStair;
                    // undo mode: clear designation
                    if(undo)
                        des_minus.bits.dig = tile_dig_designation::No;
                    MCache->setDesignationAt(current-1,des_minus,priority);

                    des.bits.dig = tile_dig_designation::DownStair;
                }
                if(current.z < z_max - 1 && above && vmat_plus == -1 && bmat_plus == basemat)
                {
                    flood.push(current+ 1);

                    if(des_plus.bits.dig == tile_dig_designation::UpStair)
                        des_plus.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des_plus.bits.dig = tile_dig_designation::DownStair;
                    // undo mode: clear designation
                    if(undo)
                        des_plus.bits.dig = tile_dig_designation::No;
                    MCache->setDesignationAt(current+1,des_plus,priority);

                    if(des.bits.dig == tile_dig_designation::DownStair)
                        des.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des.bits.dig = tile_dig_designation::UpStair;
                }
            }
            if(des.bits.dig == tile_dig_designation::No)
                des.bits.dig = tile_dig_designation::Default;
            // undo mode: clear designation
            if(undo)
                des.bits.dig = tile_dig_designation::No;
            MCache->setDesignationAt(current,des,priority);
        }
    }
    MCache->WriteAll();
    delete MCache;
    return CR_OK;
}


command_result digauto (color_ostream &out, vector <string> & parameters)
{
    return CR_NOT_IMPLEMENTED;
}

command_result digtype (color_ostream &out, vector <string> & parameters)
{
    //mostly copy-pasted from digv
    int32_t priority = parse_priority(out, parameters);
    CoreSuspender suspend;
    if ( parameters.size() > 1 )
    {
        out.printerr("Too many parameters.\n");
        return CR_FAILURE;
    }

    uint32_t targetDigType;
    if ( parameters.size() == 1 )
    {
        string parameter = parameters[0];
        if ( parameter == "clear" )
            targetDigType = tile_dig_designation::No;
        else if ( parameter == "dig" )
            targetDigType = tile_dig_designation::Default;
        else if ( parameter == "updown" )
            targetDigType = tile_dig_designation::UpDownStair;
        else if ( parameter == "channel" )
            targetDigType = tile_dig_designation::Channel;
        else if ( parameter == "ramp" )
            targetDigType = tile_dig_designation::Ramp;
        else if ( parameter == "down" )
            targetDigType = tile_dig_designation::DownStair;
        else if ( parameter == "up" )
            targetDigType = tile_dig_designation::UpStair;
        else
        {
            out.printerr("Invalid parameter.\n");
            return CR_FAILURE;
        }
    }
    else
    {
        targetDigType = -1;
    }

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int32_t cx, cy, cz;
    uint32_t xMax,yMax,zMax;
    Maps::getSize(xMax,yMax,zMax);
    uint32_t tileXMax = xMax * 16;
    uint32_t tileYMax = yMax * 16;
    Gui::getCursorCoords(cx,cy,cz);
    if (cx == -30000)
    {
        out.printerr("Cursor is not active. Point the cursor at a vein.\n");
        return CR_FAILURE;
    }
    DFHack::DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    MapExtras::MapCache * mCache = new MapExtras::MapCache;
    df::tile_designation baseDes = mCache->designationAt(xy);
    df::tiletype tt = mCache->tiletypeAt(xy);
    int16_t veinmat = mCache->veinMaterialAt(xy);
    if( veinmat == -1 )
    {
        out.printerr("This tile is not a vein.\n");
        delete mCache;
        return CR_FAILURE;
    }
    out.print("(%d,%d,%d) tiletype: %d, veinmat: %d, designation: 0x%x ... DIGGING!\n", cx,cy,cz, tt, veinmat, baseDes.whole);

    if ( targetDigType != -1 )
    {
        baseDes.bits.dig = (tile_dig_designation::tile_dig_designation)targetDigType;
    }
    else
    {
        if ( baseDes.bits.dig == tile_dig_designation::No )
        {
            baseDes.bits.dig = tile_dig_designation::Default;
        }
    }

    for( uint32_t z = 0; z < zMax; z++ )
    {
        for( uint32_t x = 1; x < tileXMax-1; x++ )
        {
            for( uint32_t y = 1; y < tileYMax-1; y++ )
            {
                DFHack::DFCoord current(x,y,z);
                int16_t vmat2 = mCache->veinMaterialAt(current);
                if ( vmat2 != veinmat )
                    continue;
                tt = mCache->tiletypeAt(current);
                if (!DFHack::isWallTerrain(tt))
                    continue;

                //designate it for digging
                df::tile_designation des = mCache->designationAt(current);
                if ( !mCache->testCoord(current) )
                {
                    out.printerr("testCoord failed at (%d,%d,%d)\n", x, y, z);
                    delete mCache;
                    return CR_FAILURE;
                }

                df::tile_designation designation = mCache->designationAt(current);
                designation.bits.dig = baseDes.bits.dig;
                mCache->setDesignationAt(current, designation,priority);
            }
        }
    }

    mCache->WriteAll();
    delete mCache;
    return CR_OK;
}

