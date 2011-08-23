/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

#ifndef MEMINFO_H_INCLUDED
#define MEMINFO_H_INCLUDED

#include "dfhack/Pragma.h"
#include "dfhack/Export.h"
#include "dfhack/Types.h"
#include <maps>
#include <sys/types.h>
#include <vector>

namespace DFHack
{
    /*
    * Stubs
    */
    class Process;
    class XMLPP;
    struct t_class;
    class VersionInfoPrivate;
    class OffsetGroupPrivate;

    enum INVAL_TYPE
    {
        NOT_SET,
        IS_INVALID,
        IS_VALID
    };

    /*
     * Offset Group
     */
    class DFHACK_EXPORT OffsetGroup
    {
    protected:
        OffsetGroupPrivate * OGd;
    public:
        OffsetGroup();
        OffsetGroup(const std::string & _name, OffsetGroup * parent = 0);
        ~OffsetGroup();

        void copy(const OffsetGroup * old); // recursive
        void RebaseAddresses( int32_t offset ); // recursive

        void createOffset (const std::string & key);
        void createAddress (const std::string & key);
        void createHexValue (const std::string & key);
        void createString (const std::string & key);
        OffsetGroup * createGroup ( const std::string & name );

        int32_t getOffset (const std::string & key);
        uint32_t getAddress (const std::string & key);
        uint32_t getHexValue (const std::string & key);
        std::string getString (const std::string & key);
        OffsetGroup * getGroup ( const std::string & name );

        bool getSafeOffset (const std::string & key, int32_t & out);
        bool getSafeAddress (const std::string & key, uint32_t & out);

        void setOffset (const std::string& key, const std::string& value, const DFHack::INVAL_TYPE inval = IS_VALID);
        void setOffsetValidity(const std::string& key, const DFHack::INVAL_TYPE inval = IS_VALID);
        void setAddress (const std::string& key, const std::string& value, const DFHack::INVAL_TYPE inval = IS_VALID);
        void setAddressValidity(const std::string& key, const DFHack::INVAL_TYPE inval = IS_VALID);
        void setHexValue (const std::string& key, const std::string& value, const DFHack::INVAL_TYPE inval = IS_VALID);
        void setHexValueValidity(const std::string& key, const DFHack::INVAL_TYPE inval = IS_VALID);
        void setString (const std::string& key, const std::string& value, const DFHack::INVAL_TYPE inval = IS_VALID);
        void setStringValidity(const std::string& key, const DFHack::INVAL_TYPE inval = IS_VALID);
        std::string PrintOffsets(int indentation);
        std::string getName();
        std::string getFullName();
        OffsetGroup * getParent();
        void setInvalid(INVAL_TYPE arg1);
    };

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
    class DFHACK_EXPORT VersionInfo : public OffsetGroup
    {
    private:
        VersionInfoPrivate * d;
    public:
        VersionInfo();
        VersionInfo(const VersionInfo&);
        void copy(const DFHack::VersionInfo* old);
        ~VersionInfo();

        void RebaseAddresses(const int32_t new_base);
        void RebaseAll(const int32_t new_base);
        uint32_t getBase () const;
        void setBase (const std::string&);
        void setBase (const uint32_t);

        void setMD5 (const std::string & _md5);
        bool getMD5(std::string & output);

        void setPE (uint32_t PE_);
        bool getPE(uint32_t & output);

        std::string getMood(const uint32_t moodID);
        std::string getString (const std::string&);
        std::string getProfession(const uint32_t) const;
        std::string getJob(const uint32_t) const;
        std::string getSkill (const uint32_t) const;
        std::string getTrait (const uint32_t, const uint32_t) const;
        std::string getTraitName(const uint32_t) const;
        std::string getLabor (const uint32_t);
        std::vector< std::vector<std::string> > const& getAllTraits();
        std::map<uint32_t, std::string> const& getAllLabours();

        DFHack::t_level getLevelInfo(const uint32_t level) const;

        void setVersion(const char *);
        void setVersion(const std::string&);
        std::string getVersion();

        void setOS(const char *);
        void setOS(const std::string&);
        void setOS(const OSType);
        OSType getOS() const;

        void setProfession(const std::string & id, const std::string & name);
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
        bool resolveClassnameToVPtr ( const std::string classname, void * & vptr );

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
