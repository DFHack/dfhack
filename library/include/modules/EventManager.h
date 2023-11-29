#pragma once
#ifndef EVENT_MANAGER_H_INCLUDED
#define EVENT_MANAGER_H_INCLUDED

#include "Core.h"
#include "Export.h"
#include "ColorText.h"
#include "PluginManager.h"
#include "Console.h"
#include "DataDefs.h"

#include "df/coord.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"
#include "df/unit_wound.h"
#include "df/construction.h"

namespace DFHack {
    namespace EventManager {
        namespace EventType {
            // NOTICE: keep this list synchronized with the eventHandlers array
            // in plugins/eventful.cpp or else events will go to the wrong
            // handlers.
            enum EventType {
                TICK,
                JOB_INITIATED,
                JOB_STARTED, //has a worker
                JOB_COMPLETED,
                UNIT_NEW_ACTIVE,
                UNIT_DEATH,
                ITEM_CREATED,
                BUILDING,
                CONSTRUCTION,
                SYNDROME,
                INVASION,
                INVENTORY_CHANGE,
                REPORT,
                UNIT_ATTACK,
                UNLOAD,
                INTERACTION,
                EVENT_MAX
            };
        }

        struct EventHandler {
            typedef void (*callback_t)(color_ostream&, void*); //called when the event happens
            const callback_t eventHandler;
            const int32_t freq; //how often event is allowed to fire (in ticks) use 0 to always fire when possible
            int32_t when = -1; //when to fire event (global tick count)

            EventHandler(callback_t eventHandlerIn, int32_t freqIn) :
                    eventHandler(eventHandlerIn),
                    freq(freqIn) {}

            bool operator==(const EventHandler& handle) const {
                return eventHandler == handle.eventHandler && freq == handle.freq;
            }
            bool operator!=(const EventHandler& handle) const {
                return !( *this == handle);
            }
        };

        struct SyndromeData {
            int32_t unitId;
            int32_t syndromeIndex;
            SyndromeData(int32_t unitId_in, int32_t syndromeIndex_in): unitId(unitId_in), syndromeIndex(syndromeIndex_in) {}
            bool operator==(const SyndromeData &other) const {
                return unitId == other.unitId && syndromeIndex == other.syndromeIndex;
            }
        };

        struct InventoryItem {
            //it has to keep the id of an item because the item itself may have been deallocated
            int32_t itemId;
            df::unit_inventory_item item;
            InventoryItem() {}
            InventoryItem(int32_t id_in, df::unit_inventory_item item_in): itemId(id_in), item(item_in) {}
        };
        struct InventoryChangeData {
            int32_t unitId;
            InventoryItem* item_old;
            InventoryItem* item_new;
            InventoryChangeData() {}
            InventoryChangeData(int32_t id_in, InventoryItem* old_in, InventoryItem* new_in): unitId(id_in), item_old(old_in), item_new(new_in) {}
            bool operator==(const InventoryChangeData &other) const {
                bool unit = unitId == other.unitId;
                bool newItem = (item_new && other.item_new && item_new->itemId == other.item_new->itemId) ||
                               (!item_new && item_new == other.item_new);
                bool oldItem = (item_old && other.item_old && item_old->itemId == other.item_old->itemId) ||
                               (!item_old && item_old == other.item_old);
                return unit && newItem && oldItem;
            }
        };

        struct UnitAttackData {
            int32_t report_id;
            int32_t attacker;
            int32_t defender;
            int32_t wound;

            bool operator==(const UnitAttackData &other) const {
                // fairly sure the report_id is the only thing that matters
                return report_id == other.report_id && wound == other.wound;
            }
        };

        struct InteractionData {
            std::string attackVerb;
            std::string defendVerb;
            int32_t attacker;
            int32_t defender;
            int32_t attackReport;
            int32_t defendReport;
            bool operator==(const InteractionData &other) const {
                bool reports = attackReport == other.attackReport && defendReport == other.defendReport;
                // based on code in the manager it doesn't need reports or verbs checked.
                // since the units are deduced (it seems) from the reports... this
                return reports;
            }
        };

        DFHACK_EXPORT void registerListener(EventType::EventType e, EventHandler handler, Plugin* plugin);
        DFHACK_EXPORT int32_t registerTick(EventHandler handler, int32_t when, Plugin* plugin, bool absolute=false);
        DFHACK_EXPORT void unregister(EventType::EventType e, EventHandler handler, Plugin* plugin);
        DFHACK_EXPORT void unregisterAll(Plugin* plugin);
        void manageEvents(color_ostream& out);
        void onStateChange(color_ostream& out, state_change_event event);
    }
}

namespace std {
    template <>
    struct hash<df::coord> {
        std::size_t operator()(const df::coord& c) const {
            size_t r = 17;
            const size_t m = 65537;
            r = m*(r+c.x);
            r = m*(r+c.y);
            r = m*(r+c.z);
            return r;
        }
    };
    template <>
    struct hash<DFHack::EventManager::EventHandler> {
        std::size_t operator()(const DFHack::EventManager::EventHandler& h) const {
            size_t r = 17;
            const size_t m = 65537;
            r = m*(r+(intptr_t)h.eventHandler);
            r = m*(r+h.freq);
            return r;
        }
    };
    template <>
    struct hash<df::construction> {
        std::size_t operator()(const df::construction& construct) const {
            auto &c = construct.pos;
            size_t r = 17;
            const size_t m = 65537;
            r = m*(r+c.x);
            r = m*(r+c.y);
            r = m*(r+c.z);
            return r;
        }
    };
    template <>
    struct hash<DFHack::EventManager::SyndromeData> {
        std::size_t operator()(const DFHack::EventManager::SyndromeData& syndrome) const {
            size_t r = 43;
            const size_t m = 65537;
            r = m*(r+syndrome.unitId);
            r = m*(r+syndrome.syndromeIndex);
            return r;
        }
    };
    template <>
    struct hash<DFHack::EventManager::InventoryChangeData> {
        std::size_t operator()(const DFHack::EventManager::InventoryChangeData& icd) const {
            size_t r = 43;
            const size_t m = 65537;
            r = m*(r+icd.unitId);
            if (icd.item_new) {
                r=m*(r+icd.item_new->itemId);
            }
            if (icd.item_old) {
                r=m*(r+(2*icd.item_old->itemId));
            }
            return r;
        }
    };
    template <>
    struct hash<DFHack::EventManager::UnitAttackData> {
        std::size_t operator()(const DFHack::EventManager::UnitAttackData& uad) const {
            size_t r = 43;
            const size_t m = 65537;
            r = m*(r+uad.report_id);
            r = m*(r+uad.attacker);
            r = m*(r+uad.defender);
            r = m*(r+uad.wound);
            return r;
        }
    };
    template <>
    struct hash<DFHack::EventManager::InteractionData> {
        std::size_t operator()(const DFHack::EventManager::InteractionData& interactionData) const {
            size_t r = 43;
            const size_t m = 65537;
            r = m*(r+interactionData.attackReport);
            r = m*(r+interactionData.defendReport);
            r = m*(r+interactionData.attacker);
            r = m*(r+interactionData.defender);
            return r;
        }
    };
}

namespace df{
    inline bool operator==(const df::construction &A, const df::construction &B){
        return A.pos == B.pos;
    }
}

#endif
