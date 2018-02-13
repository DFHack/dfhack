#pragma once

#include "PluginManager.h"

#include "DataDefs.h"

#include "defs.h"

using namespace DFHack;

namespace embark_assist {
    namespace finder_ui {
        void init(DFHack::Plugin *plugin_self, embark_assist::defs::find_callbacks find_callback, uint16_t max_inorganic);
        void activate();
        void shutdown();
    }
}