// Triggers a strange mood using (mostly) the same logic used in-game

#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Gui.h"
#include "modules/Units.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Translation.h"
#include "modules/Random.h"

#include "df/builtin_mats.h"
#include "df/caste_raw.h"
#include "df/caste_raw_flags.h"
#include "df/creature_raw.h"
#include "df/d_init.h"
#include "df/entity_raw.h"
#include "df/general_ref_unit_workerst.h"
#include "df/historical_entity.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/map_block.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/unit_preference.h"
#include "df/unit_relationship_type.h"
#include "df/unit_skill.h"
#include "df/unit_soul.h"
#include "df/world.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("strangemood");

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(d_init);
REQUIRE_GLOBAL(created_item_count);
REQUIRE_GLOBAL(created_item_type);
REQUIRE_GLOBAL(created_item_subtype);
REQUIRE_GLOBAL(created_item_mattype);
REQUIRE_GLOBAL(created_item_matindex);
using df::global::debug_nomoods;

Random::MersenneRNG rng;

bool isUnitMoodable (df::unit *unit)
{
    if (!Units::isCitizen(unit))
        return false;
    if (!unit->status2.limbs_grasp_count)
        return false;
    if (unit->mood != mood_type::None)
        return false;
    if (!ENUM_ATTR(profession,moodable,unit->profession))
        return false;
    if (!Units::casteFlagSet(unit->race, unit->caste, caste_raw_flags::STRANGE_MOODS))
        return false;
    return true;
}

df::job_skill getMoodSkill (df::unit *unit)
{
    if (!unit->status.current_soul)
        return job_skill::STONECRAFT;
    df::historical_entity *civ = df::historical_entity::find(unit->civ_id);
    df::unit_soul *soul = unit->status.current_soul;
    vector<df::job_skill> skills;
    df::skill_rating level = skill_rating::Dabbling;
    for (size_t i = 0; i < soul->skills.size(); i++)
    {
        df::unit_skill *skill = soul->skills[i];
        switch (skill->id)
        {
        case job_skill::MINING:
        case job_skill::CARPENTRY:
        case job_skill::DETAILSTONE:
        case job_skill::MASONRY:
        case job_skill::TANNER:
        case job_skill::WEAVING:
        case job_skill::CLOTHESMAKING:
        case job_skill::FORGE_WEAPON:
        case job_skill::FORGE_ARMOR:
        case job_skill::FORGE_FURNITURE:
        case job_skill::CUTGEM:
        case job_skill::ENCRUSTGEM:
        case job_skill::WOODCRAFT:
        case job_skill::STONECRAFT:
        case job_skill::METALCRAFT:
        case job_skill::GLASSMAKER:
        case job_skill::LEATHERWORK:
        case job_skill::BONECARVE:
        case job_skill::BOWYER:
        case job_skill::MECHANICS:
            if (skill->rating > level)
            {
                skills.clear();
                level = skill->rating;
            }
            if (skill->rating == level)
                skills.push_back(skill->id);
            break;
        default:
            break;
        }
    }
    if (!skills.size() && civ)
    {
        if (civ->resources.permitted_skill[job_skill::WOODCRAFT])
            skills.push_back(job_skill::WOODCRAFT);
        if (civ->resources.permitted_skill[job_skill::STONECRAFT])
            skills.push_back(job_skill::STONECRAFT);
        if (civ->resources.permitted_skill[job_skill::BONECARVE])
            skills.push_back(job_skill::BONECARVE);
    }
    if (!skills.size())
        skills.push_back(job_skill::STONECRAFT);
    return skills[rng.df_trandom(skills.size())];
}

int getCreatedMetalBars (int32_t idx)
{
    for (size_t i = 0; i < created_item_type->size(); i++)
    {
        if (created_item_type->at(i) == item_type::BAR &&
            created_item_subtype->at(i) == -1 &&
            created_item_mattype->at(i) == 0 &&
            created_item_matindex->at(i) == idx)
            return created_item_count->at(i);
    }
    return 0;
}

void selectWord (const df::language_word_table &table, int32_t &word, df::part_of_speech &part, int mode)
{
    if (table.parts[mode].size())
    {
        int offset = rng.df_trandom(table.parts[mode].size());
        word = table.words[mode][offset];
        part = table.parts[mode][offset];
    }
    else
    {
        word = rng.df_trandom(world->raws.language.words.size());
        part = (df::part_of_speech)(rng.df_trandom(9));
        Core::getInstance().getConsole().printerr("Impoverished Word Selector");
    }
}

void generateName(df::language_name &output, int language, int mode, const df::language_word_table &table1, const df::language_word_table &table2)
{
    for (int i = 0; i < 100; i++)
    {
        if (mode != 8 && mode != 9)
        {
            output = df::language_name();
            if (language == -1)
                language = rng.df_trandom(world->raws.language.translations.size());
            output.unknown = mode;
            output.language = language;
        }
        output.has_name = 1;
        if (output.language == -1)
            output.language = rng.df_trandom(world->raws.language.translations.size());
        int r, r2, r3;
        switch (mode)
        {
        case 0: case 9: case 10:
            if (mode != 9)
            {
                int32_t word; df::part_of_speech part;
                output.first_name.clear();
                selectWord(table1, word, part, 2);
                if (word >= 0 && size_t(word) < world->raws.language.words.size())
                    output.first_name = *world->raws.language.translations[language]->words[word];
            }
            if (mode != 10)
            {
        case 4: case 37: // this is not a typo
                if (rng.df_trandom(2))
                {
                    selectWord(table2, output.words[0], output.parts_of_speech[0], 0);
                    selectWord(table1, output.words[1], output.parts_of_speech[1], 1);
                }
                else
                {
                    selectWord(table1, output.words[0], output.parts_of_speech[0], 0);
                    selectWord(table2, output.words[1], output.parts_of_speech[1], 1);
                }
            }
            break;

        case 1: case 13: case 20:
            r = rng.df_trandom(3);
            if (r == 0 || r == 1)
            {
                if (rng.df_trandom(2))
                {
                    selectWord(table2, output.words[0], output.parts_of_speech[0], 0);
                    selectWord(table1, output.words[1], output.parts_of_speech[1], 1);
                }
                else
                {
                    selectWord(table1, output.words[0], output.parts_of_speech[0], 0);
                    selectWord(table2, output.words[1], output.parts_of_speech[1], 1);
                }
            }
            if (r == 1 || r == 2)
            {
        case 3: case 8: case 11: // this is not a typo either
                r2 = rng.df_trandom(2);
                if (r2)
                    selectWord(table1, output.words[5], output.parts_of_speech[5], 2);
                else
                    selectWord(table2, output.words[5], output.parts_of_speech[5], 2);
                r3 = rng.df_trandom(3);
                if (rng.df_trandom(50))
                    r3 = rng.df_trandom(2);
                switch (r3)
                {
                case 0:
                case 2:
                    if (r3 == 2)
                        r2 = rng.df_trandom(2);
                    if (r2)
                        selectWord(table2, output.words[6], output.parts_of_speech[6], 5);
                    else
                        selectWord(table1, output.words[6], output.parts_of_speech[6], 5);
                    if (r3 == 0)
                        break;
                    r2 = -r2;
                case 1:
                    if (r2)
                        selectWord(table1, output.words[2], output.parts_of_speech[2], 3);
                    else
                        selectWord(table2, output.words[2], output.parts_of_speech[2], 3);
                    if (!(rng.df_trandom(100)))
                        selectWord(table1, output.words[3], output.parts_of_speech[3], 3);
                    break;
                }
            }
            if (rng.df_trandom(100))
            {
                if (rng.df_trandom(2))
                    selectWord(table1, output.words[4], output.parts_of_speech[4], 4);
                else
                    selectWord(table2, output.words[4], output.parts_of_speech[4], 4);
            }
            if ((mode == 3) && (output.parts_of_speech[5] == part_of_speech::Noun) && (output.words[5] != -1) && (world->raws.language.words[output.words[5]]->forms[1].length()))
                output.parts_of_speech[5] = part_of_speech::NounPlural;
            break;

        case 2: case 5: case 6: case 12: case 14: case 15: case 16: case 17: case 18: case 19:
        case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30:
        case 31: case 32: case 33: case 34: case 35: case 36: case 38: case 39:
            selectWord(table1, output.words[5], output.parts_of_speech[5], 2);
            r3 = rng.df_trandom(3);
            if (rng.df_trandom(50))
                r3 = rng.df_trandom(2);
            switch (r3)
            {
            case 0:
            case 2:
                selectWord(table2, output.words[6], output.parts_of_speech[6], 5);
                if (r3 == 0)
                    break;
            case 1:
                selectWord(table2, output.words[2], output.parts_of_speech[2], 3);
                if (!(rng.df_trandom(100)))
                    selectWord(table2, output.words[3], output.parts_of_speech[3], 3);
                break;
            }
            if (rng.df_trandom(100))
                selectWord(table2, output.words[4], output.parts_of_speech[4], 4);
            break;

        case 7:
            r = rng.df_trandom(3);
            if (r == 0 || r == 1)
            {
                selectWord(table2, output.words[0], output.parts_of_speech[0], 0);
                selectWord(table1, output.words[1], output.parts_of_speech[1], 1);
            }
            if (r == 1 || r == 2)
            {
                r2 = rng.df_trandom(2);
                if (r == 2 || r2 == 1)
                    selectWord(table1, output.words[5], output.parts_of_speech[5], 2);
                else
                    selectWord(table2, output.words[5], output.parts_of_speech[5], 2);
                r3 = rng.df_trandom(3);
                if (rng.df_trandom(50))
                    r3 = rng.df_trandom(2);
                switch (r3)
                {
                case 0:
                case 2:
                    selectWord(table1, output.words[6], output.parts_of_speech[6], 5);
                    if (r3 == 0)
                        break;
                case 1:
                    selectWord(table2, output.words[2], output.parts_of_speech[2], 3);
                    if (!(rng.df_trandom(100)))
                        selectWord(table2, output.words[3], output.parts_of_speech[3], 3);
                    break;
                }
            }
            if (rng.df_trandom(100))
                selectWord(table2, output.words[4], output.parts_of_speech[4], 4);
            break;
        }
        if (output.words[2] != -1 && output.words[3] != -1 &&
            world->raws.language.words[output.words[3]]->adj_dist < world->raws.language.words[output.words[2]]->adj_dist)
        {
            std::swap(output.words[2], output.words[3]);
            std::swap(output.parts_of_speech[2], output.parts_of_speech[3]);
        }
        bool next = false;
        if ((output.parts_of_speech[5] == df::part_of_speech::NounPlural) && (output.parts_of_speech[6] == df::part_of_speech::NounPlural))
            next = true;
        if (output.words[0] != -1)
        {
            if (output.words[6] == -1) next = true;
            if (output.words[4] == -1) next = true;
            if (output.words[2] == -1) next = true;
            if (output.words[3] == -1) next = true;
            if (output.words[5] == -1) next = true;
        }
        if (output.words[1] != -1)
        {
            if (output.words[6] == -1) next = true;
            if (output.words[4] == -1) next = true;
            if (output.words[2] == -1) next = true;
            if (output.words[3] == -1) next = true;
            if (output.words[5] == -1) next = true;
        }
        if (output.words[4] != -1)
        {
            if (output.words[6] == -1) next = true;
            if (output.words[2] == -1) next = true;
            if (output.words[3] == -1) next = true;
            if (output.words[5] == -1) next = true;
        }
        if (output.words[2] != -1)
        {
            if (output.words[6] == -1) next = true;
            if (output.words[3] == -1) next = true;
            if (output.words[5] == -1) next = true;
        }
        if (output.words[3] != -1)
        {
            if (output.words[6] == -1) next = true;
            if (output.words[5] == -1) next = true;
        }
        if (output.words[5] != -1)
        {
            if (output.words[6] == -1) next = true;
        }
        if (!next)
            return;
    }
}

command_result df_strangemood (color_ostream &out, vector <string> & parameters)
{
    if (!Translation::IsValid())
    {
        out.printerr("Translation data unavailable!\n");
        return CR_FAILURE;
    }
    bool force = false;
    df::unit *unit = NULL;
    df::mood_type type = mood_type::None;
    df::job_skill skill = job_skill::NONE;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
            return CR_WRONG_USAGE;
        else if(parameters[i] == "-force")
            force = true;
        else if(parameters[i] == "-unit")
        {
            unit = DFHack::Gui::getSelectedUnit(out);
            if (!unit)
                return CR_FAILURE;
        }
        else if (parameters[i] == "-type")
        {
            i++;
            if (i == parameters.size())
            {
                out.printerr("No mood type specified!\n");
                return CR_WRONG_USAGE;
            }
            if (parameters[i] == "fey")
                type = mood_type::Fey;
            else if (parameters[i] == "secretive")
                type = mood_type::Secretive;
            else if (parameters[i] == "possessed")
                type = mood_type::Possessed;
            else if (parameters[i] == "fell")
                type = mood_type::Fell;
            else if (parameters[i] == "macabre")
                type = mood_type::Macabre;
            else
            {
                out.printerr("Mood type '%s' not recognized!\n", parameters[i].c_str());
                return CR_WRONG_USAGE;
            }
        }
        else if (parameters[i] == "-skill")
        {
            i++;
            if (i == parameters.size())
            {
                out.printerr("No mood skill specified!\n");
                return CR_WRONG_USAGE;
            }
            else if (parameters[i] == "miner")
                skill = job_skill::MINING;
            else if (parameters[i] == "carpenter")
                skill = job_skill::CARPENTRY;
            else if (parameters[i] == "engraver")
                skill = job_skill::DETAILSTONE;
            else if (parameters[i] == "mason")
                skill = job_skill::MASONRY;
            else if (parameters[i] == "tanner")
                skill = job_skill::TANNER;
            else if (parameters[i] == "weaver")
                skill = job_skill::WEAVING;
            else if (parameters[i] == "clothier")
                skill = job_skill::CLOTHESMAKING;
            else if (parameters[i] == "weaponsmith")
                skill = job_skill::FORGE_WEAPON;
            else if (parameters[i] == "armorsmith")
                skill = job_skill::FORGE_ARMOR;
            else if (parameters[i] == "metalsmith")
                skill = job_skill::FORGE_FURNITURE;
            else if (parameters[i] == "gemcutter")
                skill = job_skill::CUTGEM;
            else if (parameters[i] == "gemsetter")
                skill = job_skill::ENCRUSTGEM;
            else if (parameters[i] == "woodcrafter")
                skill = job_skill::WOODCRAFT;
            else if (parameters[i] == "stonecrafter")
                skill = job_skill::STONECRAFT;
            else if (parameters[i] == "metalcrafter")
                skill = job_skill::METALCRAFT;
            else if (parameters[i] == "glassmaker")
                skill = job_skill::GLASSMAKER;
            else if (parameters[i] == "leatherworker")
                skill = job_skill::LEATHERWORK;
            else if (parameters[i] == "bonecarver")
                skill = job_skill::BONECARVE;
            else if (parameters[i] == "bowyer")
                skill = job_skill::BOWYER;
            else if (parameters[i] == "mechanic")
                skill = job_skill::MECHANICS;
            else
            {
                out.printerr("Mood skill '%s' not recognized!\n", parameters[i].c_str());
                return CR_WRONG_USAGE;
            }
        }
        else
        {
            out.printerr("Unrecognized parameter: %s\n", parameters[i].c_str());
            return CR_WRONG_USAGE;
        }
    }

    CoreSuspender suspend;

    // First, check if moods are enabled at all
    if (!d_init->flags4.is_set(d_init_flags4::ARTIFACTS))
    {
        out.printerr("ARTIFACTS are not enabled!\n");
        return CR_FAILURE;
    }
    if (debug_nomoods && *debug_nomoods)
    {
        out.printerr("Strange moods disabled via debug flag!\n");
        return CR_FAILURE;
    }
    if (ui->mood_cooldown && !force)
    {
        out.printerr("Last strange mood happened too recently!\n");
        return CR_FAILURE;
    }

    // Also, make sure there isn't a mood already running
    for (size_t i = 0; i < world->units.active.size(); i++)
    {
        df::unit *cur = world->units.active[i];
        if (Units::isCitizen(cur) && cur->flags1.bits.has_mood)
        {
            ui->mood_cooldown = 1000;
            out.printerr("A strange mood is already in progress!\n");
            return CR_FAILURE;
        }
    }

    // See which units are eligible to enter moods
    vector<df::unit *> moodable_units;
    bool mood_available = false;
    for (size_t i = 0; i < world->units.active.size(); i++)
    {
        df::unit *cur = world->units.active[i];
        if (!isUnitMoodable(cur))
            continue;
        if (!cur->flags1.bits.had_mood)
            mood_available = true;
        moodable_units.push_back(cur);
    }
    if (!mood_available)
    {
        out.printerr("No dwarves are available to enter a mood!\n");
        return CR_FAILURE;
    }

    // If unit was manually selected, redo checks explicitly
    if (unit)
    {
        if (!isUnitMoodable(unit))
        {
            out.printerr("Selected unit is not eligible to enter a strange mood!\n");
            return CR_FAILURE;
        }
        if (unit->flags1.bits.had_mood)
        {
            out.printerr("Selected unit has already had a strange mood!\n");
            return CR_FAILURE;
        }
    }

    // Obey in-game mood limits
    if (!force)
    {
        if (moodable_units.size() < 20)
        {
            out.printerr("Fortress is not eligible for a strange mood at this time - not enough moodable units.\n");
            return CR_FAILURE;
        }
        int num_items = 0;
        for (size_t i = 0; i < created_item_count->size(); i++)
            num_items += created_item_count->at(i);

        int num_revealed_tiles = 0;
        for (size_t i = 0; i < world->map.map_blocks.size(); i++)
        {
            df::map_block *blk = world->map.map_blocks[i];
            for (int x = 0; x < 16; x++)
                for (int y = 0; y < 16; y++)
                    if (blk->designation[x][y].bits.subterranean && !blk->designation[x][y].bits.hidden)
                        num_revealed_tiles++;
        }
        if (num_revealed_tiles / 2304 < ui->tasks.num_artifacts)
        {
            out.printerr("Fortress is not eligible for a strange mood at this time - not enough subterranean tiles revealed.\n");
            return CR_FAILURE;
        }
        if (num_items / 200 < ui->tasks.num_artifacts)
        {
            out.printerr("Fortress is not eligible for a strange mood at this time - not enough items created\n");
            return CR_FAILURE;
        }
    }

    // Randomly select a unit to enter a mood
    if (!unit)
    {
        vector<int32_t> tickets;
        for (size_t i = 0; i < moodable_units.size(); i++)
        {
            df::unit *cur = moodable_units[i];
            if (cur->flags1.bits.had_mood)
                continue;
            if (cur->relationship_ids[df::unit_relationship_type::Dragger] != -1)
                continue;
            if (cur->relationship_ids[df::unit_relationship_type::Draggee] != -1)
                continue;
            tickets.push_back(i);
            for (int j = 0; j < 5; j++)
                tickets.push_back(i);
            switch (cur->profession)
            {
            case profession::WOODWORKER:
            case profession::CARPENTER:
            case profession::BOWYER:
            case profession::STONEWORKER:
            case profession::MASON:
                for (int j = 0; j < 5; j++)
                    tickets.push_back(i);
                break;
            case profession::METALSMITH:
            case profession::WEAPONSMITH:
            case profession::ARMORER:
            case profession::BLACKSMITH:
            case profession::METALCRAFTER:
            case profession::JEWELER:
            case profession::GEM_CUTTER:
            case profession::GEM_SETTER:
            case profession::CRAFTSMAN:
            case profession::WOODCRAFTER:
            case profession::STONECRAFTER:
            case profession::LEATHERWORKER:
            case profession::BONE_CARVER:
            case profession::WEAVER:
            case profession::CLOTHIER:
            case profession::GLASSMAKER:
                for (int j = 0; j < 15; j++)
                    tickets.push_back(i);
                break;
            default:
                break;
            }
        }
        if (!tickets.size())
        {
            out.printerr("No units are eligible to enter a mood!\n");
            return CR_FAILURE;
        }
        unit = moodable_units[tickets[rng.df_trandom(tickets.size())]];
    }
    df::unit_soul *soul = unit->status.current_soul;

    // Cancel selected unit's current job
    if (unit->job.current_job)
    {
        // TODO: cancel job
        out.printerr("Chosen unit '%s' has active job, cannot start mood!\n", Translation::TranslateName(&unit->name, false).c_str());
        return CR_FAILURE;
    }

    ui->mood_cooldown = 1000;
    // If no mood type was specified, pick one randomly
    if (type == mood_type::None)
    {
        if (soul && (
            (soul->personality.stress_level >= 500000) ||
            (soul->personality.stress_level >= 250000 && !rng.df_trandom(2)) ||
            (soul->personality.stress_level >= 100000 && !rng.df_trandom(10))
            ))
        {
            switch (rng.df_trandom(2))
            {
            case 0: type = mood_type::Fell;    break;
            case 1: type = mood_type::Macabre; break;
            }
        }
        else
        {
            switch (rng.df_trandom(3))
            {
            case 0: type = mood_type::Fey;         break;
            case 1: type = mood_type::Secretive;   break;
            case 2: type = mood_type::Possessed;   break;
            }
        }
    }

    // Display announcement and start setting up the mood job
    int color = 0;
    bool bright = false;
    string msg = Translation::TranslateName(&unit->name, false) + ", " + Units::getProfessionName(unit);

    switch (type)
    {
    case mood_type::Fey:
        color = 7;
        bright = true;
        msg += " is taken by a fey mood!";
        break;
    case mood_type::Secretive:
        color = 7;
        bright = false;
        msg += " withdraws from society...";
        break;
    case mood_type::Possessed:
        color = 5;
        bright = true;
        msg += " has been possessed!";
        break;
    case mood_type::Macabre:
        color = 0;
        bright = true;
        msg += " begins to stalk and brood...";
        break;
    case mood_type::Fell:
        color = 5;
        bright = false;
        msg += " looses a roaring laughter, fell and terrible!";
        break;
    default:
        out.printerr("Invalid mood type selected?\n");
        return CR_FAILURE;
    }

    unit->mood = type;
    unit->mood_copy = unit->mood;
    Gui::showAutoAnnouncement(announcement_type::STRANGE_MOOD, unit->pos, msg, color, bright);

    // TODO: make sure unit drops any wrestle items
    unit->job.mood_timeout = 50000;
    unit->flags1.bits.has_mood = true;
    unit->flags1.bits.had_mood = true;
    if (skill == job_skill::NONE)
        skill = getMoodSkill(unit);

    unit->job.mood_skill = skill;
    df::job *job = new df::job();
    Job::linkIntoWorld(job);

    // Choose the job type
    if (unit->mood == mood_type::Fell)
        job->job_type = job_type::StrangeMoodFell;
    else if (unit->mood == mood_type::Macabre)
        job->job_type = job_type::StrangeMoodBrooding;
    else
    {
        switch (skill)
        {
        case job_skill::MINING:
        case job_skill::MASONRY:
            job->job_type = job_type::StrangeMoodMason;
            break;
        case job_skill::CARPENTRY:
            job->job_type = job_type::StrangeMoodCarpenter;
            break;
        case job_skill::DETAILSTONE:
        case job_skill::WOODCRAFT:
        case job_skill::STONECRAFT:
        case job_skill::BONECARVE:
        case job_skill::PAPERMAKING:    // These aren't actually moodable skills
        case job_skill::BOOKBINDING:    // but the game still checks for them anyways
            job->job_type = job_type::StrangeMoodCrafter;
            break;
        case job_skill::TANNER:
        case job_skill::LEATHERWORK:
            job->job_type = job_type::StrangeMoodTanner;
            break;
        case job_skill::WEAVING:
        case job_skill::CLOTHESMAKING:
            job->job_type = job_type::StrangeMoodWeaver;
            break;
        case job_skill::FORGE_WEAPON:
        case job_skill::FORGE_ARMOR:
        case job_skill::FORGE_FURNITURE:
        case job_skill::METALCRAFT:
            job->job_type = job_type::StrangeMoodForge;
            break;
        case job_skill::CUTGEM:
        case job_skill::ENCRUSTGEM:
            job->job_type = job_type::StrangeMoodJeweller;
            break;
        case job_skill::GLASSMAKER:
            job->job_type = job_type::StrangeMoodGlassmaker;
            break;
        case job_skill::BOWYER:
            job->job_type = job_type::StrangeMoodBowyer;
            break;
        case job_skill::MECHANICS:
            job->job_type = job_type::StrangeMoodMechanics;
            break;
        default:
            break;
        }
    }
    // Check which types of glass are available - we'll need this information later
    bool have_glass[3] = {false, false, false};
    for (size_t i = 0; i < created_item_type->size(); i++)
    {
        if (created_item_type->at(i) == item_type::ROUGH)
        {
            switch (created_item_mattype->at(i))
            {
            case builtin_mats::GLASS_GREEN:
                have_glass[0] = true;
                break;
            case builtin_mats::GLASS_CLEAR:
                have_glass[1] = true;
                break;
            case builtin_mats::GLASS_CRYSTAL:
                have_glass[2] = true;
                break;
            }
        }
    }

    // The dwarf will want 1-3 of the base material
    int base_item_count = 1 + rng.df_trandom(3);
    // Gem Cutters and Gem Setters have a 50% chance of using only one base item
    if (((skill == job_skill::CUTGEM) || (skill == job_skill::ENCRUSTGEM)) && (rng.df_trandom(2)))
        base_item_count = 1;

    // Choose the base material
    df::job_item *item;
    if (job->job_type == job_type::StrangeMoodFell)
    {
        job->job_items.push_back(item = new df::job_item());
        item->item_type = item_type::CORPSE;
        item->flags1.bits.allow_buryable = true;
        item->flags1.bits.murdered = true;
        item->quantity = 1;
        item->vector_id = job_item_vector_id::ANY_MURDERED;
    }
    else if (job->job_type == job_type::StrangeMoodBrooding)
    {
        switch (rng.df_trandom(3))
        {
        case 0:
            job->job_items.push_back(item = new df::job_item());
            item->item_type = item_type::REMAINS;
            item->flags1.bits.allow_buryable = true;
            item->quantity = 1;
            break;
        case 1:
            job->job_items.push_back(item = new df::job_item());
            item->flags1.bits.allow_buryable = true;
            item->flags2.bits.bone = true;
            item->flags2.bits.body_part = true;
            item->quantity = 1;
            break;
        case 2:
            job->job_items.push_back(item = new df::job_item());
            item->flags1.bits.allow_buryable = true;
            item->flags2.bits.totemable = true;
            item->flags2.bits.body_part = true;
            item->quantity = base_item_count;
            break;
        }
    }
    else
    {
        df::item *filter;
        bool found_pref;
        switch (skill)
        {
        case job_skill::MINING:
        case job_skill::DETAILSTONE:
        case job_skill::MASONRY:
        case job_skill::STONECRAFT:
        case job_skill::MECHANICS:
            job->job_items.push_back(item = new df::job_item());
            item->item_type = item_type::BOULDER;
            item->quantity = base_item_count;
            item->flags3.bits.hard = true;
            found_pref = false;
            if (soul)
            {
                for (size_t i = 0; i < soul->preferences.size(); i++)
                {
                    df::unit_preference *pref = soul->preferences[i];
                    if (pref->active == 1 &&
                        pref->type == df::unit_preference::T_type::LikeMaterial &&
                        pref->mattype == builtin_mats::INORGANIC)
                    {
                        item->mat_type = pref->mattype;
                        found_pref = true;
                        break;
                    }
                }
            }
            if (!found_pref)
                item->mat_type = builtin_mats::INORGANIC;
            break;

        case job_skill::CARPENTRY:
        case job_skill::WOODCRAFT:
        case job_skill::BOWYER:
        case job_skill::PAPERMAKING:
        case job_skill::BOOKBINDING:
            job->job_items.push_back(item = new df::job_item());
            item->item_type = item_type::WOOD;
            item->quantity = base_item_count;
            break;

        case job_skill::TANNER:
        case job_skill::LEATHERWORK:
            job->job_items.push_back(item = new df::job_item());
            item->item_type = item_type::SKIN_TANNED;
            item->quantity = base_item_count;
            break;

        case job_skill::WEAVING:
        case job_skill::CLOTHESMAKING:
            filter = NULL;
            // TODO: do proper search through world->items.other[items_other_id::ANY_GENERIC36] for item_type CLOTH, mat_type 0, flags2.deep_material, and min_dimension 10000
            for (size_t i = 0; i < world->items.other[items_other_id::ANY_GENERIC36].size(); i++)
            {
                filter = world->items.other[items_other_id::ANY_GENERIC36][i];
                if (filter->getType() != item_type::CLOTH)
                {
                    filter = NULL;
                    continue;
                }
                if (filter->getMaterial() != 0)
                {
                    filter = NULL;
                    continue;
                }
                if (filter->getTotalDimension() < 10000)
                {
                    filter = NULL;
                    continue;
                }
                MaterialInfo mat(filter->getMaterial(), filter->getMaterialIndex());
                if (!mat.inorganic->flags.is_set(inorganic_flags::DEEP_SPECIAL))
                {
                    filter = NULL;
                    continue;
                }
                break;
            }
            if (filter)
            {
                job->job_items.push_back(item = new df::job_item());
                item->item_type = item_type::CLOTH;
                item->mat_type = filter->getMaterial();
                item->mat_index = filter->getMaterialIndex();
                item->quantity = base_item_count * 10000;
                item->min_dimension = 10000;
            }
            else
            {
                job->job_items.push_back(item = new df::job_item());
                item->item_type = item_type::CLOTH;
                bool found_pref = false;
                if (soul)
                {
                    for (size_t i = 0; i < soul->preferences.size(); i++)
                    {
                        df::unit_preference *pref = soul->preferences[i];
                        if (pref->active == 1 &&
                            pref->type == df::unit_preference::T_type::LikeMaterial)
                        {
                            MaterialInfo mat(pref->mattype, pref->matindex);
                            if (mat.material->flags.is_set(material_flags::SILK))
                                item->flags2.bits.silk = true;
                            else if (mat.material->flags.is_set(material_flags::THREAD_PLANT))
                                item->flags2.bits.plant = true;
                            else if (mat.material->flags.is_set(material_flags::YARN))
                                item->flags2.bits.yarn = true;
                            else
                                continue;
                            found_pref = true;
                            break;
                        }
                    }
                }
                if (!found_pref)
                {
                    switch (rng.df_trandom(3))
                    {
                    case 0:
                        item->flags2.bits.silk = true;
                        break;
                    case 1:
                        item->flags2.bits.plant = true;
                        break;
                    case 2:
                        item->flags2.bits.yarn = true;
                        break;
                    }
                }
                item->quantity = base_item_count * 10000;
                item->min_dimension = 10000;
            }
            break;

        case job_skill::FORGE_WEAPON:
        case job_skill::FORGE_ARMOR:
            // there are actually 2 distinct cases here, but they're identical
        case job_skill::FORGE_FURNITURE:
        case job_skill::METALCRAFT:
            filter = NULL;
            // TODO: do proper search through world->items.other[items_other_id::ANY_GENERIC36] for item_type BAR, mat_type 0, and flags2.deep_material
            for (size_t i = 0; i < world->items.other[items_other_id::ANY_GENERIC36].size(); i++)
            {
                filter = world->items.other[items_other_id::ANY_GENERIC36][i];
                if (filter->getType() != item_type::BAR)
                {
                    filter = NULL;
                    continue;
                }
                if (filter->getMaterial() != 0)
                {
                    filter = NULL;
                    continue;
                }
                MaterialInfo mat(filter->getMaterial(), filter->getMaterialIndex());
                if (!mat.inorganic->flags.is_set(inorganic_flags::DEEP_SPECIAL))
                {
                    filter = NULL;
                    continue;
                }
                break;
            }
            if (filter)
            {
                job->job_items.push_back(item = new df::job_item());
                item->item_type = item_type::BAR;
                item->mat_type = filter->getMaterial();
                item->mat_index = filter->getMaterialIndex();
                item->quantity = base_item_count * 150; // BUGFIX - the game does not adjust here!
                item->min_dimension = 150;
            }
            else
            {
                job->job_items.push_back(item = new df::job_item());
                item->item_type = item_type::BAR;
                item->mat_type = 0;
                vector<int32_t> mats;
                if (soul)
                {
                    for (size_t i = 0; i < soul->preferences.size(); i++)
                    {
                        df::unit_preference *pref = soul->preferences[i];
                        if (pref->active == 1 &&
                            pref->type == df::unit_preference::T_type::LikeMaterial &&
                            pref->mattype == 0 && getCreatedMetalBars(pref->matindex) > 0)
                            mats.push_back(pref->matindex);
                    }
                }
                if (mats.size())
                    item->mat_index = mats[rng.df_trandom(mats.size())];
                item->quantity = base_item_count * 150; // BUGFIX - the game does not adjust here!
                item->min_dimension = 150;
            }
            break;

        case job_skill::CUTGEM:
        case job_skill::ENCRUSTGEM:
            job->job_items.push_back(item = new df::job_item());
            item->item_type = item_type::ROUGH;
            item->mat_type = 0;
            item->quantity = base_item_count;
            break;

        case job_skill::GLASSMAKER:
            job->job_items.push_back(item = new df::job_item());
            item->item_type = item_type::ROUGH;
            found_pref = false;
            if (soul)
            {
                for (size_t i = 0; i < soul->preferences.size(); i++)
                {
                    df::unit_preference *pref = soul->preferences[i];
                    if (pref->active == 1 &&
                        pref->type == df::unit_preference::T_type::LikeMaterial &&
                        ((pref->mattype == builtin_mats::GLASS_GREEN) ||
                         (pref->mattype == builtin_mats::GLASS_CLEAR && have_glass[1]) ||
                         (pref->mattype == builtin_mats::GLASS_CRYSTAL && have_glass[2])))
                    {
                        item->mat_type = pref->mattype;
                        item->mat_index = pref->matindex;
                        found_pref = true;
                    }
                }
            }
            if (!found_pref)
            {
                vector<int32_t> mats;
                mats.push_back(builtin_mats::GLASS_GREEN);
                if (have_glass[1])
                    mats.push_back(builtin_mats::GLASS_CLEAR);
                if (have_glass[2])
                    mats.push_back(builtin_mats::GLASS_CRYSTAL);
                item->mat_type = mats[rng.df_trandom(mats.size())];
            }
            item->quantity = base_item_count;
            break;

        case job_skill::BONECARVE:
            found_pref = false;
            if (soul)
            {
                for (size_t i = 0; i < soul->preferences.size(); i++)
                {
                    df::unit_preference *pref = soul->preferences[i];
                    if (pref->active == 1 &&
                        pref->type == df::unit_preference::T_type::LikeMaterial)
                    {
                        MaterialInfo mat(pref->mattype, pref->matindex);
                        if (mat.material->flags.is_set(material_flags::BONE))
                        {
                            job->job_items.push_back(item = new df::job_item());
                            item->flags2.bits.bone = true;
                            item->flags2.bits.body_part = true;
                            found_pref = true;
                            break;
                        }
                        else if (mat.material->flags.is_set(material_flags::SHELL))
                        {
                            job->job_items.push_back(item = new df::job_item());
                            item->flags2.bits.shell = true;
                            item->flags2.bits.body_part = true;
                            found_pref = true;
                            break;
                        }
                    }
                }
            }
            if (!found_pref)
            {
                job->job_items.push_back(item = new df::job_item());
                item->flags2.bits.bone = true;
                item->flags2.bits.body_part = true;
                found_pref = true;
            }
            item->quantity = base_item_count;
            break;
        default:
            break;
        }
    }

    // Choose additional mood materials
    // Gem cutters/setters using a single gem require nothing else, and fell moods need only their corpse
    if (!(
         (((skill == job_skill::CUTGEM) || (skill == job_skill::ENCRUSTGEM)) && base_item_count == 1) ||
         (job->job_type == job_type::StrangeMoodFell)
       ))
    {
        int extra_items = std::min(rng.df_trandom((ui->tasks.num_artifacts * 20 + moodable_units.size()) / 20 + 1), 7);
        df::item_type avoid_type = item_type::NONE;
        int avoid_glass = 0;
        switch (skill)
        {
        case job_skill::MINING:
        case job_skill::DETAILSTONE:
        case job_skill::MASONRY:
        case job_skill::STONECRAFT:
            avoid_type = item_type::BLOCKS;
            break;
        case job_skill::CARPENTRY:
        case job_skill::WOODCRAFT:
        case job_skill::BOWYER:
            avoid_type = item_type::WOOD;
            break;
        case job_skill::TANNER:
        case job_skill::LEATHERWORK:
            avoid_type = item_type::SKIN_TANNED;
            break;
        case job_skill::WEAVING:
        case job_skill::CLOTHESMAKING:
            avoid_type = item_type::CLOTH;
            break;
        case job_skill::FORGE_WEAPON:
        case job_skill::FORGE_ARMOR:
        case job_skill::FORGE_FURNITURE:
        case job_skill::METALCRAFT:
            avoid_type = item_type::BAR;
            break;
        case job_skill::CUTGEM:
        case job_skill::ENCRUSTGEM:
            avoid_type = item_type::SMALLGEM;
        case job_skill::GLASSMAKER:
            avoid_glass = 1;
            break;
        default:
            break;
        }
        for (int i = 0; i < extra_items; i++)
        {
            if ((job->job_type == job_type::StrangeMoodBrooding) && (rng.df_trandom(2)))
            {
                switch (rng.df_trandom(2))
                {
                case 0:
                    job->job_items.push_back(item = new df::job_item());
                    item->item_type = item_type::REMAINS;
                    item->flags1.bits.allow_buryable = true;
                    item->quantity = 1;
                    break;
                case 1:
                    job->job_items.push_back(item = new df::job_item());
                    item->flags1.bits.allow_buryable = true;
                    item->flags2.bits.bone = true;
                    item->flags2.bits.body_part = true;
                    item->quantity = 1;
                    break;
                }
            }
            else
            {
                df::item_type item_type;
                int16_t mat_type;
                df::job_item_flags2 flags2;
                do
                {
                    item_type = item_type::NONE;
                    mat_type = -1;
                    flags2.whole = 0;
                    switch (rng.df_trandom(10))
                    {
                    case 0:
                        item_type = item_type::WOOD;
                        mat_type = -1;
                        break;
                    case 1:
                        item_type = item_type::BAR;
                        mat_type = builtin_mats::INORGANIC;
                        break;
                    case 2:
                        item_type = item_type::SMALLGEM;
                        mat_type = -1;
                        break;
                    case 3:
                        item_type = item_type::BLOCKS;
                        mat_type = builtin_mats::INORGANIC;
                        break;
                    case 4:
                        item_type = item_type::ROUGH;
                        mat_type = builtin_mats::INORGANIC;
                        break;
                    case 5:
                        item_type = item_type::BOULDER;
                        mat_type = builtin_mats::INORGANIC;
                        break;
                    case 6:
                        flags2.bits.bone = true;
                        flags2.bits.body_part = true;
                        break;
                    case 7:
                        item_type = item_type::SKIN_TANNED;
                        mat_type = -1;
                        break;
                    case 8:
                        item_type = item_type::CLOTH;
                        mat_type = -1;
                        switch (rng.df_trandom(3))
                        {
                        case 0:
                            flags2.bits.plant = true;
                            break;
                        case 1:
                            flags2.bits.silk = true;
                            break;
                        case 2:
                            flags2.bits.yarn = true;
                            break;
                        }
                        break;
                    case 9:
                        item_type = item_type::ROUGH;
                        switch (rng.df_trandom(3))
                        {
                        case 0:
                            mat_type = builtin_mats::GLASS_GREEN;
                            break;
                        case 1:
                            mat_type = builtin_mats::GLASS_CLEAR;
                            break;
                        case 2:
                            mat_type = builtin_mats::GLASS_CRYSTAL;
                            break;
                        }
                        break;
                    }
                    item = job->job_items[0];
                    if (item->item_type == item_type && item->mat_type == mat_type)
                        continue;
                    if (item_type == avoid_type)
                        continue;
                    if (avoid_glass && ((mat_type == builtin_mats::GLASS_GREEN) || (mat_type == builtin_mats::GLASS_CLEAR) || (mat_type == builtin_mats::GLASS_CRYSTAL)))
                        continue;
                    if ((mat_type == builtin_mats::GLASS_GREEN) && !have_glass[0])
                        continue;
                    if ((mat_type == builtin_mats::GLASS_CLEAR) && !have_glass[1])
                        continue;
                    if ((mat_type == builtin_mats::GLASS_CRYSTAL) && !have_glass[2])
                        continue;
                    break;
                } while (1);
                job->job_items.push_back(item = new df::job_item());
                item->item_type = item_type;
                item->mat_type = mat_type;
                item->flags2.whole = flags2.whole;
                item->quantity = 1;
                if (item_type == item_type::BAR)
                {
                    item->quantity *= 150; // BUGFIX - the game does not adjust here!
                    item->min_dimension = 150;
                }
                if (item_type == item_type::CLOTH)
                {
                    item->quantity *= 10000; // BUGFIX - the game does not adjust here!
                    item->min_dimension = 10000;
                }
            }
        }
    }

    // Attach the Strange Mood job to the dwarf
    unit->path.dest.x = -30000;
    unit->path.dest.y = -30000;
    unit->path.dest.z = -30000;
    unit->path.goal = unit_path_goal::None;
    unit->path.path.x.clear();
    unit->path.path.y.clear();
    unit->path.path.z.clear();
    job->flags.bits.special = true;
    df::general_ref *ref = df::allocate<df::general_ref_unit_workerst>();
    ref->setID(unit->id);
    job->general_refs.push_back(ref);
    unit->job.current_job = job;
    job->wait_timer = 0;

    // Generate the artifact's name
    if (type == mood_type::Fell || type == mood_type::Macabre)
        generateName(unit->status.artifact_name, unit->name.language, 1, world->raws.language.word_table[0][2], world->raws.language.word_table[1][2]);
    else
    {
        generateName(unit->status.artifact_name, unit->name.language, 1, world->raws.language.word_table[0][1], world->raws.language.word_table[1][1]);
        if (!rng.df_trandom(100))
            unit->status.artifact_name = unit->name;
    }
    unit->unk_18e = 0;
    return CR_OK;
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand("strangemood", "Force a strange mood to happen.", df_strangemood, false,
        "Options:\n"
         "  -force         - Ignore standard mood preconditions.\n"
         "  -unit          - Use the selected unit instead of picking one randomly.\n"
         "  -type <type>   - Force the mood to be of a specific type.\n"
         "                   Valid types: fey, secretive, possessed, fell, macabre\n"
         "  -skill <skill> - Force the mood to use a specific skill.\n"
         "                   Skill name must be lowercase and without spaces.\n"
         "                   Example: miner, gemcutter, metalcrafter, bonecarver, mason\n"
    ));
    rng.init();

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
