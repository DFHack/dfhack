#pragma once

#include "engines.h"
#include "workdetails.h"

// The classic autolabor engine: decides which units should have each labor
// and writes unit->status.labors directly. While enabled it sets
// game->external_flag.automatic_professions_disabled so the vanilla work
// detail system does not fight the plugin's writes.
class LegacyEngine : public LaborEngine {
public:
    LegacyEngine(WorkDetailManager *wdm) : wdm(wdm) {}

    void enable(color_ostream &out) override;
    void disable(color_ostream &out) override;
    void update(color_ostream &out) override;
    bool command(color_ostream &out, std::vector<std::string> &parameters) override;

    void map_unload();

private:
    WorkDetailManager *wdm;
    bool initialized = false;
};
