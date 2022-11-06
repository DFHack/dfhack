#pragma once

#include <modules/Maps.h>
#include <df/coord.h>
#include <df/tiletype.h>

#include <map>

class TileCache {
private:
    TileCache() = default;
    std::map<df::coord, df::tiletype> locations;
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
        if (locations.count(pos)) {
            if (type != locations.find(pos)->second){
                return true;
            }
        }
        return false;
    }
};
