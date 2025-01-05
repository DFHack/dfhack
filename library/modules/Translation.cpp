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

#include "VersionInfo.h"
#include "MemAccess.h"
#include "Types.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "Error.h"
#include "DataDefs.h"

#include "modules/Translation.h"
#include "modules/Random.h"

#include "df/d_init.h"
#include "df/language_name.h"
#include "df/language_translation.h"
#include "df/language_word.h"
#include "df/language_name_component.h"
#include "df/language_word_table_index.h"
#include "df/world.h"

#include <string>
#include <vector>
#include <map>

using namespace DFHack;
using namespace df::enums;

using std::vector, std::string;

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

    *target = *source;
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

static string translate_word(const df::language_name * name, size_t word_idx) {
    CHECK_NULL_POINTER(name);

    auto translation = vector_get(world->raws.language.translations, name->language);
    if (!translation)
        return "";

    auto word = vector_get(translation->words, word_idx);
    if (!word)
        return "";

    return *word;
}

static string translate_english_word(const df::language_name * name, size_t part_idx) {
    CHECK_NULL_POINTER(name);

    if (part_idx >= 7)
        return "";

    auto words = vector_get(world->raws.language.words, name->words[part_idx]);
    if (!words)
        return "";

    df::part_of_speech part = name->parts_of_speech[part_idx];
    if (part < df::part_of_speech::Noun || part > df::part_of_speech::VerbGerund)
        return "";

    return words->forms[part];
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
            switch ((d_init && gametype) ? d_init->display.nickname[*gametype] : d_init_nickname::CENTRALIZE)
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
                word.append(translate_word(name, name->words[0]));
            if (name->words[1] >= 0)
                word.append(translate_word(name, name->words[1]));
            addNameWord(out, word);
        }
        word.clear();
        for (int i = 2; i <= 5; i++)
            if (name->words[i] >= 0)
                word.append(translate_word(name, name->words[i]));
        addNameWord(out, word);
        if (name->words[6] >= 0)
        {
            word.clear();
            word.append(translate_word(name, name->words[6]));
            addNameWord(out, word);
        }
    }
    else
    {
        if (name->words[0] >= 0 || name->words[1] >= 0)
        {
            word.clear();
            if (name->words[0] >= 0)
                word.append(translate_english_word(name, 0));
            if (name->words[1] >= 0)
                word.append(translate_english_word(name, 1));
            addNameWord(out, word);
        }
        if (name->words[2] >= 0 || name->words[3] >= 0 || name->words[4] >= 0 || name->words[5] >= 0)
        {
            if (out.length() > 0)
                out.append(" the");
            else
                out.append("The");
        }
        for (size_t i = 2; i <= 5; i++)
        {
            if (name->words[i] >= 0)
                addNameWord(out, translate_english_word(name, i));
        }
        if (name->words[6] >= 0)
        {
            if (out.length() > 0)
                out.append(" of");
            else
                out.append("Of");

            addNameWord(out, translate_english_word(name, 6));
        }
    }

    return out;
}

Random::MersenneRNG rng;
bool rng_inited = false;
// void word_selectorst::choose_word(int32_t &index,short &asp,WordPlace place)
void ChooseWord (const df::language_word_table *table, int32_t &index, df::part_of_speech &asp, df::language_word_table_index place)
{
    if (table->parts[place].size())
    {
        int offset = rng.df_trandom(table->parts[place].size());
        index = table->words[place][offset];
        asp = table->parts[place][offset];
    }
    else
    {
        index = rng.df_trandom(world->raws.language.words.size());
        asp = (df::part_of_speech)(rng.df_trandom(9));
        Core::getInstance().getConsole().printerr("Impoverished Word Selector");
    }
}

// void generatename(namest &name,int32_t language_index,short nametype,word_selectorst &major_selector,word_selectorst &minor_selector)
void Translation::GenerateName(df::language_name *name, int language_index, df::language_name_type nametype, df::language_word_table *major_selector, df::language_word_table *minor_selector)
{
    CHECK_NULL_POINTER(name);
    CHECK_NULL_POINTER(major_selector);
    CHECK_NULL_POINTER(minor_selector);
    if (!rng_inited)
    {
        rng.init();
        rng_inited = true;
    }
    for (int i = 0; i < 100; i++)
    {
        if ((nametype != language_name_type::LegendaryFigure) && (nametype != language_name_type::FigureNoFirst))
        {
            *name = df::language_name();
            if (language_index == -1)
                language_index = rng.df_trandom(world->raws.language.translations.size());
            name->type = nametype;
            name->language = language_index;
        }
        name->has_name = 1;
        if (name->language == -1)
            name->language = rng.df_trandom(world->raws.language.translations.size());

        int r; // persistent random number for various cases
        switch (nametype)
        {
        case language_name_type::Figure:
        case language_name_type::FigureNoFirst:
        case language_name_type::FigureFirstOnly:
        case language_name_type::TrueName:
            if (nametype != language_name_type::FigureNoFirst)
            {
                name->first_name = "";
                int32_t index;
                df::part_of_speech asp;
                ChooseWord(major_selector, index, asp, language_word_table_index::FirstName);
                // language_handlerst::addnativesyllable(index, name->first_name, name->language)
                if (name->language >= 0 && (size_t)name->language < world->raws.language.translations.size())
                {
                    auto trans = world->raws.language.translations[name->language];
                    if (index >= 0 && (size_t)index < trans->words.size())
                        name->first_name.append(trans->words[index]->c_str());
                }
            }
            if (nametype != language_name_type::FigureFirstOnly)
            {
                switch (rng.df_trandom(2))
                {
                case 0:
                    ChooseWord(major_selector, name->words[language_name_component::FrontCompound], name->parts_of_speech[language_name_component::FrontCompound], language_word_table_index::FrontCompound);
                    ChooseWord(minor_selector, name->words[language_name_component::RearCompound], name->parts_of_speech[language_name_component::RearCompound], language_word_table_index::RearCompound);
                    break;
                case 1:
                    ChooseWord(minor_selector, name->words[language_name_component::FrontCompound], name->parts_of_speech[language_name_component::FrontCompound], language_word_table_index::FrontCompound);
                    ChooseWord(major_selector, name->words[language_name_component::RearCompound], name->parts_of_speech[language_name_component::RearCompound], language_word_table_index::RearCompound);
                    break;
                }
            }
            break;
        case language_name_type::Artifact:
        case language_name_type::ElfTree:
        case language_name_type::River:
            r = rng.df_trandom(3);
            if (r < 2)
            {
                switch (rng.df_trandom(2))
                {
                case 0:
                    ChooseWord(major_selector, name->words[language_name_component::FrontCompound], name->parts_of_speech[language_name_component::FrontCompound], language_word_table_index::FrontCompound);
                    ChooseWord(minor_selector, name->words[language_name_component::RearCompound], name->parts_of_speech[language_name_component::RearCompound], language_word_table_index::RearCompound);
                    break;
                case 1:
                    ChooseWord(minor_selector, name->words[language_name_component::FrontCompound], name->parts_of_speech[language_name_component::FrontCompound], language_word_table_index::FrontCompound);
                    ChooseWord(major_selector, name->words[language_name_component::RearCompound], name->parts_of_speech[language_name_component::RearCompound], language_word_table_index::RearCompound);
                    break;
                }
            }
            if (r == 0)
                break;
            // fall through if r > 0
        case language_name_type::LegendaryFigure:
            // if it was this specific type, 1/3 chance to just pick an Adjective and then bail out
            // 40d didn't appear to have this logic
            if (nametype == language_name_type::LegendaryFigure && !rng.df_trandom(3))
            {
                ChooseWord(major_selector, name->words[language_name_component::FirstAdjective], name->parts_of_speech[language_name_component::FirstAdjective], language_word_table_index::Adjectives);
                break;
            }
            // fall through
        case language_name_type::ArtImage:
            r = rng.df_trandom(2);
            ChooseWord(r ? major_selector : minor_selector, name->words[language_name_component::TheX], name->parts_of_speech[language_name_component::TheX], language_word_table_index::FirstName);

            switch (rng.df_trandom(50) ? rng.df_trandom(2) : rng.df_trandom(3))
            {
            case 0:
                ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                break;
            case 2:
                ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                r = rng.df_trandom(2);
                // fall through
            case 1:
                ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::FirstAdjective], name->parts_of_speech[language_name_component::FirstAdjective], language_word_table_index::Adjectives);
                if (!rng.df_trandom(100))
                    ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::SecondAdjective], name->parts_of_speech[language_name_component::SecondAdjective], language_word_table_index::Adjectives);
                break;
            }
            if (!rng.df_trandom(100))
            {
                r = rng.df_trandom(2);
                ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::HyphenCompound], name->parts_of_speech[language_name_component::HyphenCompound], language_word_table_index::TheX);
            }
            break;
        case language_name_type::Civilization:
        case language_name_type::World:
        case language_name_type::Region:
        case language_name_type::EntitySite:
        case language_name_type::NomadicGroup:
        case language_name_type::MigratingGroup:
        case language_name_type::Vessel:
        case language_name_type::MilitaryUnit:
        case language_name_type::Religion:
        case language_name_type::MountainPeak:
        case language_name_type::Temple:
        case language_name_type::Keep:
        case language_name_type::MeadHall:
        case language_name_type::CraftStore:
        case language_name_type::WeaponStore:
        case language_name_type::ArmorStore:
        case language_name_type::GeneralStore:
        case language_name_type::FoodStore:
        case language_name_type::War:
        case language_name_type::Battle:
        case language_name_type::Siege:
        case language_name_type::Road:
        case language_name_type::Wall:
        case language_name_type::Bridge:
        case language_name_type::Tunnel:
        case language_name_type::HighPriest:
        case language_name_type::Tomb:
        case language_name_type::OutcastGroup:
        case language_name_type::PerformanceTroupe:
        case language_name_type::Library:
        case language_name_type::PoeticForm:
        case language_name_type::MusicalForm:
        case language_name_type::DanceForm:
        case language_name_type::Festival:
        case language_name_type::CountingHouse:
        case language_name_type::Guildhall:
        case language_name_type::NecromancerTower:
        case language_name_type::Hospital:
            ChooseWord(major_selector, name->words[language_name_component::TheX], name->parts_of_speech[language_name_component::TheX], language_word_table_index::FirstName);

            switch (rng.df_trandom(50) ? rng.df_trandom(2) : rng.df_trandom(3))
            {
            case 0:
                ChooseWord(minor_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                break;
            case 2:
                ChooseWord(minor_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                // fall through
            case 1:
                ChooseWord(minor_selector, name->words[language_name_component::FirstAdjective], name->parts_of_speech[language_name_component::FirstAdjective], language_word_table_index::Adjectives);
                if (!rng.df_trandom(100))
                    ChooseWord(minor_selector, name->words[language_name_component::SecondAdjective], name->parts_of_speech[language_name_component::SecondAdjective], language_word_table_index::Adjectives);
                break;
            }
            if (!rng.df_trandom(100))
            {
                r = rng.df_trandom(2);
                ChooseWord(minor_selector, name->words[language_name_component::HyphenCompound], name->parts_of_speech[language_name_component::HyphenCompound], language_word_table_index::TheX);
            }
            break;
        case language_name_type::Squad:
            r = rng.df_trandom(2);
            ChooseWord(r ? major_selector : minor_selector, name->words[language_name_component::TheX], name->parts_of_speech[language_name_component::TheX], language_word_table_index::FirstName);

            switch (rng.df_trandom(50) ? rng.df_trandom(2) : rng.df_trandom(3))
            {
            case 0:
                ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                break;
            case 2:
                ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                r = rng.df_trandom(2);
                // fall through
            case 1:
                ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::FirstAdjective], name->parts_of_speech[language_name_component::FirstAdjective], language_word_table_index::Adjectives);
                if (!rng.df_trandom(100))
                    ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::SecondAdjective], name->parts_of_speech[language_name_component::SecondAdjective], language_word_table_index::Adjectives);
                break;
            }
            if (!rng.df_trandom(100))
            {
                r = rng.df_trandom(2);
                ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::HyphenCompound], name->parts_of_speech[language_name_component::HyphenCompound], language_word_table_index::TheX);
            }
            // If TheX is a singular noun, then switch it to Plural if possible
            if (name->parts_of_speech[language_name_component::TheX] == part_of_speech::Noun)
            {
                int word = name->words[language_name_component::TheX];
                if (word >= 0 && (size_t)word < world->raws.language.words.size() && world->raws.language.words[word]->forms[part_of_speech::NounPlural].length() > 0)
                    name->parts_of_speech[language_name_component::TheX] = part_of_speech::NounPlural;
            }
            break;
        case language_name_type::Site:
        case language_name_type::Monument:
        case language_name_type::Vault:
            switch (rng.df_trandom(2))
            {
            case 0:
                ChooseWord(major_selector, name->words[language_name_component::FrontCompound], name->parts_of_speech[language_name_component::FrontCompound], language_word_table_index::FrontCompound);
                ChooseWord(minor_selector, name->words[language_name_component::RearCompound], name->parts_of_speech[language_name_component::RearCompound], language_word_table_index::RearCompound);
                break;
            case 1:
                ChooseWord(minor_selector, name->words[language_name_component::FrontCompound], name->parts_of_speech[language_name_component::FrontCompound], language_word_table_index::FrontCompound);
                ChooseWord(major_selector, name->words[language_name_component::RearCompound], name->parts_of_speech[language_name_component::RearCompound], language_word_table_index::RearCompound);
                break;
            }
            break;
        case language_name_type::Dungeon:
            r = rng.df_trandom(3);
            if (r < 2)
            {
                ChooseWord(minor_selector, name->words[language_name_component::FrontCompound], name->parts_of_speech[language_name_component::FrontCompound], language_word_table_index::FrontCompound);
                ChooseWord(major_selector, name->words[language_name_component::RearCompound], name->parts_of_speech[language_name_component::RearCompound], language_word_table_index::RearCompound);
            }
            if (r != 0)
            {
                ChooseWord((r == 2 || rng.df_trandom(2)) ? major_selector : minor_selector, name->words[language_name_component::TheX], name->parts_of_speech[language_name_component::TheX], language_word_table_index::FirstName);
                switch (rng.df_trandom(50) ? rng.df_trandom(2) : rng.df_trandom(3))
                {
                case 0:
                    ChooseWord(minor_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                    break;
                case 2:
                    ChooseWord(minor_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                    // fall through
                case 1:
                    ChooseWord(minor_selector, name->words[language_name_component::FirstAdjective], name->parts_of_speech[language_name_component::FirstAdjective], language_word_table_index::Adjectives);
                    if (!rng.df_trandom(100))
                        ChooseWord(minor_selector, name->words[language_name_component::SecondAdjective], name->parts_of_speech[language_name_component::SecondAdjective], language_word_table_index::Adjectives);
                    break;
                }
                if (!rng.df_trandom(100))
                {
                    r = rng.df_trandom(2);
                    ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::HyphenCompound], name->parts_of_speech[language_name_component::HyphenCompound], language_word_table_index::TheX);
                }
            }
            break;
        case language_name_type::FalseIdentity:
            if (rng.df_trandom(3))
            {
                r = rng.df_trandom(2);
                ChooseWord(r ? major_selector : minor_selector, name->words[language_name_component::TheX], name->parts_of_speech[language_name_component::TheX], language_word_table_index::FirstName);

                switch (rng.df_trandom(50) ? rng.df_trandom(2) : rng.df_trandom(3))
                {
                case 0:
                    ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                    break;
                case 2:
                    ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                    r = rng.df_trandom(2);
                    // fall through
                case 1:
                    ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::FirstAdjective], name->parts_of_speech[language_name_component::FirstAdjective], language_word_table_index::Adjectives);
                    if (!rng.df_trandom(100))
                        ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::SecondAdjective], name->parts_of_speech[language_name_component::SecondAdjective], language_word_table_index::Adjectives);
                    break;
                }
                if (!rng.df_trandom(100))
                {
                    r = rng.df_trandom(2);
                    ChooseWord(r ? minor_selector : major_selector, name->words[language_name_component::HyphenCompound], name->parts_of_speech[language_name_component::HyphenCompound], language_word_table_index::TheX);
                }
                // If TheX is a plural noun, then switch it to singular if possible
                if (name->parts_of_speech[language_name_component::TheX] == part_of_speech::NounPlural)
                {
                    int word = name->words[language_name_component::TheX];
                    if (word >= 0 && (size_t)word < world->raws.language.words.size() && world->raws.language.words[word]->forms[part_of_speech::Noun].length() > 0)
                        name->parts_of_speech[language_name_component::TheX] = part_of_speech::Noun;
                }
                break;
            }
            if (!rng.df_trandom(2))
            {
                switch (rng.df_trandom(2))
                {
                case 0:
                    ChooseWord(major_selector, name->words[language_name_component::FrontCompound], name->parts_of_speech[language_name_component::FrontCompound], language_word_table_index::FrontCompound);
                    ChooseWord(minor_selector, name->words[language_name_component::RearCompound], name->parts_of_speech[language_name_component::RearCompound], language_word_table_index::RearCompound);
                    break;
                case 1:
                    ChooseWord(minor_selector, name->words[language_name_component::FrontCompound], name->parts_of_speech[language_name_component::FrontCompound], language_word_table_index::FrontCompound);
                    ChooseWord(major_selector, name->words[language_name_component::RearCompound], name->parts_of_speech[language_name_component::RearCompound], language_word_table_index::RearCompound);
                    break;
                }
            }
            else
            {
                name->first_name = "";
                int32_t index;
                df::part_of_speech asp;
                ChooseWord(major_selector, index, asp, language_word_table_index::FirstName);
                // language_handlerst::addnativesyllable(index, name->first_name, name->language)
                if (name->language >= 0 && (size_t)name->language < world->raws.language.translations.size())
                {
                    auto trans = world->raws.language.translations[name->language];
                    if (index >= 0 && (size_t)index < trans->words.size())
                        name->first_name.append(trans->words[index]->c_str());
                }
            }
            ChooseWord(major_selector, name->words[language_name_component::FirstAdjective], name->parts_of_speech[language_name_component::FirstAdjective], language_word_table_index::Adjectives);
            break;
        case language_name_type::MerchantCompany:
        case language_name_type::CraftGuild:
            ChooseWord(major_selector, name->words[language_name_component::TheX], name->parts_of_speech[language_name_component::TheX], language_word_table_index::FirstName);

            switch (rng.df_trandom(50) ? rng.df_trandom(2) : rng.df_trandom(3))
            {
            case 0:
                ChooseWord(minor_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                break;
            case 2:
                ChooseWord(minor_selector, name->words[language_name_component::OfX], name->parts_of_speech[language_name_component::OfX], language_word_table_index::OfX);
                r = rng.df_trandom(2);
                // fall through
            case 1:
                ChooseWord(minor_selector, name->words[language_name_component::FirstAdjective], name->parts_of_speech[language_name_component::FirstAdjective], language_word_table_index::Adjectives);
                if (!rng.df_trandom(100))
                    ChooseWord(minor_selector, name->words[language_name_component::SecondAdjective], name->parts_of_speech[language_name_component::SecondAdjective], language_word_table_index::Adjectives);
                break;
            }
            if (!rng.df_trandom(100))
            {
                r = rng.df_trandom(2);
                ChooseWord(minor_selector, name->words[language_name_component::HyphenCompound], name->parts_of_speech[language_name_component::HyphenCompound], language_word_table_index::TheX);
            }
            // If TheX is a plural noun, then switch it to Singular if possible
            if (name->parts_of_speech[language_name_component::TheX] == part_of_speech::NounPlural)
            {
                int word = name->words[language_name_component::TheX];
                if (word >= 0 && (size_t)word < world->raws.language.words.size() && world->raws.language.words[word]->forms[part_of_speech::Noun].length() > 0)
                    name->parts_of_speech[language_name_component::TheX] = part_of_speech::Noun;
            }
            break;
        default:
            break;
        }

        int adj1 = name->words[language_name_component::FirstAdjective];
        int adj2 = name->words[language_name_component::SecondAdjective];
        if (adj1 != -1 && adj2 != -1 && (size_t)adj1 < world->raws.language.words.size() && (size_t)adj2 < world->raws.language.words.size() &&
            world->raws.language.words[adj1]->adj_dist < world->raws.language.words[adj2]->adj_dist)
        {
            std::swap(name->words[language_name_component::FirstAdjective], name->words[language_name_component::SecondAdjective]);
            std::swap(name->parts_of_speech[language_name_component::FirstAdjective], name->parts_of_speech[language_name_component::SecondAdjective]);
        }
        bool reject = false;
        if ((name->parts_of_speech[language_name_component::TheX] == df::part_of_speech::NounPlural) && (name->parts_of_speech[language_name_component::OfX] == df::part_of_speech::NounPlural))
            reject = true;
        if (name->words[language_name_component::FrontCompound] != -1)
        {
            int word = name->words[language_name_component::FrontCompound];
            if (name->words[language_name_component::RearCompound] == word) reject = true;
            if (name->words[language_name_component::OfX] == word) reject = true;
            if (name->words[language_name_component::HyphenCompound] == word) reject = true;
            if (name->words[language_name_component::FirstAdjective] == word) reject = true;
            if (name->words[language_name_component::SecondAdjective] == word) reject = true;
            if (name->words[language_name_component::TheX] == word) reject = true;
        }
        if (name->words[language_name_component::RearCompound] != -1)
        {
            int word = name->words[language_name_component::RearCompound];
            if (name->words[language_name_component::OfX] == word) reject = true;
            if (name->words[language_name_component::HyphenCompound] == word) reject = true;
            if (name->words[language_name_component::FirstAdjective] == word) reject = true;
            if (name->words[language_name_component::SecondAdjective] == word) reject = true;
            if (name->words[language_name_component::TheX] == word) reject = true;
        }
        if (name->words[language_name_component::HyphenCompound] != -1)
        {
            int word = name->words[language_name_component::HyphenCompound];
            if (name->words[language_name_component::OfX] == word) reject = true;
            if (name->words[language_name_component::FirstAdjective] == word) reject = true;
            if (name->words[language_name_component::SecondAdjective] == word) reject = true;
            if (name->words[language_name_component::TheX] == word) reject = true;
        }
        if (name->words[language_name_component::FirstAdjective] != -1)
        {
            int word = name->words[language_name_component::FirstAdjective];
            if (name->words[language_name_component::OfX] == word) reject = true;
            if (name->words[language_name_component::SecondAdjective] == word) reject = true;
            if (name->words[language_name_component::TheX] == word) reject = true;
        }
        if (name->words[language_name_component::SecondAdjective] != -1)
        {
            int word = name->words[language_name_component::SecondAdjective];
            if (name->words[language_name_component::OfX] == word) reject = true;
            if (name->words[language_name_component::TheX] == word) reject = true;
        }
        if (name->words[language_name_component::TheX] != -1)
        {
            int word = name->words[language_name_component::TheX];
            if (name->words[language_name_component::OfX] == word) reject = true;
        }
        if (!reject)
            return;
    }
}
