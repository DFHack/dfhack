#pragma once

#include "Console.h"
#include "DataDefs.h"

#include "modules/Maps.h"
#include "modules/MapCache.h"

#include "df/coord.h"

#include <unordered_map>
#include <vector>

//cost is [path cost, building destruction cost, dig cost, construct cost]. Minimize constructions, then minimize dig cost, then minimize path cost.
enum CostDimension {
    Walk,
    DestroyBuilding,
    Dig,
    DestroyRoughConstruction,
    DestroySmoothConstruction,
    //Construct,
    costDim
};

typedef int64_t cost_t;

struct DigAbilities {
    cost_t costWeight[costDim];
    int32_t jobDelay[costDim];
};

//extern cost_t costWeight[costDim];
//extern int32_t jobDelay[costDim];
extern std::unordered_map<std::string, DigAbilities> digAbilities;
/*
const cost_t costWeight[] = {
//Distance
1,
//Destroy Building
2,
//Dig
10000,
//DestroyConstruction
100,
};
*/

class Edge {
public:
    //static map<df::coord, int32_t> pointCost;
    df::coord p1;
    df::coord p2;
    cost_t cost;
    Edge() {
        cost = -1;
    }
    Edge(const Edge& e): p1(e.p1), p2(e.p2), cost(e.cost) {

    }
    Edge(df::coord p1In, df::coord p2In, cost_t costIn): cost(costIn) {
        if ( p2In < p1In ) {
            p1 = p2In;
            p2 = p1In;
        } else {
            p1 = p1In;
            p2 = p2In;
        }
    }

    bool operator==(const Edge& e) const {
        return (cost == e.cost && p1 == e.p1 && p2 == e.p2);
    }

    bool operator<(const Edge& e) const {
        if ( cost != e.cost )
            return cost < e.cost;
        if ( p1.z != e.p1.z )
            return p1.z < e.p1.z;
        if ( p1 != e.p1 )
            return p1 < e.p1;
        if ( p2.z != e.p2.z )
            return p2.z < e.p2.z;
        if ( p2 != e.p2 )
            return p2 < e.p2;
        return false;
    }
};

struct PointHash {
    size_t operator()(const df::coord c) const {
        return c.x * 65537 + c.y * 17 + c.z;
    }
};

cost_t getEdgeCost(DFHack::color_ostream& out, df::coord pt1, df::coord pt2, DigAbilities& abilities);
std::vector<Edge>* getEdgeSet(DFHack::color_ostream &out, df::coord point, MapExtras::MapCache& cache, int32_t xMax, int32_t yMax, int32_t zMax, DigAbilities& abilities);
