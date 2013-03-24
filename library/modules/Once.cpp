
#include "modules/Once.h"
#include <unordered_set>

using namespace std;

static unordered_set<string> thingsDone;

bool DFHack::Once::alreadyDone(string bob) {
    return thingsDone.find(bob) != thingsDone.end();
}

bool DFHack::Once::doOnce(string bob) {
    return thingsDone.insert(bob).second;
}

