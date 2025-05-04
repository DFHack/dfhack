#include "Debug.h"
#include "Error.h"
#include "MiscUtils.h"
#include "PluginManager.h"

#include "modules/Maps.h"
#include "modules/Translation.h"

#include "df/creature_raw.h"
#include "df/entity_raw.h"
#include "df/historical_entity.h"
#include "df/map_block.h"
#include "df/region_map_entry.h"
#include "df/site_map_infost.h"
#include "df/world_data.h"
#include "df/world_landmass.h"
#include "df/world_region_details.h"
#include "df/world_region.h"
#include "df/world_river.h"
#include "df/world.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <functional>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("export-map");

REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(exportmap, log, DebugCategory::LINFO);
    DBG_DECLARE(exportmap, warning, DebugCategory::LWARNING);
}

template<>
struct std::hash<df::coord2d>
{
    std::size_t operator()(const df::coord2d& pos) const noexcept
    {
        // hashing is easy, if values are smaller than the hash
        return ((std::size_t)pos.x << 16) | (std::size_t)pos.y;
    }
};

// were only dealing with 2D coordinates in this file
using coord = df::coord2d;

static int wdim = 768; // dimension of a world tile
static int rdim = 48;  // dimension of a region tile

/**
 * Takes a vector of coordinates interpreted as global region tile coordinates
 * (i.e. 16 region tiles per world tile) and emits a WKT path in GIS-compatible
 * local tile coordinates (negative y-coordinates, 48 stepts per region tile)
 */
auto print_path(std::ostream &out, const std::vector<coord> &path) {
    auto scale = rdim;
    assert(path.size());
    auto print_point = [scale](std::ostream &out, const coord &pos){
        out << scale * pos.x << " " << -scale * pos.y;};
    print_range(out, path, print_point, "(", ",", ")");
}

df::coord2d get_world_index(int16_t world_x, int16_t world_y, int8_t dir) {
    switch (dir) {
        case 1: world_x--   ; world_y++; break;
        case 2:             ; world_y++; break;
        case 3: world_x++   ; world_y++; break;
        case 4: world_x--   ;          ; break;
        // case 5 induces no change
        case 6: world_x++   ;          ; break;
        case 7: world_x--   ; world_y--; break;
        case 8:             ; world_y--; break;
        case 9: world_x++   ; world_y--; break;
    }
    world_x = std::clamp(world_x,(int16_t)0,(int16_t)(world->world_data->world_width - 1));
    world_y = std::clamp(world_y,(int16_t)0,(int16_t)(world->world_data->world_height - 1));
    return { world_x, world_y };
}

const char* describe_surroundings(int savagery, int evilness) {
    constexpr std::array<const char*,9>surroundings{
        "Serene",   "Mirthful",     "Joyous Wilds",
        "Calm",     "Wilderness",   "Untamed Wilds",
        "Sinister", "Haunted",      "Terrifying"
    };
    auto savagery_index = savagery < 33 ? 0 : (savagery > 65 ? 2 : 1);
    auto evilness_index = evilness < 33 ? 0 : (evilness > 65 ? 2 : 1);
    return surroundings[3 * evilness_index + savagery_index];
}


static command_result do_command(color_ostream &out, vector<string> &parameters);
DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(log,out).print("initializing %s\n", plugin_name);

    commands.push_back(PluginCommand(
        plugin_name,
        "Export the world map.",
        do_command));

    return CR_OK;
}

static command_result export_region_tiles(color_ostream &out);
static command_result export_sites(color_ostream &out);
static command_result export_rivers(color_ostream &out);

static command_result do_command(color_ostream &out, vector<string> &parameters)
{
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()){
        out.printerr("This command requires a world to be loaded\n");
        return CR_WRONG_USAGE;
    }

    if (parameters.size() && parameters[0] == "sites")
    {
        return export_sites(out);
    }
    else if (parameters.size() && parameters[0] == "rivers") {
        return export_rivers(out);
    }
    else
    {
        return export_region_tiles(out);
    }
}

/********************************************************************** */

static command_result export_sites(color_ostream &out)
{
    out.print("exporting sites... ");
    out.flush();
    const auto start{std::chrono::steady_clock::now()};

    // ensure that we have an output file
    std::string filename("sites.csv");
    std::ofstream out_file(filename, std::ios::out | std::ios::trunc);
    if (!out_file) {
        return CR_FAILURE;
    }

    // If you change anything in this vector, don't forget to change the
    // corresponding comments and arguments in the call to print_csv below
    vector<std::string> headings = {
        "site_id", "civ_id", "created_year", "cur_owner_id", "type",
        "site_name_df", "site_name_en", "civ_name_df", "civ_name_en", "site_government_df", "site_government_en", "owner_race"
    };
    print_range(out_file, headings,"",";",";boundary_wkt\n" );

    #define TRANSLATE_DF_EN(guard, name_object)\
        guard ? DF2UTF(Translation::translateName(&name_object, false)) : "NONE",\
        guard ? DF2UTF(Translation::translateName(&name_object, true)) : "NONE"

    for (auto const site : world->world_data->sites)
    {
        auto civ = df::historical_entity::find(site->civ_id);
        auto owner = df::historical_entity::find(site->cur_owner_id);

        df::creature_raw *race = nullptr;
        if (owner){
            race = df::creature_raw::find(owner->race);
            DEBUG(warning, out).print("owner (%d) of site (%d) has undefined race (%d)", owner->id, site->id, owner->race);
            if (!race)            {
                df::creature_raw::find(civ->race);
            }
        }

        auto print_csv = [&out_file](auto ...args){ ([&]{ out_file << args << ";" ; }() ,...); };
        print_csv(
            //  "site_id", "civ_id", "created_year", "cur_owner_id", "type",
            site->id,
            site->civ_id,
            site->created_year,
            site->cur_owner_id,
            DFHack::Maps::getSiteTypeName(site),
            // "site_name_df", "site_name_en", "civ_name_df", "civ_name_en", "site_government_df", "site_government_en", "owner_race"
            TRANSLATE_DF_EN(true, site->name),
            TRANSLATE_DF_EN(civ, civ->name),
            TRANSLATE_DF_EN(owner, owner->name),
            race ? race->name[2] : "NONE"
        );
        const vector<coord> path{
            coord(site->global_min_x, site->global_min_y),
            coord(site->global_max_x+1, site->global_min_y),
            coord(site->global_max_x+1, site->global_max_y+1),
            coord(site->global_min_x, site->global_max_y+1),
            coord(site->global_min_x, site->global_min_y)
        };
        print_range(out_file, std::vector<vector<coord>>{path}, print_path , "POLYGON(", ",", ")\n" );
    }


    const auto finish{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{finish - start};
    out.print("done in %.2fs !\n", elapsed_seconds.count());
    return CR_OK;
}

/********************************************************************** */



bool region_order(coord p1, coord p2) {
    return p1.y < p2.y || (p1.y == p2.y && p1.x < p2.x);
};

enum class direction : int { North = 0, West = 1, South = 2, East = 3 };

direction turn_left(direction dir) {
    return (direction)(((int)dir+1) % 4);
}

direction turn_right(direction dir) {
    return (direction)(((int)dir+3) % 4);
}

coord as_offset(direction dir) {
    switch (dir) {
        case direction::North:
            return { 0, -1 };
        case direction::West:
            return { -1, 0 };
        case direction::South:
            return { 0, 1 };
        case direction::East:
            return { 1, 0 };
        default:
            abort();
    }
}

coord advance(coord pos, direction dir) {
    return pos + as_offset(dir);
}

std::pair<bool,bool> ahead(const std::vector<coord> &component, coord pos, direction dir) {
    auto test = [&](int16_t x, int16_t y){
        coord offset{x,y};
        return std::ranges::binary_search(component, pos + offset, region_order);
    };

    switch (dir) {
        case direction::North:
            return { test(-1,-1), test(0,-1)};
        case direction::West:
            return { test(-1,0), test(-1,-1)};
        case direction::South:
            return { test(0,0), test(-1, 0)};
        case direction::East:
            return { test(0,-1), test(0, 0)};
        default:
            abort();
    }
}

static command_result export_region_tiles(color_ostream &out)
{
    out.print("%lu / %d region map tiles loaded\n",
        world->world_data->midmap_data.region_details.size(),
        world->world_data->world_width * world->world_data->world_height
    );
    out.print("exporting map... \n");
    out.flush();
    const auto start{std::chrono::steady_clock::now()};

    // ensure that we have an output file
    std::string filename("map.csv");
    std::ofstream out_file(filename, std::ios::out | std::ios::trunc);
    if (!out_file) {
        return CR_FAILURE;
    }

    // If you change anything in this vector, don't forget to change the
    // corresponding comments and arguments in the call to print_csv below
    vector<std::string> headings = {
        "world_x", "world_y", "num_tiles", "num_components", "biome_type",
        "region_id", "region_name_en", "region_name_df", "landmass_id", "landmass_name_en", "landmass_name_df",
        "evilness", "savagery", "volcanism", "drainage", "temperature", "vegetation", "rainfall", "salinity",
        "surroundings", "elevation", "reanimating", "has_bogeymen"
    };
    print_range(out_file, headings,"",";",";boundary_wkt\n" );

    /* Preprocessing: cluster region tiles by the world tile used for the biome information */

    // map world tile coord -> vector of region tiles referencing of world title
    std::unordered_map<coord,std::vector<coord>> world_tile_region;

    // iterating over the region details allows the user to do partial map exports
    // by manually scrolling on the embark site selection
    for (auto const region_details : world->world_data->midmap_data.region_details) {
        auto world_x = region_details->pos.x;
        auto world_y = region_details->pos.y;
        for (int region_x = 0; region_x < 16; ++region_x) {
            for (int region_y = 0; region_y < 16; ++region_y)
            {
                auto biome_tile = get_world_index(world_x, world_y, region_details->biome[region_x][region_y]);
                world_tile_region[biome_tile].emplace_back(16 * world_x + region_x, 16 * world_y + region_y);
            }
        }
    }

    out.print("processing %ld world tile regions\n", world_tile_region.size());


    static std::array<coord,4> directions{
        as_offset(direction::North),
        as_offset(direction::West),
        as_offset(direction::South),
        as_offset(direction::East)
    };

    for (auto& [biome_tile, region] : world_tile_region)
    {
        assert(region.size() > 0);

         // sorting the region provides O(log n) membership test.
         std::ranges::sort(region, region_order);

        /* Phase I : compute the connected components of the world tile region */

        // component_assignment[i] is the component id of region[i]
        std::vector<unsigned int> component_assignment;
        component_assignment.resize(region.size(),0);

        // (indices of) blocks in the current component that have been discovered but not yet explored
        std::deque<size_t> agenda;
        unsigned int current_component = 0;

        for (size_t i = 0; i < region.size(); ++i) {
            if (component_assignment[i]) {
                // skip region tiles that have already been assigned a component
                continue;
            } else {
                // start a new component for tiles that haven't been assigned yet
                ++current_component;
                component_assignment[i] = current_component;
                agenda.push_back(i);
            }
            while(!agenda.empty()) {
                auto pos_idx = agenda.front(); agenda.pop_front();
                auto pos = region[pos_idx];

                for (const auto& offset : directions) {
                    auto lb = std::ranges::lower_bound(region, pos + offset, region_order);
                    if (lb != region.end() && *lb == pos + offset) {
                        auto n_idx = std::distance(region.begin(), lb);
                        if (component_assignment[n_idx] == 0) {
                            component_assignment[n_idx] = current_component;
                            agenda.push_back(n_idx);
                        }
                    }
                }
            }
        }

        // check that all parts of the region are accounted for
        assert(std::ranges::all_of(component_assignment, [](int comp){ return comp > 0;}));

        // distribute region tiles according to their component assignment (preserves region order)
        std::vector<std::vector<coord>> components;
        components.resize(current_component);
        for (size_t i = 0; i < region.size(); ++i) {
            components.at(component_assignment.at(i) - 1).push_back(region.at(i));
        }

        /* Phase II : create paths by clockwise traversal along the outside of every component */
        /**
         * Note: DF uses "picture coordinates" (positive y values go "south")
         * while in GIS software positve y values go "north". Thus, [print_path]
         * negates the y-coordinates, turning the clockwise traversals into
         * counterclockwise traversals as specified by WKT.
         * https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry
         */

        std::vector<std::vector<coord>> paths;
        for (auto const &component : components) {

            // start at the NW corner of the west-most tile of the northmost row...
            auto start = component.at(0);
            std::vector<coord> path;
            path.push_back(start);

            // ... ensuring that a step to the east is a valid clockwise step along the boundary.
            auto current_direction = direction::East;
            auto current_position = advance(start,current_direction);

            while (current_position != start)
            {
                auto [left, right] = ahead(component, current_position, current_direction);
                if (left && right) {
                    // in front of a wall: turn right
                    path.push_back(current_position);
                    current_direction = turn_left(current_direction);
                }
                else if (!left && !right) {
                    // no walls ahead: turn left
                    path.push_back(current_position);
                    current_direction = turn_right(current_direction);
                }
                else if (left && !right) {
                    // diagonal step: turn right (following the outline of the inclusion)
                    // this does not seem to happen with the maps currently generated by DF
                    DEBUG(warning,out).print("Region has self-intersecting outline");
                    path.push_back(current_position);
                    current_direction = turn_right(current_direction);
                }
                // case !left && right requires no turn; advance the position in all cases
                current_position = advance(current_position, current_direction);
            }
            // close the path
            path.push_back(current_position);
            paths.push_back(std::move(path));
            path.clear();
        }
        assert(paths.size() > 0);

        /* Phase III: output the CSV line */

        auto& region_map_entry = world->world_data->region_map[biome_tile.x][biome_tile.y];
        auto world_region = df::world_region::find(region_map_entry.region_id);
        auto landmass = df::world_landmass::find(region_map_entry.landmass_id);

        auto print_csv = [&out_file](auto ...args){ ([&]{ out_file << args << ";" ; }() ,...); };
        print_csv(
            // "world_x", "world_y", "num_tiles", "num_components", "biome_type",
            biome_tile.x,
            biome_tile.y,
            region.size(),
            components.size(),
            ENUM_KEY_STR(biome_type,Maps::getBiomeType(biome_tile.x, biome_tile.y)),
            // "region_id", "region_name_en", "region_name_df", "landmass_id", "landmass_name_en", "landmass_name_df"
            region_map_entry.region_id,
            world_region ? DF2UTF(Translation::translateName(&world_region->name, true)) : "NONE",
            world_region ? DF2UTF(Translation::translateName(&world_region->name, false)) : "NONE",
            region_map_entry.landmass_id,
            landmass ? DF2UTF(Translation::translateName(&landmass->name, true)) : "NONE",
            landmass ? DF2UTF(Translation::translateName(&landmass->name, false)) : "NONE",
            // "evilness", "savagery", "volcanism", "drainage", "temperature", "vegetation", "rainfall", "salinity"
            region_map_entry.evilness,
            region_map_entry.savagery,
            region_map_entry.volcanism,
            region_map_entry.drainage,
            region_map_entry.temperature,
            region_map_entry.vegetation,
            region_map_entry.rainfall,
            region_map_entry.salinity,
            // "surroundings", "elevation", "reanimating", "has_bogeymen"
            describe_surroundings(region_map_entry.savagery, region_map_entry.evilness),
            region_map_entry.elevation,
            world_region->reanimating,
            world_region->has_bogeymen
        );

        // output geometry as WKT
        if (paths.size() == 1) {
            print_range(out_file, paths, print_path , "POLYGON(", ",", ")\n" );
        } else {
            print_range(out_file, paths, print_path , "MULTIPOLYGON((", "),(", "))\n" );
        }
    }

    const auto finish{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{finish - start};
    out.print("done in %f s !\n", elapsed_seconds.count());
    return CR_OK;
}

/********************************************************************** */

// used for global coordinates at local tile granularity (129*768 = 99072 doesn't fit into df::coord2d)
struct gcoord {
    int x,y;
};

struct river_tile {
    using polygon_t = std::list<gcoord>;
    polygon_t polygon;
    //std::optional<polygon_t::iterator> north, south, west, east;
};

struct gate {
    int active,min,max;

    static gate get(const df::world_region_details *const region_details, int region_x, int region_y, direction dir) {
        auto& vertical = region_details->rivers_vertical;
        auto& horizontal = region_details->rivers_horizontal;
        switch (dir) {
            case direction::North:
                return { vertical.active[region_x][region_y], vertical.x_min[region_x][region_y], vertical.x_max[region_x][region_y] };
            case direction::West:
                return { horizontal.active[region_x][region_y], horizontal.y_min[region_x][region_y], horizontal.y_max[region_x][region_y] };
            case direction::South:
                return { vertical.active[region_x][region_y+1], vertical.x_min[region_x][region_y+1], vertical.x_max[region_x][region_y+1] };
            case direction::East:
                return { horizontal.active[region_x+1][region_y], horizontal.y_min[region_x+1][region_y], horizontal.y_max[region_x+1][region_y] };
            default:
                assert(false);
                return {};
        }
    }

    bool is_valid() const {
        return active != 0 && min != -30000 && max != -30000;
    }
};


bool is_land(const df::world_region_details *const region_details, int16_t region_x, int16_t region_y) {
    CHECK_NULL_POINTER(region_details);
    auto [world_x, world_y] = region_details->pos;
    auto biome_tile = get_world_index(world_x, world_y, region_details->biome[region_x][region_y]);
    auto region_map_entry = Maps::getRegionBiome(biome_tile);
    CHECK_NULL_POINTER(region_map_entry);
    return region_map_entry->elevation >= 100 && !region_map_entry->flags.is_set(df::enums::region_map_entry_flags::is_lake);
}

static command_result export_rivers(color_ostream &out)
{
    // ensure that we have an output file
    std::string filename("rivers.csv");
    std::ofstream out_file(filename, std::ios::out | std::ios::trunc);
    if (!out_file) {
        return CR_FAILURE;
    }

    // create lookup table for rivers based on world tile coordinates
    std::unordered_map<coord,size_t> world_river;

    // assign river end first, so that it can be overridden by proper path elements
    for (size_t r_idx = 0; r_idx < df::global::world->world_data->rivers.size(); ++r_idx) {
        auto river = df::global::world->world_data->rivers[r_idx];
        world_river[river->end_pos] = r_idx;
    }

    for (size_t r_idx = 0; r_idx < df::global::world->world_data->rivers.size(); ++r_idx) {
        auto river = df::global::world->world_data->rivers[r_idx];
        for (size_t i = 0; i < river->path.size(); ++i) {
            auto pos = river->path[i];
            world_river[pos] = r_idx;
        }
    }

    // river idx -> region tile coord -> tile
    std::unordered_map<size_t,std::unordered_map<coord,river_tile>> tile_index;

    for (auto const region_details : world->world_data->midmap_data.region_details) {
        auto [world_x, world_y] = region_details->pos;
        for (int region_x = 0; region_x < 16; ++region_x) {
            for (int region_y = 0; region_y < 16; ++region_y)
            {
                gcoord base = { world_x * wdim + region_x * rdim, world_y * wdim + region_y * rdim };

                auto north = gate::get(region_details, region_x, region_y, direction::North);
                auto west = gate::get(region_details, region_x, region_y, direction::West);
                auto south = gate::get(region_details, region_x, region_y, direction::South);
                auto east = gate::get(region_details, region_x, region_y, direction::East);

                // skip tiles without any gates
                if (!(north.is_valid() || west.is_valid() || south.is_valid() || east.is_valid()))
                    continue;

                // skip any river tiles that are on oceans or lakes
                if (!is_land(region_details, region_x, region_y))
                    continue;

                river_tile tile;

                if (north.is_valid()) {
                    tile.polygon.emplace_back(base.x + north.max, base.y);
                    tile.polygon.emplace_back(base.x + north.min, base.y);
                }
                if (west.is_valid()) {
                    tile.polygon.emplace_back(base.x, base.y + west.min);
                    tile.polygon.emplace_back(base.x, base.y + west.max);
                }
                if (south.is_valid()) {
                    tile.polygon.emplace_back(base.x + south.min, base.y + rdim);
                    tile.polygon.emplace_back(base.x + south.max, base.y + rdim);
                }
                if (east.is_valid()) {
                    tile.polygon.emplace_back(base.x + rdim, base.y + east.max);
                    tile.polygon.emplace_back(base.x + rdim, base.y + east.min);
                }

                auto r_idx = world_river.at({world_x, world_y});
                tile_index[r_idx][ coord(world_x * 16 + region_x, world_y * 16 + region_y) ] = std::move(tile);
            }
        }
    }


    // generate output
    out_file << "name_df;name_en;geometry_wkt\n";

    for (auto& [r_idx, river_index] : tile_index) {
        auto river = world->world_data->rivers.at(r_idx);
        out_file << DF2UTF(Translation::translateName(&river->name, false)) << ";";
        out_file << DF2UTF(Translation::translateName(&river->name, true)) << ";";
        out_file << "MULTIPOLYGON(";
        bool first = true;
        for (auto &[tile_pos, tile] : river_index) {
                tile.polygon.emplace_back(*tile.polygon.begin());
                auto print_position = [](std::ostream &out, gcoord pos) {
                    out << pos.x << " " << -pos.y;
                };
                if (first) {
                    first = false;
                } else {
                    out_file << ",";
                }
                print_range(out_file, tile.polygon, print_position, "((", ",", "))");
        }
        out_file << ")\n";
    }


    return CR_OK;
}
