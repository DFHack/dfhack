#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <zmq.hpp>
#ifndef LINUX_BUILD
#include <windows.h>
#endif
using namespace DFHack;

// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result server (Core * c, std::vector <std::string> & parameters);

// A plugins must be able to return its name. This must correspond to the filename - skeleton.plug.so or skeleton.plug.dll
DFhackCExport const char * plugin_name ( void )
{
    return "server";
}

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.clear();
    commands.push_back(PluginCommand("server",
                                     "Inane zeromq example turned into a plugin.",
                                     server));
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown ( Core * c )
{
    // You *MUST* kill all threads you created before this returns.
    // If everythin fails, just return CR_FAILURE. Your plugin will be
    // in a zombie state, but things won't crash.
    return CR_OK;
}

// This is WRONG and STUPID. Never use this as an example!
command_result server (Core * c, std::vector <std::string> & parameters)
{
    // It's nice to provide a 'help' option for your command.
    // It's also nice to print the same help if you get invalid options from the user instead of just acting strange
    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            // Core has a handle to the console. The console is thread-safe.
            // Only one thing can read from it at a time though...
            c->con.print("This command is a simple Hello World example for zeromq!\n");
            return CR_OK;
        }
    }
    // Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5555");

    while (true)
    {
        zmq::message_t request;

        // Wait for next request from client
        socket.recv (&request);
        c->con.print("Received Hello\n");

        // Do some 'work'
#ifdef LINUX_BUILD
        sleep (1);
#else
		Sleep(1000);
#endif

        // Send reply back to client
        zmq::message_t reply (5);
        memcpy ((void *) reply.data (), "World", 5);
        socket.send (reply);
    }
    return CR_OK;
}
