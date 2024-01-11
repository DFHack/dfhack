/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "Core.h"
#include "Internal.h"

#include "modules/Filesystem.h"
#include "modules/Gui.h"
#include "modules/Persistence.h"
#include "modules/World.h"

#include "df/world.h"
#include "df/world_data.h"
#include "df/world_site.h"

#include <json/json.h>

#include <unordered_map>

using namespace DFHack;

static std::unordered_map<int, std::multimap<std::string, std::shared_ptr<Persistence::DataEntry>>> store;
static std::unordered_map<size_t, std::shared_ptr<Persistence::DataEntry>> entry_cache;

size_t next_entry_id = 0;   // goes more positive
int next_fake_df_id = -101; // goes more negative

struct Persistence::DataEntry {
    const size_t entry_id;
    const int entity_id;
    const std::string key;
    int fake_df_id;
    std::string str_value;
    std::array<int, PersistentDataItem::NumInts> int_values;

    explicit DataEntry(int entity_id, const std::string &key)
    : entry_id(next_entry_id++), entity_id(entity_id), key(key) {
        fake_df_id = 0;
        for (size_t i = 0; i < PersistentDataItem::NumInts; i++)
            int_values.at(i) = -1;
    }

    explicit DataEntry(int entity_id, Json::Value &json)
    : DataEntry(entity_id, json["k"].asString()) {
        if (json.isMember("f"))
            fake_df_id = json["f"].asInt();
        str_value = json["s"].asString();
        if (json.isMember("i")) {
            for (size_t i = 0; i < PersistentDataItem::NumInts; i++)
                int_values.at(i) = json["i"].get(i, Json::Value(-1)).asInt();
        }
    }

    Json::Value toJSON() const {
        Json::Value json(Json::objectValue);
        json["k"] = key;
        if (fake_df_id < 0)
            json["f"] = fake_df_id;
        if (str_value.size())
            json["s"] = str_value;
        size_t num_set_ints = 0;
        for (size_t i = 0; i < PersistentDataItem::NumInts; i++) {
            if (int_values.at(i) != -1)
                num_set_ints = i + 1;
        }
        if (num_set_ints) {
            Json::Value ints(Json::arrayValue);
            for (size_t i = 0; i < num_set_ints; i++)
                ints.append(int_values.at(i));
            json["i"] = std::move(ints);
        }
        return json;
    }

    bool isReferencedBy(const PersistentDataItem & item) {
        return item.data.get() == this;
    }
};

int PersistentDataItem::entity_id() const {
    CHECK_INVALID_ARGUMENT(isValid());
    return data->entity_id;
}

const std::string &PersistentDataItem::key() const
{
    CHECK_INVALID_ARGUMENT(isValid());
    return data->key;
}

std::string &PersistentDataItem::val()
{
    CHECK_INVALID_ARGUMENT(isValid());
    return data->str_value;
}
const std::string &PersistentDataItem::val() const
{
    CHECK_INVALID_ARGUMENT(isValid());
    return data->str_value;
}
int &PersistentDataItem::ival(int i)
{
    CHECK_INVALID_ARGUMENT(isValid());
    CHECK_INVALID_ARGUMENT(i >= 0 && i < (int)NumInts);
    return data->int_values.at(i);
}
int PersistentDataItem::ival(int i) const
{
    CHECK_INVALID_ARGUMENT(isValid());
    CHECK_INVALID_ARGUMENT(i >= 0 && i < (int)NumInts);
    return data->int_values.at(i);
}

bool PersistentDataItem::isValid() const
{
    if (data == nullptr)
        return false;

    CoreSuspender suspend;

    return entry_cache.contains(data->entry_id) && entry_cache.at(data->entry_id) == data;
}

int PersistentDataItem::fake_df_id() {
    if (!isValid())
        return 0;

    // set it if unset
    if (data->fake_df_id == 0)
        data->fake_df_id = next_fake_df_id--;

    return data->fake_df_id;
}

void Persistence::Internal::clear(color_ostream& out) {
    CoreSuspender suspend;

    store.clear();
    entry_cache.clear();
    next_entry_id = 0;
    next_fake_df_id = -101;
}

static std::string filterSaveFileName(std::string s) {
    for (auto &ch : s) {
        if (!isalnum(ch) && ch != '-' && ch != '_')
            ch = '_';
    }
    return s;
}

static std::string getSavePath(const std::string &world) {
    return "save/" + world;
}

static std::string getSaveFilePath(const std::string &world, const std::string &name) {
    return getSavePath(world) + "/dfhack-" + filterSaveFileName(name) + ".dat";
}

void Persistence::Internal::save(color_ostream& out) {
    if (!Core::getInstance().isWorldLoaded())
        return;

    CoreSuspender suspend;

    for (auto & entity_store_entry : store) {
        int entity_id = entity_store_entry.first;
        Json::Value json(Json::arrayValue);
        for (auto & entries : entity_store_entry.second) {
            if (entries.second == nullptr)
                continue;
            json.append(entries.second->toJSON());
        }
        std::string name = (entity_id == Persistence::WORLD_ENTITY_ID) ?
            "world" : "entity-" + int_to_string(entity_id);
        auto file = std::ofstream(getSaveFilePath("current", name));
        file << json;
    }
}

static bool get_entity_id(const std::string & fname, int & entity_id) {
    if (!fname.starts_with("dfhack-entity-"))
        return false;
    entity_id = string_to_int(fname.substr(14, fname.length() - 18), -1);
    return true;
}

static void add_entry(std::multimap<std::string, std::shared_ptr<Persistence::DataEntry>> & entity_store_entry,
        std::shared_ptr<Persistence::DataEntry> entry) {
    entity_store_entry.emplace(entry->key, entry);
    entry_cache.emplace(entry->entry_id, entry);
}

static void add_entry(int entity_id, std::shared_ptr<Persistence::DataEntry> entry) {
    add_entry(store[entity_id], entry);
}

static bool load_file(const std::string & path, int entity_id) {
    Json::Value json;
    try {
        std::ifstream file(path);
        file >> json;
    } catch (std::exception &) {
        // empty file?
        return false;
    }

    if (json.isArray()) {
        auto & entity_store_entry = store[entity_id];
        for (auto & value : json) {
            std::shared_ptr<Persistence::DataEntry> entry(new Persistence::DataEntry(entity_id, value));
            if (entry->key.empty())
                continue;
            // ensure fake DF IDs remain globally unique
            next_fake_df_id = std::min(next_fake_df_id, entry->fake_df_id - 1);
            add_entry(entity_store_entry, entry);
        }
    }

    return true;
}

void Persistence::Internal::load(color_ostream& out) {
    CoreSuspender suspend;

    clear(out);

    // if we're creating a new world, there is no save directory yet
    if (Gui::matchFocusString("new_region", Gui::getDFViewscreen(true)))
        return;

    std::string world_name = World::ReadWorldFolder();
    std::string save_path = getSavePath(world_name);
    std::vector<std::string> files;
    if (0 != Filesystem::listdir(save_path, files)) {
        out.printerr("Unable to find save directory: '%s'\n", save_path.c_str());
        return;
    }

    bool found = false;
    for (auto & fname : files) {
        int entity_id = Persistence::WORLD_ENTITY_ID;
        if (fname != "dfhack-world.dat" && !get_entity_id(fname, entity_id))
            continue;

        found = true;
        std::string path = save_path + "/" + fname;
        if (!load_file(path, entity_id))
            out.printerr("Cannot load data from: '%s'\n", path.c_str());
    }

    if (found)
        return;

    // new file formats not found; attempt to load legacy file
    const std::string legacy_fname = getSaveFilePath(world_name, "legacy-data");
    if (Filesystem::exists(legacy_fname)) {
        int synthesized_entity_id = Persistence::WORLD_ENTITY_ID;
        using df::global::world;
        if (world && world->world_data && !world->world_data->active_site.empty())
            synthesized_entity_id = world->world_data->active_site[0]->id;
        load_file(legacy_fname, synthesized_entity_id);
    }
}

static bool is_good_entity_id(int entity_id) {
    return entity_id == Persistence::WORLD_ENTITY_ID || entity_id >= 0;
}

PersistentDataItem Persistence::addItem(int entity_id, const std::string &key) {
    if (!is_good_entity_id(entity_id) || key.empty() || !Core::getInstance().isWorldLoaded())
        return PersistentDataItem();

    CoreSuspender suspend;

    auto ptr = std::shared_ptr<DataEntry>(new DataEntry(entity_id, key));
    add_entry(entity_id, ptr);
    return PersistentDataItem(ptr);
}

PersistentDataItem Persistence::getByKey(int entity_id, const std::string &key, bool *added) {
    if (!is_good_entity_id(entity_id) || key.empty() || !Core::getInstance().isWorldLoaded())
        return PersistentDataItem();

    CoreSuspender suspend;

    bool found = store.contains(entity_id) && store[entity_id].contains(key);
    if (added)
        *added = !found;
    if (found)
        return PersistentDataItem(store[entity_id].find(key)->second);
    if (!added)
        return PersistentDataItem();
    return addItem(entity_id, key);
}

bool Persistence::deleteItem(const PersistentDataItem &item) {
    if (!item.isValid())
        return false;

    CoreSuspender suspend;

    auto range = store[item.entity_id()].equal_range(item.key());
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second->isReferencedBy(item)) {
            entry_cache.erase(it->second->entry_id);
            store[item.entity_id()].erase(it);
            break;
        }
    }
    return true;
}

void Persistence::getAll(std::vector<PersistentDataItem> &vec, int entity_id) {
    vec.clear();

    if (!is_good_entity_id(entity_id) || !Core::getInstance().isWorldLoaded())
        return;

    CoreSuspender suspend;

    if (!store.contains(entity_id))
        return;

    for (auto & entries : store[entity_id])
        vec.emplace_back(entries.second);
}

void Persistence::getAllByKeyRange(std::vector<PersistentDataItem> &vec, int entity_id,
        const std::string &min, const std::string &max) {
    vec.clear();

    if (!is_good_entity_id(entity_id) || !Core::getInstance().isWorldLoaded())
        return;

    CoreSuspender suspend;

    if (!store.contains(entity_id))
        return;

    auto begin = store[entity_id].lower_bound(min);
    auto end = store[entity_id].lower_bound(max);
    for (auto it = begin; it != end; ++it)
        vec.emplace_back(it->second);
}

void Persistence::getAllByKey(std::vector<PersistentDataItem> &vec, int entity_id, const std::string &key) {
    vec.clear();

    if (!is_good_entity_id(entity_id) || !Core::getInstance().isWorldLoaded())
        return;

    CoreSuspender suspend;

    if (!store.contains(entity_id))
        return;

    auto range = store[entity_id].equal_range(key);
    for (auto it = range.first; it != range.second; ++it)
        vec.emplace_back(it->second);
}
