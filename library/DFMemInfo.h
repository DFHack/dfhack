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

#include "Export.h"
#include <map>
#include <vector>
#include <string>

namespace DFHack
{
    class DFHACK_EXPORT memory_info
    {
    public:
        enum OSType
        {
            OS_WINDOWS,
            OS_LINUX,
            OS_BAD
        };
        struct t_class
        {
            string classname;
            uint32_t vtable;
            bool is_multiclass;
            uint32_t multi_index;
            uint32_t assign;// index to typeclass array if multiclass. return value if not.
            uint32_t type_offset; // offset of type data for multiclass
        };
        struct t_type
        {
            string classname;
            uint32_t assign;
            uint32_t type;
        };
        memory_info();
        memory_info(const memory_info&);


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
        void setClass (const char * name, const char * vtable);
        uint32_t setMultiClass (const char * name, const char * vtable, const char * typeoffset);
        void setMultiClassChild (uint32_t multi_index, const char * name, const char * type);

        // ALERT: uses memory reading directly
        bool resolveClassId(const uint32_t address, int32_t & classid);
        void copyBuildings(vector<string> & v_buildingtypes);

        void flush();
        
    private:
        map <string, uint32_t> addresses;
        map <string, uint32_t> offsets;
        map <string, uint32_t> hexvals;
        map <string, string> strings;

        vector<string> professions;
        vector<string> jobs;
        vector<string> skills;
        vector< vector<string> > traits;
        map <uint32_t, string> labors;

        vector<t_class> classes;
        vector<vector<t_type> > classsubtypes;
        int32_t base;
        uint32_t classindex;
        string version;
        OSType OS;
    };
}
#endif // MEMINFO_H_INCLUDED
