#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Gui.h>
#include <modules/Screen.h>
#include <vector>
#include <cstdio>
#include <stack>
#include <string>
#include <cmath>

#include <VTableInterpose.h>
#include "df/item.h"
#include "df/unit.h"
#include "df/world.h"
#include "df/general_ref_item.h"
#include "df/general_ref_unit.h"

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;

using df::global::gps;

DFHACK_PLUGIN("ref-index");

#define global_id id

template<class T>
T get_from_global_id_vector(int32_t id, const std::vector<T> &vect, int32_t *cache)
{
    size_t size = vect.size();
    int32_t start=0;
    int32_t end=(int32_t)size-1;

    // Check the cached location. If it is a match, this provides O(1) lookup.
    // Otherwise it works like one binsearch iteration.
    if (size_t(*cache) < size)
    {
        T cptr = vect[*cache];
        if (cptr->global_id == id)
            return cptr;
        if (cptr->global_id < id)
            start = *cache+1;
        else
            end = *cache-1;
    }

    // Regular binsearch. The end check provides O(1) caching for missing item.
    if (start <= end && vect[end]->global_id >= id)
    {
        do {
            int32_t mid=(start+end)>>1;

            T cptr=vect[mid];
            if(cptr->global_id==id)
            {
                *cache = mid;
                return cptr;
            }
            else if(cptr->global_id>id)end=mid-1;
            else start=mid+1;
        } while(start<=end);
    }

    *cache = end+1;
    return NULL;
}

template<class T> T *find_object(int32_t id, int32_t *cache);
template<> df::item *find_object<df::item>(int32_t id, int32_t *cache) {
    return get_from_global_id_vector(id, df::global::world->items.all, cache);
}
template<> df::unit *find_object<df::unit>(int32_t id, int32_t *cache) {
    return get_from_global_id_vector(id, df::global::world->units.all, cache);
}

template<class T>
struct CachedRef {
    int32_t id;
    int32_t cache;
    CachedRef(int32_t id = -1) : id(id), cache(-1) {}
    T *target() { return find_object<T>(id, &cache); }
};

#ifdef LINUX_BUILD
struct item_hook : df::general_ref_item {
    typedef df::general_ref_item interpose_base;

    DEFINE_VMETHOD_INTERPOSE(df::item*, getItem, ())
    {
        // HUGE HACK: ASSUMES THERE ARE 4 USABLE BYTES AFTER THE OBJECT
        // This actually is true with glibc allocator due to granularity.
        return find_object<df::item>(item_id, 1+&item_id);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(item_hook, getItem);

struct unit_hook : df::general_ref_unit {
    typedef df::general_ref_unit interpose_base;

    DEFINE_VMETHOD_INTERPOSE(df::unit*, getUnit, ())
    {
        // HUGE HACK: ASSUMES THERE ARE 4 USABLE BYTES AFTER THE OBJECT
        // This actually is true with glibc allocator due to granularity.
        return find_object<df::unit>(unit_id, 1+&unit_id);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(unit_hook, getUnit);

command_result hook_refs(color_ostream &out, vector <string> & parameters)
{
    auto &hook = INTERPOSE_HOOK(item_hook, getItem);
    if (hook.is_applied())
    {
        hook.remove();
        INTERPOSE_HOOK(unit_hook, getUnit).remove();
    }
    else
    {
        hook.apply();
        INTERPOSE_HOOK(unit_hook, getUnit).apply();
    }

    if (hook.is_applied())
        out.print("Hook is applied.\n");
    else
        out.print("Hook is not applied.\n");
    return CR_OK;
}
#endif

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
#ifdef LINUX_BUILD
    commands.push_back(PluginCommand("hook-refs","Inject O(1) cached lookup into general refs.",hook_refs));
#endif

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
