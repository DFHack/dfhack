/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "Internal.h"
#include "dfhack/Core.h"
#include "dfhack/Process.h"
#include "dfhack/PluginManager.h"
#include "dfhack/Console.h"
using namespace DFHack;

#include <string>
#include <vector>
#include <map>
using namespace std;

#ifdef LINUX_BUILD
    #include <dirent.h>
    #include <errno.h>
#else
    #include "wdirent.h"
#endif

static int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
    {
        dfout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }
    while ((dirp = readdir(dp)) != NULL) {
    files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

bool hasEnding (std::string const &fullString, std::string const &ending)
{
    if (fullString.length() > ending.length())
    {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    }
    else
    {
        return false;
    }
}

Plugin::Plugin(Core * core, const std::string & file)
{
    filename = file;
    plugin_lib = 0;
    plugin_init = 0;
    plugin_shutdown = 0;
    plugin_status = 0;
    loaded = false;
    DFLibrary * plug = OpenPlugin(file.c_str());
    if(!plug)
    {
        dfout << "Can't load plugin " << filename << endl;
        return;
    }
    const char * (*_PlugName)() =(const char * (*)()) LookupPlugin(plug, "plugin_name");
    if(!_PlugName)
    {
        dfout << "Plugin " << filename << " has no name." << endl;
        ClosePlugin(plug);
        return;
    }
    plugin_init = (command_result (*)(Core *, std::vector <PluginCommand> &)) LookupPlugin(plug, "plugin_init");
    if(!plugin_init)
    {
        dfout << "Plugin " << filename << " has no init function." << endl;
        ClosePlugin(plug);
        return;
    }
    plugin_status = (command_result (*)(Core *, std::string &)) LookupPlugin(plug, "plugin_status");
    plugin_shutdown = (command_result (*)(Core *)) LookupPlugin(plug, "plugin_shutdown");
    name = _PlugName();
    plugin_lib = plug;
    loaded = true;
    dfout << "Found plugin " << name << endl;
    if(plugin_init(core,commands) == CR_OK)
    {
        for(int i = 0; i < commands.size();i++)
        {
            dfout << commands[i].name << " : " << commands[i].description << std::endl;
        }
    }
    else
    {
        // horrible!
    }
}

Plugin::~Plugin()
{
    if(loaded)
        ClosePlugin(plugin_lib);
}

bool Plugin::isLoaded()
{
    return loaded;
}

PluginManager::PluginManager(Core * core)
{
#ifdef LINUX_BUILD
    string path = core->p->getPath() + "/plugins/";
    const string searchstr = ".plug.so";
#else
    string path = core->p->getPath() + "\\plugins\\";
    const string searchstr = ".plug.dll";
#endif
    vector <string> filez;
    getdir(path, filez);
    for(int i = 0; i < filez.size();i++)
    {
        if(hasEnding(filez[i],searchstr))
        {
            Plugin * p = new Plugin(core, path + filez[i]);
            for(int j = 0; j < p->commands.size();j++)
            {
                commands[p->commands[j].name] = &p->commands[j];
            }
            all_plugins.push_back(p);
        }
    }
}

PluginManager::~PluginManager()
{
    
}
Plugin *PluginManager::getPluginByName (const std::string & name)
{
    
}
command_result PluginManager::InvokeCommand( std::string & command, std::vector <std::string> & parameters)
{
    Core * c = &Core::getInstance();
    map <string, PluginCommand *>::iterator iter = commands.find(command);
    if(iter != commands.end())
    {
        return iter->second->function(c,parameters);
    }
    return CR_NOT_IMPLEMENTED;
}
/*
for (map <string, int (*)(Core *)>::iterator iter = plugins.begin(); iter != plugins.end(); iter++)
{
    dfout << iter->first << endl;
}
*/
/*

*/