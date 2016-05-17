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

namespace DFHack {
    namespace EventManager {
        namespace EventType {
            enum EventType {
                TICK,
                JOB_INITIATED,
                JOB_COMPLETED,
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
                PRESAVE,
                EVENT_MAX
            };
        }

        struct EventHandler {
            typedef void (*callback_t)(color_ostream&, void*); //called when the event happens
            callback_t eventHandler;
            int32_t freq;

            EventHandler(callback_t eventHandlerIn, int32_t freqIn): eventHandler(eventHandlerIn), freq(freqIn) {
            }

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
            SyndromeData(int32_t unitId_in, int32_t syndromeIndex_in): unitId(unitId_in), syndromeIndex(syndromeIndex_in) {

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
        };

        struct UnitAttackData {
            int32_t attacker;
            int32_t defender;
            int32_t wound;
        };

        struct InteractionData {
            std::string attackVerb;
            std::string defendVerb;
            int32_t attacker;
            int32_t defender;
            int32_t attackReport;
            int32_t defendReport;
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
            r = m*(r+(int32_t)h.eventHandler);
            r = m*(r+h.freq);
            return r;
        }
    };
}

#endif
