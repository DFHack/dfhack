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
    /**
     * Used to enumerate, create and destroy Contexts. The very base of DFHack.
     * @see DFHack::Context
     */
    class DFHACK_EXPORT ContextManager
    {
        class Private;
        Private * const d;
    public:
        /**
        * Constructs the ContextManager.
        * @param path_to_xml the path to the file that defines memory offsets. (Memory.xml)
        */
        ContextManager(const std::string path_to_xml);

        /**
        * Destroys the ContextManager.
        */
        ~ContextManager();

        /**
        * Refresh the internal list of valid Context objects.
        * @param bad_contexts pointer to a BadContexts object. Not required. All contexts are automatically destroyed if the object is not provided.
        * @see DFHack::BadContexts
        * @return Number of tracked contexts
        */
        uint32_t Refresh(BadContexts* bad_contexts = 0);

        /**
        * Get the number of tracked contexts.
        * @return Number of tracked contexts
        */
        uint32_t size();

        /**
        * Get a context by index
        * @param index index of the context to be returned
        * @return pointer to a Context. 0 if the index is out of range.
        */
        Context * operator[](uint32_t index);

        /**
        * Convenience method to return a single valid Context
        * @return pointer to a Context
        */
        Context * getSingleContext();

        /**
        * Destroy all tracked Context objects
        * Normally called during object destruction. Calling this from outside ContextManager is nasty.
        */
        void purge(void);
    };

    /**
     * Class used for holding a set of invalidated Context AND Process objects temporarily and destroy them safely.
     * @see DFHack::Context
     * @see DFHack::Process
     */
    class DFHACK_EXPORT BadContexts
    {
        class Private;
        Private * const d;
        void push_back(Context * c);
        friend class ContextManager;
        void clear();
    public:
        BadContexts();
        /**
         * Destructor.
         * All Processes and Contexts tracked by the BadContexts object will be destroyed also.
         */
        ~BadContexts();

        /**
         * Test if a Context is among the invalidated Contexts
         * @param c pointer to a Context to be checked
         * @return true if the Context is among the invalidated. false otherwise.
         */
        bool Contains(Context* c);

        /**
         * Test if a Process is among the invalidated Processes/Contexts
         * @param p pointer to a Process to be checked
         * @see DFHack::Process
         * @return true if the Process is among the invalidated. false otherwise.
         */
        bool Contains(Process* p);

        // TODO: Add excise(Context *) method

        /**
         * Get the number of tracked invalid contexts.
         * @return Number of tracked invalid contexts
         */
        uint32_t size();

        /**
         * Get an invalid Context by index
         * @param index index of the invalid Context to be returned
         * @return pointer to an invalid Context
         */
        Context * operator[](uint32_t index);
    };
} // namespace DFHack
#endif // CONTEXTMANAGER_H_INCLUDED
