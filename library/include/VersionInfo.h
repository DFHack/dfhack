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

#include <algorithm>
#include <iostream>
#include <map>
#include <sys/types.h>
#include <vector>

#include "Export.h"

namespace DFHack
{
    /*
     * Version Info
     */
    enum OSType
    {
        OS_WINDOWS,
        OS_LINUX,
        OS_APPLE,
        OS_BAD
    };
    struct DFHACK_EXPORT VersionInfo
    {
    private:
        std::vector <std::string> md5_list;
        std::vector <uintptr_t> PE_list;
        std::map <std::string, uintptr_t> Addresses;
        std::map <std::string, uintptr_t> VTables;
        uintptr_t base;
        intptr_t rebase_delta;
        std::string version;
        OSType OS;
    public:
        VersionInfo()
        {
            base = 0; rebase_delta = 0;
            version = "invalid";
            OS = OS_BAD;
        };
        VersionInfo(const VersionInfo& rhs)
        {
            md5_list = rhs.md5_list;
            PE_list = rhs.PE_list;
            Addresses = rhs.Addresses;
            VTables = rhs.VTables;
            base = rhs.base;
            rebase_delta = rhs.rebase_delta;
            version = rhs.version;
            OS = rhs.OS;
        };

        uintptr_t getBase () const { return base; };
        intptr_t getRebaseDelta() const { return rebase_delta; }
        void setBase (const uintptr_t _base) { base = _base; };
        void rebaseTo(const uintptr_t new_base)
        {
            int64_t old = base;
            int64_t newx = new_base;
            int64_t rebase = newx - old;
            base = new_base;
            rebase_delta += rebase;
            for (auto iter = Addresses.begin(); iter != Addresses.end(); ++iter)
                iter->second += rebase;
            for (auto iter = VTables.begin(); iter != VTables.end(); ++iter)
                iter->second += rebase;
        };

        void addMD5 (const std::string & _md5)
        {
            md5_list.push_back(_md5);
        };
        bool hasMD5 (const std::string & _md5) const
        {
            return std::find(md5_list.begin(), md5_list.end(), _md5) != md5_list.end();
        };

        void addPE (uintptr_t PE_)
        {
            PE_list.push_back(PE_);
        };
        bool hasPE (uintptr_t PE_) const
        {
            return std::find(PE_list.begin(), PE_list.end(), PE_) != PE_list.end();
        };

        void setVersion(const std::string& v)
        {
            version = v;
        };
        std::string getVersion() const { return version; };

        void setAddress (const std::string& key, const uintptr_t value)
        {
            Addresses[key] = value;
        };
        template <typename T>
        bool getAddress (const std::string& key, T & value)
        {
            auto i = Addresses.find(key);
            if(i == Addresses.end())
                return false;
            value = (T) (*i).second;
            return true;
        };

        uintptr_t getAddress (const std::string& key) const
        {
            auto i = Addresses.find(key);
            if(i == Addresses.end())
                return 0;
            return (*i).second;
        }

        void setVTable (const std::string& key, const uintptr_t value)
        {
            VTables[key] = value;
        };
        void *getVTable (const std::string& key) const
        {
            auto i = VTables.find(key);
            if(i == VTables.end())
                return 0;
            return (void*)i->second;
        }
        bool getVTableName (const void *vtable, std::string &out) const
        {
            for (auto i = VTables.begin(); i != VTables.end(); ++i)
            {
                if ((void*)i->second == vtable)
                {
                    out = i->first;
                    return true;
                }
            }
            return false;
        }

        void setOS(const OSType os)
        {
            OS = os;
        };
        OSType getOS() const
        {
            return OS;
        };

        void ValidateOS() {
#if defined(_WIN32)
            const OSType expected = OS_WINDOWS;
#elif defined(_DARWIN)
            const OSType expected = OS_APPLE;
#else
            const OSType expected = OS_LINUX;
#endif
            if (expected != getOS()) {
                std::cerr << "OS mismatch; resetting to " << int(expected) << std::endl;
                setOS(expected);
            }
        }
    };
}
