#pragma once

#include <string>
#include <vector>

#include "ColorText.h"
#include "PluginManager.h"

using namespace DFHack;

// One labor-management strategy. Exactly one engine is active at a time,
// selected by the plugin's persisted mode. Engines are stateless between
// update() calls except for persisted config and small persistent caches.
class LaborEngine {
public:
    virtual ~LaborEngine() {}

    // Called when the plugin is enabled in this engine's mode (including on
    // map load). Must leave the fort's labor state consistent.
    virtual void enable(color_ostream &out) = 0;

    // Called when the plugin is disabled or the mode is switched away.
    // Must undo any labor-affecting state so vanilla behaves normally.
    virtual void disable(color_ostream &out) = 0;

    // Called every frame while active; engines throttle internally.
    virtual void update(color_ostream &out) = 0;

    // Handle the engine's command dialect. Returns false if the command
    // isn't recognized so the caller can try shared handling.
    virtual bool command(color_ostream &out, std::vector<std::string> &parameters) = 0;
};
