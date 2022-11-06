#pragma once
#include <PluginManager.h>
#include <modules/Job.h>
#include <map>

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
    using Jobs = std::set<df::coord>; // job* will exist until it is complete, and likely beyond
    Jobs jobs;
public:
    void load_channel_jobs();
    void clear();
    Jobs::iterator erase(const df::coord &map_pos);
    Jobs::const_iterator find(const df::coord &map_pos) const;
    Jobs::const_iterator begin() const;
    Jobs::const_iterator end() const;
};
