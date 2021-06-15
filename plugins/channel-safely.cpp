/* Prevent channeling down into known open space.
Author:  Josh Cooper
Created: Aug. 4 2020
Updated: Jun. 15 2021
*/

#include <PluginManager.h>
#include <modules/Maps.h>
#include <modules/Job.h>
#include <df/map_block.h>
#include <df/world.h>

#include <map>


using namespace DFHack;

DFHACK_PLUGIN_IS_ENABLED(is_enabled);
command_result channelSafely_cmd (color_ostream &out, std::vector <std::string> &parameters);


DFhackCExport command_result plugin_init( color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("channel-safely",
                                     "A tool to manage active channel designations.",
                                     channelSafely_cmd,
                                     false,
                                     "\n"));

    return CR_OK;
}
/*
    -onTickPeriod <number of ticks|daily|monthly|yearly>
    -onJob <completion|initiation>
    -onDesignation
*/

void onNewTickPeriod(color_ostream& out, void* eventData);
void onJobCompleted(color_ostream& out, void* job);

command_result channelSafely_cmd(color_ostream &out, std::vector<std::string> &parameters){
    if(!parameters.empty()){
        for(int i = 0; i + 1 < parameters.size(); ++i){
            auto &p1 = parameters[i];
            auto &p2 = parameters[i+1];
            if(p1.c_str()[0] == '-' && p2.c_str()[0] != '-'){
//                if(p1 == ""){
//
//                }
            } else {
                out.printerr("Invalid parameter or invalid parameter argument.");
                return CR_FAILURE;
            }
        }
    }
    //print status
}

bool is_dig(df::job* job) {
    return job->job_type == df::job_type::Dig;
}

bool is_dig(df::tile_designation &designation) {
    return designation.bits.dig == df::tile_dig_designation::Default;
}

bool is_channel(df::job* job){
    return job->job_type == df::job_type::DigChannel;
}

bool is_channel(df::tile_designation &designation){
    return designation.bits.dig == df::tile_dig_designation::Channel;
}

bool is_designated(df::tile_designation &designation){
    return is_dig(designation) || is_channel(designation);
}

bool is_marked(df::tile_occupancy &occupancy){
    return occupancy.bits.dig_marked;
}

void cancelJob(df::job* job){
    if(job) {
        df::coord &pos = job->pos;
        df::map_block *job_block = Maps::getTileBlock(pos);
        uint16_t x, y;
        x = pos.x % 16;
        y = pos.y % 16;
        df::tile_designation &designation = job_block->designation[x][y];
        designation.bits.dig = is_channel(job) ?
                df::tile_dig_designation::Channel : df::tile_dig_designation::Default;
        Job::removeJob(job);
    }
}








class ChannelManager {
private:
    std::multimap<int16_t, df::job*> dig_jobs;
protected:
    void find_dig_jobs() {
        df::job_list_link* cur = df::global::world->jobs.list.next;
        while (cur) {
            df::job* item = cur->item;
            if (is_dig(item)) {
                dig_jobs.emplace(item->pos.z, item);
            }
            cur = cur->next;
        }
    }

    df::job* find_job(df::coord &tile_pos) {
        auto iter = dig_jobs.lower_bound(tile_pos.z);
        while (iter != dig_jobs.end()) {
            df::coord &pos = iter->second->pos;
            if (pos == tile_pos) {
                return iter->second;
            }
            iter++;
        }
        return nullptr;
    }

    void foreach_block_column() {
        uint32_t x, y, z;
        Maps::getSize(x, y, z);
        for (int ix = 0; ix < x; ++ix) {
            for (int iy = 0; iy < y; ++iy) {
                for (int iz = z - 1; iz > 0; --iz) { //
                    df::map_block* top = Maps::getBlock(x, y, iz);
                    df::map_block* bottom = Maps::getBlock(x, y, iz - 1);
                    if (top->flags.bits.designated &&
                        bottom->flags.bits.designated) {
                        foreach_tile(top, bottom);
                    }
                }
            }
        }
    }

    void foreach_tile(df::map_block* top, df::map_block* bottom) {
        /** Safety checks
         * is the top tile being channeled or designated to be channeled
         * - Yes
         *      is the bottom tile being dug or designated and not marked
         *      - Yes
         *           try to cancelJob
         *           mark it for later
         * - No
         *      is the bottom tile designated
         *      - Yes
         *           unmark for later
         */
        for (int x = 0; x < 16; ++x) {
            for (int y = 0; y < 16; ++y) {
                auto &top_d = top->designation[x][y];
                auto &top_o = top->occupancy[x][y];
                auto &bottom_d = bottom->designation[x][y];
                auto &bottom_o = bottom->occupancy[x][y];
                df::job* top_job = find_job(top->map_pos);
                df::job* bottom_job = find_job(bottom->map_pos);
                if (top_job || is_channel(top_d)) {
                    if (bottom_job ||
                        !is_marked(bottom_o) && is_designated(bottom_d)) {
                        cancelJob(bottom_job);
                        bottom_o.bits.dig_marked = true;
                    }
                } else if (is_marked(bottom_o) && is_designated(bottom_d)) {
                    bottom_o.bits.dig_marked = false;
                }
            }
        }
    }

public:
    void manage_designations() {
        dig_jobs.clear();
        find_dig_jobs();
        foreach_block_column();
    }
};
