// https://github.com/cvarin/IniParser/
/******************************************************************************/
#include "../../include/util/IniParser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXVALSZ    1024 /** Maximum value size for integers and doubles. */
#define DICTMINSZ   128  /** Minimal allocated number of entries in a dictionary */
#define DICT_INVALID_KEY    ((char*)-1) /** Invalid key token */
#define ASCIILINESZ         (1024)
#define INI_INVALID_KEY     ((char*)-1)

/******************************************************************************/
/******************************************************************************/
IniParser::IniParser(const char *ini_file)
{
  dic = iniparser_load(ini_file);
  if(dic == NULL)
  {
    fprintf(stderr, "cannot parse file: %s\n", ini_file);
    _init_ok = false;
  }
  _init_ok = true;
}

/******************************************************************************/
IniParser::~IniParser()
{
     iniparser_freedict(dic);
}

/******************************************************************************/
/******************************************************************************/
int IniParser::dictionary_set(dictionary *d, char * key, char * val)
/**
  @brief    Set a value in a dictionary.
  @param    d       Dictionary to set
  @param    key     Key to modify or add.
  @param    val     Value to add.
  @return   int     0 if Ok, anything else otherwise
  If the given key is found in the dictionary, the associated value is
  replaced by the provided one. If the key cannot be found in the
  dictionary, it is added to it.
  It is Ok to provide a NULL value for val, but NULL values for the dictionary
  or the key are considered as errors: the function will return immediately
  in such a case.
  Notice that if you dictionary_set a variable to NULL, a call to
  dictionary_get will return a NULL value: the variable will be found, and
  its value (NULL) is returned. In other words, setting the variable
  content to NULL is equivalent to deleting the variable from the
  dictionary. It is not possible (in this implementation) to have a key in
  the dictionary without value.
  This function returns non-zero in case of failure.
 */
{
    int         i ;
    unsigned    hash ;

    if (d==NULL || key==NULL) return -1 ;

    /* Compute hash for this key */
    hash = dictionary_hash(key) ;
    /* Find if value is already in dictionary */
    if (d->n>0) {
        for (i=0 ; i<d->size ; i++) {
            if (d->key[i]==NULL)
                continue ;
            if (hash==d->hash[i]) { /* Same hash value */
                if (!strcmp(key, d->key[i])) {   /* Same key */
                    /* Found a value: modify and return */
                    if (d->val[i]!=NULL)
                        free(d->val[i]);
                    d->val[i] = val ? xstrdup(val) : NULL ;
                    /* Value has been modified: return */
                    return 0 ;
                }
            }
        }
    }
    /* Add a new value */
    /* See if dictionary needs to grow */
    if (d->n==d->size) {

        /* Reached maximum size: reallocate dictionary */
        d->val  = (char **)mem_double(d->val,  d->size * sizeof(char*)) ;
        d->key  = (char **)mem_double(d->key,  d->size * sizeof(char*)) ;
        d->hash = (unsigned int *)mem_double(d->hash, d->size * sizeof(unsigned)) ;
        if ((d->val==NULL) || (d->key==NULL) || (d->hash==NULL)) {
            /* Cannot grow dictionary */
            return -1 ;
        }
        /* Double size */
        d->size *= 2 ;
    }

    /* Insert key in the first empty slot. Start at d->n and wrap at
       d->size. Because d->n < d->size this will necessarily
       terminate. */
    for (i=d->n ; d->key[i] ; ) {
        if(++i == d->size) i = 0;
    }
    /* Copy key */
    d->key[i]  = xstrdup(key);
    d->val[i]  = val ? xstrdup(val) : NULL ;
    d->hash[i] = hash;
    d->n ++ ;
    return 0 ;
}

/******************************************************************************/
void IniParser::dump(FILE * f)
/**
  @brief    Dump a dictionary to an opened file pointer.
  @param    f   Opened file pointer to dump to.
  @return   void
  This function prints out the contents of a dictionary, one element by
  line, onto the provided file pointer. It is OK to specify @c stderr
  or @c stdout as output files. This function is meant for debugging
  purposes mostly.
 */
{
    int i;

    if (dic==NULL || f==NULL) return ;
    for (i=0 ; i<dic->size ; i++) {
        if (dic->key[i]==NULL)
            continue ;
        if (dic->val[i]!=NULL) {
            fprintf(f, "[%s]=[%s]\n", dic->key[i], dic->val[i]);
        } else {
            fprintf(f, "[%s]=UNDEF\n", dic->key[i]);
        }
    }
    return ;
}

/******************************************************************************/
int IniParser::find_entry(char *entry)
/**
  @brief    Finds out if a given entry exists in a dictionary
  @param    entry   Name of the entry to look for
  @return   integer 1 if entry exists, 0 otherwise
  Finds out if a given entry exists in the dictionary. Since sections
  are stored as keys with NULL associated values, this is the only way
  of querying for the presence of sections in a dictionary.
 */
{
    int found=0 ;
    if (getstring(entry, INI_INVALID_KEY)!=INI_INVALID_KEY) {
        found = 1 ;
    }
    return found ;
}

/******************************************************************************/
int IniParser::getboolean(const char * key, int notfound)

/**
  @brief    Get the string associated to a key, convert to a boolean
  @param    key Key string to look for
  @param    notfound Value to return in case of error
  @return   integer
  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the notfound value is returned.
  A true boolean is found if one of the following is matched:
  - A string starting with 'y'
  - A string starting with 'Y'
  - A string starting with 't'
  - A string starting with 'T'
  - A string starting with '1'
  A false boolean is found if one of the following is matched:
  - A string starting with 'n'
  - A string starting with 'N'
  - A string starting with 'f'
  - A string starting with 'F'
  - A string starting with '0'
  The notfound value returned if no boolean is identified, does not
  necessarily have to be 0 or 1.
 */
{
    char    *   c ;
    int         ret ;

    c = getstring(key, INI_INVALID_KEY);
    if (c==INI_INVALID_KEY) return notfound ;
    if (c[0]=='y' || c[0]=='Y' || c[0]=='1' || c[0]=='t' || c[0]=='T') {
        ret = 1 ;
    } else if (c[0]=='n' || c[0]=='N' || c[0]=='0' || c[0]=='f' || c[0]=='F') {
        ret = 0 ;
    } else {
        ret = notfound ;
    }
    return ret;
}

/******************************************************************************/
int IniParser::getint(const char * key, int notfound)
/**
  @brief    Get the string associated to a key, convert to an int
  @param    key Key string to look for
  @param    notfound Value to return in case of error
  @return   integer
  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the notfound value is returned.
  Supported values for integers include the usual C notation
  so decimal, octal (starting with 0) and hexadecimal (starting with 0x)
  are supported. Examples:
  "42"      ->  42
  "042"     ->  34 (octal -> decimal)
  "0x42"    ->  66 (hexa  -> decimal)
  Warning: the conversion may overflow in various ways. Conversion is
  totally outsourced to strtol(), see the associated man page for overflow
  handling.
  Credits: Thanks to A. Becker for suggesting strtol()
 */
{
    char    *   str ;

    str = getstring(key, INI_INVALID_KEY);
    if (str==INI_INVALID_KEY) return notfound ;
    return (int)strtol(str, NULL, 0);
}

/******************************************************************************/
double IniParser::getdouble(const char * key, double notfound)
/**
  @brief    Get the string associated to a key, convert to a double
  @param    key Key string to look for
  @param    notfound Value to return in case of error
  @return   double
  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the notfound value is returned.
 */
{
    char    *   str ;

    str = getstring(key, INI_INVALID_KEY);
    if (str==INI_INVALID_KEY) return notfound ;
    return atof(str);
}

/******************************************************************************/
char * IniParser::getstring(const char * key, const char * def)
/**
  @brief    Get the string associated to a key
  @param    key     Key string to look for
  @param    def     Default value to return if key not found.
  @return   pointer to statically allocated character string
  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the pointer passed as 'def' is returned.
  The returned char pointer is pointing to a string allocated in
  the dictionary, do not free or modify it.
 */
{
    char * lc_key ;
    char * sval ;

    if (dic==NULL || key==NULL)
        return (char*)def ;

    lc_key = strlwc(key);
    sval = dictionary_get(dic, lc_key, (char*)def);
    return sval;
}

/******************************************************************************/
int IniParser::set(char * entry, char * val)
/**
  @brief    Set an entry in a dictionary.
  @param    entry   Entry to modify (entry name)
  @param    val     New value to associate to the entry.
  @return   int 0 if Ok, -1 otherwise.
  If the given entry can be found in the dictionary, it is modified to
  contain the provided value. If it cannot be found, -1 is returned.
  It is Ok to set val to NULL.
 */
{
    return dictionary_set(dic, strlwc(entry), val) ;
}

/******************************************************************************/
char * IniParser::strlwc(const char * s)
/**
  @brief    Convert a string to lowercase.
  @param    s   String to convert.
  @return   ptr to statically allocated string.
  This function returns a pointer to a statically allocated string
  containing a lowercased version of the input string. Do not free
  or modify the returned string! Since the returned string is statically
  allocated, it will be modified at each function call (not re-entrant).
 */
{
    static char l[ASCIILINESZ+1];
    int i ;

    if (s==NULL) return NULL ;
    memset(l, 0, ASCIILINESZ+1);
    i=0 ;
    while (s[i] && i<ASCIILINESZ) {
        l[i] = (char)tolower((int)s[i]);
        i++ ;
    }
    l[ASCIILINESZ]=(char)0;
    return l ;
}

/******************************************************************************/

/*---------------------------------------------------------------------------
                            Private functions
 ---------------------------------------------------------------------------*/

/* Doubles the allocated size associated to a pointer */
/* 'size' is the current allocated size. */
void * IniParser::mem_double(void * ptr, int size)
{
    void * newptr ;

    newptr = calloc(2*size, 1);
    if (newptr==NULL) {
        return NULL ;
    }
    memcpy(newptr, ptr, size);
    free(ptr);
    return newptr ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Duplicate a string
  @param    s String to duplicate
  @return   Pointer to a newly allocated string, to be freed with free()
  This is a replacement for strdup(). This implementation is provided
  for systems that do not have it.
 */
/*--------------------------------------------------------------------------*/
char * IniParser::xstrdup(char * s)
{
    char * t ;
    if (!s)
        return NULL ;
    t = (char*)malloc(strlen(s)+1) ;
    if (t) {
        strcpy(t,s);
    }
    return t ;
}

/*---------------------------------------------------------------------------
                            Function codes
 ---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/**
  @brief    Compute the hash key for a string.
  @param    key     Character string to use for key.
  @return   1 unsigned int on at least 32 bits.
  This hash function has been taken from an Article in Dr Dobbs Journal.
  This is normally a collision-free function, distributing keys evenly.
  The key is stored anyway in the struct so that collision can be avoided
  by comparing the key itself in last resort.
 */
/*--------------------------------------------------------------------------*/
unsigned IniParser::dictionary_hash(char * key)
{
    int         len ;
    unsigned    hash ;
    int         i ;

    len = strlen(key);
    for (hash=0, i=0 ; i<len ; i++) {
        hash += (unsigned)key[i] ;
        hash += (hash<<10);
        hash ^= (hash>>6) ;
    }
    hash += (hash <<3);
    hash ^= (hash >>11);
    hash += (hash <<15);
    return hash ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Create a new dictionary object.
  @param    size    Optional initial size of the dictionary.
  @return   1 newly allocated dictionary objet.
  This function allocates a new dictionary object of given size and returns
  it. If you do not know in advance (roughly) the number of entries in the
  dictionary, give size=0.
 */
/*--------------------------------------------------------------------------*/
dictionary * IniParser::dictionary_new(int size)
{
    dictionary  *   d ;

    /* If no size was specified, allocate space for DICTMINSZ */
    if (size<DICTMINSZ) size=DICTMINSZ ;

    if (!(d = (dictionary *)calloc(1, sizeof(dictionary)))) {
        return NULL;
    }
    d->size = size ;
    d->val  = (char **)calloc(size, sizeof(char*));
    d->key  = (char **)calloc(size, sizeof(char*));
    d->hash = (unsigned int *)calloc(size, sizeof(unsigned));
    return d ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete a dictionary object
  @param    d   dictionary object to deallocate.
  @return   void
  Deallocate a dictionary object and all memory associated to it.
 */
/*--------------------------------------------------------------------------*/
void IniParser::dictionary_del(dictionary * d)
{
    int     i ;

    if (d==NULL) return ;
    for (i=0 ; i<d->size ; i++) {
        if (d->key[i]!=NULL)
            free(d->key[i]);
        if (d->val[i]!=NULL)
            free(d->val[i]);
    }
    free(d->val);
    free(d->key);
    free(d->hash);
    free(d);
    return ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get a value from a dictionary.
  @param    d       dictionary object to search.
  @param    key     Key to look for in the dictionary.
  @param    def     Default value to return if key not found.
  @return   1 pointer to internally allocated character string.
  This function locates a key in a dictionary and returns a pointer to its
  value, or the passed 'def' pointer if no such key can be found in
  dictionary. The returned character pointer points to data internal to the
  dictionary object, you should not try to free it or modify it.
 */
/*--------------------------------------------------------------------------*/
char * IniParser::dictionary_get(dictionary * d, char * key, char * def)
{
    unsigned    hash ;
    int         i ;

    hash = dictionary_hash(key);
    for (i=0 ; i<d->size ; i++) {
        if (d->key[i]==NULL)
            continue ;
        /* Compare hash */
        if (hash==d->hash[i]) {
            /* Compare string, to avoid hash collisions */
            if (!strcmp(key, d->key[i])) {
                return d->val[i] ;
            }
        }
    }
    return def ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete a key in a dictionary
  @param    d       dictionary object to modify.
  @param    key     Key to remove.
  @return   void
  This function deletes a key in a dictionary. Nothing is done if the
  key cannot be found.
 */
/*--------------------------------------------------------------------------*/
void IniParser::dictionary_unset(dictionary * d, char * key)
{
    unsigned    hash ;
    int         i ;

    if (key == NULL) {
        return;
    }

    hash = dictionary_hash(key);
    for (i=0 ; i<d->size ; i++) {
        if (d->key[i]==NULL)
            continue ;
        /* Compare hash */
        if (hash==d->hash[i]) {
            /* Compare string, to avoid hash collisions */
            if (!strcmp(key, d->key[i])) {
                /* Found key */
                break ;
            }
        }
    }
    if (i>=d->size)
        /* Key not found */
        return ;

    free(d->key[i]);
    d->key[i] = NULL ;
    if (d->val[i]!=NULL) {
        free(d->val[i]);
        d->val[i] = NULL ;
    }
    d->hash[i] = 0 ;
    d->n -- ;
    return ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Dump a dictionary to an opened file pointer.
  @param    d   Dictionary to dump
  @param    f   Opened file pointer.
  @return   void
  Dumps a dictionary onto an opened file pointer. Key pairs are printed out
  as @c [Key]=[Value], one per line. It is Ok to provide stdout or stderr as
  output file pointers.
 */
/*--------------------------------------------------------------------------*/
void IniParser::dictionary_dump(dictionary * d, FILE * out)
{
    int     i ;

    if (d==NULL || out==NULL) return ;
    if (d->n<1) {
        fprintf(out, "empty dictionary\n");
        return ;
    }
    for (i=0 ; i<d->size ; i++) {
        if (d->key[i]) {
            fprintf(out, "%20s\t[%s]\n",
                    d->key[i],
                    d->val[i] ? d->val[i] : "UNDEF");
        }
    }
    return ;
}


/* Test code */
#ifdef TESTDIC
#define NVALS 20000
int main(int argc, char *argv[])
{
    dictionary  *   d ;
    char    *   val ;
    int         i ;
    char        cval[90] ;

    /* Allocate dictionary */
    printf("allocating...\n");
    d = dictionary_new(0);

    /* Set values in dictionary */
    printf("setting %d values...\n", NVALS);
    for (i=0 ; i<NVALS ; i++) {
        sprintf(cval, "%04d", i);
        dictionary_set(d, cval, "salut");
    }
    printf("getting %d values...\n", NVALS);
    for (i=0 ; i<NVALS ; i++) {
        sprintf(cval, "%04d", i);
        val = dictionary_get(d, cval, DICT_INVALID_KEY);
        if (val==DICT_INVALID_KEY) {
            printf("cannot get value for key [%s]\n", cval);
        }
    }
    printf("unsetting %d values...\n", NVALS);
    for (i=0 ; i<NVALS ; i++) {
        sprintf(cval, "%04d", i);
        dictionary_unset(d, cval);
    }
    if (d->n != 0) {
        printf("error deleting values\n");
    }
    printf("deallocating...\n");
    dictionary_del(d);
    return 0 ;
}
#endif
/* vim: set ts=4 et sw=4 tw=75 */


/*-------------------------------------------------------------------------*/
/**
   @file    iniparser.c
   @author  N. Devillard
   @date    Sep 2007
   @version 3.0
   @brief   Parser for ini files.
*/

/*-------------------------------------------------------------------------*/
/**
  @brief    Remove blanks at the beginning and the end of a string.
  @param    s   String to parse.
  @return   ptr to statically allocated string.
  This function returns a pointer to a statically allocated string,
  which is identical to the input string, except that all blank
  characters at the end and the beg. of the string have been removed.
  Do not free or modify the returned string! Since the returned string
  is statically allocated, it will be modified at each function call
  (not re-entrant).
 */
/*--------------------------------------------------------------------------*/
char * IniParser::strstrip(char * s)
{
    static char l[ASCIILINESZ+1];
    char * last ;

    if (s==NULL) return NULL ;

    while (isspace((int)*s) && *s) s++;
    memset(l, 0, ASCIILINESZ+1);
    strcpy(l, s);
    last = l + strlen(l);
    while (last > l) {
        if (!isspace((int)*(last-1)))
            break ;
        last -- ;
    }
    *last = (char)0;
    return (char*)l ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get number of sections in a dictionary
  @param    d   Dictionary to examine
  @return   int Number of sections found in dictionary
  This function returns the number of sections found in a dictionary.
  The test to recognize sections is done on the string stored in the
  dictionary: a section name is given as "section" whereas a key is
  stored as "section:key", thus the test looks for entries that do not
  contain a colon.
  This clearly fails in the case a section name contains a colon, but
  this should simply be avoided.
  This function returns -1 in case of error.
 */
/*--------------------------------------------------------------------------*/
int IniParser::iniparser_getnsec(dictionary * d)
{
    int i ;
    int nsec ;

    if (d==NULL) return -1 ;
    nsec=0 ;
    for (i=0 ; i<d->size ; i++) {
        if (d->key[i]==NULL)
            continue ;
        if (strchr(d->key[i], ':')==NULL) {
            nsec ++ ;
        }
    }
    return nsec ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get name for section n in a dictionary.
  @param    d   Dictionary to examine
  @param    n   Section number (from 0 to nsec-1).
  @return   Pointer to char string
  This function locates the n-th section in a dictionary and returns
  its name as a pointer to a string statically allocated inside the
  dictionary. Do not free or modify the returned string!
  This function returns NULL in case of error.
 */
/*--------------------------------------------------------------------------*/
char * IniParser::iniparser_getsecname(dictionary * d, int n)
{
    int i ;
    int foundsec ;

    if (d==NULL || n<0) return NULL ;
    foundsec=0 ;
    for (i=0 ; i<d->size ; i++) {
        if (d->key[i]==NULL)
            continue ;
        if (strchr(d->key[i], ':')==NULL) {
            foundsec++ ;
            if (foundsec>n)
                break ;
        }
    }
    if (foundsec<=n) {
        return NULL ;
    }
    return d->key[i] ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Save a dictionary to a loadable ini file
  @param    d   Dictionary to dump
  @param    f   Opened file pointer to dump to
  @return   void
  This function dumps a given dictionary into a loadable ini file.
  It is Ok to specify @c stderr or @c stdout as output files.
 */
/*--------------------------------------------------------------------------*/
void IniParser::dump_ini(FILE * f)
{
    int     i ;
    int     nsec ;
    char *  secname ;

    if (dic==NULL || f==NULL) return ;

    nsec = iniparser_getnsec(dic);
    if (nsec<1) {
        /* No section in file: dump all keys as they are */
        for (i=0 ; i<dic->size ; i++) {
            if (dic->key[i]==NULL)
                continue ;
            fprintf(f, "%s = %s\n", dic->key[i], dic->val[i]);
        }
        return ;
    }
    for (i=0 ; i<nsec ; i++) {
        secname = iniparser_getsecname(dic, i) ;
        iniparser_dumpsection_ini(dic, secname, f) ;
    }
    fprintf(f, "\n");
    return ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Save a dictionary section to a loadable ini file
  @param    d   Dictionary to dump
  @param    s   Section name of dictionary to dump
  @param    f   Opened file pointer to dump to
  @return   void
  This function dumps a given section of a given dictionary into a loadable ini
  file.  It is Ok to specify @c stderr or @c stdout as output files.
 */
/*--------------------------------------------------------------------------*/
void IniParser::iniparser_dumpsection_ini(dictionary * d, char * s, FILE * f)
{
    int     j ;
    char    keym[ASCIILINESZ+1];
    int     seclen ;

    if (d==NULL || f==NULL) return ;
    if (!find_entry(s)) return ;

    seclen  = (int)strlen(s);
    fprintf(f, "\n[%s]\n", s);
    sprintf(keym, "%s:", s);
    for (j=0 ; j<d->size ; j++) {
        if (d->key[j]==NULL)
            continue ;
        if (!strncmp(d->key[j], keym, seclen+1)) {
            fprintf(f,
                    "%-30s = %s\n",
                    d->key[j]+seclen+1,
                    d->val[j] ? d->val[j] : "");
        }
    }
    fprintf(f, "\n");
    return ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the number of keys in a section of a dictionary.
  @param    d   Dictionary to examine
  @param    s   Section name of dictionary to examine
  @return   Number of keys in section
 */
/*--------------------------------------------------------------------------*/
int IniParser::iniparser_getsecnkeys(dictionary * d, char * s)
{
    int     seclen, nkeys ;
    char    keym[ASCIILINESZ+1];
    int j ;

    nkeys = 0;

    if (d==NULL) return nkeys;
    if (!find_entry(s)) return nkeys;

    seclen  = (int)strlen(s);
    sprintf(keym, "%s:", s);

    for (j=0 ; j<d->size ; j++) {
        if (d->key[j]==NULL)
            continue ;
        if (!strncmp(d->key[j], keym, seclen+1))
            nkeys++;
    }

    return nkeys;

}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the number of keys in a section of a dictionary.
  @param    d   Dictionary to examine
  @param    s   Section name of dictionary to examine
  @return   pointer to statically allocated character strings
  This function queries a dictionary and finds all keys in a given section.
  Each pointer in the returned char pointer-to-pointer is pointing to
  a string allocated in the dictionary; do not free or modify them.

  This function returns NULL in case of error.
 */
/*--------------------------------------------------------------------------*/
char ** IniParser::iniparser_getseckeys(dictionary * d, char * s)
{

    char **keys;

    int i, j ;
    char    keym[ASCIILINESZ+1];
    int     seclen, nkeys ;

    keys = NULL;

    if (d==NULL) return keys;
    if (!find_entry(s)) return keys;

    nkeys = iniparser_getsecnkeys(d, s);

    keys = (char**) malloc(nkeys*sizeof(char*));

    seclen  = (int)strlen(s);
    sprintf(keym, "%s:", s);

    i = 0;

    for (j=0 ; j<d->size ; j++) {
        if (d->key[j]==NULL)
            continue ;
        if (!strncmp(d->key[j], keym, seclen+1)) {
            keys[i] = d->key[j];
            i++;
        }
    }

    return keys;

}

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete an entry in a dictionary
  @param    ini     Dictionary to modify
  @param    entry   Entry to delete (entry name)
  @return   void
  If the given entry can be found, it is deleted from the dictionary.
 */
/*--------------------------------------------------------------------------*/
void IniParser::iniparser_unset(dictionary * ini, char * entry)
{
    dictionary_unset(ini, strlwc(entry));
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Load a single line from an INI file
  @param    input_line  Input line, may be concatenated multi-line input
  @param    section     Output space to store section
  @param    key         Output space to store key
  @param    value       Output space to store value
  @return   line_status value
 */
/*--------------------------------------------------------------------------*/
line_status IniParser::iniparser_line(char * input_line, char * section,
                                        char * key, char * value)
{
    line_status sta ;
    char        line[ASCIILINESZ+1];
    int         len ;

    strcpy(line, strstrip(input_line));
    len = (int)strlen(line);

    sta = LINE_UNPROCESSED ;
    if (len<1) {
        /* Empty line */
        sta = LINE_EMPTY ;
    } else if (line[0]=='#' || line[0]==';') {
        /* Comment line */
        sta = LINE_COMMENT ;
    } else if (line[0]=='[' && line[len-1]==']') {
        /* Section name */
        sscanf(line, "[%[^]]", section);
        strcpy(section, strstrip(section));
        strcpy(section, strlwc(section));
        sta = LINE_SECTION ;
    } else if (sscanf (line, "%[^=] = \"%[^\"]\"", key, value) == 2
           ||  sscanf (line, "%[^=] = '%[^\']'",   key, value) == 2
           ||  sscanf (line, "%[^=] = %[^;#]",     key, value) == 2) {
        /* Usual key=value, with or without comments */
        strcpy(key, strstrip(key));
        strcpy(key, strlwc(key));
        strcpy(value, strstrip(value));
        /*
         * sscanf cannot handle '' or "" as empty values
         * this is done here
         */
        if (!strcmp(value, "\"\"") || (!strcmp(value, "''"))) {
            value[0]=0 ;
        }
        sta = LINE_VALUE ;
    } else if (sscanf(line, "%[^=] = %[;#]", key, value)==2
           ||  sscanf(line, "%[^=] %[=]", key, value) == 2) {
        /*
         * Special cases:
         * key=
         * key=;
         * key=#
         */
        strcpy(key, strstrip(key));
        strcpy(key, strlwc(key));
        value[0]=0 ;
        sta = LINE_VALUE ;
    } else {
        /* Generate syntax error */
        sta = LINE_ERROR ;
    }
    return sta ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Parse an ini file and return an allocated dictionary object
  @param    ininame Name of the ini file to read.
  @return   Pointer to newly allocated dictionary
  This is the parser for ini files. This function is called, providing
  the name of the file to be read. It returns a dictionary object that
  should not be accessed directly, but through accessor functions
  instead.
  The returned dictionary must be freed using iniparser_freedict().
 */
/*--------------------------------------------------------------------------*/
dictionary *IniParser::iniparser_load(const char *ininame)
{
    FILE * in ;

    char line    [ASCIILINESZ+1] ;
    char section [ASCIILINESZ+1] ;
    char key     [ASCIILINESZ+1] ;
    char tmp     [ASCIILINESZ+1] ;
    char val     [ASCIILINESZ+1] ;

    int  last=0 ;
    int  len ;
    int  lineno=0 ;
    int  errs=0;

    dictionary * dict ;

    if ((in=fopen(ininame, "r"))==NULL) {
        fprintf(stderr, "iniparser: cannot open %s\n", ininame);
        return NULL ;
    }

    dict = dictionary_new(0) ;
    if (!dict) {
        fclose(in);
        return NULL ;
    }

    memset(line,    0, ASCIILINESZ);
    memset(section, 0, ASCIILINESZ);
    memset(key,     0, ASCIILINESZ);
    memset(val,     0, ASCIILINESZ);
    last=0 ;

    while (fgets(line+last, ASCIILINESZ-last, in)!=NULL) {
        lineno++ ;
        len = (int)strlen(line)-1;
        if (len==0)
            continue;
        /* Safety check against buffer overflows */
        if (line[len]!='\n') {
            fprintf(stderr,
                    "iniparser: input line too long in %s (%d)\n",
                    ininame,
                    lineno);
            dictionary_del(dict);
            fclose(in);
            return NULL ;
        }
        /* Get rid of \n and spaces at end of line */
        while ((len>=0) &&
                ((line[len]=='\n') || (isspace(line[len])))) {
            line[len]=0 ;
            len-- ;
        }
        /* Detect multi-line */
        if (line[len]=='\\') {
            /* Multi-line value */
            last=len ;
            continue ;
        } else {
            last=0 ;
        }
        switch (iniparser_line(line, section, key, val)) {
            case LINE_EMPTY:
            case LINE_COMMENT:
            break ;

            case LINE_SECTION:
            errs = dictionary_set(dict, section, NULL);
            break ;

            case LINE_VALUE:
            sprintf(tmp, "%s:%s", section, key);
            errs = dictionary_set(dict, tmp, val) ;
            break ;

            case LINE_ERROR:
            fprintf(stderr, "iniparser: syntax error in %s (%d):\n",
                    ininame,
                    lineno);
            fprintf(stderr, "-> %s\n", line);
            errs++ ;
            break;

            default:
            break ;
        }
        memset(line, 0, ASCIILINESZ);
        last=0;
        if (errs<0) {
            fprintf(stderr, "iniparser: memory allocation failure\n");
            break ;
        }
    }
    if (errs) {
        dictionary_del(dict);
        dict = NULL ;
    }
    fclose(in);
    return dict ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Free all memory associated to an ini dictionary
  @param    d Dictionary to free
  @return   void
  Free all memory associated to an ini dictionary.
  It is mandatory to call this function before the dictionary object
  gets out of the current context.
 */
/*--------------------------------------------------------------------------*/
void IniParser::iniparser_freedict(dictionary * d)
{
    dictionary_del(d);
}


bool IniParser::init_ok()
{
  return _init_ok;
}