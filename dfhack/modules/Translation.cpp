/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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

#include "DFCommonInternal.h"
#include "../private/APIPrivate.h"
#include "modules/Translation.h"
#include "DFMemInfo.h"
#include "DFProcess.h"
#include "DFVector.h"
#include "DFTypes.h"
#include "modules/Translation.h"

using namespace DFHack;

struct Translation::Private
{
    uint32_t genericAddress;
    uint32_t transAddress;
    uint32_t word_table_offset;
    uint32_t sizeof_string;
    
    // translation
    Dicts dicts;
    
    APIPrivate *d;
    bool Inited;
    bool Started;
};

Translation::Translation(APIPrivate * d_)
{
    d = new Private;
    d->d = d_;
    d->Inited = d->Started = false;
    memory_info * mem = d->d->offset_descriptor;
    d->genericAddress = mem->getAddress ("language_vector");
    d->transAddress = mem->getAddress ("translation_vector");
    d->word_table_offset = mem->getOffset ("word_table");
    d->sizeof_string = mem->getHexValue ("sizeof_string");
    d->Inited = true;
}

Translation::~Translation()
{
    if(d->Started)
        Finish();
    delete d;
}

bool Translation::Start()
{
    if(!d->Inited)
        return false;
    Process * p = d->d->p;
    DfVector <uint32_t> genericVec (p, d->genericAddress);
    DfVector <uint32_t> transVec (p, d->transAddress);
    DFDict & translations = d->dicts.translations;
    DFDict & foreign_languages = d->dicts.foreign_languages;

    translations.resize(10);
    for (uint32_t i = 0;i < genericVec.size();i++)
    {
        uint32_t genericNamePtr = genericVec.at(i);
        for(int i=0; i<10;i++)
        {
            string word = p->readSTLString (genericNamePtr + i * d->sizeof_string);
            translations[i].push_back (word);
        }
    }

    foreign_languages.resize(transVec.size());
    for (uint32_t i = 0; i < transVec.size();i++)
    {
        uint32_t transPtr = transVec.at(i);
        
        DfVector <uint32_t> trans_names_vec (p, transPtr + d->word_table_offset);
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

