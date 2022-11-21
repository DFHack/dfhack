#pragma once

#include <modules/Maps.h>
#include <df/coord.h>
#include <df/tiletype.h>
#include <modules/EventManager.h> //hash functions (they should probably get moved at this point, the ones that aren't specifically for EM anyway)

#include <unordered_map>

class TileCache {
private:
    TileCache() = default;
    std::unordered_map<df::coord, df::tiletype> locations;
public:
    static TileCache& Get() {
        static TileCache instance;
        return instance;
    }

    void cache(const df::coord &pos, df::tiletype type) {
        locations.emplace(pos, type);
    }

    void uncache(const df::coord &pos) {
        locations.erase(pos);
    }

    bool hasChanged(const df::coord &pos, const df::tiletype &type) {
        return locations.count(pos) && type != locations[pos];
    }
};
