#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <vector>
#include <string>
#include <modules/Materials.h>
#include <stdlib.h>
/*
using std::vector;
using std::string;
using namespace DFHack;
//FIXME: possible race conditions with calling kittens from the IO thread and shutdown from Core.
bool shutdown_flag = false;
bool final_flag = true;
bool timering = false;
uint64_t timeLast = 0;

command_result rawdump_i (Core * c, vector <string> & parameters);
command_result rawdump_p (Core * c, vector <string> & parameters);

DFHACK_PLUGIN("rawdump");

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("dump_inorganic","Dump inorganic raws.",rawdump_i));
    commands.push_back(PluginCommand("dump_plants","Dump plant raws.",rawdump_p));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

command_result rawdump_i (Core * c, vector <string> & parameters)
{
    int index = -1;
    Console & con = c->con;
    if(parameters.size())
    {
        index = atoi(parameters[0].c_str());
    }
    c->Suspend();
    Materials * mats = c->getMaterials();
    if(!mats->df_inorganic)
    {
        con.printerr("No inorganic materials :(\n");
        return CR_FAILURE;
    }
    if(index >= 0)
    {
        if( index < mats->df_inorganic->size())
        {
            df_inorganic_type * mat = mats->df_inorganic->at(index);
            // dump single material
            con.print("%-3d : [%s] %s\n",
                      index,
                      mat->ID.c_str(),
                      mat->mat.state_name[DFHack::state_solid].c_str());
            con.print("MAX EDGE: %d\n",mat->mat.MAX_EDGE);
        }
        else
        {
            con.printerr("Index out of range: %d of %d\n",index, mats->df_inorganic->size());
        }
    }
    else
    {
        // dump all materials
        for(int i = 0; i < mats->df_inorganic->size();i++)
        {
            con.print("%-3d : %s\n",i,mats->df_inorganic->at(i)->ID.c_str());
        }
    }
    c->Resume();
    return CR_OK;
}

command_result rawdump_p (Core * c, vector <string> & parameters)
{
    int index = -1;
    Console & con = c->con;
    if(parameters.size())
    {
        index = atoi(parameters[0].c_str());
    }
    c->Suspend();
    Materials * mats = c->getMaterials();
    if(!mats->df_plants)
    {
        con.printerr("No inorganic materials :(\n");
        return CR_FAILURE;
    }
    if(index >= 0)
    {
        if( index < mats->df_plants->size())
        {
            df_plant_type * mat = mats->df_plants->at(index);
            // dump single material
            con.print("%-3d : [%s] %s %s %s\n",
                      index,
                      mat->ID.c_str(),
                      mat->material_str_leaves[0].c_str(),
                      mat->material_str_leaves[1].c_str(),
                      mat->material_str_leaves[2].c_str()
                     );
            con.print("DEPTH: %d-%d\n",mat->underground_depth[0], mat->underground_depth[1]);
        }
        else
        {
            con.printerr("Index out of range: %d of %d\n",index, mats->df_plants->size());
        }
    }
    else
    {
        // dump all materials
        for(int i = 0; i < mats->df_plants->size();i++)
        {
            df_plant_type * mat = mats->df_plants->at(i);
            // dump single material
            con.print("%-3d : [%s] %s %s %s\n",
                      i,
                      mat->ID.c_str(),
                      mat->material_str_leaves[0].c_str(),
                      mat->material_str_leaves[1].c_str(),
                      mat->material_str_leaves[2].c_str()
                     );
            con.print("DEPTH: %d-%d\n",mat->underground_depth[0], mat->underground_depth[1]);
        }
    }
    c->Resume();
    return CR_OK;
}
*/
