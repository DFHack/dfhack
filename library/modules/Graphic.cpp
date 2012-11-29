/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mr�zek (peterix@gmail.com)

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

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdlib>
using namespace std;

#include "modules/Graphic.h"
#include "Error.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "ModuleFactory.h"
#include "Core.h"

using namespace DFHack;

Module* DFHack::createGraphic()
{
    return new Graphic();
}

struct Graphic::Private
{
    bool Started;
    vector<DFTileSurface* (*)(int,int)> funcs;
};

Graphic::Graphic()
{
    d = new Private;

    d->Started = true;
}

Graphic::~Graphic()
{
    delete d;
}

bool Graphic::Register(DFTileSurface* (*func)(int,int))
{
    d->funcs.push_back(func);
    return true;
}

bool Graphic::Unregister(DFTileSurface* (*func)(int,int))
{
    if ( d->funcs.empty() ) return false;

    vector<DFTileSurface* (*)(int,int)>::iterator it = d->funcs.begin();
    while ( it != d->funcs.end() )
    {
        if ( *it == func )
        {
            d->funcs.erase(it);
            return true;
        }
        it++;
    }

    return false;
}

// This will return first DFTileSurface it can get (or NULL if theres none)
DFTileSurface* Graphic::Call(int x, int y)
{
    if ( d->funcs.empty() ) return NULL;

    DFTileSurface* temp = NULL;

    vector<DFTileSurface* (*)(int,int)>::iterator it = d->funcs.begin();
    while ( it != d->funcs.end() )
    {
        temp = (*it)(x,y);
        if ( temp != NULL )
        {
            return temp;
        }
        it++;
    }

    return NULL;
}