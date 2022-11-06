#include <channel-jobs.h>
#include <inlines.h>
#include <df/world.h>
#include <df/job.h>

// iterates the DF job list and adds channel jobs to the `jobs` container
void ChannelJobs::load_channel_jobs() {
    jobs.clear();
    df::job_list_link* node = df::global::world->jobs.list.next;
    while (node) {
        df::job* job = node->item;
        node = node->next;
        if (is_dig_job(job)) {
            jobs.emplace(job->pos);
        }
    }
}

// clears the container
void ChannelJobs::clear() {
    jobs.clear();
}

// finds and erases a job corresponding to a map position, then returns the iterator following the element removed
std::set<df::coord>::iterator ChannelJobs::erase(const df::coord &map_pos) {
    auto iter = jobs.find(map_pos);
    if (iter != jobs.end()) {
        return jobs.erase(iter);
    }
    return iter;
}

// finds a job corresponding to a map position if one exists
std::set<df::coord>::const_iterator ChannelJobs::find(const df::coord &map_pos) const {
    return jobs.find(map_pos);
}

// returns an iterator to the first element stored
std::set<df::coord>::const_iterator ChannelJobs::begin() const {
    return jobs.begin();
}

// returns an iterator to after the last element stored
std::set<df::coord>::const_iterator ChannelJobs::end() const {
    return jobs.end();
}
