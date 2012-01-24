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

#include "Internal.h"

#include <string>
#include <vector>
#include <map>
#include <cassert>
using namespace std;

#include "modules/Translation.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "Types.h"
#include "ModuleFactory.h"
#include "Core.h"

using namespace DFHack;
using namespace DFHack::Simple;

#include "DataDefs.h"
#include "df/world.h"

using df::global::world;

bool Translation::IsValid ()
{
    return (world->raws.language.words.size() > 0) && (world->raws.language.translations.size() > 0);
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

string Translation::TranslateName(const df::language_name * name, bool inEnglish)
{
    string out;
    assert (d->Started);

    if(!inEnglish)
    {
        if (name->words[0] >= 0 || name->words[1] >= 0)
        {
            if (name->words[0] >= 0)
                out.append(*world->raws.language.translations[name->language]->words[name->words[0]]);
            if (name->words[1] >= 0)
                out.append(*world->raws.language.translations[name->language]->words[name->words[1]]);
            out[0] = toupper(out[0]);
        }
        if (name->words[5] >= 0)
        {
            string word;
            for (int i = 2; i <= 5; i++)
                if (name->words[i] >= 0)
                    word.append(*world->raws.language.translations[name->language]->words[name->words[i]]);
            word[0] = toupper(word[0]);
            if (out.length() > 0)
                out.append(" ");
            out.append(word);
        }
        if (name->words[6] >= 0)
        {
            string word = *world->raws.language.translations[name->language]->words[name->words[6]];

            word[0] = toupper(word[0]);
            if (out.length() > 0)
                out.append(" ");
            out.append(word);
        }
    }
    else
    {
        if (name->words[0] >= 0 || name->words[1] >= 0)
        {
            if (name->words[0] >= 0)
                out.append(world->raws.language.words[name->words[0]]->forms[name->parts_of_speech[0].value]);
            if (name->words[1] >= 0)
                out.append(world->raws.language.words[name->words[1]]->forms[name->parts_of_speech[1].value]);
            out[0] = toupper(out[0]);
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
                {
                    string word = world->raws.language.words[name->words[i]]->forms[name->parts_of_speech[i].value];
                    word[0] = toupper(word[0]);
                    out.append(" " + word);
                }
            }
        }
        if (name->words[6] >= 0)
        {
            if (out.length() > 0)
                out.append(" of");
            else
                out.append("Of");

            string word = world->raws.language.words[name->words[6]]->forms[name->parts_of_speech[6].value];
            word[0] = toupper(word[0]);
            out.append(" " + word);
        }
    }
    return out;
}
