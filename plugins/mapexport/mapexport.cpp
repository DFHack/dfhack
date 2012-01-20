#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include "proto/Map.pb.h"
using namespace DFHack;

DFhackCExport command_result mapexport (Core * c, std::vector <std::string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "mapexport";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    commands.clear();
    commands.push_back(PluginCommand("mapexport", "Starts up and shuts down protobufs.", mapexport, true));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    google::protobuf::ShutdownProtobufLibrary();
    return CR_OK;
}

DFhackCExport command_result mapexport (Core * c, std::vector <std::string> & parameters)
{
    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            c->con.print("This doesn't do anything at all yet.\n");
            return CR_OK;
        }
    }

    c->Suspend();
    dfproto::Tile tile;
    c->con.print("Hold on, I'm working on it!\n");
    c->Resume();
    return CR_OK;
}
