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
    class VersionInfo;
    class Process;
    class BadProcesses;
    /**
     * Process enumerator
     * Used to enumerate, create and destroy Processes.
     * @see DFHack::Process
     */
    class DFHACK_EXPORT ProcessEnumerator
    {
        class Private;
        Private * const d;
    public:
        /**
         * Constructs the ProcessEnumerator.
         * @param path_to_xml the path to the file that defines memory offsets. (Memory.xml)
         */
        ProcessEnumerator( string path_to_xml );

        /**
         * Destroys the ProcessEnumerator.
         */
        ~ProcessEnumerator();

        /**
         * Refresh the internal list of valid Process objects.
         * @param invalidated_processes pointer to a BadProcesses object. Not required. All processes are automatically destroyed if the object is not provided.
         * @see DFHack::BadProcesses
         * @return true if there's at least one valit Process
         */
        bool Refresh(BadProcesses * invalidated_processes = 0);

        /**
         * @return Number of tracked processes
         */
        uint32_t size();

        /**
         * Get a Process by index
         * @param index index of the Process to be returned
         * @return pointer to a Process. 0 if the index is out of range.
         */
        Process * operator[](uint32_t index);

        /**
         * Destroy all tracked Process objects
         * Normally called during object destruction. Calling this from outside ProcessManager is nasty.
         */
        void purge(void);
    };

    /**
     * Class used for holding a set of invalidated Process objects temporarily and destroy them safely.
     * @see DFHack::Process
     */
    class DFHACK_EXPORT BadProcesses
    {
        class Private;
        Private * const d;
        void push_back(Process * p);
        friend class ProcessEnumerator;
        void clear();
    public:
        BadProcesses();
        /**
         * Destructor.
         * All Processes tracked by the BadProcesses object will be destroyed also.
         */
        ~BadProcesses();

        /**
         * Test if a Process is among the invalidated Processes
         * @param p pointer to a Process to be checked
         * @return true if the Process is among the invalidated. false otherwise.
         */
        bool Contains(Process* p);

        /**
         * Remove a Process from the tracked invalidated Processes. Used by BadContexts.
         * @param p pointer to a Process to be excised
         * @see DFHack::BadContexts
         * @return true if the Process was among the invalidated. false otherwise.
         */
        bool excise (Process* p);

        /**
         * Get the number of tracked invalid contexts.
         * @return Number of tracked invalid contexts
         */
        uint32_t size();

        /**
         * Get an invalid Process by index
         * @param index index of the invalid Process to be returned
         * @return pointer to an invalid Process
         */
        Process * operator[](uint32_t index);
    };
}
#endif // PROCESSMANAGER_H_INCLUDED
