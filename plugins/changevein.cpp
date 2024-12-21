// Allow changing the material of a mineral inclusion

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "TileTypes.h"

#include "df/block_square_event.h"
#include "df/block_square_event_mineralst.h"
#include "df/inorganic_raw.h"
#include "df/map_block.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("changevein");
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(cursor);

constexpr uint8_t NORTH = 0;
constexpr uint8_t EAST = 1;
constexpr uint8_t SOUTH = 2;
constexpr uint8_t WEST = 3;
constexpr uint8_t NORTHEAST = 0;
constexpr uint8_t SOUTHEAST = 1;
constexpr uint8_t SOUTHWEST = 2;
constexpr uint8_t NORTHWEST = 3;

static const df::coord2d DIRECTION_OFFSETS[] = { df::coord2d(0, -16), df::coord2d(16, 0), df::coord2d(0, 16), df::coord2d(-16, 0)  };
static const df::coord2d DIAGONAL_OFFSETS[] = { df::coord2d(16, -16), df::coord2d(16, 16), df::coord2d(-16, -16), df::coord2d(-16, 16) };

struct VeinEdgeBitmask
{
    int32_t mat;
    // North: 0 | East: 1 | South: 2 | West: 3
    uint16_t edges[4];
    // NE: 0 | SE: 1 | SW: 2 | NW: 3
    bool corners[4];

    VeinEdgeBitmask(df::block_square_event_mineralst* event)
    {
        mat = event->inorganic_mat;

        // North & South
        edges[NORTH] = event->tile_bitmask.bits[0];
        edges[SOUTH] = event->tile_bitmask.bits[15];

        // East & West
        edges[EAST] = 0;
        edges[WEST] = 0;
        for (uint8_t i = 0; i < 16; i++)
        {
            // East-most tile is represented by the largest bit
            edges[EAST] |= (event->tile_bitmask.bits[i] & 0b1000000000000000) >> (15 - i);
            // West-most tile is represented by the smallest bit
            edges[WEST] |= (event->tile_bitmask.bits[i] & 0b0000000000000001) << i;
        }

        corners[NORTHEAST] = edges[NORTH] & 0b1000000000000000;
        corners[SOUTHEAST] = edges[SOUTH] & 0b1000000000000000;
        corners[SOUTHWEST] = edges[SOUTH] & 0b0000000000000001;
        corners[NORTHWEST] = edges[NORTH] & 0b0000000000000001;
    }

    void SmudgeBitmask()
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            edges[i] |= (edges[i] << 1) | (edges[i] >> 1);
        }
    }

    bool Merge(VeinEdgeBitmask& other)
    {
        if (other.mat != this->mat)
            return false;

        for (uint8_t i = 0; i < 4; i++)
        {
            this->edges[i] |= other.edges[i];
            this->corners[i] |= other.corners[i];
        }

        return true;
    }

    bool Merge(df::block_square_event_mineralst* event)
    {
        VeinEdgeBitmask temp = VeinEdgeBitmask(event);
        return this->Merge(temp);
    }
};

static void ChangeSameBlockVeins(df::map_block* block, df::block_square_event_mineralst* event, VeinEdgeBitmask& mask, int32_t matIndex)
{
    for (auto evt : block->block_events)
    {
        if (evt == event)
            continue;
        if (evt->getType() != block_square_event_type::mineral)
            continue;

        df::block_square_event_mineralst* cur = (df::block_square_event_mineralst*)evt;
        if (cur->inorganic_mat == mask.mat)
        {
            for (uint8_t j = 0; j < 15; j++)
            {
                uint16_t bitmask = cur->tile_bitmask.bits[j] | cur->tile_bitmask.bits[(j>0 ? j-1 : 0)] | cur->tile_bitmask.bits[std::min(j + 1, 15)];
                bitmask |= (bitmask << 1) | (bitmask >> 1);

                if (bitmask & event->tile_bitmask.bits[j])
                {
                    mask.Merge(cur);
                    cur->inorganic_mat = matIndex;
                    break;
                }
            }
        }
    }
}

static void ChangeSurroundingBlockVeins(df::map_block* block, VeinEdgeBitmask& mask, int32_t matIndex)
{
    if (!block)
        return;

    // For detecting diagonal connections
    mask.SmudgeBitmask();

    for (uint8_t i = 0; i < 4; i++)
    {
        const df::coord2d& offset = DIRECTION_OFFSETS[i];
        const uint16_t edgeBitmask = mask.edges[i];
        const uint8_t oppositeIndex = (i + 2) % 4;

        if (!edgeBitmask)
            continue;

        df::coord pos = df::coord(block->map_pos.x + offset.x, block->map_pos.y + offset.y, block->map_pos.z);
        df::map_block* newBlock = Maps::getTileBlock(pos);

        if (!newBlock)
            continue;

        for (auto evt : newBlock->block_events)
        {
            if (evt->getType() != block_square_event_type::mineral)
                continue;

            df::block_square_event_mineralst* cur = (df::block_square_event_mineralst*)evt;
            if (cur->inorganic_mat == mask.mat)
            {
                VeinEdgeBitmask newMask = VeinEdgeBitmask(cur);
                if (newMask.edges[oppositeIndex] & edgeBitmask)
                {
                    cur->inorganic_mat = matIndex;
                    ChangeSameBlockVeins(newBlock, cur, newMask, matIndex);
                    ChangeSurroundingBlockVeins(newBlock, newMask, matIndex);
                }
            }
        }
    }

    for (uint8_t i = 0; i < 4; i++)
    {
        const df::coord2d& offset = DIAGONAL_OFFSETS[i];
        const bool cornerBit = mask.corners[i];
        const uint8_t oppositeIndex = (i + 2) % 4;

        if (!cornerBit)
            continue;

        df::coord pos = df::coord(block->map_pos.x + offset.x, block->map_pos.y + offset.y, block->map_pos.z);
        df::map_block* newBlock = Maps::getTileBlock(pos);

        if (!newBlock)
            continue;

        for (auto evt : newBlock->block_events)
        {
            if (evt->getType() != block_square_event_type::mineral)
                continue;

            df::block_square_event_mineralst* cur = (df::block_square_event_mineralst*)evt;
            if (cur->inorganic_mat == mask.mat)
            {
                VeinEdgeBitmask newMask = VeinEdgeBitmask(cur);
                if (newMask.corners[oppositeIndex] && cornerBit)
                {
                    cur->inorganic_mat = matIndex;
                    ChangeSameBlockVeins(newBlock, cur, newMask, matIndex);
                    ChangeSurroundingBlockVeins(newBlock, newMask, matIndex);
                }
            }
        }
    }
}

command_result df_changevein (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size() != 1)
        return CR_WRONG_USAGE;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    if (!cursor || cursor->x == -30000)
    {
        out.printerr("No cursor detected - please place the cursor over a mineral vein.\n");
        return CR_FAILURE;
    }

    MaterialInfo mi;
    if (!mi.findInorganic(parameters[0]))
    {
        out.printerr("No such material!\n");
        return CR_FAILURE;
    }
    if (mi.inorganic->material.flags.is_set(material_flags::IS_METAL) ||
        mi.inorganic->material.flags.is_set(material_flags::NO_STONE_STOCKPILE) ||
        mi.inorganic->flags.is_set(inorganic_flags::SOIL_ANY))
    {
        out.printerr("Invalid material - you must select a type of stone or gem\n");
        return CR_FAILURE;
    }

    df::map_block *block = Maps::getTileBlock(cursor->x, cursor->y, cursor->z);
    if (!block)
    {
        out.printerr("Invalid tile selected.\n");
        return CR_FAILURE;
    }
    df::block_square_event_mineralst *mineral = NULL;
    int tx = cursor->x % 16, ty = cursor->y % 16;
    for (auto evt : block->block_events)
    {
        if (evt->getType() != block_square_event_type::mineral)
            continue;
        df::block_square_event_mineralst *cur = (df::block_square_event_mineralst *)evt;
        if (cur->getassignment(tx, ty))
            mineral = cur;
    }
    if (!mineral)
    {
        out.printerr("Selected tile does not contain a mineral vein.\n");
        return CR_FAILURE;
    }

    VeinEdgeBitmask mask = VeinEdgeBitmask(mineral);
    mineral->inorganic_mat = mi.index;
    ChangeSameBlockVeins(block, mineral, mask, mi.index);
    ChangeSurroundingBlockVeins(block, mask, mi.index);

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("changevein",
        "Change the material of a mineral inclusion.",
        df_changevein));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
