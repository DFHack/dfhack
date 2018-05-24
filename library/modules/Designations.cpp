#include "DataDefs.h"
#include "Error.h"

#include "modules/Designations.h"
#include "modules/Job.h"
#include "modules/Maps.h"

#include "df/job.h"
#include "df/map_block.h"
#include "df/plant.h"
#include "df/plant_tree_info.h"
#include "df/plant_tree_tile.h"
#include "df/tile_dig_designation.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

using df::global::world;

static df::map_block *getPlantBlock(const df::plant *plant)
{
    if (!world)
        return nullptr;
    return Maps::getTileBlock(Designations::getPlantDesignationTile(plant));
}

df::coord Designations::getPlantDesignationTile(const df::plant *plant)
{
    CHECK_NULL_POINTER(plant);

    if (!plant->tree_info)
        return plant->pos;

    int dimx = plant->tree_info->dim_x;
    int dimy = plant->tree_info->dim_y;
    int cx = dimx / 2;
    int cy = dimy / 2;

    // Find the southeast trunk tile
    int x = cx;
    int y = cy;

    while (x + 1 < dimx && y + 1 < dimy)
    {
        if (plant->tree_info->body[0][(y * dimx) + (x + 1)].bits.trunk)
            ++x;
        else if (plant->tree_info->body[0][((y + 1) * dimx) + x].bits.trunk)
            ++y;
        else
            break;
    }

    return df::coord(plant->pos.x - cx + x, plant->pos.y - cy + y, plant->pos.z);
}

bool Designations::isPlantMarked(const df::plant *plant)
{
    CHECK_NULL_POINTER(plant);

    df::coord des_pos = getPlantDesignationTile(plant);
    df::map_block *block = Maps::getTileBlock(des_pos);
    if (!block)
        return false;

    if (block->designation[des_pos.x % 16][des_pos.y % 16].bits.dig == tile_dig_designation::Default)
        return true;

    for (auto *link = world->jobs.list.next; link; link = link->next)
    {
        df::job *job = link->item;
        if (!job)
            continue;
        if (job->job_type != job_type::FellTree && job->job_type != job_type::GatherPlants)
            continue;
        if (job->pos == des_pos)
            return true;
    }
    return false;
}

bool Designations::canMarkPlant(const df::plant *plant)
{
    CHECK_NULL_POINTER(plant);

    if (!getPlantBlock(plant))
        return false;

    return !isPlantMarked(plant);
}

bool Designations::markPlant(const df::plant *plant)
{
    CHECK_NULL_POINTER(plant);

    if (canMarkPlant(plant))
    {
        df::coord des_pos = getPlantDesignationTile(plant);
        df::map_block *block = Maps::getTileBlock(des_pos);
        block->designation[des_pos.x % 16][des_pos.y % 16].bits.dig = tile_dig_designation::Default;
        block->flags.bits.designated = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool Designations::canUnmarkPlant(const df::plant *plant)
{
    CHECK_NULL_POINTER(plant);

    if (!getPlantBlock(plant))
        return false;

    return isPlantMarked(plant);
}

bool Designations::unmarkPlant(const df::plant *plant)
{
    CHECK_NULL_POINTER(plant);

    if (canUnmarkPlant(plant))
    {
        df::coord des_pos = getPlantDesignationTile(plant);
        df::map_block *block = Maps::getTileBlock(des_pos);
        block->designation[des_pos.x % 16][des_pos.y % 16].bits.dig = tile_dig_designation::No;
        block->flags.bits.designated = true;

        auto *link = world->jobs.list.next;
        while (link)
        {
            auto *next = link->next;
            df::job *job = link->item;

            if (job &&
                (job->job_type == job_type::FellTree || job->job_type == job_type::GatherPlants) &&
                job->pos == des_pos)
            {
                Job::removeJob(job);
            }

            link = next;
        }

        return true;
    }
    else
    {
        return false;
    }
}
