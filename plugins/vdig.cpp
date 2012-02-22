#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/MapCache.h"
#include <vector>
#include <cstdio>
#include <stack>
#include <string>
#include <cmath>
using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
using namespace df::enums;

command_result vdig (Core * c, vector <string> & parameters);
command_result vdigx (Core * c, vector <string> & parameters);
command_result autodig (Core * c, vector <string> & parameters);
command_result expdig (Core * c, vector <string> & parameters);
command_result digcircle (Core *c, vector <string> & parameters);


DFHACK_PLUGIN("vdig");

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand(
        "vdig","Dig a whole vein.",vdig,cursor_hotkey,
        "  Designates a whole vein under the cursor for digging.\n"
        "Options:\n"
        "  x - follow veins through z-levels with stairs.\n"
        ));
    commands.push_back(PluginCommand(
        "vdigx","Dig a whole vein, following through z-levels.",vdigx,cursor_hotkey,
        "  Designates a whole vein under the cursor for digging.\n"
        "  Also follows the vein between z-levels with stairs, like 'vdig x' would.\n"
        ));
    commands.push_back(PluginCommand("expdig","Select or designate an exploratory pattern. Use 'expdig ?' for help.",expdig));
    commands.push_back(PluginCommand("digcircle","Dig designate a circle (filled or hollow) with given radius.",digcircle));
    //commands.push_back(PluginCommand("autodig","Mark a tile for continuous digging.",autodig));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
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
    int32_t x, int32_t y, int32_t z,
    int x_max, int y_max
    )
{
    DFCoord at (x,y,z);
    auto b = MCache.BlockAt(at/16);
    if(!b || !b->valid)
        return false;
    if(x == 0 || x == x_max * 16 - 1)
    {
        //c->con.print("not digging map border\n");
        return false;
    }
    if(y == 0 || y == y_max * 16 - 1)
    {
        //c->con.print("not digging map border\n");
        return false;
    }
    df::tiletype tt = MCache.tiletypeAt(at);
    df::tile_designation des = MCache.designationAt(at);
    // could be potentially used to locate hidden constructions?
    if(tileMaterial(tt) == df::tiletype_material::CONSTRUCTION && !des.bits.hidden)
        return false;
    df::tiletype_shape ts = tileShape(tt);
    if (ts == tiletype_shape::EMPTY)
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
                && ts != tiletype_shape::TREE
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
    MCache.setDesignationAt(at,des);
    return true;
};

bool lineX (MapExtras::MapCache & MCache,
    circle_what what,
    df::tile_dig_designation type,
    int32_t y1, int32_t y2, int32_t x, int32_t z,
    int x_max, int y_max
    )
{
    for(int32_t y = y1; y <= y2; y++)
    {
        dig(MCache, what, type,x,y,z, x_max, y_max);
    }
    return true;
};

bool lineY (MapExtras::MapCache & MCache,
    circle_what what,
    df::tile_dig_designation type,
    int32_t x1, int32_t x2, int32_t y, int32_t z,
    int x_max, int y_max
    )
{
    for(int32_t x = x1; x <= x2; x++)
    {
        dig(MCache, what, type,x,y,z, x_max, y_max);
    }
    return true;
};

command_result digcircle (Core * c, vector <string> & parameters)
{
    static bool filled = false;
    static circle_what what = circle_set;
    static df::tile_dig_designation type = tile_dig_designation::Default;
    static int diameter = 0;
    auto saved_d = diameter;
    bool force_help = false;
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
        c->con.print(
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
            "\n"
            "After you have set the options, the command called with no options\n"
            "repeats with the last selected parameters:\n"
            "'digcircle filled 3' = Dig a filled circle with radius = 3.\n"
            "'digcircle' = Do it again.\n"
            );
        return CR_OK;
    }
    int32_t cx, cy, cz;
    CoreSuspender suspend(c);
    Gui * gui = c->getGui();
    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    uint32_t x_max, y_max, z_max;
    Maps::getSize(x_max,y_max,z_max);

    MapExtras::MapCache MCache;
    if(!gui->getCursorCoords(cx,cy,cz) || cx == -30000)
    {
        c->con.printerr("Can't get the cursor coords...\n");
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
            lineY(MCache,what,type, cx - r, cx + r, cy, cz,x_max,y_max);
        }
        else
        {
            dig(MCache, what, type,cx - r, cy, cz,x_max,y_max);
            dig(MCache, what, type,cx + r, cy, cz,x_max,y_max);
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
            lineY(MCache,what,type, left, right, top , cz,x_max,y_max);
            lineY(MCache,what,type, left, right, bottom , cz,x_max,y_max);
        }
        else
        {
            dig(MCache, what, type,left, top, cz,x_max,y_max);
            dig(MCache, what, type,left, bottom, cz,x_max,y_max);
            dig(MCache, what, type,right, top, cz,x_max,y_max);
            dig(MCache, what, type,right, bottom, cz,x_max,y_max);
        }
        if(!filled && diff > 1)
        {
            int lright = cx + lastwhole;
            int lleft = cx - lastwhole + adjust;
            lineY(MCache,what,type, lleft + 1, left - 1, top + 1 , cz,x_max,y_max);
            lineY(MCache,what,type, right + 1, lright - 1, top + 1 , cz,x_max,y_max);
            lineY(MCache,what,type, lleft + 1, left - 1, bottom - 1 , cz,x_max,y_max);
            lineY(MCache,what,type, right + 1, lright - 1, bottom - 1 , cz,x_max,y_max);
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

command_result expdig (Core * c, vector <string> & parameters)
{
    bool force_help = false;
    static explo_how how = EXPLO_NOTHING;
    static explo_what what = EXPLO_HIDDEN;
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
        c->con.print(
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
            "'expdig diag5 hidden' = set filter to hidden, pattern to diag5.\n"
            "'expdig' = apply the pattern with filter.\n"
            );
        return CR_OK;
    }
    CoreSuspender suspend(c);
    Gui * gui = c->getGui();
    uint32_t x_max, y_max, z_max;
    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    Maps::getSize(x_max,y_max,z_max);
    int32_t xzzz,yzzz,z_level;
    if(!gui->getViewCoords(xzzz,yzzz,z_level))
    {
        c->con.printerr("Can't get view coords...\n");
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
                    mx.setDesignationAt(pos,des);
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

command_result vdigx (Core * c, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    vector <string> lol;
    lol.push_back("x");
    return vdig(c,lol);
}

command_result vdig (Core * c, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    uint32_t x_max,y_max,z_max;
    bool updown = false;
    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters.size() && parameters[0]=="x")
            updown = true;
        else
            return CR_WRONG_USAGE;
    }

    Console & con = c->con;

    DFHack::Gui * Gui = c->getGui();
    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int32_t cx, cy, cz;
    Maps::getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;
    Gui->getCursorCoords(cx,cy,cz);
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
            MCache->clearMaterialAt(current);
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
                    MCache->setDesignationAt(current-1,des_minus);

                    des.bits.dig = tile_dig_designation::DownStair;
                }
                if(current.z < z_max - 1 && above && vmat_plus == vmat2)
                {
                    flood.push(current+ 1);

                    if(des_plus.bits.dig == tile_dig_designation::UpStair)
                        des_plus.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des_plus.bits.dig = tile_dig_designation::DownStair;
                    MCache->setDesignationAt(current+1,des_plus);

                    if(des.bits.dig == tile_dig_designation::DownStair)
                        des.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des.bits.dig = tile_dig_designation::UpStair;
                }
            }
            if(des.bits.dig == tile_dig_designation::No)
                des.bits.dig = tile_dig_designation::Default;
            MCache->setDesignationAt(current,des);
        }
    }
    MCache->WriteAll();
    return CR_OK;
}

command_result autodig (Core * c, vector <string> & parameters)
{
    return CR_NOT_IMPLEMENTED;
}
