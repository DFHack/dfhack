#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>
#include <dfhack/modules/Materials.h>
#include <stdlib.h>

using std::vector;
using std::string;
using namespace DFHack;
//FIXME: possible race conditions with calling kittens from the IO thread and shutdown from Core.
bool shutdown_flag = false;
bool final_flag = true;
bool timering = false;
uint64_t timeLast = 0;

DFhackCExport command_result rawdump (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "rawdump";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("rawdump","Dump them raws.",rawdump));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result rawdump (Core * c, vector <string> & parameters)
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
            df_inorganic_material * mat = mats->df_inorganic->at(index);
            // dump single material
            con.print("%-3d : [%s] %s\n",
                      index,
                      mat->Inorganic_ID.c_str(),
                      mat->STATE_NAME_SOLID.c_str());
            con.print("MAX EDGE: %d\n",mat->MAX_EDGE);
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
            con.print("%-3d : %s\n",i,mats->df_inorganic->at(i)->Inorganic_ID.c_str());
        }
    }
    c->Resume();
    return CR_OK;
}
