#pragma once
#ifndef EVENT_MANAGER_H_INCLUDED
#define EVENT_MANAGER_H_INCLUDED

#include "Core.h"
#include "Export.h"
#include "ColorText.h"
#include "PluginManager.h"
#include "Console.h"
#include "DataDefs.h"

#include <df/coord.h>

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
//                EQUIPMENT_CHANGE,
                EVENT_MAX
            };
        }

        struct EventHandler {
            void (*eventHandler)(color_ostream&, void*); //called when the event happens
            int32_t freq;

            EventHandler(void (*eventHandlerIn)(color_ostream&, void*), int32_t freqIn): eventHandler(eventHandlerIn), freq(freqIn) {
            }

            bool operator==(EventHandler& handle) const {
                return eventHandler == handle.eventHandler && freq == handle.freq;
            }
            bool operator!=(EventHandler& handle) const {
                return !( *this == handle);
            }
        };

        struct SyndromeData {
            int32_t unitId;
            int32_t syndromeIndex;
            SyndromeData(int32_t unitId_in, int32_t syndromeIndex_in): unitId(unitId_in), syndromeIndex(syndromeIndex_in) {

            }
        };
        
        /*struct InventoryData {
            int32_t unitId;
            vector<df::unit_inventory_item>* oldItems;
        };*/
        
        DFHACK_EXPORT void registerListener(EventType::EventType e, EventHandler handler, Plugin* plugin);
        DFHACK_EXPORT void registerTick(EventHandler handler, int32_t when, Plugin* plugin, bool absolute=false);
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
}

#endif
