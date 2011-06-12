#pragma once
#ifndef CL_MOD_TRANSLATION
#define CL_MOD_TRANSLATION
/**
 * \defgroup grp_translation Translation: DF word tables and name translation/reading
 * @ingroup grp_modules
 */

#include "dfhack/Export.h"
#include "dfhack/Module.h"
namespace DFHack
{
    class DFContextShared;
    /**
     * \ingroup grp_translation
     */
    typedef std::vector< std::vector<std::string> > DFDict;
    /**
     * \ingroup grp_translation
     */
    typedef struct
    {
        DFDict translations;
        DFDict foreign_languages;
    } Dicts;
    /**
     * The Tanslation module
     * \ingroup grp_translation
     * \ingroup grp_maps
     */
    class DFHACK_EXPORT Translation : public Module
    {
        public:
        Translation(DFContextShared * d);
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
