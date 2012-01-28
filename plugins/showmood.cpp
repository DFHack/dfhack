// Show details of currently active strange mood, if any

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Materials.h"
#include "modules/Translation.h"
#include "modules/Items.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/general_ref.h"
#include "df/builtin_mats.h"
#include "df/inorganic_raw.h"
#include "df/matter_state.h"
#include "df/unit.h"
#include "df/building.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace DFHack::Simple;
using namespace df::enums;

using df::global::world;

DFhackCExport command_result df_showmood (Core * c, vector <string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend(c);

    bool found = false;
    for (df::job_list_link *cur = world->job_list.next; cur != NULL; cur = cur->next)
    {
        df::job *job = cur->item;
        if ((job->job_type < job_type::StrangeMoodCrafter) || (job->job_type > job_type::StrangeMoodMechanics))
            continue;
        found = true;
        df::unit *unit = NULL;
        df::building *building = NULL;
        for (int i = 0; i < job->references.size(); i++)
        {
            df::general_ref *ref = job->references[i];
            if (ref->getType() == general_ref_type::UNIT_WORKER)
                unit = ref->getUnit();
            if (ref->getType() == general_ref_type::BUILDING_HOLDER)
                building = ref->getBuilding();
        }
        if (!unit)
        {
            c->con.printerr("Found strange mood not attached to any dwarf!\n");
            continue;
        }
        if (unit->mood == mood_type::None)
        {
            c->con.printerr("Dwarf with strange mood does not have a mood type!\n");
            continue;
        }
        c->con.print("%s is currently ", Translation::TranslateName(&unit->name, false).c_str());
        switch (unit->mood)
        {
        case mood_type::Macabre:
            c->con.print("in a macabre mood");
            if (job->job_type != job_type::StrangeMoodBrooding)
                c->con.print(" (but isn't actually in a macabre mood?)");
            break;

        case mood_type::Fell:
            c->con.print("in a fell mood");
            if (job->job_type != job_type::StrangeMoodFell)
                c->con.print(" (but isn't actually in a fell mood?)");
            break;

        case mood_type::Fey:
        case mood_type::Secretive:
        case mood_type::Possessed:
            switch (unit->mood)
            {
            case mood_type::Fey:
                c->con.print("in a fey mood");
                break;
            case mood_type::Secretive:
                c->con.print("in a secretive mood");
                break;
            case mood_type::Possessed:
                c->con.print("possessed");
                break;
            }
            c->con.print(" with intent to ");
            switch (job->job_type)
            {
            case job_type::StrangeMoodCrafter:
                c->con.print("become a Craftsdwarf (or Engraver)");
                break;
            case job_type::StrangeMoodJeweller:
                c->con.print("become a Jeweler");
                break;
            case job_type::StrangeMoodForge:
                c->con.print("become a Metalworker");
                break;
            case job_type::StrangeMoodMagmaForge:
                c->con.print("become a Metalworker using a Magma Forge");
                break;
            case job_type::StrangeMoodCarpenter:
                c->con.print("become a Carpenter");
                break;
            case job_type::StrangeMoodMason:
                c->con.print("become a Mason (or Miner)");
                break;
            case job_type::StrangeMoodBowyer:
                c->con.print("become a Bowyer");
                break;
            case job_type::StrangeMoodTanner:
                c->con.print("become a Leatherworker (or Tanner)");
                break;
            case job_type::StrangeMoodWeaver:
                c->con.print("become a Clothier (or Weaver)");
                break;
            case job_type::StrangeMoodGlassmaker:
                c->con.print("become a Glassmaker");
                break;
            case job_type::StrangeMoodMechanics:
                c->con.print("become a Mechanic");
                break;
            case job_type::StrangeMoodBrooding:
                c->con.print("enter a macabre mood?");
                break;
            case job_type::StrangeMoodFell:
                c->con.print("enter a fell mood?");
                break;
            }
            break;

        default:
            c->con.print("insane?");
            break;
        }
        if (building)
        {
            string name;
            building->getName(&name);
            c->con.print(" and has claimed a %s\n", name.c_str());
        }
        else
            c->con.print(" and has not yet claimed a workshop\n");

        for (int i = 0; i < job->job_items.size(); i++)
        {
            df::job_item *item = job->job_items[i];
            c->con.print("Item %i: ", i + 1);

            MaterialInfo matinfo(item->mat_type, item->mat_index);

            string mat_name = matinfo.toString();

            switch (item->item_type)
            {
            case item_type::BOULDER:
                c->con.print("%s boulder", mat_name.c_str());
                break;
            case item_type::BLOCKS:
                c->con.print("%s blocks", mat_name.c_str());
                break;
            case item_type::WOOD:
                c->con.print("%s logs", mat_name.c_str());
                break;
            case item_type::BAR:
                if (matinfo.isInorganicWildcard())
                    mat_name = "metal";
                if (matinfo.inorganic && matinfo.inorganic->flags.is_set(inorganic_flags::WAFERS))
                    c->con.print("%s wafers", mat_name.c_str());
                else
                    c->con.print("%s bars", mat_name.c_str());
                break;
            case item_type::SMALLGEM:
                c->con.print("%s cut gems", mat_name.c_str());
                break;
            case item_type::ROUGH:
                if (matinfo.isAnyInorganic())
                {
                    if (matinfo.isInorganicWildcard())
                        mat_name = "any";
                    c->con.print("%s rough gems", mat_name.c_str());
                }
                else
                    c->con.print("raw %s", mat_name.c_str());
                break;
            case item_type::SKIN_TANNED:
                c->con.print("%s leather", mat_name.c_str());
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
                c->con.print("%s cloth", mat_name.c_str());
                break;
            case item_type::REMAINS:
                c->con.print("%s remains", mat_name.c_str());
                break;
            case item_type::CORPSE:
                c->con.print("%s %scorpse", mat_name.c_str(), (item->flags1.bits.murdered ? "murdered " : ""));
                break;
            case item_type::NONE:
                if (item->flags2.bits.body_part)
                {
                    if (item->flags2.bits.bone)
                        c->con.print("%s bones", mat_name.c_str());
                    else if (item->flags2.bits.shell)
                        c->con.print("%s shells", mat_name.c_str());
                    else if (item->flags2.bits.horn)
                        c->con.print("%s horns", mat_name.c_str());
                    else if (item->flags2.bits.pearl)
                        c->con.print("%s pearls", mat_name.c_str());
                    else if (item->flags2.bits.ivory_tooth)
                        c->con.print("%s ivory/teeth", mat_name.c_str());
                    else
                        c->con.print("%s unknown body parts (%s:%s:%s)",
                                     mat_name.c_str(),
                                     bitfieldToString(item->flags1).c_str(),
                                     bitfieldToString(item->flags2).c_str(),
                                     bitfieldToString(item->flags3).c_str());
                }
                else
                    c->con.print("indeterminate %s item (%s:%s:%s)",
                                 mat_name.c_str(),
                                 bitfieldToString(item->flags1).c_str(),
                                 bitfieldToString(item->flags2).c_str(),
                                 bitfieldToString(item->flags3).c_str());
                break;
            default:
                {
                    ItemTypeInfo itinfo(item->item_type, item->item_subtype);

                    c->con.print("item %s material %s flags (%s:%s:%s)",
                                 itinfo.toString().c_str(), mat_name.c_str(),
                                 bitfieldToString(item->flags1).c_str(),
                                 bitfieldToString(item->flags2).c_str(),
                                 bitfieldToString(item->flags3).c_str());
                    break;
                }
            }

            c->con.print(", quantity %i\n", item->quantity);
        }
    }
    if (!found)
        c->con.print("No strange moods currently active.\n");

    return CR_OK;
}

DFhackCExport const char *plugin_name ( void )
{
    return "showmood";
}

DFhackCExport command_result plugin_init (Core *c, std::vector<PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("showmood", "Shows items needed for current strange mood.", df_showmood));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}