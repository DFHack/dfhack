#include "pause.h"
#include <VTableInterpose.h>
#include <df/global_objects.h>
#include <df/viewscreen_dwarfmodest.h>
#include <df/announcements.h>
#include <df/d_init.h>
#include <df/plotinfost.h>

#include <algorithm>

using namespace DFHack;
using namespace Pausing;
using namespace df::enums;

// marked by REQUIRE_GLOBAL in spectate.cpp
using df::global::plotinfo;
using df::global::d_init;

std::unordered_set<Lock*> PlayerLock::locks;
std::unordered_set<Lock*> AnnouncementLock::locks;

namespace pausing {
    AnnouncementLock announcementLock("monitor");
    PlayerLock playerLock("monitor");

    const size_t announcement_flag_arr_size = sizeof(decltype(df::announcements::flags)) / sizeof(df::announcement_flags);
    bool state_saved = false; // indicates whether a restore state is ok
    bool saved_states[announcement_flag_arr_size]; // state to restore
    bool locked_states[announcement_flag_arr_size]; // locked state (re-applied each frame)
    bool allow_player_pause = true; // toggles player pause ability

    using namespace df::enums;
    struct player_pause_hook : df::viewscreen_dwarfmodest {
        typedef df::viewscreen_dwarfmodest interpose_base;
        DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key>* input)) {
            if ((plotinfo->main.mode == ui_sidebar_mode::Default) && !allow_player_pause) {
                input->erase(interface_key::D_PAUSE);
            }
            INTERPOSE_NEXT(feed)(input);
        }
    };

    IMPLEMENT_VMETHOD_INTERPOSE(player_pause_hook, feed);
}
using namespace pausing;

template<typename Locks>
inline bool any_lock(Locks locks) {
    return std::any_of(locks.begin(), locks.end(), [](Lock* lock) { return lock->isLocked(); });
}

template<typename Locks, typename LockT>
inline bool only_lock(Locks locks, LockT* this_lock) {
    return std::all_of(locks.begin(), locks.end(), [&](Lock* lock) {
        if (lock == this_lock) {
            return lock->isLocked();
        }
        return !lock->isLocked();
    });
}

template<typename Locks, typename LockT>
inline bool only_or_none_locked(Locks locks, LockT* this_lock) {
    for (auto &L: locks) {
        if (L == this_lock) {
            continue;
        }
        if (L->isLocked()) {
            return false;
        }
    }
    return true;
}

template<typename Locks>
inline bool reportLockedLocks(color_ostream &out, Locks locks) {
    out.color(DFHack::COLOR_YELLOW);
    for (auto &L: locks) {
        if (L->isLocked()) {
            out.print("Lock: '%s'\n", L->name.c_str());
        }
    }
    out.reset_color();
    return true;
}

bool AnnouncementLock::captureState() {
    if (only_or_none_locked(locks, this)) {
        for (size_t i = 0; i < announcement_flag_arr_size; ++i) {
            locked_states[i] = d_init->announcements.flags[i].bits.PAUSE;
        }
        return true;
    }
    return false;
}

void AnnouncementLock::lock() {
    Lock::lock();
    captureState();
}

bool AnnouncementLock::isAnyLocked() const {
    return any_lock(locks);
}

bool AnnouncementLock::isOnlyLocked() const {
    return only_lock(locks, this);
}

void AnnouncementLock::reportLocks(color_ostream &out) {
    reportLockedLocks(out, locks);
}

bool PlayerLock::isAnyLocked() const {
    return any_lock(locks);
}

bool PlayerLock::isOnlyLocked() const {
    return only_lock(locks, this);
}

void PlayerLock::reportLocks(color_ostream &out) {
    reportLockedLocks(out, locks);
}

bool World::DisableAnnouncementPausing() {
    if (!announcementLock.isAnyLocked()) {
        for (auto& flag : d_init->announcements.flags) {
            flag.bits.PAUSE = false;
            //out.print("pause: %d\n", flag.bits.PAUSE);
        }
        return true;
    }
    return false;
}

bool World::SaveAnnouncementSettings() {
    if (!announcementLock.isAnyLocked()) {
        for (size_t i = 0; i < announcement_flag_arr_size; ++i) {
            saved_states[i] = d_init->announcements.flags[i].bits.PAUSE;
        }
        state_saved = true;
        return true;
    }
    return false;
}

bool World::RestoreAnnouncementSettings() {
    if (!announcementLock.isAnyLocked() && state_saved) {
        for (size_t i = 0; i < announcement_flag_arr_size; ++i) {
            d_init->announcements.flags[i].bits.PAUSE = saved_states[i];
        }
        return true;
    }
    return false;
}

bool World::EnablePlayerPausing() {
    if (!playerLock.isAnyLocked()) {
        allow_player_pause = true;
    }
    return allow_player_pause;
}

bool World::DisablePlayerPausing() {
    if (!playerLock.isAnyLocked()) {
        allow_player_pause = false;
    }
    return !allow_player_pause;
}

bool World::IsPlayerPausingEnabled() {
    return allow_player_pause;
}

void World::Update() {
    static bool did_once = false;
    if (!did_once) {
        did_once = true;
        INTERPOSE_HOOK(player_pause_hook, feed).apply();
    }
    if (announcementLock.isAnyLocked()) {
        for (size_t i = 0; i < announcement_flag_arr_size; ++i) {
            d_init->announcements.flags[i].bits.PAUSE = locked_states[i];
        }
    }
}
