/* Prevent channeling down into known open space.
Author:  Josh Cooper
Created: Aug. 4 2020
Updated: Aug. 20 2020
*/

#include "DataDefs.h"

#include "modules/Units.h"
#include "modules/World.h"
#include "df/manager_order.h"
#include "df/creature_raw.h"
#include "df/world.h"

DFHACK_PLUGIN_IS_ENABLED(is_enabled);
command_result channelSafely_cmd (color_ostream &out, vector <string> &parameters);


DFhackCExport command_result plugin_init( color_ostream &out, std::vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand("channel-safely",
                                     "A tool to manage active channel designations.",
                                     channelSafely_cmd,
                                     false,
                                     "\n"));

    return CR_OK;
}
/*
    -onTickPeriod <number of ticks|daily|monthly|yearly>
    -onJob <completion|initiation>
    -onDesignation
*/

void onNewTickPeriod(color_ostream& out, void* eventData);
void onJobCompleted(color_ostream& out, void* job);

command_result channelSafely_cmd(color_ostream &out, std::vector<string> &parameters){
    if(!parameters.empty()){
        for(int i = 0; i + 1 < parameters.size(); ++i){
            auto &p1 = parameters[i];
            auto &p2 = parameters[i+1];
            if(p1.c_str()[0] == '-' && p2.c_str()[0] != '-'){
                if(p1 == "")
            } else {
                out.printerr("Invalid parameter or invalid parameter argument.");
                return CR_FAILURE;
            }
        }
    }
    //print status
}

