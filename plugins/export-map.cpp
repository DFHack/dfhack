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
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <numeric>
#include <optional>
#include <string>
#include <regex>
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

constexpr int wdim = 768; // dimension of a world tile
constexpr int rdim = 48;  // dimension of a region tile

static std::ofstream open_output_file(
    const std::filesystem::path& filename,
    std::ios_base::openmode mode = std::ios::out | std::ios::trunc)
{
    auto base = Core::getInstance().getConfigPath() / "map-export";
    std::filesystem::create_directories(base);
    return std::ofstream(base / filename, mode);
}


/**
 * Takes a vector of coordinates interpreted as global region tile coordinates
 * (i.e. 16 region tiles per world tile) and emits a WKT path in GIS-compatible
 * local tile coordinates (negative y-coordinates, 48 stepts per region tile)
 */
static void print_path(std::ostream &out, const std::vector<coord> &path) {
    auto scale = rdim;
    assert(path.size());
    auto print_point = [scale](std::ostream &out, const coord &pos){
        out << scale * pos.x << " " << -scale * pos.y;};
    print_range(out, path, print_point, "(", ",", ")");
}

static df::coord2d get_world_index(int16_t world_x, int16_t world_y, int8_t offset_dir) {
    constexpr auto biome_offset = std::to_array<std::pair<int16_t, int16_t>>({
        {-1, 1}, {0, 1}, {1, 1},
        {-1, 0}, {0, 0}, {1, 0},
        {-1,-1}, {0,-1}, {1,-1}
    });

    auto [diff_x, diff_y] = biome_offset[std::clamp(offset_dir, (int8_t)1, (int8_t)9) - 1];
    return {
        (int16_t)std::clamp(world_x + diff_x,0,world->world_data->world_width - 1),
        (int16_t)std::clamp(world_y + diff_y,0,world->world_data->world_height - 1)
    };
}
static df::coord2d get_world_index(coord world_pos, int8_t offset_dir) {
    return get_world_index(world_pos.x, world_pos.y, offset_dir);
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
static command_result export_regions(color_ostream &out);
static command_result export_sites(color_ostream &out);
static command_result export_rivers(color_ostream &out);
static command_result export_elevation(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(log,out).print("initializing {}\n", plugin_name);

    commands.push_back(PluginCommand(
        plugin_name,
        "Export the world map.",
        do_command));

    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters)
{
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()){
        out.printerr("This command requires a world to be loaded\n");
        return CR_WRONG_USAGE;
    }

    const bool run_all = parameters.empty() || std::ranges::find(parameters, "all") != parameters.end();

    auto result = CR_WRONG_USAGE;
    const auto run_if_selected = [&](std::string name, command_result(*export_fn)(color_ostream &out)) {
        if (result != CR_FAILURE && (run_all || std::ranges::find(parameters, name) != parameters.end()))
        {
            result = export_fn(out);
        }
    };

    run_if_selected("regions", &export_regions);
    run_if_selected("sites", &export_sites);
    run_if_selected("rivers", &export_rivers);
    run_if_selected("elevation", &export_elevation);

    return result;
}

/********************************************************************** */
/* Site Export                                                          */
/********************************************************************** */


static command_result export_sites(color_ostream &out)
{
    out.print("exporting sites ... ");
    out.flush();
    const auto start{std::chrono::steady_clock::now()};

    // ensure that we have an output file
    auto out_file = open_output_file("sites.csv");
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
            DEBUG(warning, out).print("owner ({}) of site ({}) has undefined race ({})", owner->id, site->id, owner->race);
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
    out.print("done in {:2f} s !\n", elapsed_seconds.count());

    return CR_OK;
}

/********************************************************************** */
/* Region Map Export                                                    */
/********************************************************************** */

// reverse lex-ordering (topmost leftmost tile is smallest)
bool region_order(coord p1, coord p2) {
    return p1.y < p2.y || (p1.y == p2.y && p1.x < p2.x);
};

enum class direction : int { North = 0, West = 1, South = 2, East = 3 };

static constexpr direction turn_left(direction dir) {
    return (direction)(((int)dir+1) % 4);
}

static constexpr direction turn_right(direction dir) {
    return (direction)(((int)dir+3) % 4);
}

static coord as_offset(direction dir) {
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

static coord advance(coord pos, direction dir) {
    return pos + as_offset(dir);
}


/**
 *  Look-ahead relative to the direction of movement:
 *
 *  L ↑ R    | L
 *  - + -  → + →
 *    ↑      | R
 */
static std::pair<bool,bool> ahead(const std::vector<coord> &component, coord pos, direction dir) {
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

// standard DFS-based connected components algorithm.
static auto connected_components(std::vector<df::coord2d> &region) {

    static std::array<coord,4> directions{
        as_offset(direction::North),
        as_offset(direction::West),
        as_offset(direction::South),
        as_offset(direction::East)
    };

    // component_assignment[i] is the component id of region[i] (0 means unassigned)
    std::vector<unsigned int> component_assignment;
    component_assignment.resize(region.size(),0);

    // (indices of) region tiles in the current component that have been discovered but not yet explored
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
                const auto it = std::ranges::lower_bound(region, pos + offset, region_order);
                if (it != region.end() && *it == pos + offset) {
                    auto n_idx = std::distance(region.begin(), it);
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

    return components;
}


// create outlines around connected components using a clockwise walk around the perimeter
// (exploits that components do not have inclusions)
static auto create_outlines(color_ostream &out, std::vector<std::vector<df::coord2d>> components) {
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
                // in front of a wall: turn left
                path.push_back(current_position);
                current_direction = turn_left(current_direction);
            }
            else if (!left && !right) {
                // no walls ahead: turn right
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
    return paths;
}

static command_result export_regions(color_ostream &out)
{
    out.print("{} / {} region map tiles loaded\n",
        world->world_data->midmap_data.region_details.size(),
        world->world_data->world_width * world->world_data->world_height
    );
    out.print("exporting map ... ");
    out.flush();
    const auto start{std::chrono::steady_clock::now()};

    // ensure that we have an output file
    auto out_file = open_output_file("map.csv");
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

    // map world tile coord -> vector of region tiles referencing world title for biome information
    std::unordered_map<coord,std::vector<coord>> world_tile_region;

    // iterating over the region details allows the user to do partial map exports
    // by manually scrolling on the zoomed embark selection map
    for (auto const region_details : world->world_data->midmap_data.region_details) {
        auto &world_pos = region_details->pos;
        for (int region_x = 0; region_x < 16; ++region_x) {
            for (int region_y = 0; region_y < 16; ++region_y)
            {
                auto biome_tile = get_world_index(world_pos, region_details->biome[region_x][region_y]);
                world_tile_region[biome_tile].emplace_back(world_pos * 16 + coord(region_x, region_y));
            }
        }
    }

    for (auto& [biome_tile, region] : world_tile_region)
    {
        assert(region.size() > 0);

         // sorting the region provides O(log n) membership test.
         std::ranges::sort(region, region_order);

        /**
         * Phase I : compute the connected components of the world tile region
         * using DFS algorithm. (except for the southern and eastern map edge,
         * all world tile regions should have a single component)
         */

        auto components = connected_components(region);

        /* Phase II : create paths by clockwise traversal along the outside of every component */
        /**
         * Note: DF uses "picture coordinates" (positive y values go "south")
         * while in GIS software positve y values go "north". Thus, [print_path]
         * negates the y-coordinates, turning the clockwise traversals into
         * counterclockwise traversals as specified by WKT.
         * https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry
         */

        auto paths = create_outlines(out, components);

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
    out.print("done in {:2f} s !\n", elapsed_seconds.count());
    return CR_OK;
}

/********************************************************************** */
/* River Export                                                         */
/********************************************************************** */

//
// used for global coordinates at local tile granularity (129*768 = 99072 doesn't fit into df::coord2d)
template<typename T>
struct gcoord {
    T x, y;

    gcoord() = default;
    gcoord(T x, T y) : x(x), y(y) {}

    template<typename U>
    explicit gcoord(const gcoord<U> &other) : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {};

    gcoord operator+(const gcoord &other) const
    {
        return {x + other.x, y + other.y};
    }

    gcoord operator-(const gcoord &other) const
    {
        return {x - other.x, y - other.y};
    }

    gcoord operator*(T s) const
    {
        return {x * s, y * s};
    }

    gcoord operator/(T s) const
    {
        return {x / s, y / s};
    }

    static T dotp(const gcoord& a, const gcoord& b)
    {
        return a.x * b.x + a.y * b.y;
    }
};

// linear interpolation between two points
static gcoord<double> lerp(gcoord<double> A, gcoord<double> B, double t)
{
    return A + (B - A) * t;
}

// "orthogonal" projection of the point P onto the line segment AB
static gcoord<double> project_onto_line(gcoord<double> A, gcoord<double> B, gcoord<double> P)
{
    auto AB = B - A;
    auto AP = P - A;
    auto t = gcoord<double>::dotp(AP, AB) / gcoord<double>::dotp(AB, AB);
    return A + AB * std::clamp(t, 0.0, 1.0);
}


struct river_tile {
    using polygon_t = std::vector<gcoord<int>>;
    polygon_t polygon;
};

/**
 * To get reasonably-looking river confluences, we project the centroid of all
 * river gates onto the line segments between the river gates and then
 * interpolate between the centroid and the projection point.
 */
static void fix_confluence_tiles(river_tile::polygon_t& polygon)
{
    assert(polygon.size() > 4 && polygon.size() % 2 == 0);

    auto centroid =
        gcoord<double>(std::reduce(polygon.begin(), polygon.end())) / static_cast<double>(polygon.size());

    river_tile::polygon_t inset_polygon;

    for (size_t i = 0; i + 1 < polygon.size(); i += 2) {
        auto pair_start = polygon[i];
        auto pair_end = polygon[i + 1];
        auto next_pair_start = polygon[(i + 2) % polygon.size()];

        inset_polygon.emplace_back(pair_start);
        inset_polygon.emplace_back(pair_end);

        auto projection = project_onto_line(gcoord<double>(pair_end), gcoord<double>(next_pair_start), centroid);
        auto inset_point = lerp(projection, centroid, 0.6);
        inset_polygon.emplace_back(gcoord<int>(inset_point));
    }

    polygon = std::move(inset_polygon);
}



struct gate {
    int active,min,max;

    static gate get(
        const df::world_region_details *const region_details,
        int region_x, int region_y, direction dir
    ) {
        auto& vertical = region_details->rivers_vertical;
        auto& horizontal = region_details->rivers_horizontal;
        switch (dir) {
            case direction::North:
                return {
                    vertical.active[region_x][region_y],
                    vertical.x_min[region_x][region_y],
                    vertical.x_max[region_x][region_y]
                };
            case direction::West:
                return {
                    horizontal.active[region_x][region_y],
                    horizontal.y_min[region_x][region_y],
                    horizontal.y_max[region_x][region_y]
                };
            case direction::South:
                return {
                    vertical.active[region_x][region_y+1],
                    vertical.x_min[region_x][region_y+1],
                    vertical.x_max[region_x][region_y+1]
                };
            case direction::East:
                return {
                    horizontal.active[region_x+1][region_y],
                    horizontal.y_min[region_x+1][region_y],
                    horizontal.y_max[region_x+1][region_y]
                };
            default:
                assert(false);
                return {};
        }
    }

    bool is_valid() const {
        return active != 0 && min != -30000 && max != -30000;
    }
};


static bool is_land(const df::world_region_details *const region_details, int16_t region_x, int16_t region_y) {
    CHECK_NULL_POINTER(region_details);
    auto [world_x, world_y] = region_details->pos;
    auto biome_tile = get_world_index(world_x, world_y, region_details->biome[region_x][region_y]);
    auto region_map_entry = Maps::getRegionBiome(biome_tile);
    CHECK_NULL_POINTER(region_map_entry);
    return region_map_entry->elevation >= 100 && !region_map_entry->flags.is_set(df::enums::region_map_entry_flags::is_lake);
}

static command_result export_rivers(color_ostream &out)
{
    out.print("exporting rivers ... ");
    out.flush();
    const auto start{std::chrono::steady_clock::now()};

    // ensure that we have an output file
    auto out_file = open_output_file("rivers.csv");
    if (!out_file) {
        return CR_FAILURE;
    }


    /**
     * In DwarfFortress, a world tile can only have at most one river. As a
     * consequence, rivers do not end at a confluence point but they already
     * change their name when they enter the world tile containing the
     * confluence. Grouping river tiles by world tile allows us to only output
     * one multipolygon feature per river.
     */

    // create lookup table for rivers based on world tile coordinates (index into world_data->rivers)
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

    // river idx -> river tiles
    std::unordered_map<size_t,std::vector<river_tile>> tile_index;

    for (auto const region_details : world->world_data->midmap_data.region_details) {
        auto [world_x, world_y] = region_details->pos;
        for (int region_x = 0; region_x < 16; ++region_x) {
            for (int region_y = 0; region_y < 16; ++region_y)
            {
                gcoord<int> base = { world_x * wdim + region_x * rdim, world_y * wdim + region_y * rdim };

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

                if (tile.polygon.size() > 4) {
                    fix_confluence_tiles(tile.polygon);
                }

                // locate the river using world coordinates and assign the tile
                auto r_idx = world_river.at({world_x, world_y});
                tile_index[r_idx].push_back(std::move(tile));
            }
        }
    }


    // generate output
    out_file << "name_df;name_en;geometry_wkt\n";

    for (auto& [r_idx, river_tiles] : tile_index) {
        auto river = world->world_data->rivers.at(r_idx);
        out_file << DF2UTF(Translation::translateName(&river->name, false)) << ";";
        out_file << DF2UTF(Translation::translateName(&river->name, true)) << ";";
        out_file << "MULTIPOLYGON(";
        bool first = true;
        for (auto &tile : river_tiles) {
                // close the polygon
                tile.polygon.emplace_back(*tile.polygon.begin());
                auto print_position = [](std::ostream &out, gcoord<int> pos) {
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

    const auto finish{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{finish - start};
    out.print("done in {:2f} s !\n", elapsed_seconds.count());

    return CR_OK;
}

/********************************************************************** */
/* Elevation Map Export                                                 */
/********************************************************************** */

template<typename T>
class matrix {
    std::size_t ncols;
    std::size_t nrows;
    std::vector<T> _data;

public:
    matrix(std::size_t cols, std::size_t rows) : ncols(cols), nrows(rows), _data(cols * rows) {};

    T& operator()(std::size_t col, std::size_t row) {
        return _data[row * ncols + col];
    };

    T* data() { return _data.data(); };
    std::size_t size() { return _data.size(); };
};


static command_result export_elevation(color_ostream &out)
{
    out.print("exporting elevation ... ");
    out.flush();
    const auto start{std::chrono::steady_clock::now()};

    // ensure that we have an output file
    auto data_file = open_output_file("elevation.dat", std::ios::out | std::ios::trunc | std::ios::binary);
    auto vrt_file = open_output_file("elevation.vrt");
    if (!data_file || !vrt_file) {
        return CR_FAILURE;
    }

    auto world_width = world->world_data->world_width * 16;
    auto world_height = world->world_data->world_height * 16;

    matrix<int16_t> height_map(world_width, world_height);

    for (auto const region_details : world->world_data->midmap_data.region_details) {
        auto world_x = region_details->pos.x;
        auto world_y = region_details->pos.y;
        for (int region_x = 0; region_x < 16; ++region_x) {
            for (int region_y = 0; region_y < 16; ++region_y)
            {
                auto elevation = region_details->elevation[region_x][region_y];
                height_map(16 * world_x + region_x, 16 * world_y + region_y) = elevation;
            }
        }
    }

    data_file.write(reinterpret_cast<const char*>(height_map.data()), height_map.size() * sizeof(int16_t));

    // provide an interpretation for the elevation map consistent with the remaining exports
    const std::string vrtTemplate =
R"(<VRTDataset rasterXSize="{WIDTH}" rasterYSize="{HEIGHT}">
<SRS>EPSG:3857</SRS>
<GeoTransform>0,48,0,0,0,-48</GeoTransform>
<VRTRasterBand dataType="Int16" band="1" subClass="VRTRawRasterBand">
    <SourceFilename relativeToVRT="1">elevation.dat</SourceFilename>
    <ImageOffset>0</ImageOffset>
    <PixelOffset>2</PixelOffset>
    <LineOffset>{LINE_OFFSET}</LineOffset>
    <ByteOrder>LSB</ByteOrder>
</VRTRasterBand>
</VRTDataset>
)";

    auto vrt = std::regex_replace(vrtTemplate, std::regex("\\{WIDTH\\}"), std::to_string(world_width));
    vrt = std::regex_replace(vrt, std::regex("\\{HEIGHT\\}"), std::to_string(world_height));
    vrt = std::regex_replace(vrt, std::regex("\\{LINE_OFFSET\\}"), std::to_string(world_width * sizeof(int16_t)));
    vrt_file << vrt;

    const auto finish{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{finish - start};
    out.print("done in {:2f} s !\n", elapsed_seconds.count());

    return CR_OK;
}
