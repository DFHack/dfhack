//Blueprint
//By cdombroski
//Translates a region of tiles specified by the cursor and arguments/prompts into a series of blueprint files suitable for digfort/buildingplan/quickfort

#include <algorithm>

#include <Console.h>
#include <PluginManager.h>

#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/MapCache.h"

#include "df/building_axle_horizontalst.h"
#include "df/building_bridgest.h"
#include "df/building_constructionst.h"
#include "df/building_furnacest.h"
#include "df/building_rollersst.h"
#include "df/building_screw_pumpst.h"
#include "df/building_siegeenginest.h"
#include "df/building_trapst.h"
#include "df/building_water_wheelst.h"
#include "df/building_workshopst.h"

using std::string;
using std::endl;
using std::vector;
using std::ofstream;
using std::swap;
using std::find;
using std::pair;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("blueprint");

enum phase {DIG=1, BUILD=2, PLACE=4, QUERY=8};

command_result blueprint(color_ostream &out, vector <string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand("blueprint", "Convert map tiles into a blueprint", blueprint, false));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    return CR_OK;
}

command_result help(color_ostream &out)
{
    out << "blueprint width height depth name [dig] [build] [place] [query]" << endl
        << " width, height, depth: area to translate in tiles" << endl
        << " name: base name for blueprint files" << endl
        << " dig: generate blueprints for digging" << endl
        << " build: generate blueprints for building" << endl
        << " place: generate blueprints for stockpiles" << endl
        << " query: generate blueprints for querying (room designations)" << endl
        << " defaults to generating all blueprints" << endl
        << endl
        << "blueprint translates a portion of your fortress into blueprints suitable for" << endl
        << " digfort/fortplan/quickfort. Blueprints are created in the DF folder with names" << endl
        << " following a \"name-phase.csv\" pattern. Translation starts at the current" << endl
        << " cursor location and includes all tiles in the range specified." << endl;
    return CR_OK;
}

pair<uint32_t, uint32_t> get_building_size(df::building* b)
{
    return pair<uint32_t, uint32_t>(b->x2 - b->x1 + 1, b->y2 - b->y1 + 1);
}

char get_tile_dig(MapExtras::MapCache mc, int32_t x, int32_t y, int32_t z)
{
    df::tiletype tt = mc.tiletypeAt(DFCoord(x, y, z));
    df::tiletype_shape ts = tileShape(tt);
    switch (ts)
    {
    case tiletype_shape::EMPTY:
    case tiletype_shape::RAMP_TOP:
        return 'h';
    case tiletype_shape::FLOOR:
    case tiletype_shape::BOULDER:
    case tiletype_shape::PEBBLES:
    case tiletype_shape::BROOK_TOP:
        return 'd';
    case tiletype_shape::FORTIFICATION:
        return 'F';
    case tiletype_shape::STAIR_UP:
        return 'u';
    case tiletype_shape::STAIR_DOWN:
        return 'j';
    case tiletype_shape::STAIR_UPDOWN:
        return 'i';
    case tiletype_shape::RAMP:
        return 'r';
    default:
        return ' ';

    }
}

string get_tile_build(uint32_t x, uint32_t y, df::building* b)
{
    if (! b)
        return " ";
    bool at_nw_corner = x == b->x1 && y == b->y1;
    bool at_se_corner = x == b->x2 && y == b->y2;
    bool at_center = x == b->centerx && y == b->centery;
    pair<uint32_t, uint32_t> size = get_building_size(b);
    stringstream out;// = stringstream();
    switch(b->getType())
    {
    case building_type::Armorstand:
        return "a";
    case building_type::Bed:
        return "b";
    case building_type::Chair:
        return "c";
    case building_type::Door:
        return "d";
    case building_type::Floodgate:
        return "x";
    case building_type::Cabinet:
        return "f";
    case building_type::Box:
        return "h";
        //case building_type::Kennel is missing
    case building_type::FarmPlot:
        if(!at_nw_corner)
            return "`";
        out << "p(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::Weaponrack:
        return "r";
    case building_type::Statue:
        return "s";
    case building_type::Table:
        return "t";
    case building_type::RoadPaved:
        if(! at_nw_corner)
            return "`";
        out << "o(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::RoadDirt:
        if(! at_nw_corner)
            return "`";
        out << "O(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::Bridge:
        if(! at_nw_corner)
            return "`";
        switch(((df::building_bridgest*) b)->direction)
        {
        case df::building_bridgest::T_direction::Down:
            out << "gx";
            break;
        case df::building_bridgest::T_direction::Left:
            out << "ga";
            break;
        case df::building_bridgest::T_direction::Up:
            out << "gw";
            break;
        case df::building_bridgest::T_direction::Right:
            out << "gd";
            break;
        case df::building_bridgest::T_direction::Retracting:
            out << "gs";
            break;
        }
        out << "(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::Well:
        return "l";
    case building_type::SiegeEngine:
        if (! at_center)
            return "`";
        return ((df::building_siegeenginest*) b)->type == df::siegeengine_type::Ballista ? "ib" : "ic";
    case building_type::Workshop:
        if (! at_center)
            return "`";
        switch (((df::building_workshopst*) b)->type)
        {
        case workshop_type::Leatherworks:
            return "we";
        case workshop_type::Quern:
            return "wq";
        case workshop_type::Millstone:
            return "wM";
        case workshop_type::Loom:
            return "wo";
        case workshop_type::Clothiers:
            return "wk";
        case workshop_type::Bowyers:
            return "wb";
        case workshop_type::Carpenters:
            return "wc";
        case workshop_type::MetalsmithsForge:
            return "wf";
        case workshop_type::MagmaForge:
            return "wv";
        case workshop_type::Jewelers:
            return "wj";
        case workshop_type::Masons:
            return "wm";
        case workshop_type::Butchers:
            return "wu";
        case workshop_type::Tanners:
            return "wn";
        case workshop_type::Craftsdwarfs:
            return "wr";
        case workshop_type::Siege:
            return "ws";
        case workshop_type::Mechanics:
            return "wt";
        case workshop_type::Still:
            return "wl";
        case workshop_type::Farmers:
            return "ww";
        case workshop_type::Kitchen:
            return "wz";
        case workshop_type::Fishery:
            return "wh";
        case workshop_type::Ashery:
            return "wy";
        case workshop_type::Dyers:
            return "wd";
        case workshop_type::Custom:
            //can't do anything with custom workshop
            return "`";
        }
    case building_type::Furnace:
        if (! at_center)
            return "`";
        switch (((df::building_furnacest*) b)->type)
        {
        case furnace_type::WoodFurnace:
            return "ew";
        case furnace_type::Smelter:
            return "es";
        case furnace_type::GlassFurnace:
            return "eg";
        case furnace_type::Kiln:
            return "ek";
        case furnace_type::MagmaSmelter:
            return "el";
        case furnace_type::MagmaGlassFurnace:
            return "ea";
        case furnace_type::MagmaKiln:
            return "en";
        case furnace_type::Custom:
            //can't do anything with custom furnace
            return "`";
        }
    case building_type::WindowGlass:
        return "y";
    case building_type::WindowGem:
        return "Y";
    case building_type::Construction:
        switch (((df::building_constructionst*) b)->type)
        {
        case construction_type::Fortification:
            return "CF";
        case construction_type::Wall:
            return "CW";
        case construction_type::Floor:
            return "Cf";
        case construction_type::UpStair:
            return "Cu";
        case construction_type::DownStair:
            return "Cj";
        case construction_type::UpDownStair:
            return "Cx";
        case construction_type::Ramp:
            return "Cr";
        case construction_type::TrackN:
            return "trackN";
        case construction_type::TrackS:
            return "trackS";
        case construction_type::TrackE:
            return "trackE";
        case construction_type::TrackW:
            return "trackW";
        case construction_type::TrackNS:
            return "trackNS";
        case construction_type::TrackNE:
            return "trackNE";
        case construction_type::TrackNW:
            return "trackNW";
        case construction_type::TrackSE:
            return "trackSE";
        case construction_type::TrackSW:
            return "trackSW";
        case construction_type::TrackEW:
            return "trackEW";
        case construction_type::TrackNSE:
            return "trackNSE";
        case construction_type::TrackNSW:
            return "trackNSW";
        case construction_type::TrackNEW:
            return "trackNEW";
        case construction_type::TrackSEW:
            return "trackSEW";
        case construction_type::TrackNSEW:
            return "trackNSEW";
        case construction_type::TrackRampN:
            return "trackrampN";
        case construction_type::TrackRampS:
            return "trackrampS";
        case construction_type::TrackRampE:
            return "trackrampE";
        case construction_type::TrackRampW:
            return "trackrampW";
        case construction_type::TrackRampNS:
            return "trackrampNS";
        case construction_type::TrackRampNE:
            return "trackrampNE";
        case construction_type::TrackRampNW:
            return "trackrampNW";
        case construction_type::TrackRampSE:
            return "trackrampSE";
        case construction_type::TrackRampSW:
            return "trackrampSW";
        case construction_type::TrackRampEW:
            return "trackrampEW";
        case construction_type::TrackRampNSE:
            return "trackrampNSE";
        case construction_type::TrackRampNSW:
            return "trackrampNSW";
        case construction_type::TrackRampNEW:
            return "trackrampNEW";
        case construction_type::TrackRampSEW:
            return "trackrampSEW";
        case construction_type::TrackRampNSEW:
            return "trackrampNSEW";
        }
    case building_type::Shop:
        if (! at_center)
            return "`";
        return "z";
    case building_type::AnimalTrap:
        return "m";
    case building_type::Chain:
        return "v";
    case building_type::Cage:
        return "j";
    case building_type::TradeDepot:
        if (! at_center)
            return "`";
        return "D";
    case building_type::Trap:
        switch (((df::building_trapst*) b)->trap_type)
        {
        case trap_type::StoneFallTrap:
            return "Ts";
        case trap_type::WeaponTrap:
            return "Tw";
        case trap_type::Lever:
            return "Tl";
        case trap_type::PressurePlate:
            return "Tp";
        case trap_type::CageTrap:
            return "Tc";
        case trap_type::TrackStop:
            df::building_trapst* ts = (df::building_trapst*) b;
            out << "CS";
            if (ts->use_dump)
            {
                if (ts->dump_x_shift == 0)
                {
                    if (ts->dump_y_shift > 0)
                        out << "dd";
                    else
                        out << "d";
                }
                else
                {
                    if (ts->dump_x_shift > 0)
                        out << "ddd";
                    else
                        out << "dddd";
                }
            }
            switch (ts->friction)
            {
            case 10:
                out << "a";
            case 50:
                out << "a";
            case 500:
                out << "a";
            case 10000:
                out << "a";
            }
            return out.str();
        }
    case building_type::ScrewPump:
        if (! at_se_corner) //screw pumps anchor at bottom/right
            return "`";
        switch (((df::building_screw_pumpst*) b)->direction)
        {
        case screw_pump_direction::FromNorth:
            return "Msu";
        case screw_pump_direction::FromEast:
            return "Msk";
        case screw_pump_direction::FromSouth:
            return "Msm";
        case screw_pump_direction::FromWest:
            return "Msh";
        }
    case building_type::WaterWheel:
        if (! at_center)
            return "`";
        //s swaps orientation which defaults to vertical
        return ((df::building_water_wheelst*) b)->is_vertical ? "Mw" : "Mws";
    case building_type::Windmill:
        if (! at_center)
            return "`";
        return "Mm";
    case building_type::GearAssembly:
        return "Mg";
    case building_type::AxleHorizontal:
        if (! at_nw_corner) //a guess based on how constructions work
            return "`";
        //same as water wheel but reversed
        out << "Mh" << (((df::building_axle_horizontalst*) b)->is_vertical ? "s" : "")
            << "(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::AxleVertical:
        return "Mv";
    case building_type::Rollers:
        if (! at_nw_corner)
            return "`";
        out << "Mr";
        switch (((df::building_rollersst*) b)->direction)
        {
        case screw_pump_direction::FromNorth:
            break;
        case screw_pump_direction::FromEast:
            out << "s";
        case screw_pump_direction::FromSouth:
            out << "s";
        case screw_pump_direction::FromWest:
            out << "s";
        }
        out << "(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::Support:
        return "S";
    case building_type::ArcheryTarget:
        return "A";
    case building_type::TractionBench:
        return "R";
    case building_type::Hatch:
        return "H";
    case building_type::Slab:
        //how to mine alt key?!?
        //alt+s
        return " ";
    case building_type::NestBox:
        return "N";
    case building_type::Hive:
        //alt+h
        return " ";
    case building_type::GrateWall:
        return "W";
    case building_type::GrateFloor:
        return "G";
    case building_type::BarsVertical:
        return "B";
    case building_type::BarsFloor:
        //alt+b
        return " ";
    default:
        return " ";
    }
}

string get_tile_place(uint32_t x, uint32_t y, df::building* b)
{
    if (! b || b->getType() != building_type::Stockpile)
        return " ";
    if (b->x1 != x || b->y1 != y)
        return "`";
    pair<uint32_t, uint32_t> size = get_building_size(b);
    df::building_stockpilest* sp = (df::building_stockpilest*) b;
    stringstream out;// = stringstream();
    switch (sp->settings.flags.whole)
    {
    case df::stockpile_group_set::mask_animals:
        out << "a";
        break;
    case df::stockpile_group_set::mask_food:
        out << "f";
        break;
    case df::stockpile_group_set::mask_furniture:
        out << "u";
        break;
    case df::stockpile_group_set::mask_corpses:
        out << "y";
        break;
    case df::stockpile_group_set::mask_refuse:
        out << "r";
        break;
    case df::stockpile_group_set::mask_wood:
        out << "w";
        break;
    case df::stockpile_group_set::mask_stone:
        out << "s";
        break;
    case df::stockpile_group_set::mask_gems:
        out << "e";
        break;
    case df::stockpile_group_set::mask_bars_blocks:
        out << "b";
        break;
    case df::stockpile_group_set::mask_cloth:
        out << "h";
        break;
    case df::stockpile_group_set::mask_leather:
        out << "l";
        break;
    case df::stockpile_group_set::mask_ammo:
        out << "z";
        break;
    case df::stockpile_group_set::mask_coins:
        out << "n";
        break;
    case df::stockpile_group_set::mask_finished_goods:
        out << "g";
        break;
    case df::stockpile_group_set::mask_weapons:
        out << "p";
        break;
    case df::stockpile_group_set::mask_armor:
        out << "d";
        break;
    default: //multiple stockpile type
        return "`";
    }
    out << "("<< size.first << "x" << size.second << ")";
    return out.str();
}

string get_tile_query(df::building* b)
{
    if (b && b->is_room)
        return "r+";
    return " ";
}

command_result do_transform(DFCoord start, DFCoord end, string name, uint32_t phases)
{
    ofstream dig, build, place, query;
    if (phases & QUERY)
    {
        //query = ofstream((name + "-query.csv").c_str(), ofstream::trunc);
        query.open(name+"-query.csv", ofstream::trunc);
        query << "#query" << endl;
    }
    if (phases & PLACE)
    {
        //place = ofstream(name + "-place.csv", ofstream::trunc);
        place.open(name+"-place.csv", ofstream::trunc);
        place << "#place" << endl;
    }
    if (phases & BUILD)
    {
        //build = ofstream(name + "-build.csv", ofstream::trunc);
        build.open(name+"-build.csv", ofstream::trunc);
        build << "#build" << endl;
    }
    if (phases & DIG)
    {
        //dig = ofstream(name + "-dig.csv", ofstream::trunc);
        dig.open(name+"-dig.csv", ofstream::trunc);
        dig << "#dig" << endl;
    }
    if (start.x > end.x)
    {
        swap(start.x, end.x);
        start.x++;
        end.x++;
    }
    if (start.y > end.y)
    {
        swap(start.y, end.y);
        start.y++;
        end.y++;
    }
    if (start.z > end.z)
    {
        swap(start.z, end.z);
        start.z++;
        end.z++;
    }

    MapExtras::MapCache mc;
    for (int32_t z = start.z; z < end.z; z++)
    {
        for (int32_t y = start.y; y < end.y; y++)
        {
            for (int32_t x = start.x; x < end.x; x++)
            {
                df::building* b = DFHack::Buildings::findAtTile(DFCoord(x, y, z));
                if (phases & QUERY)
                    query << get_tile_query(b) << ',';
                if (phases & PLACE)
                    place << get_tile_place(x, y, b) << ',';
                if (phases & BUILD)
                    build << get_tile_build(x, y, b) << ',';
                if (phases & DIG)
                    dig << get_tile_dig(mc, x, y, z) << ',';
            }
            if (phases & QUERY)
                query << "#" << endl;
            if (phases & PLACE)
                place << "#" << endl;
            if (phases & BUILD)
                build << "#" << endl;
            if (phases & DIG)
                dig << "#" << endl;
        }
        if (z < end.z - 1)
        {
            if (phases & QUERY)
                query << "#<" << endl;
            if (phases & PLACE)
                place << "#<" << endl;
            if (phases & BUILD)
                build << "#<" << endl;
            if (phases & DIG)
                dig << "#<" << endl;
        }
    }
    if (phases & QUERY)
        query.close();
    if (phases & PLACE)
        place.close();
    if (phases & BUILD)
        build.close();
    if (phases & DIG)
        dig.close();
    return CR_OK;
}

bool cmd_option_exists(vector<string>& parameters, const string& option)
{
    return  find(parameters.begin(), parameters.end(), option) != parameters.end();
}

command_result blueprint(color_ostream &out, vector<string> &parameters)
{
    if (parameters.size() < 4 || parameters.size() > 8)
        return help(out);
    CoreSuspender suspend;
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    int32_t x, y, z;
    if (!Gui::getCursorCoords(x, y, z))
    {
        out.printerr("Can't get cursor coords! Make sure you have an active cursor in DF.\n");
        return CR_FAILURE;
    }
    DFCoord start (x, y, z);
    DFCoord end (x + stoi(parameters[0]), y + stoi(parameters[1]), z + stoi(parameters[2]));
    if (parameters.size() == 4)
        return do_transform(start, end, parameters[3], DIG | BUILD | PLACE | QUERY);
    uint32_t option = 0;
    if (cmd_option_exists(parameters, "dig"))
        option |= DIG;
    if (cmd_option_exists(parameters, "build"))
        option |= BUILD;
    if (cmd_option_exists(parameters, "place"))
        option |= PLACE;
    if (cmd_option_exists(parameters, "query"))
        option |= QUERY;
    return do_transform(start, end, parameters[3], option);
}
