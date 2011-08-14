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
DFhackCExport command_result autodig (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "vein digger";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("vdig","Dig a whole vein. With 'x' option, dig stairs between z-levels.",vdig));
    //commands.push_back(PluginCommand("autodig","Mark a tile for continuous digging.",autodig));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
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