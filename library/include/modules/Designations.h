#pragma once

namespace df {
    struct plant;
}

namespace DFHack {
    namespace Designations {
        // Mark or un-mark a plant (e.g. fell trees, gather plants)
        // Return value indicates whether the plant's designation was changed or not
        // (This can be false if markPlant() is called on an already-designated plant, for example)
        DFHACK_EXPORT bool markPlant(const df::plant *plant);
        DFHACK_EXPORT bool unmarkPlant(const df::plant *plant);
        DFHACK_EXPORT bool canMarkPlant(const df::plant *plant);
        DFHACK_EXPORT bool canUnmarkPlant(const df::plant *plant);
        DFHACK_EXPORT bool isPlantMarked(const df::plant *plant);

        // Return the tile that should be designated for this plant
        DFHACK_EXPORT df::coord getPlantDesignationTile(const df::plant *plant);
    }
}
