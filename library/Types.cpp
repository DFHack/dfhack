/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include "Export.h"
#include "MiscUtils.h"
#include "Error.h"
#include "Types.h"

#include "modules/Filesystem.h"

#ifndef LINUX_BUILD
    #include <Windows.h>
    #include "wdirent.h"
#else
    #include <sys/time.h>
    #include <ctime>
    #include <dirent.h>
    #include <errno.h>
#endif

#include <ctype.h>
#include <stdarg.h>

#include <sstream>
#include <map>


int DFHack::getdir(std::string dir, std::vector<std::string> &files)
{
    return DFHack::Filesystem::listdir(dir, files);
}

bool DFHack::hasEnding (std::string const &fullString, std::string const &ending)
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

df::general_ref *DFHack::findRef(std::vector<df::general_ref*> &vec, df::general_ref_type type)
{
    for (int i = vec.size()-1; i >= 0; i--)
    {
        df::general_ref *ref = vec[i];
        if (ref->getType() == type)
            return ref;
    }

    return NULL;
}

bool DFHack::removeRef(std::vector<df::general_ref*> &vec, df::general_ref_type type, int id)
{
    for (int i = vec.size()-1; i >= 0; i--)
    {
        df::general_ref *ref = vec[i];
        if (ref->getType() != type || ref->getID() != id)
            continue;

        vector_erase_at(vec, i);
        delete ref;
        return true;
    }

    return false;
}

df::item *DFHack::findItemRef(std::vector<df::general_ref*> &vec, df::general_ref_type type)
{
    auto ref = findRef(vec, type);
    return ref ? ref->getItem() : NULL;
}

df::building *DFHack::findBuildingRef(std::vector<df::general_ref*> &vec, df::general_ref_type type)
{
    auto ref = findRef(vec, type);
    return ref ? ref->getBuilding() : NULL;
}

df::unit *DFHack::findUnitRef(std::vector<df::general_ref*> &vec, df::general_ref_type type)
{
    auto ref = findRef(vec, type);
    return ref ? ref->getUnit() : NULL;
}

df::specific_ref *DFHack::findRef(std::vector<df::specific_ref*> &vec, df::specific_ref_type type)
{
    for (int i = vec.size()-1; i >= 0; i--)
    {
        df::specific_ref *ref = vec[i];
        if (ref->type == type)
            return ref;
    }

    return NULL;
}

bool DFHack::removeRef(std::vector<df::specific_ref*> &vec, df::specific_ref_type type, void *ptr)
{
    for (int i = vec.size()-1; i >= 0; i--)
    {
        df::specific_ref *ref = vec[i];
        if (ref->type != type || ref->object != ptr)
            continue;

        vector_erase_at(vec, i);
        delete ref;
        return true;
    }

    return false;
}
