
#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/EventManager.h"
#include "modules/Units.h"
#include "modules/Maps.h"

#include "df/caste_raw.h"
#include "df/caste_raw_flags.h"
#include "df/creature_raw.h"
#include "df/profession.h"
#include "df/unit.h"
#include "df/world.h"

#include <map>
#include <vector>

using namespace DFHack;
using namespace std;

DFHACK_PLUGIN("petcapRemover");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

static int32_t howOften = 10000;
static int32_t popcap = 100;
static int32_t pregtime = 200000;

command_result petcapRemover (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "petcapRemover",
        "Modify the pet population cap.",
        petcapRemover));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

bool impregnate(df::unit* female, df::unit* male);
void impregnateMany() {
    map<int32_t, vector<int32_t> > males;
    map<int32_t, vector<int32_t> > females;
    map<int32_t, int32_t> popcount;
    auto units = world->units.all;
    for ( size_t a = 0; a < units.size(); a++ ) {
        df::unit* unit = units[a];
        if ( !Units::isActive(unit) || unit->flags1.bits.active_invader || unit->flags2.bits.underworld || unit->flags2.bits.visitor_uninvited || unit->flags2.bits.visitor )
            continue;
        popcount[unit->race]++;
        if ( unit->pregnancy_genes ) {
            //already pregnant
            //for player convenience and population stability, count the fetus toward the population cap
            popcount[unit->race]++;
            continue;
        }
        if ( unit->flags1.bits.caged )
            continue;
        //must have PET or PET_EXOTIC
        if ( !Units::isTamable(unit))
            continue;
        //check for adulthood
        if ( Units::isBaby(unit) || Units::isChild(unit))
            continue;
        if ( Units::isMale(unit))
            males[unit->race].push_back(a);
        else
            females[unit->race].push_back(a);
    }

    for ( auto i = females.begin(); i != females.end(); i++ ) {
        int32_t race = i->first;
        vector<int32_t>& femalesList = i->second;
        for ( size_t a = 0; a < femalesList.size(); a++ ) {
            if ( popcap > 0 && popcount[race] >= popcap )
                break;
            vector<int32_t> compatibles;
            df::coord pos1 = units[femalesList[a]]->pos;

            if ( males.find(i->first) == males.end() )
                continue;

            vector<int32_t>& malesList = males[i->first];
            for ( size_t b = 0; b < malesList.size(); b++ ) {
                df::coord pos2 = units[malesList[b]]->pos;
                if ( Maps::canWalkBetween(pos1,pos2) )
                    compatibles.push_back(malesList[b]);
            }
            if ( compatibles.empty() )
                continue;

            size_t maleIndex = (size_t)(compatibles.size()*((float)rand() / (1+(float)RAND_MAX)));
            if ( impregnate(units[femalesList[a]], units[compatibles[maleIndex]]) )
                popcount[race]++;
        }
    }
}

bool impregnate(df::unit* female, df::unit* male) {
    if ( !female || !male )
        return false;
    if ( female->pregnancy_genes )
        return false;

    df::unit_genes* preg = new df::unit_genes;
    *preg = male->appearance.genes;
    female->pregnancy_genes = preg;
    female->pregnancy_timer = pregtime; //300000 for dwarves
    female->pregnancy_caste = male->caste;
    return true;
}

void tickHandler(color_ostream& out, void* data) {
    if ( !is_enabled )
        return;
    CoreSuspender suspend;
    impregnateMany();

    EventManager::unregisterAll(plugin_self);
    EventManager::EventHandler handle(tickHandler, howOften);
    EventManager::registerTick(handle, howOften, plugin_self);
}

command_result petcapRemover (color_ostream &out, std::vector <std::string> & parameters)
{
    CoreSuspender suspend;

    for ( size_t a = 0; a < parameters.size(); a++ ) {
        if ( parameters[a] == "every" ) {
            if ( a+1 >= parameters.size() )
                return CR_WRONG_USAGE;
            int32_t old = howOften;
            howOften = atoi(parameters[a+1].c_str());
            if (howOften < -1) {
                howOften = old;
                return CR_WRONG_USAGE;
            }
            a++;
            continue;
        } else if ( parameters[a] == "cap" ) {
            if ( a+1 >= parameters.size() )
                return CR_WRONG_USAGE;
            int32_t old = popcap;
            popcap = atoi(parameters[a+1].c_str());
            if ( popcap < 0 ) {
                popcap = old;
                return CR_WRONG_USAGE;
            }
            a++;
            continue;
        } else if ( parameters[a] == "pregtime" ) {
            if ( a+1 >= parameters.size() )
                return CR_WRONG_USAGE;
            int32_t old = pregtime;
            pregtime = atoi(parameters[a+1].c_str());
            if ( pregtime <= 0 ) {
                pregtime = old;
                return CR_WRONG_USAGE;
            }
            a++;
            continue;
        }
        out.print("%s, line %d: invalid argument: %s\n", __FILE__, __LINE__, parameters[a].c_str());
        return CR_WRONG_USAGE;
    }

    if ( howOften < 0 ) {
        is_enabled = false;
        return CR_OK;
    }

    is_enabled = true;
    EventManager::unregisterAll(plugin_self);
    EventManager::EventHandler handle(tickHandler, howOften);
    EventManager::registerTick(handle, howOften, plugin_self);
    out.print("petcapRemover: howOften = every %d ticks, popcap per species = %d, preg time = %d ticks.\n", howOften, popcap, pregtime);

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (enable != is_enabled)
    {
        is_enabled = enable;
        if ( !is_enabled ) {
            EventManager::unregisterAll(plugin_self);
        } else {
            // start the cycle
            vector<string> params;
            return petcapRemover(out, params);
        }
    }

    return CR_OK;
}
