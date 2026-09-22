#pragma once

#include "engines.h"
#include "laborcommon.h"

#include <df/coord.h>
#include <df/job_type.h>

// Monitor mode: watches the job board and feeds the task starvation
// notification, but performs no labor management at all -- everything is
// left to the player's own work details and the labor matrix.
//
// The implementation lives in modernengine.cpp so it shares the
// posting-tracking machinery (posting ages, starvation stats, skill usage
// history, persisted state) with the modern engine; switching between
// modes keeps the accumulated history.
class MonitorEngine : public LaborEngine {
public:
    void enable(color_ostream &out) override;
    void disable(color_ostream &out) override;
    void update(color_ostream &out) override;
    bool command(color_ostream &out, std::vector<std::string> &parameters) override;

    void map_unload();

    int starving_jobs();
    df::job_type oldest_starving_job();
    int32_t oldest_starving_wait();
    df::coord oldest_starving_pos();
    std::string oldest_starving_name();

private:
    bool initialized = false;
};
