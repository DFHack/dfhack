#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "modules/Persistence.h"

#include <df/unit.h>
#include <df/work_detail.h>
#include <df/work_detail_icon_type.h>
#include <df/work_detail_mode.h>

using namespace DFHack;
using namespace df::enums;

// Manages the plugin-owned work details through which the modern engine
// expresses assignments. Managed details are identified by an "auto:" name
// prefix so the player can see (and, with caveats, edit) what the tool is
// doing in the vanilla work details screen.
//
// The plugin never writes to unit->status.labors directly; all changes go
// through work detail membership and df::unit flags4.only_do_assigned_jobs,
// followed by a call to Units::setAutomaticProfessions() which asks the game
// to recompute the labor matrix.

class WorkDetailManager {
public:
    static const char * const NAME_PREFIX; // "auto:"

    // Rescan labor_info.work_details, adopting any surviving managed details
    // and reloading the flagged-unit set. Call on map load before use.
    void reset();

    // Remove all managed details, clear specialization flags we set, and
    // recompute every citizen. Call when the modern engine is disabled.
    void shutdown();

    // Find a managed detail by exact name (including prefix).
    df::work_detail *find_detail(const std::string &name);

    // Find or create a managed detail with the given name, icon, and mode.
    // Coverage (allowed_labors) is only set on creation; callers that change
    // coverage or mode afterwards must call touch_all() since pass 1 of the
    // game's labor computation clears covered labors for every unit.
    df::work_detail *ensure_detail(const std::string &name,
        df::work_detail_icon_type icon, df::work_detail_mode mode);

    // Delete a managed detail entirely (labor becomes unrestricted again).
    void delete_detail(df::work_detail *wd);

    // Set the detail's coverage of a labor. Requires touch_all() semantics;
    // we just mark every citizen dirty.
    void cover(df::work_detail *wd, df::unit_labor labor, bool on);

    // Add/remove a unit from a detail's assigned_units (kept sorted).
    void set_membership(df::work_detail *wd, int32_t unit_id, bool want);

    // Mark a unit as only doing assigned work (the game's "specialized"
    // flag). We track which units we flagged so shutdown() doesn't strip
    // flags the player set themselves.
    void set_specialized(df::unit *u, bool on);

    // Queue a unit for labor recomputation at commit().
    void touch(df::unit *u);

    // Queue every citizen for recomputation; needed when detail coverage or
    // mode changes since non-Everybody details clear their labors globally.
    void touch_all();

    // Apply queued recomputations via Units::setAutomaticProfessions().
    void commit();

    // Enumerate details whose name starts with the managed prefix.
    std::vector<df::work_detail*> managed_details();

    // Unit ids we have flagged as only_do_assigned_jobs.
    const std::set<int32_t> &flagged_units() const { return flagged; }

    bool is_managed(df::work_detail *wd);

private:
    void load_flagged();
    void save_flagged();

    std::set<int32_t> dirty;
    std::set<int32_t> flagged;
    bool recompute_all = false;
    PersistentDataItem flagged_cfg;
};
