#pragma once
#include <PluginManager.h>
#include <modules/Job.h>
#include <modules/EventManager.h> //hash functions (they should probably get moved at this point, the ones that aren't specifically for EM anyway)
#include <df/world.h>
#include <df/job.h>

#include <unordered_set>

using namespace DFHack;

/* Used to read/store/iterate channel digging jobs
 * jobs: list of coordinates with channel jobs associated to them
 * load_channel_jobs: iterates world->jobs.list to find channel jobs and adds them into the `jobs` map
 * clear: empties the container
 * erase: finds a job corresponding to a coord, removes the mapping in jobs, and calls Job::removeJob, then returns an iterator following the element removed
 * find: returns an iterator to a job if one exists for a map coordinate
 * begin: returns jobs.begin()
 * end: returns jobs.end()
 */
class ChannelJobs {
private:
    friend class ChannelGroup;

    using Jobs = std::unordered_set<df::coord>; // job* will exist until it is complete, and likely beyond
    Jobs locations;
public:
    void load_channel_jobs();
    void clear() {
        locations.clear();
    }
    int count(const df::coord &map_pos) const { return locations.count(map_pos); }
    Jobs::iterator erase(const df::coord &map_pos) {
        auto iter = locations.find(map_pos);
        if (iter != locations.end()) {
            return locations.erase(iter);
        }
        return iter;
    }
    Jobs::const_iterator find(const df::coord &map_pos) const { return locations.find(map_pos); }
    Jobs::const_iterator begin() const { return locations.begin(); }
    Jobs::const_iterator end() const { return locations.end(); }
};
