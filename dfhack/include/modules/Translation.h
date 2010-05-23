#ifndef CL_MOD_TRANSLATION
#define CL_MOD_TRANSLATION
/*
* DF translation tables and name translation
*/
#include "Export.h"
namespace DFHack
{
    class DFContextPrivate;
    typedef std::vector< std::vector<std::string> > DFDict;
    typedef struct
    {
        DFDict translations;
        DFDict foreign_languages;
    } Dicts;
    
    class DFHACK_EXPORT Translation
    {
        public:
        Translation(DFContextPrivate * d);
        ~Translation();
        bool Start();
        bool Finish();

        // Get pointer to the two dictionary structures
        Dicts * getDicts();
        // translate a name using the loaded dictionaries
        std::string TranslateName(const DFHack::t_name& name, bool inEnglish = true);
        
        private:
        struct Private;
        Private *d;
    };
}
#endif
