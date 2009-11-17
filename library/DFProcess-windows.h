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

/*
DO NOT USE DIRECTLY, use DFProcess.h instead.
*/

#ifndef PROCESS_WIN_H_INCLUDED
#define PROCESS_WIN_H_INCLUDED

namespace DFHack
{
    class DFHACK_EXPORT Process
    {
        friend class ProcessEnumerator;
        private:
            Process(DataModel * dm, memory_info* mi, ProcessHandle ph, uint32_t pid);
            ~Process();
            DataModel* my_datamodel;
            memory_info * my_descriptor;
            ProcessHandle my_handle;
            uint32_t my_pid;
            string memFile;
            bool attached;
            bool suspended;
            void freeResources();
            void setMemFile(const string & memf);
        public:
            // Set up stuff so we can read memory
            bool attach();
            bool detach();
            
            bool suspend();
            bool resume();
            
            bool isSuspended() {return suspended;};
            bool isAttached() {return attached;};
            
            bool getThreadIDs(vector<uint32_t> & threads );
            void getMemRanges( vector<t_memrange> & ranges );
            // is the process still there?
            memory_info *getDescriptor();
            DataModel *getDataModel();
    };
}
#endif