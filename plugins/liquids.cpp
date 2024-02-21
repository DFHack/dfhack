// plugin liquids
//
// This is a rewrite of the liquids module which can also be used non-interactively (hotkey).
// First the user sets the mode and other parameters with the interactive command liqiudsgo
// just like in the original liquids module.
// They are stored in statics to allow being used after the interactive session was closed.
// After defining an action the non-interactive command liquids-here can be used to call the
// execute method without the necessity to go back to the console. This allows convenient painting
// of liquids and obsidian using the ingame cursor and a hotkey.
//
// Commands:
// liquids      - basically the original liquids with the map changing stuff moved to an execute method
// liquids-here - runs the execute method with the last settings from liquids
//                (intended to be mapped to a hotkey)
// Options:
// ?, help        - print some help
//
// TODO:
// - maybe allow all parameters be passed as command line options? tedious parsing but might be useful
// - grab the code from digcircle to get a circle brush - could be nice when painting with obsidian
// - maybe store the last parameters in a file to make them persistent after dfhack is closed?

#include "Console.h"
#include "Core.h"
#include "Export.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Gui.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"

#include "df/map_block.h"
#include "df/tile_liquid_flow_dir.h"
#include "df/world.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

using std::vector;
using std::string;
using std::endl;
using std::set;

#include "Brushes.h"

using namespace MapExtras;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("liquids");
REQUIRE_GLOBAL(world);

static const char * HISTORY_FILE = "dfhack-config/liquids.history";
CommandHistory liquids_hist;

command_result df_liquids (color_ostream &out, vector <string> & parameters);
command_result df_liquids_here (color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    liquids_hist.load(HISTORY_FILE);
    commands.push_back(PluginCommand(
        "liquids",
        "Place magma, water or obsidian.",
        df_liquids,
        true)); // interactive, needs console for prompt
    commands.push_back(PluginCommand(
        "liquids-here",
        "Use settings from liquids at cursor position.",
        df_liquids_here,
        Gui::cursor_hotkey)); // non-interactive, needs ingame cursor
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    liquids_hist.save(HISTORY_FILE);
    return CR_OK;
}

enum BrushType {
    B_POINT, B_RANGE, B_BLOCK, B_COLUMN, B_FLOOD
};

static const char *brush_name[] = {
    "point", "range", "block", "column", "flood", NULL
};

enum PaintMode {
    P_WATER, P_MAGMA, P_OBSIDIAN, P_OBSIDIAN_FLOOR,
    P_RIVER_SOURCE, P_FLOW_BITS, P_WCLEAN
};

static const char *paint_mode_name[] = {
    "water", "magma", "obsidian", "obsidian_floor",
    "riversource", "flowbits", "wclean", NULL
};

enum ModifyMode {
    M_INC, M_KEEP, M_DEC
};

static const char *modify_mode_name[] = {
    "+", ".", "-", NULL
};

enum PermaflowMode {
    PF_KEEP, PF_NONE,
    PF_NORTH, PF_SOUTH, PF_EAST, PF_WEST,
    PF_NORTHEAST, PF_NORTHWEST, PF_SOUTHEAST, PF_SOUTHWEST
};

static const char *permaflow_name[] = {
    ".", "-", "N", "S", "E", "W",
    "NE", "NW", "SE", "SW", NULL
};

#define X(name) tile_liquid_flow_dir::name
static const df::tile_liquid_flow_dir permaflow_id[] = {
    X(none), X(none), X(north), X(south), X(east), X(west),
    X(northeast), X(northwest), X(southeast), X(southwest)
};
#undef X

struct OperationMode {
    BrushType brush;
    PaintMode paint;
    ModifyMode flowmode;
    ModifyMode setmode;
    PermaflowMode permaflow;
    unsigned int amount;
    df::coord size;

    OperationMode() :
        brush(B_POINT), paint(P_MAGMA),
        flowmode(M_INC), setmode(M_KEEP), permaflow(PF_KEEP), amount(7),
        size(1,1,1)
    {}
} cur_mode;

command_result df_liquids_execute(color_ostream &out);
command_result df_liquids_execute(color_ostream &out, OperationMode &mode, df::coord pos);

static void print_prompt(std::ostream &str, OperationMode &cur_mode)
{
    str <<"[" << paint_mode_name[cur_mode.paint] << ":" << brush_name[cur_mode.brush];
    if (cur_mode.brush == B_RANGE)
        str << "(w" << cur_mode.size.x << ":h" << cur_mode.size.y << ":z" << cur_mode.size.z << ")";
    str << ":" << cur_mode.amount << ":f" << modify_mode_name[cur_mode.flowmode]
        << ":s" << modify_mode_name[cur_mode.setmode]
        << ":pf" << permaflow_name[cur_mode.permaflow]
        << "]";
}

command_result df_liquids (color_ostream &out_, vector <string> & parameters)
{
    if(!out_.is_console())
        return CR_FAILURE;
    Console &out = static_cast<Console&>(out_);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
            return CR_WRONG_USAGE;
    }

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    std::vector<std::string> commands;
    bool end = false;

    out << "Welcome to the liquid spawner.\nType 'help' or '?' for a list of available commands, 'q' to quit.\nPress return after a command to confirm." << std::endl;

    while(!end)
    {
        string input = "";

        std::stringstream str;
        print_prompt(str, cur_mode);
        str << "# ";
        int rv;
        while ((rv = out.lineedit(str.str(),input,liquids_hist))
                == Console::RETRY);
        if (rv <= Console::FAILURE)
            return rv == Console::FAILURE ? CR_FAILURE : CR_OK;
        liquids_hist.add(input);

        commands.clear();
        Core::cheap_tokenise(input, commands);
        string command =  commands.empty() ? "" : commands[0];

        if(command=="help" || command == "?")
        {
            out << "Modes:" << endl
                 << "m             - switch to magma" << endl
                 << "w             - switch to water" << endl
                 << "o             - make obsidian wall instead" << endl
                 << "of            - make obsidian floors" << endl
                 << "rs            - make a river source" << endl
                 << "f             - flow bits only" << endl
                 << "wclean        - remove salt and stagnant flags from tiles" << endl
                 << "Set-Modes (only for magma/water):" << endl
                 << "s+            - only add" << endl
                 << "s.            - set" << endl
                 << "s-            - only remove" << endl
                 << "Properties (only for magma/water):" << endl
                 << "f+            - make the spawned liquid flow" << endl
                 << "f.            - don't change flow state (read state in flow mode)" << endl
                 << "f-            - make the spawned liquid static" << endl
                 << "Permaflow (only for water):" << endl
                 << "pf.           - don't change permaflow state" << endl
                 << "pf-           - make the spawned liquid static" << endl
                 << "pf[NS][EW]    - make the spawned liquid permanently flow" << endl
                 << "0-7           - set liquid amount" << endl
                 << "Brush:" << endl
                 << "point         - single tile [p]" << endl
                 << "range         - block with cursor at bottom north-west [r]" << endl
                 << "                (any place, any size)" << endl
                 << "block         - DF map block with cursor in it" << endl
                 << "                (regular spaced 16x16x1 blocks)" << endl
                 << "column        - Column from cursor, up through free space" << endl
                 << "flood         - Flood-fill water tiles from cursor" << endl
                 << "                (only makes sense with wclean)" << endl
                 << "Other:" << endl
                 << "q             - quit" << endl
                 << "help or ?     - print this list of commands" << endl
                 << "empty line    - put liquid" << endl
                 << endl
                 << "Usage: point the DF cursor at a tile you want to modify" << endl
                 << "and use the commands available :)" << endl;
            out << endl << "Settings will be remembered until you quit DF. You can call liquids-here to execute the last configured action. Useful in combination with keybindings." << endl;
        }
        else if(command == "m")
        {
            cur_mode.paint = P_MAGMA;
        }
        else if(command == "o")
        {
            cur_mode.paint = P_OBSIDIAN;
        }
        else if(command == "of")
        {
            cur_mode.paint = P_OBSIDIAN_FLOOR;
        }
        else if(command == "w")
        {
            cur_mode.paint = P_WATER;
        }
        else if(command == "f")
        {
            cur_mode.paint = P_FLOW_BITS;
        }
        else if(command == "rs")
        {
            cur_mode.paint = P_RIVER_SOURCE;
        }
        else if(command == "wclean")
        {
            cur_mode.paint = P_WCLEAN;
        }
        else if(command == "point" || command == "p")
        {
            cur_mode.brush = B_POINT;
        }
        else if(command == "range" || command == "r")
        {
            int width = 1, height = 1, z_levels = 1;
            command_result res = parseRectangle(out, commands, 1, commands.size(),
                                                width, height, z_levels);
            if (res != CR_OK)
            {
                return res;
            }

            if (width == 1 && height == 1 && z_levels == 1)
            {
                cur_mode.brush = B_POINT;
                cur_mode.size = df::coord(1, 1, 1);
            }
            else
            {
                cur_mode.brush = B_RANGE;
                cur_mode.size = df::coord(width, height, z_levels);
            }
        }
        else if(command == "block")
        {
            cur_mode.brush = B_BLOCK;
        }
        else if(command == "column")
        {
            cur_mode.brush = B_COLUMN;
        }
        else if(command == "flood")
        {
            cur_mode.brush = B_FLOOD;
        }
        else if(command == "q")
        {
            end = true;
        }
        else if(command == "f+")
        {
            cur_mode.flowmode = M_INC;
        }
        else if(command == "f-")
        {
            cur_mode.flowmode = M_DEC;
        }
        else if(command == "f.")
        {
            cur_mode.flowmode = M_KEEP;
        }
        else if(command == "s+")
        {
            cur_mode.setmode = M_INC;
        }
        else if(command == "s-")
        {
            cur_mode.setmode = M_DEC;
        }
        else if(command == "s.")
        {
            cur_mode.setmode = M_KEEP;
        }
        else if (command.size() > 2 && memcmp(command.c_str(), "pf", 2) == 0)
        {
            auto *tail = command.c_str()+2;
            for (int pm = PF_KEEP; pm <= PF_SOUTHWEST; pm++)
            {
                if (strcmp(tail, permaflow_name[pm]) != 0)
                    continue;
                cur_mode.permaflow = PermaflowMode(pm);
                tail = NULL;
                break;
            }
            if (tail)
                out << command << " : invalid permaflow mode" << endl;
        }
        // blah blah, bad code, bite me.
        else if(command == "0")
            cur_mode.amount = 0;
        else if(command == "1")
            cur_mode.amount = 1;
        else if(command == "2")
            cur_mode.amount = 2;
        else if(command == "3")
            cur_mode.amount = 3;
        else if(command == "4")
            cur_mode.amount = 4;
        else if(command == "5")
            cur_mode.amount = 5;
        else if(command == "6")
            cur_mode.amount = 6;
        else if(command == "7")
            cur_mode.amount = 7;
        else if(command.empty())
        {
            df_liquids_execute(out);
        }
        else
        {
            out << command << " : unknown command." << endl;
        }
    }
    return CR_OK;
}

command_result df_liquids_here (color_ostream &out, vector <string> & parameters)
{
    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
            return CR_WRONG_USAGE;
    }

    out.print("Run liquids-here with these parameters: ");
    print_prompt(out, cur_mode);
    out << endl;

    return df_liquids_execute(out);
}

command_result df_liquids_execute(color_ostream &out)
{
    CoreSuspender suspend;

    auto cursor = Gui::getCursorPos();
    if (!cursor.isValid())
    {
        out.printerr("Can't get cursor coords! Make sure you have a cursor active in DF.\n");
        return CR_WRONG_USAGE;
    }

    auto rv = df_liquids_execute(out, cur_mode, cursor);
    if (rv == CR_OK)
        out << "OK" << endl;
    return rv;
}

command_result df_liquids_execute(color_ostream &out, OperationMode &cur_mode, df::coord cursor)
{
    // create brush type depending on old parameters
    std::unique_ptr<Brush> brush;

    switch (cur_mode.brush)
    {
    case B_POINT:
        brush.reset(new RectangleBrush(1,1,1,0,0,0));
        break;
    case B_RANGE:
        brush.reset(new RectangleBrush(cur_mode.size.x,cur_mode.size.y,cur_mode.size.z,0,0,0));
        break;
    case B_BLOCK:
        brush.reset(new BlockBrush());
        break;
    case B_COLUMN:
        brush.reset(new ColumnBrush());
        break;
    case B_FLOOD:
        brush.reset(new FloodBrush(&Core::getInstance()));
        break;
    default:
        // this should never happen!
        out << "Old brushtype is invalid! Resetting to point brush.\n";
        cur_mode.brush = B_POINT;
        brush.reset(new RectangleBrush(1,1,1,0,0,0));
    }

    if (!Maps::IsValid())
    {
        out << "Can't see any DF map loaded." << endl;
        return CR_FAILURE;
    }

    MapCache mcache;
    coord_vec all_tiles = brush->points(mcache,cursor);

    // Force the game to recompute its walkability cache
    world->reindex_pathfinding = true;

    switch (cur_mode.paint)
    {
    case P_OBSIDIAN:
        {
            coord_vec::iterator iter = all_tiles.begin();
            while (iter != all_tiles.end())
            {
                mcache.setTiletypeAt(*iter, tiletype::LavaWall);
                mcache.setTemp1At(*iter,10015);
                mcache.setTemp2At(*iter,10015);
                df::tile_designation des = mcache.designationAt(*iter);
                des.bits.flow_size = 0;
                des.bits.flow_forbid = false;
                mcache.setDesignationAt(*iter, des);
                iter ++;
            }
            break;
        }
    case P_OBSIDIAN_FLOOR:
        {
            coord_vec::iterator iter = all_tiles.begin();
            while (iter != all_tiles.end())
            {
                mcache.setTiletypeAt(*iter, findRandomVariant(tiletype::LavaFloor1));
                iter ++;
            }
            break;
        }
    case P_RIVER_SOURCE:
        {
            coord_vec::iterator iter = all_tiles.begin();
            while (iter != all_tiles.end())
            {
                mcache.setTiletypeAt(*iter, tiletype::RiverSource);

                df::tile_designation a = mcache.designationAt(*iter);
                a.bits.liquid_type = tile_liquid::Water;
                a.bits.liquid_static = false;
                a.bits.flow_size = 7;
                mcache.setTemp1At(*iter,10015);
                mcache.setTemp2At(*iter,10015);
                mcache.setDesignationAt(*iter,a);

                Block * b = mcache.BlockAt((*iter)/16);
                b->enableBlockUpdates(true);

                iter++;
            }
            break;
        }
    case P_WCLEAN:
        {
            coord_vec::iterator iter = all_tiles.begin();
            while (iter != all_tiles.end())
            {
                DFHack::DFCoord current = *iter;
                df::tile_designation des = mcache.designationAt(current);
                des.bits.water_salt = false;
                des.bits.water_stagnant = false;
                mcache.setDesignationAt(current,des);
                iter++;
            }
            break;
        }
    case P_MAGMA:
    case P_WATER:
    case P_FLOW_BITS:
        {
            set <Block *> seen_blocks;
            coord_vec::iterator iter = all_tiles.begin();
            while (iter != all_tiles.end())
            {
                DFHack::DFCoord current = *iter; // current tile coord
                DFHack::DFCoord curblock = current /16; // current block coord
                // check if the block is actually there
                auto block = mcache.BlockAt(curblock);
                if(!block)
                {
                    iter ++;
                    continue;
                }
                auto raw_block = block->getRaw();
                df::tile_designation des = mcache.designationAt(current);
                df::tiletype tt = mcache.tiletypeAt(current);
                // don't put liquids into places where they don't belong...
                if(!DFHack::FlowPassable(tt))
                {
                    iter++;
                    continue;
                }
                if(cur_mode.paint != P_FLOW_BITS)
                {
                    unsigned old_amount = des.bits.flow_size;
                    unsigned new_amount = old_amount;
                    df::tile_liquid old_liquid = des.bits.liquid_type;
                    df::tile_liquid new_liquid = old_liquid;
                    // Compute new liquid type and amount
                    switch (cur_mode.setmode)
                    {
                    case M_KEEP:
                        new_amount = cur_mode.amount;
                        break;
                    case M_INC:
                        if(old_amount < cur_mode.amount)
                            new_amount = cur_mode.amount;
                        break;
                    case M_DEC:
                        if (old_amount > cur_mode.amount)
                            new_amount = cur_mode.amount;
                    }
                    if (cur_mode.paint == P_MAGMA)
                        new_liquid = tile_liquid::Magma;
                    else if (cur_mode.paint == P_WATER)
                        new_liquid = tile_liquid::Water;
                    // Store new amount and type
                    des.bits.flow_size = new_amount;
                    des.bits.liquid_type = new_liquid;
                    // Compute temperature
                    if (!old_amount)
                        old_liquid = tile_liquid::Water;
                    if (!new_amount)
                        new_liquid = tile_liquid::Water;
                    if (old_liquid != new_liquid)
                    {
                        if (new_liquid == tile_liquid::Water)
                        {
                            mcache.setTemp1At(current,10015);
                            mcache.setTemp2At(current,10015);
                        }
                        else
                        {
                            mcache.setTemp1At(current,12000);
                            mcache.setTemp2At(current,12000);
                        }
                    }
                    // mark the tile passable or impassable like the game does
                    des.bits.flow_forbid = (new_liquid == tile_liquid::Magma || new_amount > 3);
                    mcache.setDesignationAt(current,des);
                    // request flow engine updates
                    block->enableBlockUpdates(new_amount != old_amount, new_liquid != old_liquid);
                }
                if (cur_mode.permaflow != PF_KEEP && raw_block)
                {
                    auto &flow = raw_block->liquid_flow[current.x&15][current.y&15];
                    flow.bits.perm_flow_dir = permaflow_id[cur_mode.permaflow];
                    flow.bits.temp_flow_timer = 0;
                }
                seen_blocks.insert(block);
                iter++;
            }
            set <Block *>::iterator biter = seen_blocks.begin();
            while (biter != seen_blocks.end())
            {
                switch (cur_mode.flowmode)
                {
                case M_INC:
                    (*biter)->enableBlockUpdates(true);
                    break;
                case M_DEC:
                    if (auto block = (*biter)->getRaw())
                    {
                        block->flags.bits.update_liquid = false;
                        block->flags.bits.update_liquid_twice = false;
                    }
                    break;
                case M_KEEP:
                    {
                        auto bflags = (*biter)->BlockFlags();
                        out << "flow bit 1 = " << bflags.bits.update_liquid << endl;
                        out << "flow bit 2 = " << bflags.bits.update_liquid_twice << endl;
                    }
                }
                biter ++;
            }
            break;
        }
    }

    if(!mcache.WriteAll())
    {
        out << "Something failed horribly! RUN!" << endl;
        return CR_FAILURE;
    }

    return CR_OK;
}

static int paint(lua_State *L)
{
    df::coord pos;
    OperationMode mode;

    lua_settop(L, 8);
    Lua::CheckDFAssign(L, &pos, 1);
    if (!pos.isValid())
        luaL_argerror(L, 1, "invalid cursor position");
    mode.brush = (BrushType)luaL_checkoption(L, 2, NULL, brush_name);
    mode.paint = (PaintMode)luaL_checkoption(L, 3, NULL, paint_mode_name);
    mode.amount = luaL_optint(L, 4, 7);
    if (mode.amount < 0 || mode.amount > 7)
        luaL_argerror(L, 4, "invalid liquid amount");
    if (!lua_isnil(L, 5))
        Lua::CheckDFAssign(L, &mode.size, 5);
    mode.setmode = (ModifyMode)luaL_checkoption(L, 6, ".", modify_mode_name);
    mode.flowmode = (ModifyMode)luaL_checkoption(L, 7, "+", modify_mode_name);
    mode.permaflow = (PermaflowMode)luaL_checkoption(L, 8, ".", permaflow_name);

    lua_pushboolean(L, df_liquids_execute(*Lua::GetOutput(L), mode, pos));
    return 1;
}

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(paint),
    DFHACK_LUA_END
};
