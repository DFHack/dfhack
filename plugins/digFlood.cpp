#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"

#include "df/coord.h"
#include "df/global_objects.h"
#include "df/job.h"
#include "df/map_block.h"
#include "df/tile_dig_designation.h"
#include "df/world.h"

#include <set>
#include <string>
#include <vector>

using namespace DFHack;
using namespace std;

DFHACK_PLUGIN("digFlood");
REQUIRE_GLOBAL(world);

command_result digFlood (color_ostream &out, std::vector <std::string> & parameters);

void onDig(color_ostream& out, void* ptr);
void maybeExplore(color_ostream& out, MapExtras::MapCache& cache, df::coord pt, set<df::coord>& jobLocations);
EventManager::EventHandler digHandler(plugin_self, onDig, 0);

//bool enabled = false;
DFHACK_PLUGIN_IS_ENABLED(enabled);
bool digAll = false;
set<string> autodigMaterials;

DFhackCExport command_result plugin_enable(color_ostream& out, bool enable) {
    if (enabled == enable)
        return CR_OK;

    enabled = enable;
    if ( enabled ) {
        EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, digHandler, plugin_self);
    } else {
        EventManager::unregisterAll(plugin_self);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "digFlood",
        "Automatically dig out veins as you discover them.",
        digFlood));
    return CR_OK;
}

void onDig(color_ostream& out, void* ptr) {
    df::job* job = (df::job*)ptr;
    if ( job->completion_timer > 0 )
        return;

    if ( job->job_type != df::enums::job_type::Dig &&
         job->job_type != df::enums::job_type::CarveUpwardStaircase &&
         job->job_type != df::enums::job_type::CarveDownwardStaircase &&
         job->job_type != df::enums::job_type::CarveUpDownStaircase &&
         job->job_type != df::enums::job_type::CarveRamp &&
         job->job_type != df::enums::job_type::DigChannel )
        return;

    set<df::coord> jobLocations;
    for ( df::job_list_link* link = &world->jobs.list; link != NULL; link = link->next ) {
        if ( link->item == NULL )
            continue;

        if ( link->item->job_type != df::enums::job_type::Dig &&
             link->item->job_type != df::enums::job_type::CarveUpwardStaircase &&
             link->item->job_type != df::enums::job_type::CarveDownwardStaircase &&
             link->item->job_type != df::enums::job_type::CarveUpDownStaircase &&
             link->item->job_type != df::enums::job_type::CarveRamp &&
             link->item->job_type != df::enums::job_type::DigChannel )
            continue;

        jobLocations.insert(link->item->pos);
    }

    MapExtras::MapCache cache;
    df::coord pos = job->pos;
    for ( int16_t a = -1; a <= 1; a++ ) {
        for ( int16_t b = -1; b <= 1; b++ ) {
            maybeExplore(out, cache, df::coord(pos.x+a,pos.y+b,pos.z), jobLocations);
        }
    }
    cache.trash();
}

void maybeExplore(color_ostream& out, MapExtras::MapCache& cache, df::coord pt, set<df::coord>& jobLocations) {
    if ( !Maps::isValidTilePos(pt) ) {
        return;
    }

    df::map_block* block = Maps::getTileBlock(pt);
    if (!block)
        return;

    if ( block->designation[pt.x&0xF][pt.y&0xF].bits.hidden )
        return;

    df::tiletype type = block->tiletype[pt.x&0xF][pt.y&0xF];
    if ( ENUM_ATTR(tiletype, material, type) != df::enums::tiletype_material::MINERAL )
        return;
    if ( ENUM_ATTR(tiletype, shape, type) != df::enums::tiletype_shape::WALL )
        return;

    if ( block->designation[pt.x&0xF][pt.y&0xF].bits.dig != df::enums::tile_dig_designation::No )
        return;

    uint32_t xMax,yMax,zMax;
    Maps::getSize(xMax,yMax,zMax);
    if ( pt.x == 0 || pt.y == 0 || pt.x+1 == int32_t(xMax)*16 || pt.y+1 == int32_t(yMax)*16 )
        return;
    if ( jobLocations.find(pt) != jobLocations.end() ) {
        return;
    }

    int16_t mat = cache.veinMaterialAt(pt);
    if ( mat == -1 )
        return;
    if ( !digAll ) {
        df::inorganic_raw* inorganic = world->raws.inorganics[mat];
        if ( autodigMaterials.find(inorganic->id) == autodigMaterials.end() ) {
            return;
        }
    }

    block->designation[pt.x&0xF][pt.y&0xF].bits.dig = df::enums::tile_dig_designation::Default;
    block->flags.bits.designated = true;
//    *process_dig  = true;
//    *process_jobs = true;
}

command_result digFlood (color_ostream &out, std::vector <std::string> & parameters)
{
    bool adding = true;
    set<string> toAdd, toRemove;
    for ( size_t a = 0; a < parameters.size(); a++ ) {
        int32_t i = (int32_t)strtol(parameters[a].c_str(), NULL, 0);
        if ( i == 0 && parameters[a] == "0" ) {
            plugin_enable(out, false);
            adding = false;
            continue;
        } else if ( i == 1 ) {
            plugin_enable(out, true);
            adding = true;
            continue;
        }

        if ( parameters[a] == "CLEAR" ) {
            autodigMaterials.clear();
            continue;
        }

        if ( parameters[a] == "digAll0" ) {
            digAll = false;
            continue;
        }
        if ( parameters[a] == "digAll1" ) {
            digAll = true;
            continue;
        }

        for ( size_t b = 0; b < world->raws.inorganics.size(); b++ ) {
            df::inorganic_raw* inorganic = world->raws.inorganics[b];
            if ( parameters[a] == inorganic->id ) {
                if ( adding )
                    toAdd.insert(parameters[a]);
                else
                    toRemove.insert(parameters[a]);
                goto loop;
            }
        }

        out.print("Could not find material \"%s\".\n", parameters[a].c_str());
        return CR_WRONG_USAGE;

        loop: continue;
    }

    autodigMaterials.insert(toAdd.begin(), toAdd.end());
    for ( auto a = toRemove.begin(); a != toRemove.end(); a++ )
        autodigMaterials.erase(*a);

    return CR_OK;
}
