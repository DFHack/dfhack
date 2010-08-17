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
#include "DFMemInfoManager.h"

#include "dfhack/DFProcess.h"
#include "dfhack/DFProcessEnumerator.h"
#include "dfhack/DFError.h"

#include "dfhack/DFContext.h"
#include "dfhack/DFContextManager.h"

#include <shms.h>
#include <mod-core.h>
#include <mod-maps.h>
#include <mod-creature40d.h>
#include "private/ContextShared.h"

using namespace DFHack;
namespace DFHack
{
    class ContextManager::Private
    {
        public:
            Private(){};
            ~Private(){};
            string xml; // path to xml
            vector <Context *> contexts;
            ProcessEnumerator * pEnum;
    };
}
class DFHack::BadContexts::Private
{
    public:
        Private(){};
        vector <Context *> bad;
};


BadContexts::BadContexts():d(new Private()){}

BadContexts::~BadContexts()
{
    clear();
    delete d;
}

bool BadContexts::Contains(Process* p)
{
    for(int i = 0; i < d->bad.size(); i++)
    {
        if((d->bad[i])->getProcess() == p)
            return true;
    }
    return false;
}

bool BadContexts::Contains(Context* c)
{
    for(int i = 0; i < d->bad.size(); i++)
    {
        if(d->bad[i] == c)
            return true;
    }
        return false;
}

uint32_t BadContexts::size()
{
    return d->bad.size();
}

void BadContexts::clear()
{
    for(int i = 0; i < d->bad.size(); i++)
    {
        // delete both Process and Context!
        // process has to be deleted after context, because Context does some
        // cleanup on delete (detach, etc.)
        Process * to_kill = d->bad[i]->getProcess();
        delete d->bad[i];
        delete to_kill;
    }
    d->bad.clear();
}

void BadContexts::push_back(Context* c)
{
    if(c)
        d->bad.push_back(c);
}

Context * BadContexts::operator[](uint32_t index)
{
    if(index < d->bad.size())
        return d->bad[index];
    return 0;
}

ContextManager::ContextManager (const string path_to_xml) : d (new Private())
{
    d->xml = QUOT (MEMXML_DATA_PATH);
    d->xml += "/";
    d->xml += path_to_xml;
    d->pEnum = new ProcessEnumerator(d->xml);
}

ContextManager::~ContextManager()
{
    purge();
    // process enumerator has to be destroyed after we detach from processes
    // because it tracks and destroys them
    if(d->pEnum)
    {
        delete d->pEnum;
        d->pEnum = 0;
    }
    delete d;
}

uint32_t ContextManager::Refresh( BadContexts* bad_contexts )
{
    // handle expired processes, remove stale Contexts 
    {
        BadProcesses expired;
        // get new list od living and expired Process objects
        d->pEnum->Refresh(&expired);

        // scan expired, kill contexts if necessary
        vector <Context*>::iterator it = d->contexts.begin();;
        while(it != d->contexts.end())
        {
            Process * test = (*it)->getProcess();
            if(expired.Contains(test))
            {
                // ok. we have an expired context here.
                if(!bad_contexts)
                {
                    // with nowhere to put the context, we have to destroy it
                    delete *it;
                    // stop tracking it and advance the iterator
                    it = d->contexts.erase(it);
                    continue;
                }
                else
                {
                    // we stuff the context into bad_contexts
                    bad_contexts->push_back(*it);
                    // stop tracking it and advance the iterator
                    it = d->contexts.erase(it);
                    // remove process from the 'expired' container, it is tracked by bad_contexts now
                    // (which is responsible for freeing it).
                    expired.excise(test);
                    continue;
                }
            }
            else it++; // not expired, just advance to next one
        }
        // no expired contexts are in the d->contexts vector now
        // all processes remaining in 'expired' are now destroyed along with it
    }
    int numProcesses = d->pEnum->size();
    int numContexts = d->contexts.size();
    vector <Context *> newContexts;
    // enumerate valid processes
    for(int i = 0; i < numProcesses; i++)
    {
        Process * test = d->pEnum->operator[](i);
        bool exists = false;
        // scan context vector for this process
        for(int j = 0; j < numContexts; j++)
        {
            if((d->contexts[j])->getProcess() == test)
            {
                // already have that one, skip
                exists = true;
            }
        }
        if(!exists)
        {
            // new process needs a new context
            Context * c = new Context(d->pEnum->operator[](i));
            newContexts.push_back(c);
        }
    }
    d->contexts.insert(d->contexts.end(), newContexts.begin(), newContexts.end());
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
    d->pEnum->purge();
}