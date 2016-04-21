// https://github.com/cvarin/IniParser/

#ifndef INC_IniParser_hpp
#define INC_IniParser_hpp

/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct dictionary{
    int             n ;     /** Number of entries in dictionary */
    int             size ;  /** Storage size */
    char        **  val ;   /** List of string values */
    char        **  key ;   /** List of string keys */
    unsigned     *  hash ;  /** List of hash values for keys */
};

/**
 * This enum stores the status for each parsed line (internal use only).
 */
enum line_status{
    LINE_UNPROCESSED,
    LINE_ERROR,
    LINE_EMPTY,
    LINE_COMMENT,
    LINE_SECTION,
    LINE_VALUE
};

class IniParser
{
     public:
     IniParser(const char *ini_file);
     ~IniParser();

     void dump(FILE * f);
     void dump_ini(FILE * f);
     int getboolean(const char * key, int notfound);
     int getint(const char * key, int notfound);
     double getdouble(const char * key, double notfound);
     char *getstring(const char * key, const char * def);
     bool init_ok();

     private:
     bool _init_ok;
     dictionary *dic;

     unsigned dictionary_hash(char * key);
     dictionary * dictionary_new(int size);
     void dictionary_del(dictionary * vd);
     void dictionary_dump(dictionary * d, FILE * out);
     char * dictionary_get(dictionary * d, char * key, char * def);
     int dictionary_set(dictionary * d, char * key, char * val);
     void dictionary_unset(dictionary * d, char * key);
     int find_entry(char *entry);
     int iniparser_getnsec(dictionary * d);
     char * iniparser_getsecname(dictionary * d, int n);
     void iniparser_dumpsection_ini(dictionary * d, char * s, FILE * f);
     int iniparser_getsecnkeys(dictionary * d, char * s);
     char ** iniparser_getseckeys(dictionary * d, char * s);
     int iniparser_set(dictionary * ini, char * entry, char * val);
     void iniparser_unset(dictionary * ini, char * entry);
     int iniparser_find_entry(dictionary * ini, char * entry) ;
     dictionary * iniparser_load(const char *ininame);
     void iniparser_freedict(dictionary * d);
     int set(char * entry, char * val);
     line_status iniparser_line(char * input_line, char * section, char * key, char * value);
     void *mem_double(void * ptr, int size);
     char *strlwc(const char * s);
     char *strstrip(char * s);
     char *xstrdup(char * s);
};

#endif // #ifndef INC_IniParser_hpp