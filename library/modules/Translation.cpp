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

#include "dfhack/modules/Translation.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/Process.h"
#include "dfhack/Vector.h"
#include "dfhack/Types.h"
#include "ModuleFactory.h"
#include <dfhack/Core.h>
using namespace DFHack;

Module* DFHack::createTranslation()
{
    return new Translation();
}

struct Translation::Private
{
    uint32_t genericAddress;
    uint32_t transAddress;
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
    DfVector <uint32_t> genericVec (d->genericAddress);
    DfVector <uint32_t> transVec (d->transAddress);
    DFDict & translations = d->dicts.translations;
    DFDict & foreign_languages = d->dicts.foreign_languages;

    translations.resize(10);
    for (uint32_t i = 0;i < genericVec.size();i++)
    {
        uint32_t genericNamePtr = genericVec.at(i);
        for(int j=0; j<10;i++)
        {
            string word = p->readSTLString (genericNamePtr + j * d->sizeof_string);
            translations[j].push_back (word);
        }
    }

    foreign_languages.resize(transVec.size());
    for (uint32_t i = 0; i < transVec.size();i++)
    {
        uint32_t transPtr = transVec.at(i);
        DfVector <uint32_t> trans_names_vec (transPtr + d->word_table_offset);
        for (uint32_t j = 0;j < trans_names_vec.size();j++)
        {
            uint32_t transNamePtr = trans_names_vec.at(j);
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

bool Translation::readName(t_name & name, uint32_t address)
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
    p->readSTLString(address + d->name_firstname_offset , name.first_name, 128);
    p->readSTLString(address + d->name_nickname_offset , name.nickname, 128);
    p->read(address + d->name_words_offset, 7*4, (uint8_t *)name.words);
    p->read(address + d->name_parts_offset, 7*2, (uint8_t *)name.parts_of_speech);
    name.language = p->readDWord(address + d->name_language_offset);
    name.has_name = p->readByte(address + d->name_set_offset);
    return true;
}

bool Translation::copyName(uint32_t address, uint32_t target)
{
    uint8_t buf[28];

    if (address == target)
        return true;
    Core & c = Core::getInstance();
    Process * p = c.p;

    p->copySTLString(address + d->name_firstname_offset, target + d->name_firstname_offset);
    p->copySTLString(address + d->name_nickname_offset, target + d->name_nickname_offset);
    p->read(address + d->name_words_offset, 7*4, buf);
    p->write(target + d->name_words_offset, 7*4, buf);
    p->read(address + d->name_parts_offset, 7*2, buf);
    p->write(target + d->name_parts_offset, 7*2, buf);
    p->writeDWord(target + d->name_language_offset, p->readDWord(address + d->name_language_offset));
    p->writeByte(target + d->name_set_offset, p->readByte(address + d->name_set_offset));
    return true;
}

string Translation::TranslateName(const t_name &name, bool inEnglish)
{
    string out;
    assert (d->Started);

    map<string, vector<string> >::const_iterator it;

    if(!inEnglish)
    {
        if(name.words[0] >=0 || name.words[1] >=0)
        {
            if(name.words[0]>=0) out.append(d->dicts.foreign_languages[name.language][name.words[0]]);
            if(name.words[1]>=0) out.append(d->dicts.foreign_languages[name.language][name.words[1]]);
            out[0] = toupper(out[0]);
        }
        if(name.words[5] >=0)
        {
            string word;
            for(int i=2;i<=5;i++)
                if(name.words[i]>=0) word.append(d->dicts.foreign_languages[name.language][name.words[i]]);
            word[0] = toupper(word[0]);
            if(out.length() > 0) out.append(" ");
            out.append(word);
        }
        if(name.words[6] >=0)
        {
            string word;
            word.append(d->dicts.foreign_languages[name.language][name.words[6]]);
            word[0] = toupper(word[0]);
            if(out.length() > 0) out.append(" ");
            out.append(word);
        }
    }
    else
    {
        if(name.words[0] >=0 || name.words[1] >=0)
        {
            if(name.words[0]>=0) out.append(d->dicts.translations[name.parts_of_speech[0]+1][name.words[0]]);
            if(name.words[1]>=0) out.append(d->dicts.translations[name.parts_of_speech[1]+1][name.words[1]]);
            out[0] = toupper(out[0]);
        }
        if(name.words[5] >=0)
        {
            if(out.length() > 0)
                out.append(" the");
            else
                out.append("The");
            string word;
            for(int i=2;i<=5;i++)
            {
                if(name.words[i]>=0)
                {
                    word = d->dicts.translations[name.parts_of_speech[i]+1][name.words[i]];
                    word[0] = toupper(word[0]);
                    out.append(" " + word);
                }
            }
        }
        if(name.words[6] >=0)
        {
            if(out.length() > 0)
                out.append(" of");
            else
                out.append("Of");
            string word;
            word.append(d->dicts.translations[name.parts_of_speech[6]+1][name.words[6]]);
            word[0] = toupper(word[0]);
            out.append(" " + word);
        }
    }
    return out;
}

