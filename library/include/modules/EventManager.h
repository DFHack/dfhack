#pragma once
#ifndef EVENT_MANAGER_H_INCLUDED
#define EVENT_MANAGER_H_INCLUDED

#include "Core.h"
#include "Export.h"
#include "ColorText.h"
#include "PluginManager.h"
#include "Console.h"

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
        
        DFHACK_EXPORT void registerListener(EventType::EventType e, EventHandler handler, Plugin* plugin);
        DFHACK_EXPORT void registerTick(EventHandler handler, int32_t when, Plugin* plugin, bool absolute=false);
        DFHACK_EXPORT void unregister(EventType::EventType e, EventHandler handler, Plugin* plugin);
        DFHACK_EXPORT void unregisterAll(Plugin* plugin);
        void manageEvents(color_ostream& out);
        void onStateChange(color_ostream& out, state_change_event event);
    }
}

#endif
