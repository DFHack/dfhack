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

#include <iostream>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <cstdlib>
#include <sstream>
using std::vector;
using std::string;
using std::endl;
using std::set;

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Vegetation.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "TileTypes.h"
#include "modules/MapCache.h"
#include "Brushes.h"
using namespace MapExtras;
using namespace DFHack;
using namespace df::enums;

CommandHistory liquids_hist;

command_result df_liquids (color_ostream &out, vector <string> & parameters);
command_result df_liquids_here (color_ostream &out, vector <string> & parameters);
command_result df_liquids_execute (color_ostream &out);

DFHACK_PLUGIN("liquids");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    liquids_hist.load("liquids.history");
    commands.push_back(PluginCommand(
        "liquids", "Place magma, water or obsidian.",
        df_liquids, true)); // interactive, needs console for prompt
    commands.push_back(PluginCommand(
        "liquids-here", "Use settings from liquids at cursor position.",
        df_liquids_here, Gui::cursor_hotkey, // non-interactive, needs ingame cursor
        "  Identical to pressing enter in liquids, intended for use as keybinding.\n"
        "  Can (but doesn't need to) be called while liquids is running in the console."));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    liquids_hist.save("liquids.history");
    return CR_OK;
}

// static stuff to be remembered between sessions
static string brushname = "point";
static string mode="magma";
static string flowmode="f+";
static string _setmode ="s.";
static unsigned int amount = 7;
static int width = 1, height = 1, z_levels = 1;

command_result df_liquids (color_ostream &out_, vector <string> & parameters)
{
    if(!out_.is_console())
        return CR_FAILURE;
    Console &out = static_cast<Console&>(out_);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out.print(  "This tool allows placing magma, water and other similar things.\n"
                        "It is interactive and further help is available when you run it.\n"
                        "The settings will be remembered until dfhack is closed and you can call\n"
                        "'liquids-here' (mapped to a hotkey) to paint liquids at the cursor position\n"
                        "without the need to go back to the dfhack console.\n");
            return CR_OK;
        }
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
        str <<"[" << mode << ":" << brushname;
        if (brushname == "range")
            str << "(w" << width << ":h" << height << ":z" << z_levels << ")";
        str << ":" << amount << ":" << flowmode << ":" << _setmode << "]#";
        if(out.lineedit(str.str(),input,liquids_hist) == -1)
            return CR_FAILURE;
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
            mode = "magma";
        }
        else if(command == "o")
        {
            mode = "obsidian";
        }
        else if(command == "of")
        {
            mode = "obsidian_floor";
        }
        else if(command == "w")
        {
            mode = "water";
        }
        else if(command == "f")
        {
            mode = "flowbits";
        }
        else if(command == "rs")
        {
            mode = "riversource";
        }
        else if(command == "wclean")
        {
            mode = "wclean";
        }
        else if(command == "point" || command == "p")
        {
            brushname = "point";
        }
        else if(command == "range" || command == "r")
        {
            command_result res = parseRectangle(out, commands, 1, commands.size(),
                                                width, height, z_levels);
            if (res != CR_OK)
            {
                return res;
            }

            if (width == 1 && height == 1 && z_levels == 1)
            {
                brushname = "point";
            }
            else
            {
                brushname = "range";
            }
        }
        else if(command == "block")
        {
            brushname = "block";
        }
        else if(command == "column")
        {
            brushname = "column";
        }
        else if(command == "flood")
        {
            brushname = "flood";
        }
        else if(command == "q")
        {
            end = true;
        }
        else if(command == "f+")
        {
            flowmode = "f+";
        }
        else if(command == "f-")
        {
            flowmode = "f-";
        }
        else if(command == "f.")
        {
            flowmode = "f.";
        }
        else if(command == "s+")
        {
            _setmode = "s+";
        }
        else if(command == "s-")
        {
            _setmode = "s-";
        }
        else if(command == "s.")
        {
            _setmode = "s.";
        }
        // blah blah, bad code, bite me.
        else if(command == "0")
            amount = 0;
        else if(command == "1")
            amount = 1;
        else if(command == "2")
            amount = 2;
        else if(command == "3")
            amount = 3;
        else if(command == "4")
            amount = 4;
        else if(command == "5")
            amount = 5;
        else if(command == "6")
            amount = 6;
        else if(command == "7")
            amount = 7;
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
        {
            out << "This command is supposed to be mapped to a hotkey." << endl;
            out << "It will use the current/last parameters set in liquids." << endl;
            return CR_OK;
        }
    }

    out.print("Run liquids-here with these parameters: ");
    out << "[" << mode << ":" << brushname;
    if (brushname == "range")
        out << "(w" << width << ":h" << height << ":z" << z_levels << ")";
    out << ":" << amount << ":" << flowmode << ":" << _setmode << "]\n";

    return df_liquids_execute(out);
}

command_result df_liquids_execute(color_ostream &out)
{
    // create brush type depending on old parameters
    Brush * brush;

    if (brushname == "point")
    {
        brush = new RectangleBrush(1,1,1,0,0,0);
        //width = 1;
        //height = 1;
        //z_levels = 1;
    }
    else if (brushname == "range")
    {
        brush = new RectangleBrush(width,height,z_levels,0,0,0);
    }
    else if(brushname == "block")
    {
        brush = new BlockBrush();
    }
    else if(brushname == "column")
    {
        brush = new ColumnBrush();
    }
    else if(brushname == "flood")
    {
        brush = new FloodBrush(&Core::getInstance());
    }
    else
    {
        // this should never happen!
        out << "Old brushtype is invalid! Resetting to point brush.\n";
        brushname = "point";
        width = 1;
        height = 1;
        z_levels = 1;
        brush = new RectangleBrush(width,height,z_levels,0,0,0);
    }

    CoreSuspender suspend;

    do
    {
        if (!Maps::IsValid())
        {
            out << "Can't see any DF map loaded." << endl;
            break;;
        }
        int32_t x,y,z;
        if(!Gui::getCursorCoords(x,y,z))
        {
            out << "Can't get cursor coords! Make sure you have a cursor active in DF." << endl;
            break;
        }
        out << "cursor coords: " << x << "/" << y << "/" << z << endl;
        MapCache mcache;
        DFHack::DFCoord cursor(x,y,z);
        coord_vec all_tiles = brush->points(mcache,cursor);
        out << "working..." << endl;

        // Force the game to recompute its walkability cache
        df::global::world->reindex_pathfinding = true;

        if(mode == "obsidian")
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
        }
        if(mode == "obsidian_floor")
        {
            coord_vec::iterator iter = all_tiles.begin();
            while (iter != all_tiles.end())
            {
                mcache.setTiletypeAt(*iter, findRandomVariant(tiletype::LavaFloor1));
                iter ++;
            }
        }
        else if(mode == "riversource")
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
        }
        else if(mode=="wclean")
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
        }
        else if(mode== "magma" || mode== "water" || mode == "flowbits")
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
                df::tile_designation des = mcache.designationAt(current);
                df::tiletype tt = mcache.tiletypeAt(current);
                // don't put liquids into places where they don't belong...
                if(!DFHack::FlowPassable(tt))
                {
                    iter++;
                    continue;
                }
                if(mode != "flowbits")
                {
                    unsigned old_amount = des.bits.flow_size;
                    unsigned new_amount = old_amount;
                    df::tile_liquid old_liquid = des.bits.liquid_type;
                    df::tile_liquid new_liquid = old_liquid;
                    // Compute new liquid type and amount
                    if(_setmode == "s.")
                    {
                        new_amount = amount;
                    }
                    else if(_setmode == "s+")
                    {
                        if(old_amount < amount)
                            new_amount = amount;
                    }
                    else if(_setmode == "s-")
                    {
                        if (old_amount > amount)
                            new_amount = amount;
                    }
                    if (mode == "magma")
                        new_liquid = tile_liquid::Magma;
                    else if (mode == "water")
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
                seen_blocks.insert(block);
                iter++;
            }
            set <Block *>::iterator biter = seen_blocks.begin();
            while (biter != seen_blocks.end())
            {
                if(flowmode == "f+")
                {
                    (*biter)->enableBlockUpdates(true);
                }
                else if(flowmode == "f-")
                {
                    if (auto block = (*biter)->getRaw())
                    {
                        block->flags.bits.update_liquid = false;
                        block->flags.bits.update_liquid_twice = false;
                    }
                }
                else
                {
                    auto bflags = (*biter)->BlockFlags();
                    out << "flow bit 1 = " << bflags.bits.update_liquid << endl; 
                    out << "flow bit 2 = " << bflags.bits.update_liquid_twice << endl;
                }
                biter ++;
            }
        }
        if(mcache.WriteAll())
            out << "OK" << endl;
        else
            out << "Something failed horribly! RUN!" << endl;
    } while (0);

    // cleanup
    delete brush;
    return CR_OK;
}
