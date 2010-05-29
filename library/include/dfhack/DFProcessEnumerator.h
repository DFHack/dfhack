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

#ifndef PROCESSMANAGER_H_INCLUDED
#define PROCESSMANAGER_H_INCLUDED

#include "DFPragma.h"
#include "DFExport.h"

namespace DFHack
{
    class memory_info;
    class Process;
    class BadProcesses;
    /*
     * Process manager
     */
    class DFHACK_EXPORT ProcessEnumerator
    {
        class Private;
        Private * const d;
    public:
        ProcessEnumerator( string path_to_xml );
        ~ProcessEnumerator();
        bool Refresh(BadProcesses * invalidated_processes = 0);
        bool findProcessess();
        uint32_t size();
        Process * operator[](uint32_t index);
        void purge(void);
    };
    class DFHACK_EXPORT BadProcesses
    {
        class Private;
        Private * const d;
        void push_back(Process * p);
        friend class ProcessEnumerator;
    public:
        BadProcesses();
        ~BadProcesses();
        bool Contains(Process* p);
        bool excise (Process* p);
        uint32_t size();
        void clear();
        Process * operator[](uint32_t index);
    };
}
#endif // PROCESSMANAGER_H_INCLUDED
