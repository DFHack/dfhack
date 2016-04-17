// Wide-area traffic designation utility.
// Flood-fill from cursor or fill entire map.

#include <ctype.h>      //For toupper().
#include <algorithm>    //for min().
#include <map>
#include <vector>
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/Gui.h"
using std::stack;
using MapExtras::MapCache;
using namespace DFHack;
using namespace df::enums;

//Function pointer type for whole-map tile checks.
typedef void (*checkTile)(DFCoord, MapExtras::MapCache &);

//Forward Declarations for Commands
command_result filltraffic(color_ostream &out, std::vector<std::string> & params);
command_result alltraffic(color_ostream &out, std::vector<std::string> & params);
command_result restrictLiquid(color_ostream &out, std::vector<std::string> & params);
command_result restrictIce(color_ostream &out, std::vector<std::string> & params);

//Forward Declarations for Utility Functions
command_result setAllMatching(color_ostream &out, checkTile checkProc,
                              DFCoord minCoord = DFCoord(0, 0, 0),
                              DFCoord maxCoord = DFCoord(0xFFFF, 0xFFFF, 0xFFFF));

void allHigh(DFCoord coord, MapExtras::MapCache & map);
void allNormal(DFCoord coord, MapExtras::MapCache & map);
void allLow(DFCoord coord, MapExtras::MapCache & map);
void allRestricted(DFCoord coord, MapExtras::MapCache & map);

void restrictLiquidProc(DFCoord coord, MapExtras::MapCache &map);
void restrictIceProc(DFCoord coord, MapExtras::MapCache &map);

DFHACK_PLUGIN("filltraffic");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "filltraffic","Flood-fill selected traffic designation from cursor",
        filltraffic, Gui::cursor_hotkey,
        "  Flood-fill selected traffic type from the cursor.\n"
        "Traffic Type Codes:\n"
        "  H: High Traffic\n"
        "  N: Normal Traffic\n"
        "  L: Low Traffic\n"
        "  R: Restricted Traffic\n"
        "Other Options:\n"
        "  X: Fill across z-levels.\n"
        "  B: Include buildings and stockpiles.\n"
        "  P: Include empty space.\n"
        "Example:\n"
        "  filltraffic H\n"
        "    When used in a room with doors,\n"
        "    it will set traffic to HIGH in just that room.\n"
    ));
    commands.push_back(PluginCommand(
        "alltraffic","Set traffic for the entire map",
        alltraffic, false,
        "  Set traffic types for all tiles on the map.\n"
        "Traffic Type Codes:\n"
        "  H: High Traffic\n"
        "  N: Normal Traffic\n"
        "  L: Low Traffic\n"
        "  R: Restricted Traffic\n"
    ));
    commands.push_back(PluginCommand(
        "restrictliquids","Restrict on every visible square with liquid",
        restrictLiquid, false, ""
    ));
    commands.push_back(PluginCommand(
        "restrictice","Restrict traffic on squares above visible ice",
        restrictIce, false, ""
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

command_result filltraffic(color_ostream &out, std::vector<std::string> & params)
{
    // HOTKEY COMMAND; CORE ALREADY SUSPENDED

    //Maximum map size.
    uint32_t x_max,y_max,z_max;
    //Source and target traffic types.
    df::tile_traffic source = tile_traffic::Normal;
    df::tile_traffic target = tile_traffic::Normal;
    //Option flags
    bool updown        = false;
    bool checkpit      = true;
    bool checkbuilding = true;

    //Loop through parameters
    for(size_t i = 0; i < params.size();i++)
    {
        if (params[i] == "help" || params[i] == "?" || params[i].size() != 1)
            return CR_WRONG_USAGE;

        switch (toupper(params[i][0]))
        {
        case 'H':
            target = tile_traffic::High; break;
        case 'N':
            target = tile_traffic::Normal; break;
        case 'L':
            target = tile_traffic::Low; break;
        case 'R':
            target = tile_traffic::Restricted; break;
        case 'X':
            updown = true; break;
        case 'B':
            checkbuilding = false; break;
        case 'P':
            checkpit = false; break;
        default:
            return CR_WRONG_USAGE;
        }
    }

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int32_t cx, cy, cz;
    Maps::getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;
    Gui::getCursorCoords(cx,cy,cz);
    while(cx == -30000)
    {
        out.printerr("Cursor is not active.\n");
        return CR_FAILURE;
    }

    DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    MapExtras::MapCache MCache;

    df::tile_designation des = MCache.designationAt(xy);
    df::tiletype tt = MCache.tiletypeAt(xy);
    df::tile_occupancy oc;

    if (checkbuilding)
        oc = MCache.occupancyAt(xy);

    source = (df::tile_traffic)des.bits.traffic;
    if(source == target)
    {
        out.printerr("This tile is already set to the target traffic type.\n");
        return CR_FAILURE;
    }

    if(isWallTerrain(tt))
    {
        out.printerr("This tile is a wall. Please select a passable tile.\n");
        return CR_FAILURE;
    }

    if(checkpit && isOpenTerrain(tt))
    {
        out.printerr("This tile is a hole. Please select a passable tile.\n");
        return CR_FAILURE;
    }

    if(checkbuilding && oc.bits.building)
    {
        out.printerr("This tile contains a building. Please select an empty tile.\n");
        return CR_FAILURE;
    }

    out.print("%d/%d/%d  ... FILLING!\n", cx,cy,cz);

    //Naive four-way or six-way flood fill with possible tiles on a stack.
    stack <DFCoord> flood;
    flood.push(xy);

    while(!flood.empty())
    {
        xy = flood.top();
        flood.pop();

        des = MCache.designationAt(xy);
        if (des.bits.traffic != source) continue;

        tt = MCache.tiletypeAt(xy);

        if(isWallTerrain(tt)) continue;
        if(checkpit && isOpenTerrain(tt)) continue;

        if (checkbuilding)
        {
            oc = MCache.occupancyAt(xy);
            if(oc.bits.building) continue;
        }

        //This tile is ready.  Set its traffic level and add surrounding tiles.
        if (MCache.testCoord(xy))
        {
            des.bits.traffic = target;
            MCache.setDesignationAt(xy,des);

            if (xy.x > 0)
            {
                flood.push(DFCoord(xy.x - 1, xy.y, xy.z));
            }
            if (xy.x < tx_max - 1)
            {
                flood.push(DFCoord(xy.x + 1, xy.y, xy.z));
            }
            if (xy.y > 0)
            {
                flood.push(DFCoord(xy.x, xy.y - 1, xy.z));
            }
            if (xy.y < ty_max - 1)
            {
                flood.push(DFCoord(xy.x, xy.y + 1, xy.z));
            }

            if (updown)
            {
                if (xy.z > 0 && LowPassable(tt))
                {
                    flood.push(DFCoord(xy.x, xy.y, xy.z - 1));
                }
                if (xy.z < z_max && HighPassable(tt))
                {
                    flood.push(DFCoord(xy.x, xy.y, xy.z + 1));
                }
            }

        }
    }

    MCache.WriteAll();
    return CR_OK;
}

enum e_checktype {no_check, check_equal, check_nequal};

command_result alltraffic(color_ostream &out, std::vector<std::string> & params)
{
    void (*proc)(DFCoord, MapExtras::MapCache &) = allNormal;

    //Loop through parameters
    for(size_t i = 0; i < params.size();i++)
    {
        if (params[i] == "help" || params[i] == "?" || params[i].size() != 1)
            return CR_WRONG_USAGE;

        //Pick traffic type. Possibly set bounding rectangle later.
        switch (toupper(params[i][0]))
        {
        case 'H':
            proc = allHigh; break;
        case 'N':
            proc = allNormal; break;
        case 'L':
            proc = allLow; break;
        case 'R':
            proc = allRestricted; break;
        default:
            return CR_WRONG_USAGE;
        }
    }

    return setAllMatching(out, proc);
}

command_result restrictLiquid(color_ostream &out, std::vector<std::string> & params)
{
  return setAllMatching(out, restrictLiquidProc);
}

command_result restrictIce(color_ostream &out, std::vector<std::string> & params)
{
    return setAllMatching(out, restrictIceProc);
}

//Helper function for writing new functions that check every tile on the map.
//newTraffic is the traffic designation to set.
//check takes a coordinate and the map cache as arguments, and returns true if the criteria is met.
//minCoord and maxCoord can be used to specify a bounding cube.
command_result setAllMatching(color_ostream &out, checkTile checkProc,
                              DFCoord minCoord, DFCoord maxCoord)
{
    //Initialization.
    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    //Maximum map size.
    uint32_t x_max,y_max,z_max;
    Maps::getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;

    //Ensure maximum coordinate is within map.  Truncate to map edge.
    maxCoord.x = std::min((uint32_t) maxCoord.x, tx_max);
    maxCoord.y = std::min((uint32_t) maxCoord.y, ty_max);
    maxCoord.z = std::min((uint32_t) maxCoord.z,  z_max);

    //Check minimum co-ordinates against maximum map size
    if (minCoord.x > maxCoord.x)
    {
        out.printerr("Minimum x coordinate is greater than maximum x coordinate.\n");
        return CR_FAILURE;
    }
    if (minCoord.y > maxCoord.y)
    {
        out.printerr("Minimum y coordinate is greater than maximum y coordinate.\n");
        return CR_FAILURE;
    }
    if (minCoord.z > maxCoord.y)
    {
        out.printerr("Minimum z coordinate is greater than maximum z coordinate.\n");
        return CR_FAILURE;
    }

    MapExtras::MapCache MCache;

    out.print("Setting traffic...\n");

    //Loop through every single tile
    for(uint32_t x = minCoord.x; x <= maxCoord.x; x++)
    {
        for(uint32_t y = minCoord.y; y <= maxCoord.y; y++)
        {
            for(uint32_t z = minCoord.z; z <= maxCoord.z; z++)
            {
                DFCoord tile = DFCoord(x, y, z);
                checkProc(tile, MCache);
            }
        }
    }

    MCache.WriteAll();
    out.print("Complete!\n");
    return CR_OK;
}

//Unconditionally set map to target traffic type
void allHigh(DFCoord coord, MapExtras::MapCache &map)
{
    df::tile_designation des = map.designationAt(coord);
    des.bits.traffic = tile_traffic::High;
    map.setDesignationAt(coord, des);
}
void allNormal(DFCoord coord, MapExtras::MapCache &map)
{
    df::tile_designation des = map.designationAt(coord);
    des.bits.traffic = tile_traffic::Normal;
    map.setDesignationAt(coord, des);
}
void allLow(DFCoord coord, MapExtras::MapCache &map)
{
    df::tile_designation des = map.designationAt(coord);
    des.bits.traffic = tile_traffic::Low;
    map.setDesignationAt(coord, des);
}
void allRestricted(DFCoord coord, MapExtras::MapCache &map)
{
    df::tile_designation des = map.designationAt(coord);
    des.bits.traffic = tile_traffic::Restricted;
    map.setDesignationAt(coord, des);
}

//Restrict traffic if tile is visible and liquid is present.
void restrictLiquidProc(DFCoord coord, MapExtras::MapCache &map)
{
    df::tile_designation des = map.designationAt(coord);
    if ((des.bits.hidden == 0) && (des.bits.flow_size != 0))
    {
        des.bits.traffic = tile_traffic::Restricted;
        map.setDesignationAt(coord, des);
    }
}

//Restrict traffice if tile is above visible ice wall.
void restrictIceProc(DFCoord coord, MapExtras::MapCache &map)
{
    //There is no ice below the bottom of the map.
    if (coord.z == 0)
        return;

    DFCoord tile_below = DFCoord(coord.x, coord.y, coord.z - 1);
    df::tiletype tt = map.tiletypeAt(tile_below);
    df::tile_designation des = map.designationAt(tile_below);

    if ((des.bits.hidden == 0) && (tileMaterial(tt) == tiletype_material::FROZEN_LIQUID))
    {
        des = map.designationAt(coord);
        des.bits.traffic = tile_traffic::Restricted;
        map.setDesignationAt(coord, des);
    }
}
