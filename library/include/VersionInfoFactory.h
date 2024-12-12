/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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


#pragma once

#include <memory>

#include "Export.h"

class TiXmlElement;
namespace DFHack
{
    struct VersionInfo;
    class DFHACK_EXPORT VersionInfoFactory
    {
        public:
            VersionInfoFactory();
            ~VersionInfoFactory();
            bool loadFile( std::string path_to_xml);
            bool isInErrorState() const {return error;};
            std::shared_ptr<const VersionInfo> getVersionInfoByMD5(std::string md5string) const;
            std::shared_ptr<const VersionInfo> getVersionInfoByPETimestamp(uintptr_t timestamp) const;
            std::vector<std::shared_ptr<const VersionInfo>> getVersionInfosForCurOs() const;
            // trash existing list
            void clear();
        private:
            std::vector<std::shared_ptr<const VersionInfo>> versions;
            void ParseVersion (TiXmlElement* version, VersionInfo* mem);
            bool error;
    };
}
