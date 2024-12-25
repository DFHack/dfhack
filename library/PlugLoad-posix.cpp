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
    DFLibrary* GLOBAL_NAMES = (DFLibrary*)RTLD_DEFAULT;
    DFLibrary * OpenPlugin (const char * filename)
    {
        dlerror();
        DFLibrary * ret =  (DFLibrary *) dlopen(filename, RTLD_NOW | RTLD_LOCAL);
        if(!ret)
        {
            std::cerr << dlerror() << std::endl;
        }
        return ret;
    }
    void * LookupPlugin (DFLibrary * plugin ,const char * function)
    {
        return (void *) dlsym((void *)plugin, function);
    }
    bool ClosePlugin (DFLibrary * plugin)
    {
        // dlclose returns 0 on success, nonzero on failure
        int res = dlclose((void *) plugin);
        if (res != 0)
            WARN(plugins).print("dlclose failed with error code %x\n", res);
        return res == 0;
    }
}
