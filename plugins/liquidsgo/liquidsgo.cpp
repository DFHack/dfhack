// plugin liquidsgo
//
// This is a rewrite of the liquids module which can also be used non-interactively (hotkey).
// First the user sets the mode and other parameters with the interactive command liqiudsgo
// just like in the original liquids module.
// They are stored in statics to allow being used after the interactive session was closed.
// After defining an action the non-interactive command liquidsgo-here can be used to call the 
// execute method without the necessity to go back to the console. This allows convenient painting
// of liquids and obsidian using the ingame cursor and a hotkey.
//
// Commands:
// liquidsgo      - basically the original liquids with the map changing stuff moved to an execute method
// liquidsgo-here - runs the execute method with the last settings from liquidsgo
//                  (intended to be mapped to a hotkey)
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
using namespace MapExtras;
using namespace DFHack;
using namespace df::enums;
typedef vector <df::coord> coord_vec;

CommandHistory liquidsgo_hist;

command_result df_liquidsgo (color_ostream &out, vector <string> & parameters);
command_result df_liquidsgo_here (color_ostream &out, vector <string> & parameters);
command_result df_liquidsgo_execute (color_ostream &out);

DFHACK_PLUGIN("liquidsgo");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    liquidsgo_hist.load("liquidsgo.history");
    commands.clear();
    commands.push_back(PluginCommand(
        "liquidsgo", "Place magma, water or obsidian.", 
        df_liquidsgo, true)); // interactive, needs console for prompt
    commands.push_back(PluginCommand(
        "liquidsgo-here", "Use settings from liquidsgo at cursor position.",
        df_liquidsgo_here, Gui::cursor_hotkey, // non-interactive, needs ingame cursor
        "  Identical to pressing enter in liquidsgo, intended for use as keybinding.\n"
        "  Can (but doesn't need to) be called while liquidsgo is running in the console."));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    liquidsgo_hist.save("liquidsgo.history");
    return CR_OK;
}

class Brush
{
public:
    virtual ~Brush(){};
    virtual coord_vec points(MapCache & mc,DFHack::DFCoord start) = 0;
};
/**
 * generic 3D rectangle brush. you can specify the dimensions of
 * the rectangle and optionally which tile is its 'center'
 */
class RectangleBrush : public Brush
{
public:
    RectangleBrush(int x, int y, int z = 1, int centerx = -1, int centery = -1, int centerz = -1)
    {
        if(centerx == -1)
            cx_ = x/2;
        else
            cx_ = centerx;
        if(centery == -1)
            cy_ = y/2;
        else
            cy_ = centery;
        if(centerz == -1)
            cz_ = z/2;
        else
            cz_ = centerz;
        x_ = x;
        y_ = y;
        z_ = z;
    };
    coord_vec points(MapCache & mc, DFHack::DFCoord start)
    {
        coord_vec v;
        DFHack::DFCoord iterstart(start.x - cx_, start.y - cy_, start.z - cz_);
        DFHack::DFCoord iter = iterstart;
        for(int xi = 0; xi < x_; xi++)
        {
            for(int yi = 0; yi < y_; yi++)
            {
                for(int zi = 0; zi < z_; zi++)
                {
                    if(mc.testCoord(iter))
                        v.push_back(iter);
                    iter.z++;
                }
                iter.z = iterstart.z;
                iter.y++;
            }
            iter.y = iterstart.y;
            iter.x ++;
        }
        return v;
    };
    ~RectangleBrush(){};
private:
    int x_, y_, z_;
    int cx_, cy_, cz_;
};

/**
 * stupid block brush, legacy. use when you want to apply something to a whole DF map block.
 */
class BlockBrush : public Brush
{
public:
    BlockBrush(){};
    ~BlockBrush(){};
    coord_vec points(MapCache & mc, DFHack::DFCoord start)
    {
        coord_vec v;
        DFHack::DFCoord blockc = start / 16;
        DFHack::DFCoord iterc = blockc * 16;
        if( !mc.testCoord(start) )
            return v;
        auto starty = iterc.y;
        for(int xi = 0; xi < 16; xi++)
        {
            for(int yi = 0; yi < 16; yi++)
            {
                v.push_back(iterc);
                iterc.y++;
            }
            iterc.y = starty;
            iterc.x ++;
        }
        return v;
    };
};

/**
 * Column from a position through open space tiles
 * example: create a column of magma
 */
class ColumnBrush : public Brush
{
public:
    ColumnBrush(){};
    ~ColumnBrush(){};
    coord_vec points(MapCache & mc, DFHack::DFCoord start)
    {
        coord_vec v;
        bool juststarted = true;
        while (mc.testCoord(start))
        {
            df::tiletype tt = mc.tiletypeAt(start);
            if(DFHack::LowPassable(tt) || juststarted && DFHack::HighPassable(tt))
            {
                v.push_back(start);
                juststarted = false;
                start.z++;
            }
            else break;
        }
        return v;
    };
};

/**
 * Flood-fill water tiles from cursor (for wclean)
 * example: remove salt flag from a river
 */
class FloodBrush : public Brush
{
public:
    FloodBrush(Core *c){c_ = c;};
    ~FloodBrush(){};
    coord_vec points(MapCache & mc, DFHack::DFCoord start)
    {
        coord_vec v;

		std::stack<DFCoord> to_flood;
	    to_flood.push(start);

		std::set<DFCoord> seen;

		while (!to_flood.empty()) {
			DFCoord xy = to_flood.top();
			to_flood.pop();

                        df::tile_designation des = mc.designationAt(xy);

			if (seen.find(xy) == seen.end() 
                            && des.bits.flow_size
                            && des.bits.liquid_type == tile_liquid::Water) {
				v.push_back(xy);
				seen.insert(xy);

				maybeFlood(DFCoord(xy.x - 1, xy.y, xy.z), to_flood, mc);
				maybeFlood(DFCoord(xy.x + 1, xy.y, xy.z), to_flood, mc);
				maybeFlood(DFCoord(xy.x, xy.y - 1, xy.z), to_flood, mc);
				maybeFlood(DFCoord(xy.x, xy.y + 1, xy.z), to_flood, mc);

				df::tiletype tt = mc.tiletypeAt(xy);
				if (LowPassable(tt))
				{
					maybeFlood(DFCoord(xy.x, xy.y, xy.z - 1), to_flood, mc);
				}
				if (HighPassable(tt))
				{
					maybeFlood(DFCoord(xy.x, xy.y, xy.z + 1), to_flood, mc);
				}
			}
		}

		return v;
	}
private:
	void maybeFlood(DFCoord c, std::stack<DFCoord> &to_flood, MapCache &mc) {
		if (mc.testCoord(c)) {
			to_flood.push(c);
		}
	}
	Core *c_;
};


// static stuff to be remembered between sessions
static string brushname = "point";
static string mode="magma";
static string flowmode="f+";
static string setmode ="s.";
static unsigned int amount = 7;
static int width = 1, height = 1, z_levels = 1;

command_result df_liquidsgo (color_ostream &out_, vector <string> & parameters)
{
    assert(out_.is_console());
    Console &out = static_cast<Console&>(out_);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out.print(  "This tool allows placing magma, water and other similar things.\n"
                        "It is interactive and further help is available when you run it.\n"
                        "The settings will be remembered until dfhack is closed and you can call\n"
                        "'liquidsgo-here' (mapped to a hotkey) to paint liquids at the cursor position\n"
                        "without the need to go back to the dfhack console.\n");
            return CR_OK;
        }
    }

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    bool end = false;

    out << "Welcome to the liquid spawner.\nType 'help' or '?' for a list of available commands, 'q' to quit.\nPress return after a command to confirm." << std::endl;

    while(!end)
    {
        string command = "";

        std::stringstream str;
        str <<"[" << mode << ":" << brushname;
        if (brushname == "range")
            str << "(w" << width << ":h" << height << ":z" << z_levels << ")";
        str << ":" << amount << ":" << flowmode << ":" << setmode << "]#";
        if(out.lineedit(str.str(),command,liquidsgo_hist) == -1)
            return CR_FAILURE;

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
            out << endl << "Settings will be remembered until you quit DF. You can call liquidsgo-here to execute the last configured action. Useful in combination with keybindings." << endl;
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
            std::stringstream str;
            CommandHistory range_hist;
            str << " :set range width<" << width << "># ";
            out.lineedit(str.str(),command,range_hist);
            range_hist.add(command);
            width = command == "" ? width : atoi (command.c_str());
            if(width < 1) width = 1;

            str.str("");
            str << " :set range height<" << height << "># ";
            out.lineedit(str.str(),command,range_hist);
            range_hist.add(command);
            height = command == "" ? height : atoi (command.c_str());
            if(height < 1) height = 1;

            str.str("");
            str << " :set range z-levels<" << z_levels << "># ";
            out.lineedit(str.str(),command,range_hist);
            range_hist.add(command);
            z_levels = command == "" ? z_levels : atoi (command.c_str());
            if(z_levels < 1) z_levels = 1;
            if(width == 1 && height == 1 && z_levels == 1)
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
            setmode = "s+";
        }
        else if(command == "s-")
        {
            setmode = "s-";
        }
        else if(command == "s.")
        {
            setmode = "s.";
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
            df_liquidsgo_execute(out);
        }
        else
        {
            out << command << " : unknown command." << endl;
        }
    }
    
    return CR_OK;
}

command_result df_liquidsgo_here (color_ostream &out, vector <string> & parameters)
{
    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out << "This command is supposed to be mapped to a hotkey." << endl;
            out << "It will use the current/last parameters set in liquidsgo." << endl;
            return CR_OK;
        }
    }

    out.print("Run liquidsgo-here with these parameters: ");
    out << "[" << mode << ":" << brushname;
    if (brushname == "range")
        out << "(w" << width << ":h" << height << ":z" << z_levels << ")";
    out << ":" << amount << ":" << flowmode << ":" << setmode << "]\n";

    return df_liquidsgo_execute(out);
}

command_result df_liquidsgo_execute(color_ostream &out)
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
                DFHack::t_blockflags bf = b->BlockFlags();
                bf.bits.liquid_1 = true;
                bf.bits.liquid_2 = true;
                b->setBlockFlags(bf);

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
                if(!mcache.BlockAt(curblock))
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
                    if(setmode == "s.")
                    {
                        des.bits.flow_size = amount;
                    }
                    else if(setmode == "s+")
                    {
                        if(des.bits.flow_size < amount)
                            des.bits.flow_size = amount;
                    }
                    else if(setmode == "s-")
                    {
                        if (des.bits.flow_size > amount)
                            des.bits.flow_size = amount;
                    }
                    if(amount != 0 && mode == "magma")
                    {
                        des.bits.liquid_type =  tile_liquid::Magma;
                        mcache.setTemp1At(current,12000);
                        mcache.setTemp2At(current,12000);
                    }
                    else if(amount != 0 && mode == "water")
                    {
                        des.bits.liquid_type =  tile_liquid::Water;
                        mcache.setTemp1At(current,10015);
                        mcache.setTemp2At(current,10015);
                    }
                    else if(amount == 0 && (mode == "water" || mode == "magma"))
                    {
                        // reset temperature to sane default
                        mcache.setTemp1At(current,10015);
                        mcache.setTemp2At(current,10015);
                    }
                    mcache.setDesignationAt(current,des);
                }
                seen_blocks.insert(mcache.BlockAt(current / 16));
                iter++;
            }
            set <Block *>::iterator biter = seen_blocks.begin();
            while (biter != seen_blocks.end())
            {
                DFHack::t_blockflags bflags = (*biter)->BlockFlags();
                if(flowmode == "f+")
                {
                    bflags.bits.liquid_1 = true;
                    bflags.bits.liquid_2 = true;
                    (*biter)->setBlockFlags(bflags);
                }
                else if(flowmode == "f-")
                {
                    bflags.bits.liquid_1 = false;
                    bflags.bits.liquid_2 = false;
                    (*biter)->setBlockFlags(bflags);
                }
                else
                {
                    out << "flow bit 1 = " << bflags.bits.liquid_1 << endl; 
                    out << "flow bit 2 = " << bflags.bits.liquid_2 << endl;
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
