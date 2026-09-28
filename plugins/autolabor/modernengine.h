#pragma once

#include "engines.h"
#include "laborcommon.h"
#include "workdetails.h"

#include <df/coord.h>

// The modern engine: works through DF's work detail system instead of
// writing the labor matrix directly. It maintains plugin-owned work details
// (named "auto: ...", visible and editable in the vanilla UI) and uses the
// per-unit only_do_assigned_jobs flag to create dedicated pools:
//
//   - "auto: Laborers": unskilled labors only; members are specialized so
//     the game's job auction always has units that can only take unskilled
//     work, preventing starvation of hauling/cleaning jobs.
//   - "auto: <skill>": one per exercised skill; members are specialized to
//     jobs that train that skill. Specialists whose profession has a guild
//     hall get protected idle time to participate in guild activities.
//
// The balance slider trades laborer coverage against specialist protection.
class ModernEngine : public LaborEngine {
public:
    ModernEngine(WorkDetailManager *wdm) : wdm(wdm) {}

    void enable(color_ostream &out) override;
    void disable(color_ostream &out) override;
    void update(color_ostream &out) override;
    bool command(color_ostream &out, std::vector<std::string> &parameters) override;

    void map_unload();

    // status summary for the overlay, e.g. "3 laborers, 5 specialists"
    std::string status_line();

    // number of live job postings that have been on the board longer than
    // one game day (drives the task starvation notification)
    int starving_jobs();

    // the job type, age (ticks), location, and display name of the
    // longest-starving posting, for the starvation notification
    df::job_type oldest_starving_job();
    int32_t oldest_starving_wait();
    df::coord oldest_starving_pos();
    std::string oldest_starving_name();

private:
    WorkDetailManager *wdm;
    bool initialized = false;
};

namespace autolabor {

// prints a snapshot of the engine's observed state to the console for
// diagnostics (the 'dump' command); usable in modern or monitor mode
void dump_engine_state(color_ostream &out);

} // namespace autolabor
