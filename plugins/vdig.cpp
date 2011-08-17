#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Maps.h>
#include <dfhack/modules/Gui.h>
#include <dfhack/extra/MapExtras.h>
#include <vector>
#include <cstdio>
#include <stack>
#include <string>

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;

DFhackCExport command_result vdig (Core * c, vector <string> & parameters);
DFhackCExport command_result vdigx (Core * c, vector <string> & parameters);
DFhackCExport command_result autodig (Core * c, vector <string> & parameters);
DFhackCExport command_result expdig (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "vdig";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("vdig","Dig a whole vein.",vdig));
    commands.push_back(PluginCommand("vdigx","Dig a whole vein, follow vein through z-levels with stairs.",vdigx));
    commands.push_back(PluginCommand("expdig","Select or designate an exploratory pattern. Use 'expdig ?' for help.",expdig));
    //commands.push_back(PluginCommand("autodig","Mark a tile for continuous digging.",autodig));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
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
    EXPLO_LADDER,
    EXPLO_CLEAR,
    EXPLO_CROSS,
};

enum explo_what
{
    EXPLO_ALL,
    EXPLO_HIDDEN,
    EXPLO_DESIGNATED,
};

DFhackCExport command_result expdig (Core * c, vector <string> & parameters)
{
    bool force_help = false;
    static explo_how how = EXPLO_NOTHING;
    static explo_what what = EXPLO_HIDDEN;
    for(int i = 0; i < parameters.size();i++)
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
        else if(parameters[i] == "clear")
        {
            how = EXPLO_CLEAR;
        }
        else if(parameters[i] == "ladder")
        {
            how = EXPLO_LADDER;
        }
        else if(parameters[i] == "cross")
        {
            how = EXPLO_CROSS;
        }
    }
    if(force_help || how == EXPLO_NOTHING)
    {
        c->con.print("This command can be used for exploratory mining.\n"
                     "http://df.magmawiki.com/index.php/DF2010:Exploratory_mining\n"
                     "\n"
                     "There are two variables that can be set: pattern and filter.\n"
                     "Patterns:\n"
                     "  diag5 = diagonals separated by 5 tiles\n"
                     " ladder = A 'ladder' pattern\n"
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
    c->Suspend();
    Gui * gui = c->getGui();
    Maps * maps = c->getMaps();
    uint32_t x_max, y_max, z_max;
    if(!maps->Start())
    {
        c->Resume();
        c->con.printerr("Can't init the map...\n");
        return CR_FAILURE;
    }
    maps->getSize(x_max,y_max,z_max);
    int32_t xzzz,yzzz,z_level;
    if(!gui->getViewCoords(xzzz,yzzz,z_level))
    {
        c->Resume();
        c->con.printerr("Can't get view coords...\n");
        return CR_FAILURE;
    }
    auto apply = [&](uint32_t bx, uint32_t by, digmask & dm) -> bool
    {
        df_block * bl = maps->getBlock(bx,by,z_level);
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
                naked_designation & des = bl->designation[x][y].bits;
                short unsigned int tt = bl->tiletype[x][y];
                // could be potentially used to locate hidden constructions?
                if(tileMaterial(tt) == CONSTRUCTED && !des.hidden)
                    continue;
                if(!isWallTerrain(tt) && !des.hidden)
                    continue;
                if(how == EXPLO_CLEAR)
                {
                    des.dig = designation_no;
                    continue;
                }
                if(dm[y][x])
                {
                    if(what == EXPLO_ALL
                        || des.dig == designation_default && what == EXPLO_DESIGNATED
                        || des.hidden && what == EXPLO_HIDDEN)
                    {
                        des.dig = designation_default;
                    }
                }
                else if(what == EXPLO_DESIGNATED)
                {
                    des.dig = designation_no;
                }
            }
        }
        bl->flags.set(BLOCK_DESIGNATED);
        return true;
    };
    if(how == EXPLO_DIAG5)
    {
        int which;
        for(uint32_t x = 0; x < x_max; x++)
        {
            for(int32_t y = 0 ; y < y_max; y++)
            {
                which = (4*x + y) % 5;
                apply(x,y_max - 1 - y,diag5[which]);
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
                apply(x,y,ladder[which]);
            }
        }
    }
    else if(how == EXPLO_CROSS)
    {
        // middle + recentering for the image
        int xmid = x_max * 8 - 8;
        int ymid = y_max * 8 - 8;
        MapExtras::MapCache mx (maps);
        for(int x = 0; x < 16; x++)
            for(int y = 0; y < 16; y++)
            {
                DFCoord pos(xmid+x,ymid+y,z_level);
                short unsigned int tt = mx.tiletypeAt(pos);
                if(tt == 0)
                    continue;
                t_designation des = mx.designationAt(pos);
                if(cross[y][x])
                {
                    des.bits.dig = designation_default;
                    mx.setDesignationAt(pos,des);
                }
            }
        mx.WriteAll();
    }
    else for(uint32_t x = 0; x < x_max; x++)
        for(int32_t y = 0 ; y < y_max; y++)
            apply(x,y,all_tiles);
    c->Resume();
    return CR_OK;
}
DFhackCExport command_result vdigx (Core * c, vector <string> & parameters)
{
    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            c->con.print("Designates a whole vein under the cursor for digging.\n"
            "Also follows the vein between z-levels with stairs, like 'vdig x' would.\n"
            );
            return CR_OK;
        }
    }
    vector <string> lol;
    lol.push_back("x");
    return vdig(c,lol);
}

DFhackCExport command_result vdig (Core * c, vector <string> & parameters)
{
    uint32_t x_max,y_max,z_max;
    bool updown = false;
    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters.size() && parameters[0]=="x")
            updown = true;
        else if(parameters[i] == "help" || parameters[i] == "?")
        {
            c->con.print("Designates a whole vein under the cursor for digging.\n"
                         "Options:\n"
                         "x        - follow veins through z-levels with stairs.\n"
            );
            return CR_OK;
        }
    }

    Console & con = c->con;

    c->Suspend();
    DFHack::Maps * Maps = c->getMaps();
    DFHack::Gui * Gui = c->getGui();
    // init the map
    if(!Maps->Start())
    {
        con.printerr("Can't init map. Make sure you have a map loaded in DF.\n");
        c->Resume();
        return CR_FAILURE;
    }

    int32_t cx, cy, cz;
    Maps->getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;
    Gui->getCursorCoords(cx,cy,cz);
    while(cx == -30000)
    {
        con.printerr("Cursor is not active. Point the cursor at a vein.\n");
        c->Resume();
        return CR_FAILURE;
    }
    DFHack::DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    if(xy.x == 0 || xy.x == tx_max - 1 || xy.y == 0 || xy.y == ty_max - 1)
    {
        con.printerr("I won't dig the borders. That would be cheating!\n");
        c->Resume();
        return CR_FAILURE;
    }
    MapExtras::MapCache * MCache = new MapExtras::MapCache(Maps);
    DFHack::t_designation des = MCache->designationAt(xy);
    int16_t tt = MCache->tiletypeAt(xy);
    int16_t veinmat = MCache->veinMaterialAt(xy);
    if( veinmat == -1 )
    {
        con.printerr("This tile is not a vein.\n");
        delete MCache;
        c->Resume();
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
        DFHack::t_designation des = MCache->designationAt(current);
        DFHack::t_designation des_minus;
        DFHack::t_designation des_plus;
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

                    if(des_minus.bits.dig == DFHack::designation_d_stair)
                        des_minus.bits.dig = DFHack::designation_ud_stair;
                    else
                        des_minus.bits.dig = DFHack::designation_u_stair;
                    MCache->setDesignationAt(current-1,des_minus);

                    des.bits.dig = DFHack::designation_d_stair;
                }
                if(current.z < z_max - 1 && above && vmat_plus == vmat2)
                {
                    flood.push(current+ 1);

                    if(des_plus.bits.dig == DFHack::designation_u_stair)
                        des_plus.bits.dig = DFHack::designation_ud_stair;
                    else
                        des_plus.bits.dig = DFHack::designation_d_stair;
                    MCache->setDesignationAt(current+1,des_plus);

                    if(des.bits.dig == DFHack::designation_d_stair)
                        des.bits.dig = DFHack::designation_ud_stair;
                    else
                        des.bits.dig = DFHack::designation_u_stair;
                }
            }
            if(des.bits.dig == DFHack::designation_no)
                des.bits.dig = DFHack::designation_default;
            MCache->setDesignationAt(current,des);
        }
    }
    MCache->WriteAll();
    c->Resume();
    return CR_OK;
}

DFhackCExport command_result autodig (Core * c, vector <string> & parameters)
{
    return CR_NOT_IMPLEMENTED;
}