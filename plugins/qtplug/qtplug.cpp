#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Maps.h>
#include <modules/Gui.h>
//#include <extra/MapExtras.h>
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

command_result runqt (Core * c, vector <string> & parameters);

DFHACK_PLUGIN("qtplug");

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

command_result runqt (Core * c, vector <string> & parameters)
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
