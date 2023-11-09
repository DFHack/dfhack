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

#include "Internal.h"
#include <array>
#include <map>
#include <json/json.h>

#include "Core.h"
#include "DataDefs.h"
#include "modules/Persistence.h"
#include "modules/World.h"

#include "df/historical_figure.h"

using namespace DFHack;

static std::vector<std::shared_ptr<Persistence::LegacyData>> legacy_data;
static std::multimap<std::string, size_t> index_cache;

struct Persistence::LegacyData
{
    const std::string key;
    std::string str_value;
    std::array<int, PersistentDataItem::NumInts> int_values;

    explicit LegacyData(const std::string &key) : key(key)
    {
        for (int i = 0; i < PersistentDataItem::NumInts; i++)
        {
            int_values.at(i) = -1;
        }
    }
    explicit LegacyData(Json::Value &json) : key(json["k"].asString())
    {
        str_value = json["s"].asString();
        for (int i = 0; i < PersistentDataItem::NumInts; i++)
        {
            int_values.at(i) = json["i"][i].asInt();
        }
    }
    explicit LegacyData(const df::language_name &name) : key(name.first_name)
    {
        str_value = name.nickname;
        for (int i = 0; i < PersistentDataItem::NumInts; i++)
        {
            int_values.at(i) = name.words[i];
        }
    }

    Json::Value toJSON()
    {
        Json::Value json(Json::objectValue);
        json["k"] = key;
        json["s"] = str_value;
        Json::Value ints(Json::arrayValue);
        for (int i = 0; i < PersistentDataItem::NumInts; i++)
        {
            ints[i] = int_values.at(i);
        }
        json["i"] = std::move(ints);

        return json;
    }
};

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
    CHECK_INVALID_ARGUMENT(i >= 0 && i < NumInts);
    return data->int_values.at(i);
}
int PersistentDataItem::ival(int i) const
{
    CHECK_INVALID_ARGUMENT(isValid());
    CHECK_INVALID_ARGUMENT(i >= 0 && i < NumInts);
    return data->int_values.at(i);
}

bool PersistentDataItem::isValid() const
{
    if (data == nullptr)
        return false;

    CoreSuspender suspend;

    if (legacy_data.size() <= index)
        return false;

    return legacy_data.at(index) == data;
}

void Persistence::Internal::clear()
{
    CoreSuspender suspend;

    legacy_data.clear();
    index_cache.clear();
}

void Persistence::Internal::save()
{
    CoreSuspender suspend;

    Json::Value json(Json::arrayValue);
    for (size_t i = 0; i < legacy_data.size(); i++)
    {
        if (legacy_data.at(i) != nullptr)
        {
            while (json.size() < i)
            {
                json[json.size()] = Json::Value();
            }

            json[int(i)] = legacy_data.at(i)->toJSON();
        }
    }

    auto file = writeSaveData("legacy-data");
    file << json;
}

static void convertHFigs()
{
    auto &figs = df::historical_figure::get_vector();

    auto src = figs.begin();
    while (src != figs.end() && (*src)->id > -100)
    {
        ++src;
    }

    if (src == figs.end())
    {
        return;
    }

    auto dst = src;
    while (src != figs.end())
    {
        auto fig = *src;
        if (fig->id > -100)
        {
            *dst = *src;
            ++dst;
        }
        else
        {
            if (fig->name.has_name && !fig->name.first_name.empty())
            {
                size_t index = size_t(-fig->id - 100);
                if (legacy_data.size() <= index)
                {
                    legacy_data.resize(index + 1);
                }
                legacy_data.at(index) = std::shared_ptr<Persistence::LegacyData>(new Persistence::LegacyData(fig->name));
            }
            delete fig;
        }
        ++src;
    }

    figs.erase(dst, figs.end());
}

void Persistence::Internal::load()
{
    CoreSuspender suspend;

    clear();

    auto file = readSaveData("legacy-data");
    Json::Value json;
    try
    {
        file >> json;
    }
    catch (std::exception &)
    {
        // empty file?
    }

    if (json.isArray())
    {
        legacy_data.resize(json.size());
        for (size_t i = 0; i < legacy_data.size(); i++)
        {
            if (json[int(i)].isObject())
            {
                legacy_data.at(i) = std::shared_ptr<LegacyData>(new LegacyData(json[int(i)]));
            }
        }
    }

    convertHFigs();

    for (size_t i = 0; i < legacy_data.size(); i++)
    {
        if (legacy_data.at(i) == nullptr)
        {
            continue;
        }

        index_cache.insert(std::make_pair(legacy_data.at(i)->key, i));
    }
}

PersistentDataItem Persistence::addItem(const std::string &key)
{
    if (key.empty() || !Core::getInstance().isWorldLoaded())
        return PersistentDataItem();

    CoreSuspender suspend;

    size_t index = 0;
    while (index < legacy_data.size() && legacy_data.at(index) != nullptr)
    {
        index++;
    }

    auto ptr = std::shared_ptr<LegacyData>(new LegacyData(key));

    if (index == legacy_data.size())
    {
        legacy_data.push_back(ptr);
    }
    else
    {
        legacy_data.at(index) = ptr;
    }

    index_cache.insert(std::make_pair(key, index));

    return PersistentDataItem(index, ptr);
}

PersistentDataItem Persistence::getByKey(const std::string &key, bool *added)
{
    CoreSuspender suspend;

    auto it = index_cache.find(key);

    if (added)
    {
        *added = it == index_cache.end();
    }

    if (it != index_cache.end())
    {
        return PersistentDataItem(it->second, legacy_data.at(it->second));
    }

    if (!added)
    {
        return PersistentDataItem();
    }

    return addItem(key);
}

PersistentDataItem Persistence::getByIndex(size_t index)
{
    CoreSuspender suspend;

    if (index < legacy_data.size() && legacy_data.at(index) != nullptr)
    {
        return PersistentDataItem(index, legacy_data.at(index));
    }

    return PersistentDataItem();
}

bool Persistence::deleteItem(const PersistentDataItem &item)
{
    CoreSuspender suspend;

    if (!item.isValid())
    {
        return false;
    }

    size_t index = item.get_index();
    auto range = index_cache.equal_range(item.key());
    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second == index)
        {
            index_cache.erase(it);
            break;
        }
    }
    legacy_data.at(index) = nullptr;

    return true;
}

void Persistence::getAll(std::vector<PersistentDataItem> &vec)
{
    vec.clear();

    CoreSuspender suspend;

    for (size_t i = 0; i < legacy_data.size(); i++)
    {
        if (legacy_data.at(i) != nullptr)
        {
            vec.push_back(PersistentDataItem(i, legacy_data.at(i)));
        }
    }
}

void Persistence::getAllByKeyRange(std::vector<PersistentDataItem> &vec, const std::string &min, const std::string &max)
{
    vec.clear();

    CoreSuspender suspend;

    auto begin = index_cache.lower_bound(min);
    auto end = index_cache.lower_bound(max);
    for (auto it = begin; it != end; ++it)
    {
        vec.push_back(PersistentDataItem(it->second, legacy_data.at(it->second)));
    }
}

void Persistence::getAllByKey(std::vector<PersistentDataItem> &vec, const std::string &key)
{
    vec.clear();

    CoreSuspender suspend;

    auto range = index_cache.equal_range(key);
    for (auto it = range.first; it != range.second; ++it)
    {
        vec.push_back(PersistentDataItem(it->second, legacy_data.at(it->second)));
    }
}

static std::string filterSaveFileName(std::string s)
{
    for (auto &ch : s)
    {
        if (!isalnum(ch) && ch != '-' && ch != '_')
        {
            ch = '_';
        }
    }
    return s;
}

static std::string getSaveFilePath(const std::string &world, const std::string &name)
{
    return "save/" + world + "/dfhack-" + filterSaveFileName(name) + ".dat";
}

#if defined(__GNUC__) && __GNUC__ < 5
// file stream move constructors are missing in libstdc++ before version 5.
#define FSTREAM(x) Persistence::gcc_4_fstream_shim<x>
#else
#define FSTREAM(x) x
#endif
FSTREAM(std::ifstream) Persistence::readSaveData(const std::string &name)
{
    if (!Core::getInstance().isWorldLoaded())
    {
        // No world loaded - return unopened stream.
        return FSTREAM(std::ifstream)();
    }

    return FSTREAM(std::ifstream)(getSaveFilePath(World::ReadWorldFolder(), name));
}

FSTREAM(std::ofstream) Persistence::writeSaveData(const std::string &name)
{
    if (!Core::getInstance().isWorldLoaded())
    {
        // No world loaded - return unopened stream.
        return FSTREAM(std::ofstream)();
    }

    return FSTREAM(std::ofstream)(getSaveFilePath("current", name));
}
#undef FSTREAM
