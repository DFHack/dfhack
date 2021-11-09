#ifndef UNKNBLCDR_ENLINK_H
#define UNKNBLCDR_ENLINK_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/World.h"
#include "modules/Screen.h"


#include "building_activation.h"
#include "loctools.h"
#include "nysave.h"


DFHACK_PLUGIN("enlink");
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);

using building_id = int32_t;


struct enlinker
{
    template <class T, class Stream>
    inline static void bin_out(Stream &s, const T & t)
    {
        s.write((char *)&t, sizeof(T));
    }

    template <class T, class Stream>
    inline static void bin_in(Stream &s, T & t)
    {
        s.read((char *)&t, sizeof(T));
    }

    inline static bool get_building_state(const building_id id)
    {
        return BuildingActivation::is_active(df::building::find(id));
    }

    inline static void set_building_state(const building_id id, const bool st)
    {
        BuildingActivation::set_state(df::building::find(id), st);
    }

    inline static void toggle_building_state(const building_id id)
    {
        BuildingActivation::toggle_state(df::building::find(id));
    }

    struct linkable
    {
        std::vector<building_id> linked;

        uint16_t on_threshold, off_threshold;

        //When on_threshold >= off_threshold,
        //it turns on when linked to at least on_threshold objects
        //and turns off when linked to less than off_threshold objects.
        //When off_threshold > on_threshold,
        //it turns off when linked to at least off_threshold objects
        //and turns on when linked to less than on_threshold objects.

        bool on_interrupt, off_interrupt, vanilla_override;
        //on/off_interrupt control if we should interrupt the on/off counter
        //if the conditions for doing the opposite transition are fulfilled
        //vanilla_override, if true, enables the state to be overriden by DF
        //(e. g. by ordering a dwarf to pull a lever)

        uint16_t on_time, off_time;
        uint16_t on_count, off_count;

        linkable(const uint16_t on_th = 1, const uint16_t of_th = 1,
                 const uint16_t on_tm = 0, const uint16_t of_tm = 0,
                 const bool on_i = true, const bool of_i = true, const bool v_o = true) :
                on_threshold(on_th),
                off_threshold(of_th),
                on_interrupt(on_i),
                off_interrupt(of_i),
                vanilla_override(v_o),
                on_time(on_tm),
                off_time(of_tm),
                on_count(0),
                off_count(0)
        {
        }

        inline void activate(const building_id this_b_id, enlinker &enl)
        {
            on_count = 0;
            set_building_state(this_b_id, true);
            enl.watched[this_b_id].state = true;
        }

        inline void deactivate(const building_id this_b_id, enlinker &enl)
        {
            off_count = 0;
            set_building_state(this_b_id, false);
            enl.watched[this_b_id].state = false;
        }

        inline void toggle(const building_id this_b_id, enlinker &enl)
        {
            on_count = 0;
            off_count = 0;
            toggle_building_state(this_b_id);
            enl.watched[this_b_id].state = !enl.watched[this_b_id].state;
            //Vanilla DF behaviour is the same.
        }

        inline int evaluate_count(const uint32_t active_count) const
        //returns -1 if it should shut down,
        //         1 if it should turn on,
        //         0 otherwise.
        //
        //Visually,
        //                 \
        //                 |> Turns on
        //  on_threshold  -/
        //
        //                 \
        //                 |> Keeps state
        //                 /
        //
        //  off_threshold -\
        //                 |> Turns off
        //                 /
        // ---------------------------------
        //                 \
        //                 |> Turns off
        //  off_threshold -/
        //
        //                 \
        //                 |> Keeps state
        //                 /
        //
        //  on_threshold  -\
        //                 |> Turns on
        //                 /
        //---------------------------------------------
        //                            \
        //                            |> Turns on
        //                            /
        //
        // on_threshold off_threshold -> Keeps state
        //
        //                            \
        //                            |> Turns off
        //                            /
        //
        {
            if (on_threshold > off_threshold)
            {
                return (active_count >= on_threshold) - (active_count <= off_threshold);
            }
            else if (on_threshold < off_threshold)
            {
                return (active_count <= on_threshold) - (active_count >= off_threshold);
            }
            else /*if (on_threshold == off_threshold)*/
            {
                return (active_count > on_threshold) - (active_count < off_threshold);
            }
        }

        inline void update(const building_id this_b_id, enlinker &enl)
        {
            uint32_t active_count = 0;
            const uint32_t max_count = (on_threshold > off_threshold ? on_threshold : off_threshold);

            for (const auto elem : linked)
            {
                active_count += enl.get_watched_state(elem);
                if (active_count > max_count)
                {
                    break;
                    //No need to check any more.
                }
            }

            const int wanted_state = evaluate_count(active_count);

            if (wanted_state > 0 || on_count > 0)
            {
                ++on_count;
            }

            if (wanted_state < 0 || off_count > 0)
            {
                ++off_count;
            }

            if (on_count > on_time && off_count > on_time)
            {
                toggle(this_b_id, enl);
            }
            else if (on_count > on_time)
            {
                activate(this_b_id, enl);
            }
            else if (off_count > off_time)
            {
                deactivate(this_b_id, enl);
            }

            if (wanted_state > 0 && off_interrupt && off_count > 0)
            {
                off_count = 0;
            }

            if (wanted_state < 0 && on_interrupt && on_count > 0)
            {
                on_count = 0;
            }

        }

        inline void add_link(const building_id id)
        {
            const auto it = std::lower_bound(linked.begin(), linked.end(), id);
            linked.insert(it, id);
        }

        inline void remove_link(const building_id id, const bool just_one = true)
        {
            if (just_one)
            {
                auto it = std::upper_bound(linked.begin(), linked.end(), id);
                if (it != linked.end() && it != linked.begin())
                {
                    linked.erase(--it);
                }
                else if (it == linked.end())
                {
                    if (linked.back() == id)
                    {
                        linked.pop_back();
                    }
                }
            }
            else
            {
                const auto it_pair = std::equal_range(linked.begin(), linked.end(), id);
                linked.erase(it_pair.first, it_pair.second);
            }
        }

        template <class Stream>
        inline void binary_output(Stream & s) const
        {
            int8_t bool_out = on_interrupt + 2 * off_interrupt + 4 * vanilla_override;
            //Yeah, binary operations would be marginally clearer, this is shorter...)
            bin_out(s, on_threshold);
            bin_out(s, off_threshold);
            bin_out(s, bool_out);
            bin_out(s, on_time);
            bin_out(s, off_time);
            bin_out(s, on_count);
            bin_out(s, off_count);
            bin_out(s, linked.size());
            for (size_t i = 0; i < linked.size(); ++i)
            {
                bin_out(s, linked[i]);
            }
        }

        template <class Stream, class ... Args>
        inline void binary_input(Stream & s, Args && ... args)
        {
            int8_t bool_in;
            bin_in(s, on_threshold);
            bin_in(s, off_threshold);
            bin_in(s, bool_in);

            on_interrupt = bool_in & 1;
            off_interrupt = bool_in & 2;
            vanilla_override = bool_in & 4;

            bin_in(s, on_time);
            bin_in(s, off_time);
            bin_in(s, on_count);
            bin_in(s, off_count);

            size_t new_size;

            bin_in(s, new_size);

            linked.resize(new_size);

            for (size_t i = 0; i < new_size; ++i)
            {
                bin_in(s, linked[i]);
            }
        }


        template <class Stream, class Map>
        inline void binary_input(Stream & s, const Map & mp)
        {
            int8_t bool_in;
            bin_in(s, on_threshold);
            bin_in(s, off_threshold);
            bin_in(s, bool_in);

            on_interrupt = bool_in & 1;
            off_interrupt = bool_in & 2;
            vanilla_override = bool_in & 4;

            bin_in(s, on_time);
            bin_in(s, off_time);
            bin_in(s, on_count);
            bin_in(s, off_count);

            size_t new_size;

            bin_in(s, new_size);

            linked.reserve(new_size);

            for (size_t i = 0; i < new_size; ++i)
            {
                building_id temp;
                bin_in(s, temp);
                const auto it = mp.find(temp);
                if (it != mp.end() && it->second > std::numeric_limits<int32_t>::min())
                {
                    linked.push_back(it->second);
                }
            }

        }

    };

    struct watched_state
    {
        int32_t link_counts;
        bool state;
        watched_state(const bool st = false, int32_t lk_cnt = 0) :
                link_counts(lk_cnt), state(st)
        {
        }

        template <class Stream>
        inline void binary_output(Stream &s) const
        {
            bin_out(s, link_counts);
            bin_out(s, state);
        }

        template <class Stream, class ... Args>
        inline void binary_input(Stream &s, Args && ... args)
        {
            bin_in(s, link_counts);
            bin_in(s, state);
        }
    };

    std::unordered_map<building_id, watched_state> watched;

    std::unordered_map<building_id, linkable> managed;

    inline bool get_watched_state(const building_id id) const
    {
        auto it = watched.find(id);
        if (it != watched.end())
        {
            return it->second.state;
        }
        return false;
    }

    inline void add_watched(const building_id id, const bool st)
    {
        auto it = watched.find(id);
        if (it != watched.end())
        {
            ++(watched[id].link_counts);
        }
        else
        {
            watched[id] = watched_state(st, 1);
        }
    }

    template <class ... Args>
    inline void add_managed(const building_id id, const bool st, Args && ... args)
    {
        add_watched(id, st);
        managed[id] = linkable(std::forward<Args>(args)...);
    }

    inline void remove_managed(const building_id id)
    {
        managed.erase(id);
        if (--(watched[id].link_counts) <= 0)
        {
            watched.erase(id);
        }
    }

    inline void add_link(const building_id receiver, const building_id emitter, const bool emitter_state)
    //Pre-condition: receiver is managed!
    {
        add_watched(emitter, emitter_state);
        managed[receiver].add_link(emitter);
    }

    inline void remove_link(const building_id receiver, const building_id emitter)
    //Pre-condition: receiver is managed, emitter is watched!
    {
        managed[receiver].remove_link(receiver, true);
        if (--(watched[emitter].link_counts) <= 0)
        {
            watched.erase(emitter);
        }
    }

    static constexpr uint32_t cull_interval = 65536;

    uint32_t cull_counter;

    std::unordered_set<int32_t> cull_site_ids;

    inline void add_to_cull(int32_t site_id)
    {
        cull_site_ids.insert(site_id);
    }

    inline void cull()
    {
        cull_counter = 0;
        std::unordered_set<building_id> del_id;
        for (const auto& kv : watched)
        {
            df::building *ptr = df::building::find(kv.first);
            if (!ptr)
            {
                del_id.insert(kv.first);
            }
            else if (cull_site_ids.count(ptr->site_id) > 0)
            {
                del_id.insert(kv.first);
            }
        }
        for (auto& kv : managed)
        {
            if (del_id.count(kv.first) > 0)
            {
                managed.erase(kv.first);
            }
            else
            {
                for (const auto& id : del_id)
                {
                    kv.second.remove_link(id, false);
                }
            }
        }
        cull_site_ids.clear();
    }

    inline void process_tick()
    //Process a tick.
    {
        for (auto& kv : managed)
        {
            if (kv.second.vanilla_override)
            {
                set_building_state(kv.first, get_watched_state(kv.first));
            }
        }

        for (auto& kv : watched)
        {
            kv.second.state = get_building_state(kv.first);
        }

        for (auto& kv : managed)
        {
            kv.second.update(kv.first, *this);
        }

        ++cull_counter;

        if (cull_counter >= cull_interval)
        {
            cull();
        }

    }

    inline void clear()
    {
        watched.clear();
        managed.clear();
        cull_counter = 0;
    }

    template <class Stream, class MapT>
    inline static bool write_map_bin(Stream & s, const MapT & map)
    //Returns `true` on error.
    {
        bin_out(s, map.size());
        for (const auto& kv : map)
        {
            typename MapT::key_type tempk = kv.first;
            bin_out(s, tempk);
            kv.second.binary_output(s);
            if (!s.good())
            {
                return true;
            }
        }
        return false;
    }

    template <class Stream, class MapT, class IdEquivMap>
    inline static bool read_map_bin(Stream & s, MapT & map, const IdEquivMap &equiv)
    //Returns `true` on error.
    {
        size_t map_size;
        bin_in(s, map_size);
        map.reserve(map_size);
        for (size_t i = 0; i < map_size; ++i)
        {
            typename MapT::key_type tempk;
            typename MapT::mapped_type tempv;
            bin_in(s, tempk);
            tempv.binary_input(s, equiv);
            if (s.fail())
            {
                return true;
            }
            const auto it = equiv.find(tempk);
            if (it != equiv.end() && it->second > std::numeric_limits<int32_t>::min())
            {
                map[it->second] = tempv;
            }
        }
        return false;
    }

    struct building_pos_seri
    {
        int32_t type;
        int32_t x, y, z;
        inline void set(const df::building_type t, const int32_t cx, const int32_t cy, const int32_t cz)
        {
            type = t;
            x = cx;
            y = cy;
            z = cz;
            LocTools::loc2glob(x, y, z);
        }
        inline void get(df::building_type& t, int32_t &cx, int32_t &cy, int32_t &cz) const
        {
            t = df::building_type(type);
            cx = x;
            cy = y;
            cz = z;
            LocTools::glob2loc(cx, cy, cz);
        }

        inline bool operator==(const building_pos_seri & other) const
        {
            return (type == other.type && x == other.x && y == other.y && z == other.z);
        }
    };

    template <class Stream>
    bool write_position_bin(Stream & s)
    {
        cull();
        bin_out(s, watched.size());
        for (const auto & kv : watched)
        {
            auto *ptr = df::building::find(kv.first);
            if (ptr)
            {
                building_id id = kv.first;
                building_pos_seri bps;
                bps.set(ptr->getType(), ptr->centerx, ptr->centery, ptr->z);

                bin_out(s, id);
                bin_out(s, bps);
            }
            else
            {
                return true;
            }
            if (s.fail())
            {
                return true;
            }
        }
        return false;
    }

    template <class Stream, class Map>
    bool read_position_bin(Stream & s, Map & map)
    {
        size_t new_size;
        bin_in(s, new_size);
        std::unordered_map<building_pos_seri, building_id> pre_map;
        pre_map.reserve(new_size);
        map.reserve(new_size);
        for (size_t i = 0; i < new_size; ++i)
        {
            building_id id;
            building_pos_seri bps;
            bin_in(s, id);
            bin_in(s, bps);

            if (s.fail())
            {
                return true;
            }

            pre_map[bps] = id;
            map[id] = std::numeric_limits<int32_t>::min();
        }

        for (const auto & ptr : df::global::world->buildings.all)
        {
            building_pos_seri bps;
            bps.set(ptr->getType(), ptr->centerx, ptr->centery, ptr->z);
            const auto it = pre_map.find(bps);
            if (it != pre_map.end())
            {
                map[it->second] = ptr->id;
            }
        }

        return false;
    }

    inline static std::string get_name()
    {
        return std::string("EnlinkInfo");
    }

    inline bool save(const std::string &sstr)
    //Returns `true` on error.
    {
        std::ofstream out;
        NySave::open_site_stream(get_name(), sstr, out, std::ios::binary);

        if (!out.is_open())
        {
            return true;
        }


        if (write_position_bin(out))
        {
            return true;
        }

        if (write_map_bin(out, watched))
        {
            return true;
        }

        if (write_map_bin(out, managed))
        {
            return true;
        }

        const bool ret = !out.good();

        out.close();

        return ret;
    }

    inline bool save()
    {
        return save(NySave::get_current_site_string());
    }

    inline bool load(const std::string &site_str)
    //Returns `true` on error.
    {
        std::ifstream in;
        NySave::open_site_stream(get_name(), site_str, in, std::ios::binary);

        if (!in.is_open())
        {
            return false;
            //Might be error,
            //but might just be unsaved.
        }

        std::unordered_map<building_id, building_id> id_map;

        if (read_position_bin(in, id_map))
        {
            return true;
        }

        if (read_map_bin(in, watched, id_map))
        {
            return true;
        }

        if (read_map_bin(in, managed, id_map))
        {
            return true;
        }

        in.close();

        return false;
    }

    inline bool load()
    {
        return load(NySave::get_current_site_string());
    }


};

namespace std
{
    template <>
    struct hash<enlinker::building_pos_seri>
    {
        size_t operator()(const enlinker::building_pos_seri& bps) const noexcept
        {
            // Compute individual hash values for the fields
            // http://stackoverflow.com/a/1646913/126995
            size_t ret = 17;
            ret = ret * 31 + hash<int32_t>()(bps.type);
            ret = ret * 31 + hash<int32_t>()(bps.x);
            ret = ret * 31 + hash<int32_t>()(bps.y);
            ret = ret * 31 + hash<int32_t>()(bps.z);
            return ret;
        }
    };
}

static std::unique_ptr<enlinker> globie;

struct enlinker_cacher
{
    std::unordered_map<int32_t, int> cached_count;
    int32_t curr_reg_x = -1, curr_reg_y = -1;
    int32_t curr_tick = 0;
    inline void clear()
    {
        cached_count.clear();
    }
};

static std::unique_ptr<enlinker_cacher> cachie;

using namespace DFHack;

DFHACK_PLUGIN_IS_ENABLED(enlink_enabled);

DFhackCExport DFHack::command_result enlinker_update(DFHack::color_ostream &out, std::vector <std::string> & parameters)
{

    if ( !NySave::can_get_site_string() || !globie || !cachie ||
            (DFHack::World::isFortressMode() && DFHack::World::ReadPauseState()) ||
            (DFHack::World::isAdventureMode() && DF_GLOBAL_VALUE(cur_year_tick_advmode, 1) == cachie->curr_tick) )
    {
        return DFHack::CR_OK;
    }

    if (DFHack::World::isAdventureMode())
        //We must take into account the possibility of moving into/out of sites with enlinked stuff.
    {
        cachie->curr_tick = DF_GLOBAL_VALUE(cur_year_tick_advmode, 1);
        const int32_t x = df::global::world->map.region_x, y = df::global::world->map.region_y;
        if (x != cachie->curr_reg_x || y != cachie->curr_reg_y)
        {
            cachie->curr_reg_x = x;
            cachie->curr_reg_y = y;
            std::vector<int32_t> cvs;
            LocTools::get_possibly_visible_sites(cvs);
            for (const auto & site : cvs)
            {
                auto it = cachie->cached_count.find(site);
                if (it == cachie->cached_count.end())
                {
                    out.print("Loading: %d\n", site);
                    if (globie->load(std::to_string(site)))
                    {
                        out.printerr("ERROR!");
                        return DFHack::CR_FAILURE;
                    }
                    cachie->cached_count[site] = -1;
                }
                else
                {
                    it->second = -1;
                }
            }
            for (auto & kv : cachie->cached_count)
            {
                ++(kv.second);
                if (kv.second > 3)
                {
                    out.print("Unloading: %d\n", kv.first);
                    globie->add_to_cull(kv.first);
                    cachie->cached_count.erase(kv.first);
                }
            }
        }
    }

    globie->process_tick();
    return DFHack::CR_OK;
}

/*
command_result make_enlinked(color_ostream &out, std::vector <std::string> & parameters)
{
  const building_id man = std::stoi(parameters[0]);
  const building_id lnk = std::stoi(parameters[1]);
  const bool st = std::stoi(parameters[2]);
  globie->add_link(man, lnk, st);
  return CR_OK;
}
*/

DFhackCExport DFHack::command_result enlinker_load(DFHack::color_ostream &out, std::vector <std::string> & parameters)
{
    if (!NySave::can_get_site_string() || !globie || !cachie)
    {
        return DFHack::CR_OK;
    }
    if (parameters.empty() || parameters[0] == "curr")
    {
        const int32_t siteid = LocTools::get_current_site();
        if (cachie)
        {
            cachie->cached_count[siteid] = 0;
        }
        out.print("Loading: %d\n", siteid);
        if (globie->load(std::to_string(siteid)))
        {
            return DFHack::CR_FAILURE;
        }
    }
    else
    {
        for (const auto& site_str : parameters)
        {
            out.print("Loading: %s\n", site_str.c_str());
            if (cachie)
            {
                const int32_t siteid = std::stof(site_str);
                cachie->cached_count[siteid] = 0;
            }
            if (globie->load(site_str))
            {
                return DFHack::CR_FAILURE;
            }
        }
    }
    return DFHack::CR_OK;
}


DFhackCExport DFHack::command_result enlinker_save(DFHack::color_ostream &out, std::vector <std::string> & parameters)
{
    if (!globie)
    {
        return DFHack::CR_FAILURE;
    }
    if (cachie)
    {
        for (auto & kv : cachie->cached_count)
        {
            if (kv.second > 0)
            {
                globie->add_to_cull(kv.first);
                cachie->cached_count.erase(kv.first);
            }
        }
    }
    if (parameters.empty() || parameters[0] == "curr")
    {
        if (!!df::global::ui)
        {
            const int32_t siteid = LocTools::get_current_site();
            out.print("Saving: %d\n", siteid);
            if (globie->save())
            {
                return DFHack::CR_FAILURE;
            }
        }
        else
        {
            out.print("Saving: previous\n");
            if (globie->save("0"))
            {
                return DFHack::CR_FAILURE;
            }
        }
    }
    else if (parameters.size() > 1)
    {
        return DFHack::CR_WRONG_USAGE;
    }
    else
    {
        if (globie->save(parameters[0]))
        {
            out.print("Saving: %s\n", parameters[0].c_str());
            return DFHack::CR_FAILURE;
        }
    }
    return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result enlinker_start(DFHack::color_ostream &out, std::vector <std::string> & parameters)
{
    enlink_enabled = true;
    return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result enlinker_stop(DFHack::color_ostream &out, std::vector <std::string> & parameters)
{
    enlink_enabled = false;
    return DFHack::CR_OK;
}

// Mandatory init function. If you have some global state, create it here.
DFhackCExport DFHack::command_result plugin_init(DFHack::color_ostream &out, std::vector <PluginCommand> &commands)
{

    globie.reset(new enlinker);
    cachie.reset(new enlinker_cacher);

    commands.push_back(PluginCommand(
                           "enlinker-load", "Load the enlinker set-up.",
                           enlinker_load, false,
                           "  Loads the enlinker set-up for the current site-id, or for the site-ids provided as arguments.\n"
                           "Example:\n"
                           "  enlinker-load\n"
                           "    Loads the enlinker set-up for the current site.\n"
                           "  enlinker-load 42\n"
                           "    Loads the enlinker set-up for the site with id 42.\n"
                           "  enlinker-load 1 2 3\n"
                           "    Loads the enlinker set-up for the sites with ids 1, 2 and 3.\n"
                       ));

    commands.push_back(PluginCommand(
                           "enlinker-save", "Save the enlinker set-up.",
                           enlinker_save, false,
                           "  Saves the enlinker set-up for the current site-id, or for the site-id provided as an argument.\n"
                           "Example:\n"
                           "  enlinker-save\n"
                           "    Saves the current enlinker set-up like it belonged to the current site.\n"
                           "  enlinker-save 42\n"
                           "    Saves the current enlinker set-up like it belonged to the site with id 42.\n"
                       ));

    commands.push_back(PluginCommand(
                           "enlinker-update", "Process a tick.",
                           enlinker_update, false,
                           "  Processes a single tick. Should not be needed in normal operation."
                       ));

    commands.push_back(PluginCommand(
                           "enlinker-start", "Start processing.",
                           enlinker_start, false,
                           "  Starts processing. Should be added to 'dfhack.init'."
                       ));

    commands.push_back(PluginCommand(
                           "enlinker-stop", "Stops processing.",
                           enlinker_stop, false,
                           "  Stops processing. Should not be needed in normal operation."
                       ));

    return DFHack::CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport DFHack::command_result plugin_shutdown(DFHack::color_ostream &out)
{
    globie.reset(nullptr);
    cachie.reset(nullptr);

    return DFHack::CR_OK;
}

// If you need to save or load world-specific data, define these functions.
// plugin_save_data is called when the game might be about to save the world,
// and plugin_load_data is called whenever a new world is loaded. If the plugin
// is loaded or unloaded while a world is active, plugin_save_data or
// plugin_load_data will be called immediately.

DFhackCExport DFHack::command_result plugin_save_data(DFHack::color_ostream &out)
{
    if (DFHack::World::isFortressMode())
    {
        out.print("Saving enlinker.\n");
        std::vector<std::string> dummy;
        return enlinker_save(out, dummy);
    }
    return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result plugin_load_data(DFHack::color_ostream &out)
{
    out.print("Loading enlinker.\n");
    std::vector<std::string> dummy;
    return enlinker_load(out, dummy);
}


DFhackCExport DFHack::command_result plugin_onstatechange(DFHack::color_ostream &out, DFHack::state_change_event event)
{
    switch (event)
    {
    case SC_WORLD_UNLOADED:
        globie->clear();
        cachie->clear();
        break;
    case SC_MAP_LOADED:
        {
            std::vector<std::string> dummy;
            enlinker_load(out, dummy);
        }
        break;
    default:
        break;
    }

    return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result plugin_onupdate(DFHack::color_ostream &out)
{
    if (!enlink_enabled)
    {
        return CR_OK;
    }
    std::vector<std::string> dummy;
    return enlinker_update(out, dummy);
}

static bool make_enlinked(building_id id)
{
    globie->add_managed(id, globie->get_building_state(id));

    return true;
}

static bool unmake_enlinked(building_id id)
{
    globie->remove_managed(id);

    return true;
}

static bool add_enlink(building_id managed, building_id watched)
{
    globie->add_link(managed, watched, globie->get_building_state(watched));

    return true;
}

static bool remove_enlink(building_id managed, building_id watched)
{
    globie->remove_link(managed, watched);

    return true;
}

static uint32_t get_enlink_info_p1(building_id id)
{
    const auto it = globie->managed.find(id);
    if (it == globie->managed.end())
    {
        return 0;
    }
    else
    {
        const auto & link = it->second;
        return (uint32_t(1) << 3) | (uint32_t(link.on_interrupt) << 2) | (uint32_t(link.off_interrupt) << 1) | (uint32_t(link.vanilla_override));
    }
}

static uint32_t get_enlink_info_p2(building_id id)
{
    const auto it = globie->managed.find(id);
    if (it == globie->managed.end())
    {
        return 0;
    }
    else
    {
        const auto & link = it->second;
        return (uint32_t(link.on_threshold) << 16) | uint32_t(link.off_threshold);
    }
}


static uint32_t get_enlink_info_p3(building_id id)
{
    const auto it = globie->managed.find(id);
    if (it == globie->managed.end())
    {
        return 0;
    }
    else
    {
        const auto & link = it->second;
        return (uint32_t(link.on_time) << 16) | uint32_t(link.off_time);
    }
}


static bool set_enlink_info(building_id id, uint32_t p1, uint32_t p2, uint32_t p3)
{
    const auto it = globie->managed.find(id);
    if (it == globie->managed.end())
    {
        return false;
    }
    else
    {
        auto & link = it->second;
        link.on_interrupt = p1 & 4;
        link.off_interrupt = p1 & 2;
        link.vanilla_override = p1 & 1;
        link.on_threshold = uint16_t(p2 >> 16);
        link.off_threshold = uint16_t(p2);
        link.on_time = uint16_t(p3 >> 16);
        link.off_time = uint16_t(p3);
        return true;
    }
}

static int32_t get_num_links(building_id id)
{
    const auto it = globie->managed.find(id);
    if (it == globie->managed.end())
    {
        return 0;
    }
    else
    {
        return it->second.linked.size();
    }
}


static building_id get_linked_building(building_id id, int32_t num)
{
    const auto it = globie->managed.find(id);
    building_id ret = -1;
    if (it != globie->managed.end())
    {
        ret = it->second.linked[num];
    }

    return ret;
}

static bool can_be_enlinked(building_id id)
{
    return BuildingActivation::get_activator(df::building::find(id)) != nullptr;
}

static bool is_building_active(building_id id)
{
    return globie->get_building_state(id);
}

static bool set_building_active(building_id id, bool state)
{
    globie->set_building_state(id, state);
    return true;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(make_enlinked),
    DFHACK_LUA_FUNCTION(unmake_enlinked),
    DFHACK_LUA_FUNCTION(add_enlink),
    DFHACK_LUA_FUNCTION(remove_enlink),
    DFHACK_LUA_FUNCTION(get_enlink_info_p1),
    DFHACK_LUA_FUNCTION(get_enlink_info_p2),
    DFHACK_LUA_FUNCTION(get_enlink_info_p3),
    DFHACK_LUA_FUNCTION(set_enlink_info),
    DFHACK_LUA_FUNCTION(get_num_links),
    DFHACK_LUA_FUNCTION(get_linked_building),
    DFHACK_LUA_FUNCTION(can_be_enlinked),
    DFHACK_LUA_FUNCTION(is_building_active),
    DFHACK_LUA_FUNCTION(set_building_active),
    DFHACK_LUA_END
};

#endif

