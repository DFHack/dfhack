// Show details of currently active strange mood, if any

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Materials.h"
#include "modules/Translation.h"
#include "modules/Items.h"
#include "modules/Units.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/job_item_ref.h"
#include "df/general_ref.h"
#include "df/builtin_mats.h"
#include "df/inorganic_raw.h"
#include "df/matter_state.h"
#include "df/unit.h"
#include "df/building.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("showmood");
REQUIRE_GLOBAL(world);

command_result df_showmood (color_ostream &out, vector <string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    if (!Translation::IsValid())
    {
        out.printerr("Translation data unavailable!\n");
        return CR_FAILURE;
    }

    CoreSuspender suspend;

    bool found = false;
    for (df::job_list_link *cur = world->job_list.next; cur != NULL; cur = cur->next)
    {
        df::job *job = cur->item;
        if ((job->job_type < job_type::StrangeMoodCrafter) || (job->job_type > job_type::StrangeMoodMechanics))
            continue;
        found = true;
        df::unit *unit = NULL;
        df::building *building = NULL;
        for (size_t i = 0; i < job->general_refs.size(); i++)
        {
            df::general_ref *ref = job->general_refs[i];
            if (ref->getType() == general_ref_type::UNIT_WORKER)
                unit = ref->getUnit();
            if (ref->getType() == general_ref_type::BUILDING_HOLDER)
                building = ref->getBuilding();
        }
        if (!unit)
        {
            out.printerr("Found strange mood not attached to any dwarf!\n");
            continue;
        }
        if (unit->mood == mood_type::None)
        {
            out.printerr("Dwarf with strange mood does not have a mood type!\n");
            continue;
        }
        out.print("%s is currently ", DF2CONSOLE(Translation::TranslateName(&unit->name, false)).c_str());
        switch (unit->mood)
        {
        case mood_type::Macabre:
            out.print("in a macabre mood");
            if (job->job_type != job_type::StrangeMoodBrooding)
                out.print(" (but isn't actually in a macabre mood?)");
            break;

        case mood_type::Fell:
            out.print("in a fell mood");
            if (job->job_type != job_type::StrangeMoodFell)
                out.print(" (but isn't actually in a fell mood?)");
            break;

        case mood_type::Fey:
        case mood_type::Secretive:
        case mood_type::Possessed:
            switch (unit->mood)
            {
            case mood_type::Fey:
                out.print("in a fey mood");
                break;
            case mood_type::Secretive:
                out.print("in a secretive mood");
                break;
            case mood_type::Possessed:
                out.print("possessed");
                break;
            }
            out.print(" with intent to ");
            switch (job->job_type)
            {
            case job_type::StrangeMoodCrafter:
                out.print("claim a Craftsdwarf's Workshop");
                break;
            case job_type::StrangeMoodJeweller:
                out.print("claim a Jeweler's Workshop");
                break;
            case job_type::StrangeMoodForge:
                out.print("claim a Metalsmith's Forge");
                break;
            case job_type::StrangeMoodMagmaForge:
                out.print("claim a Magma Forge");
                break;
            case job_type::StrangeMoodCarpenter:
                out.print("claim a Carpenter's Workshop");
                break;
            case job_type::StrangeMoodMason:
                out.print("claim a Mason's Workshop");
                break;
            case job_type::StrangeMoodBowyer:
                out.print("claim a Boywer's Workshop");
                break;
            case job_type::StrangeMoodTanner:
                out.print("claim a Leather Works");
                break;
            case job_type::StrangeMoodWeaver:
                out.print("claim a Clothier's Shop");
                break;
            case job_type::StrangeMoodGlassmaker:
                out.print("claim a Glass Furnace");
                break;
            case job_type::StrangeMoodMechanics:
                out.print("claim a Mechanic's Workshop");
                break;
            case job_type::StrangeMoodBrooding:
                out.print("enter a macabre mood?");
                break;
            case job_type::StrangeMoodFell:
                out.print("enter a fell mood?");
                break;
            default:
                out.print("do something else...");
                break;
            }
            out.print(" and become a legendary %s", ENUM_ATTR_STR(job_skill, caption_noun, unit->job.mood_skill));
            if (unit->mood == mood_type::Possessed)
                out.print(" (but not really)");
            break;
        default:
            out.print("insane?");
            break;
        }
        out.print(".\n");
        if (Units::isMale(unit))
            out.print("He has ");
        else
            out.print("She has ");
        if (building)
        {
            string name;
            building->getName(&name);
            out.print("claimed a %s and wants", name.c_str());
        }
        else
            out.print("not yet claimed a workshop but will want");
        out.print(" the following items:\n");

        for (size_t i = 0; i < job->job_items.size(); i++)
        {
            df::job_item *item = job->job_items[i];
            out.print("Item %i: ", i + 1);

            MaterialInfo matinfo(item->mat_type, item->mat_index);

            string mat_name = matinfo.toString();

            switch (item->item_type)
            {
            case item_type::BOULDER:
                out.print("%s boulder", mat_name.c_str());
                break;
            case item_type::BLOCKS:
                out.print("%s blocks", mat_name.c_str());
                break;
            case item_type::WOOD:
                out.print("%s logs", mat_name.c_str());
                break;
            case item_type::BAR:
                if (matinfo.isInorganicWildcard())
                    mat_name = "metal";
                if (matinfo.inorganic && matinfo.inorganic->flags.is_set(inorganic_flags::WAFERS))
                    out.print("%s wafers", mat_name.c_str());
                else
                    out.print("%s bars", mat_name.c_str());
                break;
            case item_type::SMALLGEM:
                out.print("%s cut gems", mat_name.c_str());
                break;
            case item_type::ROUGH:
                if (matinfo.isAnyInorganic())
                {
                    if (matinfo.isInorganicWildcard())
                        mat_name = "any";
                    out.print("%s rough gems", mat_name.c_str());
                }
                else
                    out.print("raw %s", mat_name.c_str());
                break;
            case item_type::SKIN_TANNED:
                out.print("%s leather", mat_name.c_str());
                break;
            case item_type::CLOTH:
                if (matinfo.isNone())
                {
                    if (item->flags2.bits.plant)
                        mat_name = "any plant fiber";
                    else if (item->flags2.bits.silk)
                        mat_name = "any silk";
                    else if (item->flags2.bits.yarn)
                        mat_name = "any yarn";
                }
                out.print("%s cloth", mat_name.c_str());
                break;
            case item_type::REMAINS:
                out.print("%s remains", mat_name.c_str());
                break;
            case item_type::CORPSE:
                out.print("%s %scorpse", mat_name.c_str(), (item->flags1.bits.murdered ? "murdered " : ""));
                break;
            case item_type::NONE:
                if (item->flags2.bits.body_part)
                {
                    if (item->flags2.bits.bone)
                        out.print("%s bones", mat_name.c_str());
                    else if (item->flags2.bits.shell)
                        out.print("%s shells", mat_name.c_str());
                    else if (item->flags2.bits.horn)
                        out.print("%s horns", mat_name.c_str());
                    else if (item->flags2.bits.pearl)
                        out.print("%s pearls", mat_name.c_str());
                    else if (item->flags2.bits.ivory_tooth)
                        out.print("%s ivory/teeth", mat_name.c_str());
                    else
                        out.print("%s unknown body parts (%s:%s:%s)",
                                     mat_name.c_str(),
                                     bitfield_to_string(item->flags1).c_str(),
                                     bitfield_to_string(item->flags2).c_str(),
                                     bitfield_to_string(item->flags3).c_str());
                }
                else
                    out.print("indeterminate %s item (%s:%s:%s)",
                                 mat_name.c_str(),
                                 bitfield_to_string(item->flags1).c_str(),
                                 bitfield_to_string(item->flags2).c_str(),
                                 bitfield_to_string(item->flags3).c_str());
                break;
            default:
                {
                    ItemTypeInfo itinfo(item->item_type, item->item_subtype);

                    out.print("item %s material %s flags (%s:%s:%s)",
                                 itinfo.toString().c_str(), mat_name.c_str(),
                                 bitfield_to_string(item->flags1).c_str(),
                                 bitfield_to_string(item->flags2).c_str(),
                                 bitfield_to_string(item->flags3).c_str());
                    break;
                }
            }

            // count how many items of this type the crafter already collected
            {
                int count_got = 0;
                for (size_t j = 0; j < job->items.size(); j++)
                {
                    if(job->items[j]->job_item_idx == i)
                    {
                        if (item->item_type == item_type::BAR || item->item_type == item_type::CLOTH)
                            count_got += job->items[j]->item->getTotalDimension();
                        else
                            count_got += 1;
                    }
                }
                out.print(", quantity %i (got %i)\n", item->quantity, count_got);
            }
        }
    }
    if (!found)
        out.print("No strange moods currently active.\n");

    return CR_OK;
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand("showmood", "Shows items needed for current strange mood.", df_showmood, false,
        "Run this command without any parameters to display information on the currently active Strange Mood."));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
