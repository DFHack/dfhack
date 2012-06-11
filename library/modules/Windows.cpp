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

#include "Export.h"
#include "Module.h"
#include "BitArray.h"
#include <string>

#include "DataDefs.h"
#include "df/init.h"
#include "df/ui.h"
#include <df/graphic.h>
#include "modules/Windows.h"

using namespace DFHack;
using df::global::gps;

Windows::df_screentile *Windows::getScreenBuffer()
{
    if (!gps) return NULL;
    return (df_screentile *) gps->screen;
}

Windows::df_window::df_window(int x, int y, unsigned int width, unsigned int height)
:buffer(0), width(width), height(height), parent(0), left(x), top(y), current_painter(NULL)
{
    buffer = 0;
};
Windows::df_window::~df_window()
{
    for(auto iter = children.begin();iter != children.end();iter++)
    {
        delete *iter;
    }
    children.clear();
};
Windows::painter * Windows::df_window::lock()
{
    locked = true;
    current_painter = new Windows::painter(this,buffer,width, height);
    return current_painter;
};

bool Windows::df_window::addChild( df_window * child)
{
    children.push_back(child);
    child->parent = this;
    return true;
}

bool Windows::df_window::unlock (painter * painter)
{
    if(current_painter == painter)
    {
        delete current_painter;
        current_painter = 0;
        locked = false;
        return true;
    }
    return false;
}

Windows::top_level_window::top_level_window() : df_window(0,0,gps ? gps->dimx : 80,gps ? gps->dimy : 25)
{
    buffer = 0;
}

bool Windows::top_level_window::move (int left_, int top_, unsigned int width_, unsigned int height_)
{
    width = width_;
    height = height_;
    // what if we are painting already? Is that possible?
    return true;
};

Windows::painter * Windows::top_level_window::lock()
{
    buffer = getScreenBuffer();
    return df_window::lock();
}

void Windows::top_level_window::paint ()
{
    for(auto iter = children.begin();iter != children.end();iter++)
    {
        (*iter)->paint();
    }
};

Windows::df_tilebuf Windows::top_level_window::getBuffer()
{
    df_tilebuf buf;
    buf.data = getScreenBuffer();
    buf.height = df::global::gps->dimy;
    buf.width = df::global::gps->dimx;
    return buf;
}