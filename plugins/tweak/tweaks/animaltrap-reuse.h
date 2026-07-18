#include "modules/Job.h"
#include "modules/Items.h"

#include "df/building_animaltrapst.h"
#include "df/buildingitemst.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_contained_in_itemst.h"
#include "df/general_ref_contains_itemst.h"
#include "df/job.h"

using namespace df::enums;

struct animaltrap_reuse_hook : df::building_animaltrapst {
    typedef df::building_animaltrapst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, updateAction, ())
    {
        // Skip processing if the trap isn't fully built
        if (getBuildStage() != getMaxBuildStage()) return;

        // The first item is always the trap itself, and the second item (if any) is always either the Bait or the caught Vermin
        if ((contained_items.size() > 1) && (contained_items[1]->item->getType() == df::item_type::VERMIN))
        {
            auto trap = contained_items[0]->item;
            auto vermin = contained_items[1]->item;

            // Make sure we don't already have a "Release Small Creature" job
            for (size_t j = 0; j < jobs.size(); j++)
            {
                if (jobs[j]->job_type == df::job_type::ReleaseSmallCreature)
                    return;
                // Also bail out if the player marked the building for destruction
                if (jobs[j]->job_type == df::job_type::DestroyBuilding)
                    return;
            }

            // Create the job
            auto job = new df::job();
            Job::linkIntoWorld(job, true);

            job->job_type = df::job_type::ReleaseSmallCreature;
            job->pos.x = centerx;
            job->pos.y = centery;
            job->pos.z = z;

            // Attach the vermin to the job
            Job::attachJobItem(job, vermin, df::job_role_type::Hauled, -1, -1);

            // Link the job to the building
            df::general_ref *ref = df::allocate<df::general_ref_building_holderst>();
            ref->setID(id);
            job->general_refs.push_back(ref);

            jobs.push_back(job);

            // Hack: put the vermin inside the trap ITEM right away, otherwise the job will cancel.
            // Normally, this doesn't happen until after the trap is deconstructed, but the game still
            // seems to handle everything correctly and doesn't leave any bad references afterwards.
            if (!Items::getContainer(vermin))
            {
                // We can't use Items::moveToContainer, because that would remove it from the Building
                // (and cause the game to no longer recognize the trap as being "full").
                // Instead, manually add the references and set the necessary bits.

                ref = df::allocate<df::general_ref_contained_in_itemst>();
                ref->setID(trap->id);
                vermin->general_refs.push_back(ref);

                ref = df::allocate<df::general_ref_contains_itemst>();
                ref->setID(vermin->id);
                trap->general_refs.push_back(ref);

                vermin->flags.bits.in_inventory = true;
                trap->flags.bits.weight_computed = false;
                // Don't set this flag here (even though it would normally get set),
                // since the game doesn't clear it after the vermin gets taken out
                // trap->flags.bits.container = true;
            }
            return;
        }
        INTERPOSE_NEXT(updateAction)();
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(animaltrap_reuse_hook, updateAction);
