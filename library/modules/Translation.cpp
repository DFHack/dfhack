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

Module* DFHack::createTranslation()
{
    return new Translation();
}

struct Translation::Private
{
    void * genericAddress;
    void * transAddress;
    uint32_t word_table_offset;
    uint32_t sizeof_string;

    // translation
    Dicts dicts;

    bool Inited;
    bool Started;
    // names
    uint32_t name_firstname_offset;
    uint32_t name_nickname_offset;
    uint32_t name_words_offset;
    uint32_t name_parts_offset;
    uint32_t name_language_offset;
    uint32_t name_set_offset;
    bool namesInited;
    bool namesFailed;
};

Translation::Translation()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->Inited = d->Started = false;
    OffsetGroup * OG_Translation = c.vinfo->getGroup("Translations");
    OffsetGroup * OG_String = c.vinfo->getGroup("string");
    d->genericAddress = OG_Translation->getAddress ("language_vector");
    d->transAddress = OG_Translation->getAddress ("translation_vector");
    d->word_table_offset = OG_Translation->getOffset ("word_table");
    d->sizeof_string = OG_String->getHexValue ("sizeof");
    d->Inited = true;
    d->namesInited = false;
    d->namesFailed = false;
}

Translation::~Translation()
{
    if(d->Started)
        Finish();
    delete d;
}

bool Translation::Start()
{
    Core & c = Core::getInstance();
    if(!d->Inited)
        return false;
    Process * p = c.p;
    Finish();
    vector <char *> & genericVec = *(vector <char *> *) d->genericAddress;
    vector <char *> & transVec = *(vector <char *> *) d->transAddress;
    DFDict & translations = d->dicts.translations;
    DFDict & foreign_languages = d->dicts.foreign_languages;

    translations.resize(10);
    for (uint32_t i = 0;i < genericVec.size();i++)
    {
        char * genericNamePtr = genericVec[i];
        for(int j=0; j<10;j++)
        {
            string word = p->readSTLString (genericNamePtr + j * d->sizeof_string);
            translations[j].push_back (word);
        }
    }

    foreign_languages.resize(transVec.size());
    for (uint32_t i = 0; i < transVec.size();i++)
    {
        char * transPtr = transVec.at(i);
        vector <void *> & trans_names_vec = *(vector <void *> *) (transPtr + d->word_table_offset);
        for (uint32_t j = 0;j < trans_names_vec.size();j++)
        {
            void * transNamePtr = trans_names_vec[j];
            string name = p->readSTLString (transNamePtr);
            foreign_languages[i].push_back (name);
        }
    }
    d->Started = true;
    return true;
}

bool Translation::Finish()
{
    d->dicts.foreign_languages.clear();
    d->dicts.translations.clear();
    d->Started = false;
    return true;
}

Dicts * Translation::getDicts()
{
    assert(d->Started);

    if(d->Started)
        return &d->dicts;
    return 0;
}

bool Translation::InitReadNames()
{
    Core & c = Core::getInstance();
    try
    {
        OffsetGroup * OG = c.vinfo->getGroup("name");
        d->name_firstname_offset = OG->getOffset("first");
        d->name_nickname_offset = OG->getOffset("nick");
        d->name_words_offset = OG->getOffset("second_words");
        d->name_parts_offset = OG->getOffset("parts_of_speech");
        d->name_language_offset = OG->getOffset("language");
        d->name_set_offset = OG->getOffset("has_name");
    }
    catch(exception &)
    {
        d->namesFailed = true;
        return false;
    }
    d->namesInited = true;
    return true;
}

bool Translation::readName(t_name & name, df::language_name * source)
{
    Core & c = Core::getInstance();
    Process * p = c.p;
    if(d->namesFailed)
    {
        return false;
    }
    if(!d->namesInited)
    {
        if(!InitReadNames()) return false;
    }
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
    Core & c = Core::getInstance();
    Process * p = c.p;

    target->first_name = source->first_name;
    target->nickname = source->nickname;
    target->has_name = source->has_name;
    target->language = source->language;
    memcpy(&target->parts_of_speech, &source->parts_of_speech, sizeof (source->parts_of_speech));
    memcpy(&target->words, &source->words, sizeof (source->words));
    target->unknown = source->unknown;
    return true;
}

string Translation::TranslateName(const df::language_name * name, bool inEnglish)
{
    string out;
    assert (d->Started);

    map<string, vector<string> >::const_iterator it;

    if(!inEnglish)
    {
        if(name->words[0] >=0 || name->words[1] >=0)
        {
            if(name->words[0]>=0) out.append(d->dicts.foreign_languages[name->language][name->words[0]]);
            if(name->words[1]>=0) out.append(d->dicts.foreign_languages[name->language][name->words[1]]);
            out[0] = toupper(out[0]);
        }
        if(name->words[5] >=0)
        {
            string word;
            for(int i=2;i<=5;i++)
                if(name->words[i]>=0) word.append(d->dicts.foreign_languages[name->language][name->words[i]]);
            word[0] = toupper(word[0]);
            if(out.length() > 0) out.append(" ");
            out.append(word);
        }
        if(name->words[6] >=0)
        {
            string word;
            word.append(d->dicts.foreign_languages[name->language][name->words[6]]);
            word[0] = toupper(word[0]);
            if(out.length() > 0) out.append(" ");
            out.append(word);
        }
    }
    else
    {
        if(name->words[0] >=0 || name->words[1] >=0)
        {
            if(name->words[0]>=0) out.append(d->dicts.translations[name->parts_of_speech[0].value+1][name->words[0]]);
            if(name->words[1]>=0) out.append(d->dicts.translations[name->parts_of_speech[1].value+1][name->words[1]]);
            out[0] = toupper(out[0]);
        }
        if(name->words[5] >=0)
        {
            if(out.length() > 0)
                out.append(" the");
            else
                out.append("The");
            string word;
            for(int i=2;i<=5;i++)
            {
                if(name->words[i]>=0)
                {
                    word = d->dicts.translations[name->parts_of_speech[i].value+1][name->words[i]];
                    word[0] = toupper(word[0]);
                    out.append(" " + word);
                }
            }
        }
        if(name->words[6] >=0)
        {
            if(out.length() > 0)
                out.append(" of");
            else
                out.append("Of");
            string word;
            word.append(d->dicts.translations[name->parts_of_speech[6].value+1][name->words[6]]);
            word[0] = toupper(word[0]);
            out.append(" " + word);
        }
    }
    return out;
}

