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

#include "Internal.h"

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "modules/Translation.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "Types.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "Error.h"

using namespace DFHack;
using namespace df::enums;

#include "DataDefs.h"
#include "df/world.h"
#include "df/d_init.h"

using df::global::world;
using df::global::d_init;
using df::global::gametype;

bool Translation::IsValid ()
{
    return (world && (world->raws.language.words.size() > 0) && (world->raws.language.translations.size() > 0));
}

bool Translation::readName(t_name & name, df::language_name * source)
{
    strncpy(name.first_name,source->first_name.c_str(),127);
    strncpy(name.nickname,source->nickname.c_str(),127);
    memcpy(&name.parts_of_speech, &source->parts_of_speech, sizeof (source->parts_of_speech));
    memcpy(&name.words, &source->words, sizeof (source->words));
    name.language = source->language;
    name.has_name = source->has_name;
    return true;
}

bool Translation::copyName(df::language_name * source, df::language_name * target)
{
    if (source == target)
        return true;

    target->first_name = source->first_name;
    target->nickname = source->nickname;
    for (int i = 0; i < 7; i++)
    {
        target->words[i] = source->words[i];
        target->parts_of_speech[i] = source->parts_of_speech[i];
    }
    target->language = source->language;
    target->unknown = source->unknown;
    target->has_name = source->has_name;
    return true;
}

std::string Translation::capitalize(const std::string &str, bool all_words)
{
    string upper = str;

    if (!upper.empty())
    {
        upper[0] = toupper(upper[0]);

        if (all_words)
        {
            for (size_t i = 1; i < upper.size(); i++)
                if (isspace(upper[i-1]))
                    upper[i] = toupper(upper[i]);
        }
    }

    return upper;
}

void addNameWord (string &out, const string &word)
{
    if (word.empty())
        return;
    if (out.length() > 0)
        out.append(" ");
    out.append(Translation::capitalize(word));
}

void Translation::setNickname(df::language_name *name, std::string nick)
{
    CHECK_NULL_POINTER(name);

    if (!name->has_name)
    {
        if (nick.empty())
            return;

        *name = df::language_name();

        name->language = 0;
        name->has_name = true;
    }

    name->nickname = nick;

    // If the nick is empty, check if this made the whole name empty
    if (name->nickname.empty() && name->first_name.empty())
    {
        bool has_words = false;
        for (int i = 0; i < 7; i++)
            if (name->words[i] >= 0)
                has_words = true;

        if (!has_words)
            name->has_name = false;
    }
}

string Translation::TranslateName(const df::language_name * name, bool inEnglish, bool onlyLastPart)
{
    CHECK_NULL_POINTER(name);

    string out;
    string word;

    if (!onlyLastPart) {
        if (!name->first_name.empty())
            addNameWord(out, name->first_name);

        if (!name->nickname.empty())
        {
            word = "`" + name->nickname + "'";
            switch ((d_init && gametype) ? d_init->nickname[*gametype] : d_init_nickname::CENTRALIZE)
            {
            case d_init_nickname::REPLACE_ALL:
                out = word;
                return out;
            case d_init_nickname::REPLACE_FIRST:
                out = "";
                break;
            case d_init_nickname::CENTRALIZE:
                break;
            }
            addNameWord(out, word);
        }
    }

    if (!inEnglish)
    {
        if (name->words[0] >= 0 || name->words[1] >= 0)
        {
            word.clear();
            if (name->words[0] >= 0)
                word.append(*world->raws.language.translations[name->language]->words[name->words[0]]);
            if (name->words[1] >= 0)
                word.append(*world->raws.language.translations[name->language]->words[name->words[1]]);
            addNameWord(out, word);
        }
        if (name->words[5] >= 0)
        {
            word.clear();
            for (int i = 2; i <= 5; i++)
                if (name->words[i] >= 0)
                    word.append(*world->raws.language.translations[name->language]->words[name->words[i]]);
            addNameWord(out, word);
        }
        if (name->words[6] >= 0)
        {
            word.clear();
            word.append(*world->raws.language.translations[name->language]->words[name->words[6]]);
            addNameWord(out, word);
        }
    }
    else
    {
        if (name->words[0] >= 0 || name->words[1] >= 0)
        {
            word.clear();
            if (name->words[0] >= 0)
                word.append(world->raws.language.words[name->words[0]]->forms[name->parts_of_speech[0]]);
            if (name->words[1] >= 0)
                word.append(world->raws.language.words[name->words[1]]->forms[name->parts_of_speech[1]]);
            addNameWord(out, word);
        }
        if (name->words[5] >= 0)
        {
            if (out.length() > 0)
                out.append(" the");
            else
                out.append("The");

            for (int i = 2; i <= 5; i++)
            {
                if (name->words[i] >= 0)
                    addNameWord(out, world->raws.language.words[name->words[i]]->forms[name->parts_of_speech[i]]);
            }
        }
        if (name->words[6] >= 0)
        {
            if (out.length() > 0)
                out.append(" of");
            else
                out.append("Of");

            addNameWord(out, world->raws.language.words[name->words[6]]->forms[name->parts_of_speech[6]]);
        }
    }

    return out;
}
