#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cstdlib>
#include <sstream>
using std::vector;
using std::string;
using std::endl;

#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Maps.h>
#include <modules/Gui.h>
#include <TileTypes.h>
#include <extra/MapExtras.h>
using namespace MapExtras;
using namespace DFHack;

typedef vector <DFHack::DFCoord> coord_vec;

class Brush
{
public:
    virtual ~Brush(){};
    virtual string Name(){return "bogus";};
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
    string Name()
    {
        if(x_ == 1 && y_ == 1 && z_ == 1)
            return "point";
        else
        {
            std::stringstream str;
            str << x_ << "x" << y_;
            if(z_ != 1)
            {
                str << "x" << z_;
            }
            return str.str();
        }
    }
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
    string Name()
    {
        return "block";
    }
    coord_vec points(MapCache & mc, DFHack::DFCoord start)
    {
        coord_vec v;
        DFHack::DFCoord blockc = start % 16;
        DFHack::DFCoord iterc = blockc * 16;
        if( !mc.testCoord(start) )
            return v;

        for(int xi = 0; xi < 16; xi++)
        {
            for(int yi = 0; yi < 16; yi++)
            {
                v.push_back(iterc);
                iterc.y++;
            }
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
    string Name()
    {
        return "column";
    }
    coord_vec points(MapCache & mc, DFHack::DFCoord start)
    {
        coord_vec v;
        bool juststarted = true;
        while (mc.testCoord(start))
        {
            uint16_t tt = mc.tiletypeAt(start);
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

command_result df_tiles (Core * c, vector <string> & parameters);
command_result df_paint (Core * c, vector <string> & parameters);

struct Settings
{
    Settings()
    {
        brush = new RectangleBrush(1,1);
        mode = none;
        liquidamount = 0;
    }
    enum
    {
        none,
        water,
        magma,
        obsidian_wall,
        obsidian_floor,
        obsidian_ramp,
        river_source,
    } mode;
    int liquidamount;
    enum
    {
        set_flow,
        unset_flow,
        ignore_flow,
    } flowmode;
    enum
    {
        liquid_set,
        liquid_add,
        liquid_remove,
    } liquidmode;
    Brush * brush;
} settings;

DFHACK_PLUGIN("tiles");

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("tiles", "A tile painter. See 'tile help' for details.", df_tiles));
    commands.push_back(PluginCommand("paint", "Paint with the current tiles settings.", df_paint));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

command_result df_tiles (Core * c, vector <string> & parameters)
{
    int32_t x,y,z;
    uint32_t x_max,y_max,z_max;
    DFHack::Maps * Maps;
    DFHack::Gui * Position;
    Brush * brush = new RectangleBrush(1,1);

    Maps = c->getMaps();
    Maps->Start();
    Maps->getSize(x_max,y_max,z_max);
    Position = c->getGui();

    string command = "";
    if(command=="help" || command == "?")
    {
        c->con.print
        (
            "Usage: This command sets the properties of the tile painter brush\n"
            "       It is best used from the console, or as an alias bound to a hotkey\n"
            "       After setting the brush\n"
            "\n"
            "Modes:\n"
            "none          - nothing, the default"
            "magma [0-7]   - magma, accepts depth\n"
            "water [0-7]   - water\n"
            "obsidian      - obsidian wall\n"
            "obsfloor      - obsidian floors\n"
            "obsramp       - obsidian ramp (forces 1 z-level brush)\n"
            "riversource   - an endless source of water (floor tile)\n"
            "type ###      - plain tiletype painter. For a list of tile types see:\n"
            "                http://df.magmawiki.com/index.php/DF2010:Tile_types_in_DF_memory\n"
            "\n"
            "Set-Modes (only for magma/water):\n"
            "add           - set liquid level everywhere\n"
            "keep          - set liquid level only where liquids are already present\n"
            "\n"
            "Brush:\n"
            "point         - single tile [p]\n"
            "#x#[x#]       - block with cursor at bottom north-west [r]\n"
            "                (any place, any size)\n"
            "                Example:"
            "                3x3x2 = rectangle 3x3 x2 z-levels\n"
            "                The z-level part is optional - ommiting it is\n"
            "                the same as setting it to 1.\n"
            "h#x#[x#]      - Same as previous, only the rectangle is 'hollow'.\n"
            "block         - DF map block with cursor in it\n"
            "                (regular spaced 16x16x1 blocks)\n"
            "column        - Column from cursor, up through free space\n"
            "line          - A line between two points.\n"
            "circle [#]    - A filled circle, optional # specifies radius in tiles.\n"
            "hcircle [#,#] - A hollow circle (ring). First # specifies radius\n"
            "                second # ring thickness in tiles.\n"
            "\n"
            "Other:\n"
            "help or ?     - print this list of commands\n"
            "paint         - same effect as if you also the 'paint' command at the same time.\n"
            "\n"
        );
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
    else if(command == "point" || command == "p")
    {
        delete brush;
        brushname = "point";
        brush = new RectangleBrush(1,1);
    }
    else if(command == "range" || command == "r")
    {
        cout << " :set range width<" << width << "># ";
        getline(cin, command);
        width = command == "" ? width : atoi (command.c_str());
        if(width < 1) width = 1;

        cout << " :set range height<" << height << "># ";
        getline(cin, command);
        height = command == "" ? height : atoi (command.c_str());
        if(height < 1) height = 1;

        cout << " :set range z-levels<" << z_levels << "># ";
        getline(cin, command);
        z_levels = command == "" ? z_levels : atoi (command.c_str());
        if(z_levels < 1) z_levels = 1;
        delete brush;
        if(width == 1 && height == 1 && z_levels == 1)
        {
            brushname="point";
        }
        else
        {
            brushname = "range";
        }
        brush = new RectangleBrush(width,height,z_levels,0,0,0);
    }
    else if(command == "block")
    {
        delete brush;
        brushname = "block";
        brush = new BlockBrush();
    }
    else if(command == "column")
    {
        delete brush;
        brushname = "column";
        brush = new ColumnBrush();
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
        DF->Suspend();
        do
        {
            if(!Maps->Start())
            {
                cout << "Can't see any DF map loaded." << endl;
                break;
            }
            if(!Position->getCursorCoords(x,y,z))
            {
                cout << "Can't get cursor coords! Make sure you have a cursor active in DF." << endl;
                break;
            }
            cout << "cursor coords: " << x << "/" << y << "/" << z << endl;
            MapCache mcache(Maps);
            DFHack::DFCoord cursor(x,y,z);
            coord_vec all_tiles = brush->points(mcache,cursor);
            cout << "working..." << endl;
            if(mode == "obsidian")
            {
                coord_vec::iterator iter = all_tiles.begin();
                while (iter != all_tiles.end())
                {
                    mcache.setTiletypeAt(*iter, 331);
                    mcache.setTemp1At(*iter,10015);
                    mcache.setTemp2At(*iter,10015);
                    DFHack::t_designation des = mcache.designationAt(*iter);
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
                    mcache.setTiletypeAt(*iter, 340);
                    iter ++;
                }
            }
            else if(mode == "riversource")
            {
                set <Block *> seen_blocks;
                coord_vec::iterator iter = all_tiles.begin();
                while (iter != all_tiles.end())
                {
                    mcache.setTiletypeAt(*iter, 90);

                    DFHack::t_designation a = mcache.designationAt(*iter);
                    a.bits.liquid_type = DFHack::liquid_water;
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
            else if(mode== "magma" || mode== "water" || mode == "flowbits")
            {
                set <Block *> seen_blocks;
                coord_vec::iterator iter = all_tiles.begin();
                while (iter != all_tiles.end())
                {
                    DFHack::DFCoord current = *iter;
                    DFHack::t_designation des = mcache.designationAt(current);
                    uint16_t tt = mcache.tiletypeAt(current);
                    DFHack::naked_designation & flow = des.bits;
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
                            flow.flow_size = amount;
                        }
                        else if(setmode == "s+")
                        {
                            if(flow.flow_size < amount)
                                flow.flow_size = amount;
                        }
                        else if(setmode == "s-")
                        {
                            if (flow.flow_size > amount)
                                flow.flow_size = amount;
                        }
                        if(amount != 0 && mode == "magma")
                        {
                            flow.liquid_type =  DFHack::liquid_magma;
                            mcache.setTemp1At(current,12000);
                            mcache.setTemp2At(current,12000);
                        }
                        else if(amount != 0 && mode == "water")
                        {
                            flow.liquid_type =  DFHack::liquid_water;
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
                    seen_blocks.insert(mcache.BlockAt((*iter) / 16));
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
                        cout << "flow bit 1 = " << bflags.bits.liquid_1 << endl;
                        cout << "flow bit 2 = " << bflags.bits.liquid_2 << endl;
                    }
                    biter ++;
                }
            }
            if(mcache.WriteAll())
                cout << "OK" << endl;
            else
                cout << "Something failed horribly! RUN!" << endl;
            Maps->Finish();
        } while (0);
    }
    else
    {
        cout << command << " : unknown command." << endl;
    }
    return CR_OK;
}
