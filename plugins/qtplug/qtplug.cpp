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

#include <QtGui/QApplication>
#include "blankslade.h"

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
static int runnable(void *);
static SDL::Mutex * instance_mutex = 0;
static bool running = false;
static SDL::Thread * QTThread;

DFhackCExport command_result runqt (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "Qt Test";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    instance_mutex = SDL_CreateMutex();
    commands.clear();
    commands.push_back(PluginCommand("runqt","Open an interactive Qt gui.",runqt));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result runqt (Core * c, vector <string> & parameters)
{
    SDL_mutexP(instance_mutex);
    if(!running)
    {
        running = true;
        QTThread = SDL_CreateThread(runnable, 0);
    }
    else
    {
        c->con.printerr("The Qt test plugin is already running!\n");
    }
    SDL_mutexV(instance_mutex);
    return CR_OK;
}

static int runnable(void *)
{
	int zero = 0;
    QApplication app(zero, 0);
    blankslade appGui;
    appGui.show();
    int ret = app.exec();
    SDL_mutexP(instance_mutex);
    running = false;
    SDL_mutexV(instance_mutex);
    return ret;
}
