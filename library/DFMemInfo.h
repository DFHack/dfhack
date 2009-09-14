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

class memory_info
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


    void RebaseAddresses(int32_t new_base);
    void RebaseAll(int32_t new_base);
    uint32_t getBase ();
    void setBase (string);
    void setBase (uint32_t);

    uint32_t getOffset (string);
    uint32_t getAddress (string);
    uint32_t getHexValue (string);
    string getString (string);

    void setVersion(const char *);
    void setVersion(string);
    string getVersion();

    void setOS(const char *);
    void setOS(string);
    void setOS(OSType);
    OSType getOS();

    void setOffset (string, int32_t);
    void setAddress (string, uint32_t);
    void setHexValue (string, uint32_t);

    void setOffset (string, const char *);
    void setAddress (string, const char *);
    void setHexValue (string, const char *);
    void setString (string, const char *);

    void setOffset (string, string);
    void setAddress (string, string);
    void setHexValue (string, string);
    void setString (string, string);

    void RebaseVTable(int32_t offset);
    void setClass (const char * name, const char * vtable);
    uint32_t setMultiClass (const char * name, const char * vtable, const char * typeoffset);
    void setMultiClassChild (uint32_t multi_index, const char * name, const char * type);

    // ALERT: uses memory reading directly
    bool resolveClassId(uint32_t address, int32_t & classid);
    void copyBuildings(vector<string> & v_buildingtypes);

    void flush();
    
private:
    map <string, uint32_t> addresses;
    map <string, uint32_t> offsets;
    map <string, uint32_t> hexvals;
    map <string, string> strings;
    vector<t_class> classes;
    vector<vector<t_type> > classsubtypes;
    int32_t base;
    uint32_t classindex;
    string version;
    OSType OS;
};
#endif // MEMINFO_H_INCLUDED
