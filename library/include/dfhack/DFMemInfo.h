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

#ifndef MEMINFO_H_INCLUDED
#define MEMINFO_H_INCLUDED

#include "DFPragma.h"
#include "DFExport.h"
#include "dfhack/DFTypes.h"

namespace DFHack
{
    /*
    * Stubs
    */
    class Process;
    struct t_class;

    class DFHACK_EXPORT memory_info
    {
    private:
        class Private;
        Private * d;
    public:
        enum OSType
        {
            OS_WINDOWS,
            OS_LINUX,
            OS_BAD
        };
        memory_info();
        memory_info(const memory_info&);
        ~memory_info();

        void RebaseAddresses(const int32_t new_base);
        void RebaseAll(const int32_t new_base);
        uint32_t getBase () const;
        void setBase (const std::string&);
        void setBase (const uint32_t);

        int32_t getOffset (const std::string&);
        uint32_t getAddress (const std::string&);
        uint32_t getHexValue (const std::string&);
        int32_t getOffset (const char *);
        uint32_t getAddress (const char *);
        uint32_t getHexValue (const char *);

        std::string getMood(const uint32_t moodID);
        std::string getString (const std::string&);
        std::string getProfession(const uint32_t) const;
        std::string getJob(const uint32_t) const;
        std::string getSkill (const uint32_t) const;
        std::string getTrait (const uint32_t, const uint32_t) const;
        std::string getTraitName(const uint32_t) const;
        std::string getLabor (const uint32_t);
        std::vector< std::vector<std::string> > const& getAllTraits();

        DFHack::t_level getLevelInfo(const uint32_t level) const;

        void setVersion(const char *);
        void setVersion(const std::string&);
        std::string getVersion();

        void setOS(const char *);
        void setOS(const std::string&);
        void setOS(const OSType);
        OSType getOS() const;

        void setOffset (const std::string &, const int32_t);
        void setAddress (const std::string &, const uint32_t);
        void setHexValue (const std::string &, const uint32_t);

        void setOffset (const std::string &, const char *);
        void setAddress (const std::string &, const char *);
        void setHexValue (const std::string &, const char *);
        void setString (const std::string &, const char *);

        void setOffset (const std::string &, const std::string &);
        void setAddress (const std::string &, const std::string &);
        void setHexValue (const std::string &, const std::string &);
        void setString (const std::string &, const std::string &);

        void setProfession(const std::string &, const std::string &);
        void setJob(const std::string &, const std::string &);
        void setSkill(const std::string &, const std::string &);
        void setTrait(const std::string &, const std::string &, const std::string &,
            const std::string &, const std::string &,
            const std::string &, const std::string &, const std::string &);
        void setLabor(const std::string &, const std::string &);
        void setLevel(const std::string &nLevel, const std::string &nName,
            const std::string &nXp);
        void setMood(const std::string &id, const std::string &mood);

        void RebaseVTable(const int32_t offset);
        void setParentProcess(Process * _p);

        t_class * setClass (const char * classname, uint32_t vptr = 0, uint32_t typeoffset = 0);
        void setClassChild (t_class * parent, const char * classname, const char * type);

        /**
         * Get a classID from an address. The address has to point to the start of a virtual object (one with a virtual base class)
         *   uses memory reading directly, needs suspend. input = address of the object
         *   fails if it's unable to read from memory
         */
        bool resolveObjectToClassID (const uint32_t address, int32_t & classID);

        /**
        * Get a ClassID when you know the classname. can fail if the class is not in the cache
        */
        bool resolveClassnameToClassID (const std::string classname, int32_t & classID);

        /**
        * Get a vptr from a classname. Can fail if the type is not in the cache
        * limited to normal classes, variable-dependent types will resolve to the base class
        */
        bool resolveClassnameToVPtr ( const std::string classname, uint32_t & vptr );

        /**
        * Get a classname from a previous classID. Can fail if the type is not in the cache (you use bogus classID)
        */
        bool resolveClassIDToClassname (const int32_t classID, std::string & classname);

        /**
        * Get the internal classID->classname mapping (for speed). DO NOT MANIPULATE THE VECTOR!
        */
        const std::vector<std::string> * getClassIDMapping();
        
        /**
        * Get a string with all addresses and offsets
        */
        std::string PrintOffsets();
    };
}
#endif // MEMINFO_H_INCLUDED
