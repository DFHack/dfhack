/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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

#include "dfhack/DFProcess.h"
#include "dfhack/DFProcessEnumerator.h"
#include "dfhack/DFContext.h"
#include "dfhack/DFError.h"
#include "dfhack/DFModule.h"

#include "private/ContextShared.h"
#include "private/ModuleFactory.h"

using namespace DFHack;

Context::Context (Process* p) : d (new DFContextShared())
{
    d->p = p;
    d->offset_descriptor = p->getDescriptor();
    d->shm_start = 0;
}

Context::~Context()
{
    Detach();
    delete d;
}

bool Context::isValid()
{
    //FIXME: check for error states here
    if(d->p->isIdentified())
        return true;
    return false;
}

bool Context::Attach()
{
    if (!d->p->attach())
    {
        //throw Error::CantAttach();
        return false;
    }
    d->shm_start = d->p->getSHMStart();
    // process is attached, everything went just fine... hopefully
    return true;
}


bool Context::Detach()
{
    if (!d->p->detach())
    {
        cerr << "Context::Detach failed!" << endl;
        return false;
    }
    d->shm_start = 0;
    // invalidate all modules
    for(unsigned int i = 0 ; i < d->allModules.size(); i++)
    {
        delete d->allModules[i];
    }
    d->allModules.clear();
    memset(&(d->s_mods), 0, sizeof(d->s_mods));
    return true;
}

bool Context::isAttached()
{
    return d->p->isAttached();
}

bool Context::Suspend()
{
    return d->p->suspend();
}
bool Context::AsyncSuspend()
{
    return d->p->asyncSuspend();
}

bool Context::Resume()
{
    for(unsigned int i = 0 ; i < d->allModules.size(); i++)
    {
        d->allModules[i]->OnResume();
    }
    return d->p->resume();
}
bool Context::ForceResume()
{
    return d->p->forceresume();
}
bool Context::isSuspended()
{
    return d->p->isSuspended();
}

void Context::ReadRaw (const uint32_t offset, const uint32_t size, uint8_t *target)
{
    d->p->read (offset, size, target);
}

void Context::WriteRaw (const uint32_t offset, const uint32_t size, uint8_t *source)
{
    d->p->write (offset, size, source);
}

VersionInfo *Context::getMemoryInfo()
{
    return d->offset_descriptor;
}

Process * Context::getProcess()
{
    return d->p;
}

/*******************************************************************************
                                M O D U L E S
*******************************************************************************/

#define MODULE_GETTER(TYPE) \
TYPE * Context::get##TYPE() \
{ \
    if(!d->s_mods.p##TYPE)\
    {\
        Module * mod = create##TYPE(d);\
        d->s_mods.p##TYPE = (TYPE *) mod;\
        d->allModules.push_back(mod);\
    }\
    return d->s_mods.p##TYPE;\
}

MODULE_GETTER(Creatures);
MODULE_GETTER(Maps);
MODULE_GETTER(Gui);
MODULE_GETTER(WindowIO);
MODULE_GETTER(World);
MODULE_GETTER(Materials);
MODULE_GETTER(Items);
MODULE_GETTER(Translation);
MODULE_GETTER(Vegetation);
MODULE_GETTER(Buildings);
MODULE_GETTER(Constructions);
