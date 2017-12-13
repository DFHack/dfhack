#pragma once

#include "edgeCost.h"

#include "ColorText.h"
#include "modules/MapCache.h"

#include <unordered_map>
#include <unordered_set>

using namespace std;

int32_t assignJob(DFHack::color_ostream& out, Edge firstImportantEdge, unordered_map<df::coord,df::coord,PointHash> parentMap, unordered_map<df::coord,cost_t,PointHash>& costMap, vector<int32_t>& invaders, unordered_set<df::coord,PointHash>& requiresZNeg, unordered_set<df::coord,PointHash>& requiresZPos, MapExtras::MapCache& cache, DigAbilities& abilities);

