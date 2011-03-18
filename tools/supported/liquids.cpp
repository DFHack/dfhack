#include <iostream>
#include <vector>
#include <map>
#include <cstdlib>
using namespace std;

#include <DFHack.h>
#include <dfhack/extra/MapExtras.h>
#include <set>
using namespace MapExtras;

typedef vector <DFHack::DFCoord> coord_vec;

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

int main (int argc, char** argv)
{
    bool quiet = false;
    for(int i = 1; i < argc; i++)
    {
        string test = argv[i];
        if(test == "-q")
        {
            quiet = true;
        }
    }
    int32_t x,y,z;
    /*
    DFHack::designations40d designations;
    DFHack::tiletypes40d tiles;
    DFHack::t_temperatures temp1,temp2;
    */
    uint32_t x_max,y_max,z_max;

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF;
    DFHack::Maps * Maps;
    DFHack::Gui * Position;
    Brush * brush = new RectangleBrush(1,1);
    try
    {
        DF=DFMgr.getSingleContext();
        DF->Attach();
        Maps = DF->getMaps();
        Maps->Start();
        Maps->getSize(x_max,y_max,z_max);
        Position = DF->getGui();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    bool end = false;
    cout << "Welcome to the liquid spawner. type 'help' for a list of available commands, 'q' to quit." << endl;
    string mode="magma";

    string flowmode="f+";
    string setmode ="s.";
    int amount = 7;
    int width = 1, height = 1, z_levels = 1;
    while(!end)
    {
        DF->Resume();
        string command = "";
        cout <<"[" << mode << ":" << amount << ":" << flowmode << ":" << setmode << "]# ";
        getline(cin, command);
        if(command=="help")
        {
            cout << "Modes:" << endl
                 << "m             - switch to magma" << endl
                 << "w             - switch to water" << endl
                 << "o             - make obsidian wall instead" << endl
                 << "of            - make obsidian floors" << endl
                 << "rs            - make a river source" << endl
                 << "f             - flow bits only" << endl
                 << "Set-Modes:" << endl
                 << "s+            - only add" << endl
                 << "s.            - set" << endl
                 << "s-            - only remove" << endl
                 << "Properties:" << endl
                 << "f+            - make the spawned liquid flow" << endl
                 << "f.            - don't change flow state (read state in flow mode)" << endl
                 << "f-            - make the spawned liquid static" << endl
                 << "0-7           - set liquid amount" << endl
                 << "Brush:" << endl
                 << "point         - single tile [p]" << endl
                 << "range         - block with cursor at bottom north-west [r]" << endl
                 << "block         - DF map block with cursor in it" << endl
                 << "column        - Column up through free space" << endl
                 << "Other:" << endl
                 << "q             - quit" << endl
                 << "help          - print this list of commands" << endl
                 << "empty line    - put liquid" << endl
                 << endl
                 << "Usage: point the DF cursor at a tile you want to modify" << endl
                 << "and use the commands available :)" << endl;
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
            brush = new RectangleBrush(width,height,z_levels,0,0,0);
        }
        else if(command == "block")
        {
            delete brush;
            brush = new BlockBrush();
        }
        else if(command == "column")
        {
            delete brush;
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
                cout << "getting tiles from brush" << endl;
                DFHack::DFCoord cursor(x,y,z);
                coord_vec all_tiles = brush->points(mcache,cursor);
                cout << "doing things" << endl;
                if(mode == "obsidian")
                {
                    coord_vec::iterator iter = all_tiles.begin();
                    while (iter != all_tiles.end())
                    {
                        mcache.setTiletypeAt(*iter, 331);
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
                        if(mode != "flowbits")
                        {
                            if(!DFHack::FlowPassable(tt))
                            {
                                iter++;
                                continue;
                            }
                            // if we are setting the levels to 0 or changing magma into water
                            if(amount == 0 || des.bits.liquid_type == DFHack::liquid_magma && mode == "water")
                            {
                                // reset temperature to sane default
                                mcache.setTemp1At(current,10015);
                                mcache.setTemp2At(current,10015);
                            }
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
                            if(mode == "magma")
                                flow.liquid_type =  DFHack::liquid_magma;
                            else if(mode == "water")
                                flow.liquid_type =  DFHack::liquid_water;
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
    }
    DF->Detach();
    #ifndef LINUX_BUILD
        if(!quiet)
        {
            cout << "Done. Press any key to continue" << endl;
            cin.ignore();
        }
    #endif
    return 0;
}
