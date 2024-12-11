/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include "DataDefs.h"

namespace df {
    struct language_name;
    struct language_translation;
}

namespace DFHack
{
namespace Translation
{
// simple check to make sure if there's actual language data present
DFHACK_EXPORT bool IsValid ();

// names, used by a few other modules.
DFHACK_EXPORT bool readName(t_name & name, df::language_name * address);
DFHACK_EXPORT bool copyName(df::language_name * address, df::language_name * target);

DFHACK_EXPORT void setNickname(df::language_name *name, std::string nick);

DFHACK_EXPORT std::string capitalize(const std::string &str, bool all_words = false);

// translate a name using the loaded dictionaries
DFHACK_EXPORT std::string TranslateName (const df::language_name * name, bool inEnglish = false,
                                         bool onlyLastPart = false);
}
}
#endif
