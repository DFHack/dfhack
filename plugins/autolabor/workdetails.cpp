#include "workdetails.h"

#include <algorithm>

#include <MiscUtils.h>

#include "modules/Units.h"
#include "modules/World.h"

#include <df/global_objects.h>
#include <df/plotinfost.h>
#include <df/world.h>

using namespace DFHack;
using namespace df::enums;

using df::global::plotinfo;
using df::global::world;

const char * const WorkDetailManager::NAME_PREFIX = "auto:";

static std::vector<df::work_detail*> &details()
{
    return plotinfo->labor_info.work_details;
}

bool WorkDetailManager::is_managed(df::work_detail *wd)
{
    return wd && wd->name.compare(0, strlen(NAME_PREFIX), NAME_PREFIX) == 0;
}

void WorkDetailManager::reset()
{
    dirty.clear();
    flagged.clear();
    recompute_all = false;
    flagged_cfg = World::GetPersistentSiteData("autolabor/flagged");
    load_flagged();
}

void WorkDetailManager::load_flagged()
{
    flagged.clear();
    if (!flagged_cfg.isValid())
        return;
    size_t count = flagged_cfg.data_size() / PersistentDataItem::int28_size;
    for (size_t i = 0; i < count; i++)
        flagged.insert(flagged_cfg.get_int28(i * PersistentDataItem::int28_size));
}

void WorkDetailManager::save_flagged()
{
    if (!flagged_cfg.isValid())
        flagged_cfg = World::AddPersistentSiteData("autolabor/flagged");
    if (!flagged_cfg.isValid())
        return;

    size_t off = 0;
    for (int32_t id : flagged)
    {
        flagged_cfg.set_int28(off, id);
        off += PersistentDataItem::int28_size;
    }
    flagged_cfg.val().resize(off);
}

std::vector<df::work_detail*> WorkDetailManager::managed_details()
{
    std::vector<df::work_detail*> out;
    for (auto wd : details())
        if (is_managed(wd))
            out.push_back(wd);
    return out;
}

df::work_detail *WorkDetailManager::find_detail(const std::string &name)
{
    for (auto wd : details())
        if (is_managed(wd) && wd->name == name)
            return wd;
    return NULL;
}

df::work_detail *WorkDetailManager::ensure_detail(const std::string &name,
    df::work_detail_icon_type icon, df::work_detail_mode mode)
{
    if (auto wd = find_detail(name))
        return wd;

    auto wd = df::allocate<df::work_detail>();
    wd->name = name;
    wd->icon = icon;
    wd->flags.bits.mode = mode;
    details().push_back(wd);

    // a new non-Everybody detail clears its covered labors globally
    if (mode != work_detail_mode::EverybodyDoesThis)
        touch_all();
    else
        recompute_all = true;

    return wd;
}

void WorkDetailManager::delete_detail(df::work_detail *wd)
{
    auto &vec = details();
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        if (*it == wd)
        {
            for (int32_t id : wd->assigned_units)
                if (auto u = df::unit::find(id))
                    touch(u);
            vec.erase(it);
            delete wd;
            touch_all();
            return;
        }
    }
}

void WorkDetailManager::cover(df::work_detail *wd, df::unit_labor labor, bool on)
{
    if (labor < 0 || labor > ENUM_LAST_ITEM(unit_labor))
        return;
    if (wd->allowed_labors[labor] != on)
    {
        wd->allowed_labors[labor] = on;
        // coverage changes affect every unit, not just members
        recompute_all = true;
    }
}

void WorkDetailManager::set_membership(df::work_detail *wd, int32_t unit_id, bool want)
{
    auto &vec = wd->assigned_units;
    auto it = std::lower_bound(vec.begin(), vec.end(), unit_id);
    bool has = (it != vec.end() && *it == unit_id);

    if (want && !has)
    {
        vec.insert(it, unit_id);
        if (auto u = df::unit::find(unit_id))
            touch(u);
    }
    else if (!want && has)
    {
        vec.erase(it);
        if (auto u = df::unit::find(unit_id))
            touch(u);
    }
}

void WorkDetailManager::set_specialized(df::unit *u, bool on)
{
    if (!u)
        return;

    if (on)
    {
        if (!u->flags4.bits.only_do_assigned_jobs)
        {
            u->flags4.bits.only_do_assigned_jobs = true;
            touch(u);
        }
        flagged.insert(u->id);
    }
    else
    {
        // only strip flags we set ourselves
        if (flagged.count(u->id))
        {
            flagged.erase(u->id);
            if (u->flags4.bits.only_do_assigned_jobs)
            {
                u->flags4.bits.only_do_assigned_jobs = false;
                touch(u);
            }
        }
    }
}

void WorkDetailManager::touch(df::unit *u)
{
    if (u)
        dirty.insert(u->id);
}

void WorkDetailManager::touch_all()
{
    recompute_all = true;
}

void WorkDetailManager::commit()
{
    if (recompute_all)
    {
        dirty.clear();
        for (auto u : world->units.active)
            if (Units::isCitizen(u))
                touch(u);
        recompute_all = false;
    }

    for (int32_t id : dirty)
        if (auto u = df::unit::find(id))
            Units::setAutomaticProfessions(u);
    dirty.clear();

    save_flagged();
}

void WorkDetailManager::shutdown()
{
    // remove all managed details
    for (auto wd : managed_details())
        delete_detail(wd);

    // clear specialization flags we set on any still-living unit
    for (int32_t id : flagged)
        if (auto u = df::unit::find(id))
            if (u->flags4.bits.only_do_assigned_jobs)
            {
                u->flags4.bits.only_do_assigned_jobs = false;
                touch(u);
            }
    flagged.clear();
    save_flagged();

    // detail removal affects everyone
    touch_all();
    commit();
}
