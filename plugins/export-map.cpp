#include "Debug.h"
#include "Error.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include "modules/Maps.h"
#include "modules/Translation.h"

#include "df/entity_raw.h"
#include "df/creature_raw.h"
#include "df/historical_entity.h"
#include "df/world.h"
#include "df/map_block.h"
#include "df/world_data.h"
#include "df/world_site.h"
#include "df/region_map_entry.h"
#include "df/world_region.h"
#include "df/world_landmass.h"
#include "df/world_region_details.h"

#include "gdal/ogrsf_frmts.h"

#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <deque>


using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("export-map");

REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(exportmap, log);
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

auto setGeometry(OGRFeature *feature, double x, double y, double dimx, double dimy = 0) {
    if (dimy == 0) { dimy = dimx; }
    auto poly = new OGRPolygon();
    auto boundary = new OGRLinearRing();
    y = -y; // in GIS negative y-coordinates mean further south
    boundary->addPoint(x,y);
    boundary->addPoint(x,y-dimy);
    boundary->addPoint(x+dimx,y-dimy);
    boundary->addPoint(x+dimx,y);
    boundary->closeRings();
    //the "Directly" variants assume ownership of the objects created above
    poly->addRingDirectly(boundary);
    feature->SetGeometryDirectly( poly );
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
    world_x = std::min(std::max((int16_t)0,world_x),(int16_t)(world->world_data->world_width - 1));
    world_y = std::min(std::max((int16_t)0,world_y),(int16_t)(world->world_data->world_height - 1));
    return df::coord2d(world_x, world_y);
}

auto create_field(OGRLayer *layer, std::string name, OGRFieldType type, int width = 0, OGRFieldSubType subtype = OFSTNone) {
    OGRFieldDefn field( name.c_str() , type );
    if (subtype != OFSTNone) {
        field.SetSubType(subtype);
    }
    if (width != 0) {
        field.SetWidth(width);
    }
    // this should create a copy internally
    if( layer->CreateField( &field ) != OGRERR_NONE ){
        throw CR_FAILURE;
    }
}

// PROJ.4 description of EPSG:3857 (https://epsg.io/3857)
static const char* EPSG_3857 = "+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=0 +x_0=0 +y_0=0 +k=1 +units=m +nadgrids=@null +wktext +no_defs +type=crs";

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

// static int wdim = 768; // dimension of a world tile
static int rdim = 48;  // dimension of a region tile

static command_result export_region_tiles(color_ostream &out);
static command_result export_sites(color_ostream &out);

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
    else
    {
        return export_region_tiles(out);
    }
}

static command_result export_sites(color_ostream &out)
{
    out.print("exporting sites... ");
    out.flush();
    const auto start{std::chrono::steady_clock::now()};

    // set up coordinate system
    OGRSpatialReference CRS;
    if (CRS.importFromProj4(EPSG_3857) != OGRERR_NONE) {
        out.printerr("could not set up coordinate system");
        return CR_FAILURE;
    }

    // set up output driver
    GDALAllRegister();
    const char *driver_name = "SQLite";
    const char *extension = "sqlite";
    auto driver = GetGDALDriverManager()->GetDriverByName(driver_name);
    if (!driver) {
        out.printerr("could not find sqlite driver");
        return CR_FAILURE;
    }

    // create a dataset and associate it to a file
    std::string sites("sites.");
    sites.append(extension);
    const char* options[] = { "SPATIALITE=YES", nullptr };
    auto dataset = driver->Create( sites.c_str(), 0, 0, 0, GDT_Unknown, options);
    if (!dataset) {
        out.printerr("could not create dataset");
        return CR_FAILURE;
    }

    // create a layer for the biome data
    // const char* format[] = { "FORMAT=WKT", nullptr };
    auto layer = dataset->CreateLayer( "world_sites", &CRS, wkbPolygon, nullptr );
    if (!layer) {
        out.printerr("could not create layer");
        return CR_FAILURE;
    }

    try {
        create_field(layer, "site_id", OFTInteger);
        create_field(layer, "civ_id", OFTInteger);
        create_field(layer, "created_year", OFTInteger);
        create_field(layer, "cur_owner_id", OFTInteger);

        create_field(layer, "type", OFTString, 15);

        create_field(layer, "site_name_df", OFTString, 100);
        create_field(layer, "site_name_en", OFTString, 100);

        create_field(layer, "civ_name_df", OFTString, 100);
        create_field(layer, "civ_name_en", OFTString, 100);

        create_field(layer, "site_government_df", OFTString, 100);
        create_field(layer, "site_government_en", OFTString, 100);

        create_field(layer, "owner_race", OFTString, 15);

        // create_field(layer, "local_ruler", OFTString, 100);

    }
    catch (const DFHack::command_result& r) {
        out.printerr("could not create fields for output layer");
        return r;
    }

    if (dataset->StartTransaction() != OGRERR_NONE) {
        out.printerr("could not start a transaction\n");
    }

    for (auto const site : world->world_data->sites)
    {

        auto feature = OGRFeature::CreateFeature( layer->GetLayerDefn() );

        setGeometry(
            feature,
            site->global_min_x * rdim,
            site->global_min_y * rdim,
            (site->global_max_x - site->global_min_x + 1) * rdim,
            (site->global_max_y - site->global_min_y + 1) * rdim
        );
        feature->SetField( "site_id", site->id );
        feature->SetField( "type", ENUM_KEY_STR(world_site_type, site->type).c_str() );
        #define SET_FIELD(name) feature->SetField( #name, site->name)
        SET_FIELD(civ_id);
        SET_FIELD(created_year);
        SET_FIELD(cur_owner_id);
        #undef SET_FIELD

        #define TRANSLATE_NAME(field_name, name_object)\
            feature->SetField((#field_name"_df"), DF2UTF(Translation::translateName(&name_object, false)).c_str());\
            feature->SetField((#field_name"_en"), DF2UTF(Translation::translateName(&name_object, true)).c_str());

        TRANSLATE_NAME(site_name, site->name)

        auto civ = df::historical_entity::find(site->civ_id);
        if (civ) { TRANSLATE_NAME(civ_name,civ->name) }

        auto owner = df::historical_entity::find(site->cur_owner_id);
        if (owner) {
            TRANSLATE_NAME(site_government,owner->name)
            auto race = df::creature_raw::find(owner->race);
            if (!race){
                race = df::creature_raw::find(civ->race);
            }
            if (race) {
                feature->SetField( "owner_race", race->name[2].c_str() );
            }
        }


        // this updates the feature with the id it receives in the layer
        if( layer->CreateFeature( feature ) != OGRERR_NONE )
            return CR_FAILURE;
        // but we don't care and destroy the feature
        OGRFeature::DestroyFeature( feature );
    }

    dataset->CommitTransaction();

    GDALClose( dataset );
    const auto finish{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{finish - start};
    out.print("done in %f ms !\n", elapsed_seconds.count());
    return CR_OK;
}

/********************************************************************** */

template<>
struct std::hash<df::coord2d>
{
    std::size_t operator()(const df::coord2d& pos) const noexcept
    {
        // hashing is easy, if values are smaller than the hash
        return ((std::size_t)pos.x << 16) | (std::size_t)pos.y;
    }
};

using coord = df::coord2d;

enum class direction : int { North = 0, West = 1, South = 2, East = 3 };

direction turn_left(direction dir) {
    return (direction)(((int)dir+1) % 4);
}

direction turn_right(direction dir) {
    return (direction)(((int)dir+3) % 4);
}

df::coord2d as_offset(direction dir) {
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

auto print_path(std::vector<coord> path, int scale = rdim) {
    assert(path.size());
    // create the path in clockwise direction, so that it becomes counterclockwise with the y-flip
    std::ranges::reverse(path);
    std::ostringstream out;
    out << scale * path[0].x << " " << -scale * path[0].y;
    for (size_t i = 1; i < path.size(); ++i) {
        out << "," << scale * path[i].x << " " << -scale * path[i].y;
    }
    return out.str();
}

std::pair<bool,bool> ahead(const std::vector<coord> &component, coord pos, direction dir) {
    auto test = [&](int16_t x, int16_t y){
        coord offset{x,y};
        return std::find(component.begin(), component.end(), pos + offset) != component.end();
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
    out_file << "world_x; world_y; num_tiles; num_components; biome_type; boundary_wkt" << std::endl;

    /*
    try {
        create_field(layer, "region_id", OFTInteger);
        create_field(layer, "region_name_en", OFTString, 100);
        create_field(layer, "region_name_df", OFTString, 100);
        create_field(layer, "landmass_id", OFTInteger);
        create_field(layer, "landmass_name_en", OFTString, 100);
        create_field(layer, "landmass_name_df", OFTString, 100);

        create_field(layer, "biome_type", OFTString, 32);
        create_field(layer, "surroundings", OFTString, 16);
        create_field(layer, "elevation", OFTInteger);

        create_field(layer, "evilness", OFTInteger);
        create_field(layer, "savagery", OFTInteger);
        create_field(layer, "volcanism", OFTInteger);
        create_field(layer, "drainage", OFTInteger);
        create_field(layer, "temperature", OFTInteger);
        create_field(layer, "vegetation", OFTInteger);
        create_field(layer, "rainfall", OFTInteger);
        create_field(layer, "snowfall", OFTInteger);
        create_field(layer, "salinity", OFTInteger);

        create_field(layer, "reanimating", OFTInteger, 0, OFSTBoolean);
        create_field(layer, "has_bogeymen", OFTInteger, 0, OFSTBoolean);

    } catch (const DFHack::command_result& r) {
        out.printerr("could not create fields for output layer");
        return r;
    }
    */



    // iterating over the region details allows the user to do partial map exports
    // by manually scrolling on the embark site selection

    std::unordered_map<df::coord2d,std::vector<df::coord2d>> world_tile_region;

    for (auto const region_details : world->world_data->midmap_data.region_details) {
        auto world_x = region_details->pos.x;
        auto world_y = region_details->pos.y;
        for (int region_x = 0; region_x < 16; ++region_x) {
            for (int region_y = 0; region_y < 16; ++region_y) {

                // auto feature = OGRFeature::CreateFeature( layer->GetLayerDefn() );
                // setGeometry(
                //     feature,
                //     (double)(world_x * wdim + region_x * rdim),
                //     (double)(world_y * wdim + region_y * rdim),
                //     rdim
                // );

                // get the world tile coordinates used for the biome information of the local region tile
                auto biome_tile = get_world_index(world_x, world_y, region_details->biome[region_x][region_y]);
                world_tile_region[biome_tile].emplace_back(16 * world_x + region_x, 16 * world_y + region_y);

            }
        }
    }

    out.print("processing %ld world tile regions\n", world_tile_region.size());

    auto region_order = [](df::coord2d p1, df::coord2d p2) {
        return p1.y < p2.y || (p1.y == p2.y && p1.x < p2.x);
    };
    static std::array<coord,4> directions{
        as_offset(direction::North),
        as_offset(direction::West),
        as_offset(direction::South),
        as_offset(direction::East)
    };

    for (auto& [biome_tile, region] : world_tile_region)
    {
        assert(region.size() > 0);
        // compute the connected components of the world tile region
        std::ranges::sort(region, region_order);

        std::vector<unsigned int> component_assignment;
        component_assignment.resize(region.size(),0);
        std::deque<size_t> agenda;

        auto current_component = 0;
        for (size_t i = 0; i < region.size(); ++i) {
            if (component_assignment[i]) {
                continue;
            } else {
                ++current_component;
                component_assignment[i] = current_component;
                agenda.push_back(i);
            }
            while(!agenda.empty()) {
                auto pos_idx = agenda.front(); agenda.pop_front();
                auto pos = region[pos_idx];

                for (auto const offset : directions) {
                    // FIXME: use something O(log n) instead of the O(n) find
                    auto n_it = std::find(region.begin(), region.end(), pos + offset);
                    auto n_idx = std::distance(region.begin(), n_it);
                    if (n_it != region.end() && component_assignment[n_idx] == 0) {
                        component_assignment[n_idx] = current_component;
                        agenda.push_back(n_idx);
                    }
                }
            }
        }

        // assert that all parts of the region are accounted for
        assert(std::ranges::all_of(component_assignment, [](int comp){ return comp > 0;}));

        std::vector<std::vector<coord>> components;
        components.resize(current_component);
        for (size_t i = 0; i < region.size(); ++i) {
            components.at(component_assignment.at(i) - 1).push_back(region.at(i));
        }

        std::vector<std::vector<coord>> paths;
        for (auto const &component : components) {

            auto start = component.at(0);
            std::vector<coord> path;
            path.push_back(start);

            auto current_direction = direction::South;
            auto current_position = advance(start,current_direction);

            while (current_position != start)
            {
                auto [left, right] = ahead(component, current_position, current_direction);
                if (left && right) {
                    // in front of a wall: turn right
                    path.push_back(current_position);
                    current_direction = turn_right(current_direction);
                }
                else if (!left && !right) {
                    // no walls ahead: turn left
                    path.push_back(current_position);
                    current_direction = turn_left(current_direction);
                }
                assert(! (!left && right)); // shape has a hole or is not connected
                current_position = advance(current_position, current_direction);
            }
            // close the path
            path.push_back(current_position);
            paths.push_back(std::move(path));
            path.clear();
        }

        out_file << biome_tile.x << ";" << biome_tile.y << ";" << region.size() << ";" << components.size() << ";";
        out_file << ENUM_KEY_STR(biome_type,Maps::getBiomeType(biome_tile.x, biome_tile.y)) << ";";
        assert(paths.size() > 0);
        if (paths.size() == 1) {
            out_file << "POLYGON((" << print_path(paths[0]) << "))" << std::endl;
        } else {
            out_file << "MULTIPOLYGON(" << "((" << print_path(paths[0]) << "))";
            for (size_t i = 1; i < paths.size(); ++i) {
                out_file << ",((" << print_path(paths[i]) << "))";
            }
            out_file << ")" << std::endl;
        }
    }


                // feature->SetField( "biome_type", ENUM_KEY_STR(biome_type,Maps::getBiomeType(biome_x,biome_y)).c_str() );

                // gets supplementary information from the world tile
                // auto& region_map_entry = world->world_data->region_map[biome_x][biome_y];
                // #define SET_FIELD(name) feature->SetField( #name, region_map_entry.name)
                // SET_FIELD(region_id);
                // SET_FIELD(landmass_id);
                // SET_FIELD(evilness);
                // SET_FIELD(savagery);
                // SET_FIELD(volcanism);
                // SET_FIELD(drainage);
                // SET_FIELD(temperature);
                // SET_FIELD(vegetation);
                // SET_FIELD(rainfall);
                // SET_FIELD(snowfall);
                // SET_FIELD(salinity);
                // #undef SET_FIELD

                // feature->SetField( "surroundings", describe_surroundings(region_map_entry.savagery, region_map_entry.evilness));

                // auto region = df::world_region::find(region_map_entry.region_id);
                // if (region) {
                //     auto region_name_en = DF2UTF(Translation::translateName(&region->name, true));
                //     feature->SetField( "region_name_en", region_name_en.c_str());
                //     auto region_name_df = DF2UTF(Translation::translateName(&region->name, false));
                //     feature->SetField( "region_name_df", region_name_df.c_str());
                //     feature->SetField("reanimating", region->reanimating);
                //     feature->SetField("has_bogeymen", region->has_bogeymen);
                // }
                // auto landmass = df::world_landmass::find(region_map_entry.landmass_id);
                // if (landmass) {
                //     auto landmass_name_en = DF2UTF(Translation::translateName(&landmass->name, true));
                //     feature->SetField( "landmass_name_en", landmass_name_en.c_str());
                //     auto landmass_name_df = DF2UTF(Translation::translateName(&landmass->name, false));
                //     feature->SetField( "landmass_name_df", landmass_name_df.c_str());
                // }


    const auto finish{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{finish - start};
    out.print("done in %f ms !\n", elapsed_seconds.count());
    return CR_OK;
}
