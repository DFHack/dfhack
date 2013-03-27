#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <map>

#include "DFHack.h"
#include "PluginManager.h"
#include "Hooks.h"
#include <iostream>

/*
 * Plugin loading functions
 */
namespace DFHack
{
    DFLibrary * OpenPlugin (const char * filename)
    {
        dlerror();
        DFLibrary * ret =  (DFLibrary *) dlopen(filename, RTLD_NOW);
        if(!ret)
        {
            std::cerr << dlerror() << std::endl;
        }
        return ret;
    }
    void * LookupPlugin (DFLibrary * plugin ,const char * function)
    {
        return (DFLibrary *) dlsym((void *)plugin, function);
    }
    void ClosePlugin (DFLibrary * plugin)
    {
        dlclose((void *) plugin);
    }
}