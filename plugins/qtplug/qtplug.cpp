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
#include "tinythread.h"

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
static void runnable(void *);
static tthread::mutex * instance_mutex = 0;
static bool running = false;
static tthread::thread * QTThread;

DFhackCExport command_result runqt (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "Qt Test";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    instance_mutex = new tthread::mutex();
    commands.clear();
    commands.push_back(PluginCommand("runqt","Open an interactive Qt gui.",runqt));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_FAILURE;
}

DFhackCExport command_result runqt (Core * c, vector <string> & parameters)
{
    instance_mutex->lock();
    if(!running)
    {
        running = true;
        QTThread = new tthread::thread(runnable, 0);
    }
    else
    {
        c->con.printerr("The Qt test plugin is already running!\n");
    }
    instance_mutex->unlock();
    return CR_OK;
}

static void runnable(void *)
{
    int zero = 0;
    QApplication app(zero, 0);
    blankslade appGui;
    appGui.show();
    app.exec();
    instance_mutex->lock();
    running = false;
    instance_mutex->unlock();
}
