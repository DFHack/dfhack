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

#include "Tranquility.h"

#include "Export.h"
#include <map>
#include <vector>
#include <string>

namespace DFHack
{
    /*
    * Common data types
    */
    struct t_type
    {
        t_type(uint32_t assign, uint32_t type, string classname)
        :classname(classname),assign(assign),type(type){};
        string classname;
        uint32_t assign;
        uint32_t type;
    };

    struct t_class
    {
        t_class(const t_class &old)
        {
            classname = old.classname;
            vtable = old.vtable;
            assign = old.assign;
            type_offset = old.type_offset;
            for(uint32_t i = 0; i < old.subs.size();i++)
            {
                t_type * t = new t_type (*old.subs[i]);
                subs.push_back(t);
            }
        }
        t_class ()
        {
            vtable = 0;
            assign = 0;
            type_offset = 0;
        }
        ~t_class()
        {
            for(int i = 0; i < subs.size();i++)
            {
                delete subs[i];
            }
            subs.clear();
        }
        string classname;
        uint32_t vtable;
        uint32_t assign;// index to typeclass array if multiclass. return value if not.
        uint32_t type_offset; // offset of type data for multiclass
        vector<t_type *> subs;
    };
    
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
        void setBase (const string&);
        void setBase (const uint32_t);

        uint32_t getOffset (const string&);
        uint32_t getAddress (const string&);
        uint32_t getHexValue (const string&);
        uint32_t getOffset (const char *);
        uint32_t getAddress (const char *);
        uint32_t getHexValue (const char *);
        
        string getString (const string&);
        string getProfession(const uint32_t) const;
        string getJob(const uint32_t) const;
        string getSkill (const uint32_t) const;
        string getTrait (const uint32_t, const uint32_t) const;
        string getTraitName(const uint32_t) const;
        string getLabor (const uint32_t);

        void setVersion(const char *);
        void setVersion(const string&);
        string getVersion();

        void setOS(const char *);
        void setOS(const string&);
        void setOS(const OSType);
        OSType getOS() const;

        void setOffset (const string &, const int32_t);
        void setAddress (const string &, const uint32_t);
        void setHexValue (const string &, const uint32_t);

        void setOffset (const string &, const char *);
        void setAddress (const string &, const char *);
        void setHexValue (const string &, const char *);
        void setString (const string &, const char *);

        void setOffset (const string &, const string &);
        void setAddress (const string &, const string &);
        void setHexValue (const string &, const string &);
        void setString (const string &, const string &);

        void setProfession(const string &, const string &);
        void setJob(const string &, const string &);
        void setSkill(const string &, const string &);
        void setTrait(const string &,const string &,const string &,const string &,const string &,const string &,const string &,const string &);
        void setLabor(const string &, const string &);

        void RebaseVTable(const int32_t offset);
        
        t_class * setClass (const char * classname, uint32_t vptr = 0, uint32_t typeoffset = 0);
        void setClassChild (t_class * parent, const char * classname, const char * type);

        /*
         * Get a classID from an address. The address has to point to the start of a virtual object (one with a virtual base class)
         *   uses memory reading directly, needs suspend. input = address of the object
         *   fails if it's unable to read from memory
         */
        bool resolveObjectToClassID (const uint32_t address, int32_t & classID);
        
        /*
        * Get a classID from an address. The address has to point to the start of a virtual object (one with a virtual base class)
        *   can fail if the class is not in the cache
        */
        bool resolveClassnameToClassID (const string classname, int32_t & classID);
        //bool resolveClassnameToClassID (const char * classname, int32_t & classID);
        
        /*
        * Get a vptr from a classname. Can fail if the type is not in the cache
        * limited to normal classes, variable-dependent types will resolve to the base class
        */
        bool resolveClassnameToVPtr ( const string classname, uint32_t & vptr );
        //bool resolveClassnameToVPtr ( const char * classname, uint32_t & vptr );
        
        /*
        * Get a classname from a previous classID. Can fail if the type is not in the cache (you use bogus classID)
        */
        bool resolveClassIDToClassname (const int32_t classID, string & classname);
        
        /*
        * Get the internal classID->classname mapping (for speed). DO NOT MANIPULATE THE VECTOR!
        */
        const vector<string> * getClassIDMapping();
    };
}
#endif // MEMINFO_H_INCLUDED
