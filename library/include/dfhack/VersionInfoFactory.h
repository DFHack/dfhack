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

#ifndef MEMINFO_MANAGER_H_INCLUDED
#define MEMINFO_MANAGER_H_INCLUDED

#include "dfhack/DFPragma.h"
#include "dfhack/DFExport.h"

class TiXmlElement;
namespace DFHack
{
    class VersionInfo;
    class DFHACK_EXPORT VersionInfoFactory
    {
        friend class ProcessEnumerator;
        public:
            VersionInfoFactory(std::string path_to_xml);
            ~VersionInfoFactory();
            // memory info entries loaded from a file
            bool loadFile( std::string path_to_xml);
            bool isInErrorState() const {return error;};
            std::vector<VersionInfo*> versions;
        private:
            void ParseVTable(TiXmlElement* vtable, VersionInfo* mem);
            void ParseBase (TiXmlElement* base, VersionInfo* mem);
            void ParseVersion (TiXmlElement* version, VersionInfo* mem);
            // copy version 'base' to 'target' or throw
            void EvalVersion(std::string base, VersionInfo* target);
            void ParseOffsets(TiXmlElement* elem, VersionInfo* target, bool initial = false);

            bool error;
            typedef std::pair < TiXmlElement *, VersionInfo *> v_descr;
            std::map <std::string , v_descr  > knownVersions;
    };
}

#endif
