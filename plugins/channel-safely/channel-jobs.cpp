#include "channel-jobs.h"
#include "inlines.h"
#include <df/world.h>
#include <df/job.h>

// iterates the DF job list and adds channel jobs to the `jobs` container
void ChannelJobs::load_channel_jobs() {
    jobs.clear();
    df::job_list_link* node = df::global::world->jobs.list.next;
    while (node) {
        df::job* job = node->item;
        node = node->next;
        if (is_channel_job(job)) {
            jobs.emplace(job->pos, job);
        }
    }
}

// clears the container
void ChannelJobs::clear() {
    jobs.clear();
}

// finds and erases a job corresponding to a map position, then returns the iterator following the element removed
std::map<df::coord, df::job*>::iterator ChannelJobs::erase_and_cancel(const df::coord &map_pos) {
    auto iter = jobs.find(map_pos);
    if (iter != jobs.end()) {
        df::job* job = iter->second;
        cancel_job(job);
        return jobs.erase(iter);
    }
    return iter;
}

// finds a job corresponding to a map position if one exists
std::map<df::coord, df::job*>::const_iterator ChannelJobs::find(const df::coord &map_pos) const {
    return jobs.find(map_pos);
}

// returns an iterator to the first element stored
std::map<df::coord, df::job*>::const_iterator ChannelJobs::begin() const {
    return jobs.begin();
}

// returns an iterator to after the last element stored
std::map<df::coord, df::job*>::const_iterator ChannelJobs::end() const {
    return jobs.end();
}
