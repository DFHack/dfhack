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

#ifndef PROCESS_H_INCLUDED
#define PROCESS_H_INCLUDED

#include "Export.h"

namespace DFHack
{
    class memory_info;
    class DataModel;
    class Process;
    
    // structure describing a memory range
    struct DFHACK_EXPORT t_memrange
    {
        uint64_t start;
        uint64_t end;
        // memory range name (if any)
        char name[1024];
        // permission to read
        bool read;
        // permission to write
        bool write;
        // permission to execute
        bool execute;
        inline bool isInRange( uint64_t address)
        {
            if (address >= start && address <= end) return true;
            return false;
        }
        inline void print()
        {
            cout << hex << start << " - " << end << "|" << (read ? "r" : "-") << (write ? "w" : "-") << (execute ? "x" : "-") << "|" << name << endl;
        }
    };

    class DFHACK_EXPORT Process
    {
        friend class ProcessEnumerator;
        class Private;
        private:
            Private * const d;
            Process(uint32_t pid, vector <memory_info> & known_versions);
            ~Process();
        public:
            // Set up stuff so we can read memory
            bool attach();
            bool detach();
            
            bool suspend();
            bool resume();
            bool forceresume();
            
            bool isSuspended();
            bool isAttached();
            bool isIdentified();
            
            bool getThreadIDs(vector<uint32_t> & threads );
            void getMemRanges( vector<t_memrange> & ranges );
            // is the process still there?
            memory_info *getDescriptor();
            DataModel *getDataModel();
    };
}
#endif