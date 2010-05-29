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

#ifndef CONTEXTMANAGER_H_INCLUDED
#define CONTEXTMANAGER_H_INCLUDED


#include "DFPragma.h"
#include "DFExport.h"
#include <string>
#include <vector>
#include <map>

namespace DFHack
{
    class Context;
    class BadContexts;
    class Process;

    class DFHACK_EXPORT ContextManager
    {
        class Private;
        Private * const d;
    public:
        ContextManager(const std::string path_to_xml);
        ~ContextManager();
        uint32_t Refresh(BadContexts* bad_contexts = 0);
        uint32_t size();
        Context * operator[](uint32_t index);
        Context * getSingleContext();
        void purge(void);
    };
    class DFHACK_EXPORT BadContexts
    {
        class Private;
        Private * const d;
        void push_back(Context * c);
        friend class ContextManager;
    public:
        BadContexts();
        ~BadContexts();
        bool Contains(Context* c);
        bool Contains(Process* p);
        uint32_t size();
        void clear();
        Context * operator[](uint32_t index);
    };
} // namespace DFHack
#endif // CONTEXTMANAGER_H_INCLUDED
