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
#ifndef CL_MOD_TRANSLATION
#define CL_MOD_TRANSLATION
/**
 * \defgroup grp_translation Translation: DF word tables and name translation/reading
 * @ingroup grp_modules
 */

#include "Export.h"
#include "Module.h"
#include "Types.h"
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
        Translation();
        ~Translation();
        bool Start();
        bool Finish();

        // Get pointer to the two dictionary structures
        Dicts * getDicts();

        // names, used by a few other modules.
        bool InitReadNames();
        bool readName(t_name & name, df_name * address);
        bool copyName(df_name * address, df_name * target);
        // translate a name using the loaded dictionaries
        std::string TranslateName(const DFHack::df_name * name, bool inEnglish = true);

        private:
        struct Private;
        Private *d;
    };
}
#endif
