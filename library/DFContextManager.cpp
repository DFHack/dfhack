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

#include "dfhack/DFCommonInternal.h"

#include "dfhack/DFProcess.h"
#include "dfhack/DFProcessEnumerator.h"
#include "dfhack/DFMemInfoManager.h"
#include "dfhack/DFError.h"

#include "dfhack/DFContext.h"
#include "dfhack/DFContextManager.h"

#include <shms.h>
#include <mod-core.h>
#include <mod-maps.h>
#include <mod-creature40d.h>
#include "private/APIPrivate.h"

using namespace DFHack;
namespace DFHack
{
    class DFContextMgrPrivate
    {
        public:
            DFContextMgrPrivate(){};
            ~DFContextMgrPrivate(){};
            string xml; // path to xml
            vector <Context *> contexts;
            ProcessEnumerator * pEnum;
    };
}

ContextManager::ContextManager (const string path_to_xml) : d (new DFContextMgrPrivate())
{
    d->pEnum = 0;
    d->xml = QUOT (MEMXML_DATA_PATH);
    d->xml += "/";
    d->xml += path_to_xml;
}

ContextManager::~ContextManager()
{
    purge();
    delete d;
}

uint32_t ContextManager::Refresh()
{
    purge();
    if(d->pEnum != 0)
        d->pEnum = new ProcessEnumerator(d->xml);
    else
    {
        delete d->pEnum;
        d->pEnum = new ProcessEnumerator(d->xml);
    }
    d->pEnum->purge();
    d->pEnum->findProcessess();
    int numProcesses = d->pEnum->size();
    for(int i = 0; i < numProcesses; i++)
    {
        Context * c = new Context(d->pEnum->operator[](i));
        d->contexts.push_back(c);
    }
    return d->contexts.size();
}
uint32_t ContextManager::size()
{
    return d->contexts.size();
}
Context * ContextManager::operator[](uint32_t index)
{
    if (index < d->contexts.size())
        return d->contexts[index];
    return 0;
}
Context * ContextManager::getSingleContext()
{
    if(!d->contexts.size())
    {
        Refresh();
    }
    for(int i = 0; i < d->contexts.size();i++)
    {
        if(d->contexts[i]->isValid())
        {
            return d->contexts[i];
        }
    }
    throw DFHack::Error::NoProcess();
}
void ContextManager::purge(void)
{
    for(int i = 0; i < d->contexts.size();i++)
        delete d->contexts[i];
    d->contexts.clear();
    // process enumerator has to be destroyed after we detach from processes
    // because it tracks and destroys them
    if(d->pEnum)
    {
        delete d->pEnum;
        d->pEnum = 0;
    }
}