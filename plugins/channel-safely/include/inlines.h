#pragma once
#include "plugin.h"
#include "channel-manager.h"

#include <TileTypes.h>
#include <LuaTools.h>
#include <LuaWrapper.h>
#include <modules/Maps.h>
#include <df/job.h>

#include <cinttypes>
#include <unordered_set>
#include <random>

#define Coord(id) (id).x][(id).y
#define COORD "%" PRIi16 ",%" PRIi16 ",%" PRIi16
#define COORDARGS(id) (id).x, (id).y, (id).z

namespace CSP {
    extern std::unordered_set<df::coord> dignow_queue;
}

inline uint32_t calc_distance(df::coord p1, df::coord p2) {
    // calculate chebyshev (chessboard) distance
    uint32_t distance = abs(p2.z - p1.z);
    distance += std::max(abs(p2.x - p1.x), abs(p2.y - p1.y));
    return distance;
}

inline void get_connected_neighbours(const df::coord &map_pos, df::coord(&neighbours)[4]) {
    neighbours[0] = map_pos;
    neighbours[1] = map_pos;
    neighbours[2] = map_pos;
    neighbours[3] = map_pos;
    neighbours[0].y--;
    neighbours[1].x--;
    neighbours[2].x++;
    neighbours[3].y++;
}

inline void get_neighbours(const df::coord &map_pos, df::coord(&neighbours)[8]) {
    neighbours[0] = map_pos;
    neighbours[1] = map_pos;
    neighbours[2] = map_pos;
    neighbours[3] = map_pos;
    neighbours[4] = map_pos;
    neighbours[5] = map_pos;
    neighbours[6] = map_pos;
    neighbours[7] = map_pos;
    neighbours[0].x--; neighbours[0].y--;
    neighbours[1].y--;
    neighbours[2].x++; neighbours[2].y--;
    neighbours[3].x--;
    neighbours[4].x++;
    neighbours[5].x--; neighbours[5].y++;
    neighbours[6].y++;
    neighbours[7].x++; neighbours[7].y++;
}

inline uint8_t count_accessibility(const df::coord &unit_pos, const df::coord &map_pos) {
    df::coord neighbours[8];
    df::coord connections[4];
    get_neighbours(map_pos, neighbours);
    get_connected_neighbours(map_pos, connections);
    uint8_t accessibility = Maps::canWalkBetween(unit_pos, map_pos) ? 1 : 0;
    for (auto n: neighbours) {
        if unlikely(!Maps::isValidTilePos(n)) continue;
        if (Maps::canWalkBetween(unit_pos, n)) {
            accessibility++;
        }
    }
    for (auto n : connections) {
        if unlikely(Maps::isValidTilePos(n)) continue;
        if (Maps::canWalkBetween(unit_pos, n)) {
            accessibility++;
        }
    }
    return accessibility;
}

inline bool isEntombed(const df::coord &unit_pos, const df::coord &map_pos) {
    if likely(Maps::canWalkBetween(unit_pos, map_pos)) {
        return false;
    }
    df::coord neighbours[8];
    get_neighbours(map_pos, neighbours);
    return std::all_of(neighbours+0, neighbours+8, [&unit_pos](df::coord n) {
        return !Maps::isValidTilePos(n) || !Maps::canWalkBetween(unit_pos, n);
    });
}

inline bool is_dig_job(const df::job* job) {
    return job && (job->job_type == df::job_type::Dig || job->job_type == df::job_type::DigChannel);
}

inline bool is_channel_job(const df::job* job) {
    return job && (job->job_type == df::job_type::DigChannel);
}

inline bool is_group_job(const ChannelGroups &groups, const df::job* job) {
    return groups.count(job->pos);
}

inline bool is_dig_designation(const df::tile_designation &designation) {
    return designation.bits.dig != df::tile_dig_designation::No;
}

inline bool is_channel_designation(const df::tile_designation &designation) {
    return designation.bits.dig == df::tile_dig_designation::Channel;
}

inline bool is_safe_fall(const df::coord &map_pos) {
    df::coord below(map_pos);
    for (uint8_t zi = 0; zi < config.fall_threshold; ++zi) {
        below.z--;
        // falling out of bounds is probably considerably unsafe for a dwarf
        if unlikely(!Maps::isValidTilePos(below)) {
            return false;
        }
        // if we require vision, and we can't see below.. we'll need to assume it's safe to get anything done
        if (config.require_vision && Maps::getTileDesignation(below)->bits.hidden) {
            return true;
        }
        // finally, if we're not looking at open space (air to fall through) it's safe to fall to
        df::tiletype type = *Maps::getTileType(below);
        if (!DFHack::isOpenTerrain(type)) {
            return true;
        }
    }
    // we exceeded the fall threshold, so it's not a safe fall
    return false;
}

inline bool is_safe_to_dig_down(const df::coord &map_pos) {
    df::coord pos(map_pos);

    // todo: probably should rely on is_safe_fall, it looks like it could be simplified a great deal
    for (uint8_t zi = 0; zi <= config.fall_threshold; ++zi) {
        // if we're digging out of bounds, the game can handle that (hopefully)
        if unlikely(!Maps::isValidTilePos(pos)) {
            return true;
        }
        // if we require vision, and we can't see the tiles in question.. we'll need to assume it's safe to dig to get anything done
        if (config.require_vision && Maps::getTileDesignation(pos)->bits.hidden) {
            return true;
        }

        df::tiletype type = *Maps::getTileType(pos);
        if (zi == 0 && DFHack::isOpenTerrain(type)) {
            // todo: remove? this is probably not useful.. and seems like the only considerable difference to is_safe_fall (aside from where each stops looking)
            // the starting tile is open space, that's obviously not safe
            return false;
        } else if (!DFHack::isOpenTerrain(type)) {
            // a tile after the first one is not open space
            return true;
        }
        pos.z--; // todo: this can probably move to the beginning of the loop
    }
    return false;
}

inline bool has_unit(const df::tile_occupancy* occupancy) {
    return occupancy->bits.unit || occupancy->bits.unit_grounded;
}

inline bool has_group_above(const ChannelGroups &groups, const df::coord &map_pos) {
    df::coord above(map_pos);
    above.z++;
    if (groups.count(above)) {
        return true;
    }
    return false;
}

inline bool has_any_groups_above(const ChannelGroups &groups, const Group &group) {
    // for each designation in the group
    for (auto &pos : group) {
        df::coord above(pos);
        above.z++;
        if (groups.count(above)) {
            return true;
        }
    }
    // if there are no incomplete groups above this group, then this group is ready
    return false;
}

inline void cancel_job(df::job* job) {
    if (job) {
        const df::coord &pos = job->pos;
        df::map_block* job_block = Maps::getTileBlock(pos);
        uint16_t x, y;
        x = pos.x % 16;
        y = pos.y % 16;
        df::tile_designation &designation = job_block->designation[x][y];
        auto type = job->job_type;
        ChannelManager::Get().jobs.erase(pos);
        Job::removeWorker(job);
        Job::removePostings(job, true);
        Job::removeJob(job);
        job_block->flags.bits.designated = true;
        job_block->occupancy[x][y].bits.dig_marked = true;
        switch (type) {
            case job_type::Dig:
                designation.bits.dig = df::tile_dig_designation::Default;
                break;
            case job_type::CarveUpwardStaircase:
                designation.bits.dig = df::tile_dig_designation::UpStair;
                break;
            case job_type::CarveDownwardStaircase:
                designation.bits.dig = df::tile_dig_designation::DownStair;
                break;
            case job_type::CarveUpDownStaircase:
                designation.bits.dig = df::tile_dig_designation::UpDownStair;
                break;
            case job_type::CarveRamp:
                designation.bits.dig = df::tile_dig_designation::Ramp;
                break;
            case job_type::DigChannel:
                designation.bits.dig = df::tile_dig_designation::Channel;
                break;
            default:
                designation.bits.dig = df::tile_dig_designation::No;
                break;
        }
    }
}

inline void cancel_job(const df::coord &map_pos) {
    cancel_job(ChannelManager::Get().jobs.find_job(map_pos));
    ChannelManager::Get().jobs.erase(map_pos);
}

// executes dig designations for the specified tile coordinates
inline bool dig_now(color_ostream &out, const df::coord &map_pos) {
    static std::default_random_engine rng;
    std::uniform_int_distribution<> dist(0,5);
    out.color(color_value::COLOR_YELLOW);
    out.print("channel-safely: insta-dig: digging (" COORD ")<\n", COORDARGS(map_pos));

    df::coord below(map_pos);
    below.z--;
    auto ttype_below = *Maps::getTileType(below);
    if (isOpenTerrain(ttype_below) || isFloorTerrain(ttype_below)) {
        *Maps::getTileType(map_pos) = tiletype::OpenSpace;
    } else {
        auto ttype_p = Maps::getTileType(map_pos);
        if (isSoilMaterial(*ttype_p)) {
            switch(dist(rng)) {
                case 0:
                    *ttype_p = tiletype::SoilFloor1;
                    break;
                case 1:
                    *ttype_p = tiletype::SoilFloor2;
                    break;
                case 2:
                    *ttype_p = tiletype::SoilFloor3;
                    break;
                case 3:
                    *ttype_p = tiletype::SoilFloor4;
                    break;
                default:
                    *ttype_p = tiletype::SoilFloor1;
                    break;
            }
        } else if (isStoneMaterial(*ttype_p)) {
            switch(dist(rng)) {
                case 0:
                    *ttype_p = tiletype::FeatureFloor1;
                    break;
                case 1:
                    *ttype_p = tiletype::FeatureFloor2;
                    break;
                case 2:
                    *ttype_p = tiletype::FeatureFloor3;
                    break;
                case 3:
                    *ttype_p = tiletype::FeatureFloor4;
                    break;
                default:
                    *ttype_p = tiletype::MineralFloor1;
                    break;
            }
        } else {
            out.print("Unknown type\n");
            return false;
        }
    }

    return true;
    /*
    bool ret = false;

    lua_State* state = Lua::Core::State;
    static const char* module_name = "plugins.dig-now";
    static const char* fn_name = "dig_now_tile";
    // the stack layout isn't likely to change, ever
    static auto args_lambda = [&map_pos](lua_State* L) {
        Lua::Push(L, map_pos);
    };
    static auto res_lambda = [&ret](lua_State* L) {
        ret = lua_toboolean(L, -1);
    };

    Lua::StackUnwinder top(state);
    Lua::CallLuaModuleFunction(out, state, module_name, fn_name, 1, 1, args_lambda, res_lambda);
    return ret;
     */
}

// fully heals the unit specified, resurrecting if need be
inline void resurrect(color_ostream &out, const int32_t &unit) {
    out.color(DFHack::COLOR_RED);
    out.print("channel-safely: resurrecting [id: %d]\n", unit);
    std::vector<std::string> params{"-r", "--unit", std::to_string(unit)};
    Core::getInstance().runCommand(out,"full-heal", params);
}

template<class Ctr1, class Ctr2, class Ctr3>
void set_difference(const Ctr1 &c1, const Ctr2 &c2, Ctr3 &c3) {
    for (const auto &a : c1) {
        if (!c2.count(a)) {
            c3.emplace(a);
        }
    }
}

template<class Ctr1, class Ctr2, class Ctr3>
void map_value_difference(const Ctr1 &c1, const Ctr2 &c2, Ctr3 &c3) {
    for (const auto &a : c1) {
        bool matched = false;
        for (const auto &b : c2) {
            if (a.second == b.second) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            c3.emplace(a.second);
        }
    }
}
