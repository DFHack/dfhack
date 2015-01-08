//Blueprint
//By cdombroski
//Translates a region of tiles specified by the cursor and arguments/prompts into a series of blueprint files suitable for digfort/buildingplan/quickfort

#include <Console.h>
#include <PluginManager.h>

#include "modules/Gui.h"
#include "modules/MapCache.h"

using std::string;
using std::endl;
using std::vector;
using std::ofstream;
using namespace DFHack;

DFHACK_PLUGIN("blueprint");

#define swap(x, y)\
    x += y;\
    y = x - y;\
    x -= y

enum phase {DIG, BUILD, PLACE, QUERY};

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
    out << "blueprint width height depth name [dig [build [place [query]]]]" << endl
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

char get_tile_dig(MapExtras::MapCache mc, int32_t x, int32_t y, int32_t z)
{
    df::tiletype tt = mc.tiletypeAt(DFCoord(x, y, z));
    df::enum_traits<df::tiletype> et;
    df::tiletype_shape ts = et.attr_table[tt].shape;
    switch (ts)
    {
    case df::tiletype_shape::EMPTY:
    case df::tiletype_shape::RAMP_TOP:        
        return 'h';
    case df::tiletype_shape::FLOOR:
    case df::tiletype_shape::BOULDER:
    case df::tiletype_shape::PEBBLES:
    case df::tiletype_shape::BROOK_TOP:
        return 'd';
    case df::tiletype_shape::FORTIFICATION:
        return 'a'; //or possibly 'F' with new keybindings?
    case df::tiletype_shape::STAIR_UP:
        return 'u';
    case df::tiletype_shape::STAIR_DOWN:
        return 'j';
    case df::tiletype_shape::STAIR_UPDOWN:
        return 'i';
    case df::tiletype_shape::RAMP:
        return 'r';
    default:
        return ' ';
    
    }
}


command_result do_transform(DFCoord start, DFCoord end, string name, phase last_phase)
{
    ofstream dig, build, place, query;
    switch (last_phase)
    {
    case QUERY:
        query = ofstream(name + "-query.csv", ofstream::trunc);
        query << "#query" << endl;
    case PLACE:
        place = ofstream(name + "-place.csv", ofstream::trunc);
        place << "#place" << endl;
    case BUILD:
        build = ofstream(name + "-build.csv", ofstream::trunc);
        build << "#build" << endl;
    case DIG:
        dig = ofstream(name + "-dig.csv", ofstream::trunc);
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
                switch (last_phase) {
                case QUERY:
                case PLACE:
                case BUILD:
                case DIG:
                    dig << get_tile_dig(mc, x, y, z) << ',';
                }
            }
            switch (last_phase) {
            case QUERY:
                query << "#" << endl;
            case PLACE:
                place << "#" << endl;
            case BUILD:
                build << "#" << endl;
            case DIG:
                dig << "#" << endl;
            }
        }
        if (z < end.z - 1)
            switch (last_phase) {
            case QUERY:
                query << "#<" << endl;
            case PLACE:
                place << "#<" << endl;
            case BUILD:
                build << "#<" << endl;
            case DIG:
                dig << "#<" << endl;
            }
    }
    switch (last_phase) {
    case QUERY:
        query.close();
    case PLACE:
        place.close();
    case BUILD:
        build.close();
    case DIG:
        dig.close();
    }
    return CR_OK;
}

command_result blueprint(color_ostream &out, vector <string> &parameters)
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
    switch(parameters.size())
    {
    case 4:
    case 8:
        return do_transform(start, end, parameters[3], QUERY);
    case 5:
        return do_transform(start, end, parameters[3], DIG);
    case 6:
        return do_transform(start, end, parameters[3], BUILD);
    case 7:
        return do_transform(start, end, parameters[3], PLACE);
    default: //wtf?
        return CR_FAILURE;
    }
}