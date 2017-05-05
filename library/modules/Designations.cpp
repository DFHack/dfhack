#include "DataDefs.h"
#include "Error.h"

#include "modules/Designations.h"
#include "modules/Job.h"
#include "modules/Maps.h"

#include "df/job.h"
#include "df/map_block.h"
#include "df/plant.h"
#include "df/tile_dig_designation.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

using df::global::world;

static df::map_block *getPlantBlock(const df::plant *plant)
{
    if (!world)
        return nullptr;
    return Maps::getTileBlock(plant->pos);
}

bool Designations::isPlantMarked(const df::plant *plant)
{
    CHECK_NULL_POINTER(plant);

    df::map_block *block = getPlantBlock(plant);
    if (!block)
        return false;

    if (block->designation[plant->pos.x % 16][plant->pos.y % 16].bits.dig == tile_dig_designation::Default)
        return true;

    for (auto *link = world->job_list.next; link; link = link->next)
    {
        df::job *job = link->item;
        if (!job)
            continue;
        if (job->job_type != job_type::FellTree && job->job_type != job_type::GatherPlants)
            continue;
        if (job->pos == plant->pos)
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
        df::map_block *block = getPlantBlock(plant);
        block->designation[plant->pos.x % 16][plant->pos.y % 16].bits.dig = tile_dig_designation::Default;
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
        df::map_block *block = getPlantBlock(plant);
        block->designation[plant->pos.x % 16][plant->pos.y % 16].bits.dig = tile_dig_designation::No;
        block->flags.bits.designated = true;

        auto *link = world->job_list.next;
        while (link)
        {
            auto *next = link->next;
            df::job *job = link->item;

            if (!job)
                continue;
            if (job->job_type != job_type::FellTree && job->job_type != job_type::GatherPlants)
                continue;
            if (job->pos == plant->pos)
                Job::removeJob(job);

            link = next;
        }

        return true;
    }
    else
    {
        return false;
    }
}
