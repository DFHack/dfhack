//  stockpiles plugin
#include "OrganicMatLookup.h"
#include "StockpileSerializer.h"
#include "StockpileUtils.h"

//  dfhack
#include "Debug.h"
#include "MiscUtils.h"
#include "modules/Items.h"

//  df
#include "df/building_stockpilest.h"
#include "df/creature_raw.h"
#include "df/inorganic_raw.h"
#include "df/item_quality.h"
#include <df/itemdef_ammost.h>
#include <df/itemdef_armorst.h>
#include <df/itemdef_glovesst.h>
#include <df/itemdef_helmst.h>
#include <df/itemdef_pantsst.h>
#include <df/itemdef_shieldst.h>
#include <df/itemdef_shoesst.h>
#include <df/itemdef_trapcompst.h>
#include <df/itemdef_weaponst.h>
#include "df/furniture_type.h"
#include "df/material.h"
#include "df/plant_raw.h"
#include "df/stockpile_group_set.h"

// protobuf
#include <google/protobuf/io/zero_copy_stream_impl.h>

using std::string;
using std::vector;

using namespace DFHack;
using namespace df::enums;
using namespace google::protobuf;
using namespace dfstockpiles;
using df::global::world;

using std::placeholders::_1;

namespace DFHack {
    DBG_EXTERN(stockpiles, log);
}

static struct OtherMatsFurniture {
    const std::map<int, string> mats;

    OtherMatsFurniture() : mats(getMats()) {}

    std::map<int, string> getMats() {
        std::map<int, string> ret;
        ret.emplace(0, "WOOD");
        ret.emplace(1, "PLANT_CLOTH");
        ret.emplace(2, "BONE");
        ret.emplace(3, "TOOTH");
        ret.emplace(4, "HORN");
        ret.emplace(5, "PEARL");
        ret.emplace(6, "SHELL");
        ret.emplace(7, "LEATHER");
        ret.emplace(8, "SILK");
        ret.emplace(9, "AMBER");
        ret.emplace(10, "CORAL");
        ret.emplace(11, "GREEN_GLASS");
        ret.emplace(12, "CLEAR_GLASS");
        ret.emplace(13, "CRYSTAL_GLASS");
        ret.emplace(14, "YARN");
        return ret;
    }
} mOtherMatsFurniture;

static struct OtherMatsBars {
    const std::map<int, string> mats;

    OtherMatsBars() : mats(getMats()) {}

    std::map<int, string> getMats() {
        std::map<int, string> ret;
        ret.emplace(0, "COAL");
        ret.emplace(1, "POTASH");
        ret.emplace(2, "ASH");
        ret.emplace(3, "PEARLASH");
        ret.emplace(4, "SOAP");
        return ret;
    }
} mOtherMatsBars;

static struct OtherMatsBlocks {
    const std::map<int, string> mats;

    OtherMatsBlocks() : mats(getMats()) {}

    std::map<int, string> getMats() {
        std::map<int, string> ret;
        ret.emplace(0, "GREEN_GLASS");
        ret.emplace(1, "CLEAR_GLASS");
        ret.emplace(2, "CRYSTAL_GLASS");
        ret.emplace(3, "WOOD");
        return ret;
    }
} mOtherMatsBlocks;

static struct OtherMatsFinishedGoods {
    const std::map<int, string> mats;

    OtherMatsFinishedGoods() : mats(getMats()) {}

    std::map<int, string> getMats() {
        std::map<int, string> ret;
        ret.emplace(0, "WOOD");
        ret.emplace(1, "PLANT_CLOTH");
        ret.emplace(2, "BONE");
        ret.emplace(3, "TOOTH");
        ret.emplace(4, "HORN");
        ret.emplace(5, "PEARL");
        ret.emplace(6, "SHELL");
        ret.emplace(7, "LEATHER");
        ret.emplace(8, "SILK");
        ret.emplace(9, "AMBER");
        ret.emplace(10, "CORAL");
        ret.emplace(11, "GREEN_GLASS");
        ret.emplace(12, "CLEAR_GLASS");
        ret.emplace(13, "CRYSTAL_GLASS");
        ret.emplace(14, "YARN");
        ret.emplace(15, "WAX");
        return ret;
    }
} mOtherMatsFinishedGoods;

static struct OtherMatsWeaponsArmor {
    const std::map<int, string> mats;

    OtherMatsWeaponsArmor() : mats(getMats()) {}

    std::map<int, string> getMats() {
        std::map<int, string> ret;
        ret.emplace(0, "WOOD");
        ret.emplace(1, "PLANT_CLOTH");
        ret.emplace(2, "BONE");
        ret.emplace(3, "SHELL");
        ret.emplace(4, "LEATHER");
        ret.emplace(5, "SILK");
        ret.emplace(6, "GREEN_GLASS");
        ret.emplace(7, "CLEAR_GLASS");
        ret.emplace(8, "CRYSTAL_GLASS");
        ret.emplace(9, "YARN");
        return ret;
    }
} mOtherMatsWeaponsArmor;

StockpileSerializer::StockpileSerializer(df::building_stockpilest* stockpile)
    : mPile(stockpile) { }

StockpileSerializer::~StockpileSerializer() { }

bool StockpileSerializer::serialize_to_ostream(std::ostream* output, uint32_t includedElements) {
    if (output->fail())
        return false;
    mBuffer.Clear();
    write(includedElements);
    {
        io::OstreamOutputStream zero_copy_output(output);
        if (!mBuffer.SerializeToZeroCopyStream(&zero_copy_output))
            return false;
    }
    return output->good();
}

bool StockpileSerializer::serialize_to_file(const std::string& file, uint32_t includedElements) {
    std::fstream output(file, std::ios::out | std::ios::binary | std::ios::trunc);
    if (output.fail()) {
        WARN(log).print("ERROR: failed to open file for writing: '%s'\n",
                file.c_str());
        return false;
    }
    return serialize_to_ostream(&output, includedElements);
}

bool StockpileSerializer::parse_from_istream(std::istream* input, DeserializeMode mode, const std::string& filter) {
    if (input->fail())
        return false;
    mBuffer.Clear();
    io::IstreamInputStream zero_copy_input(input);
    const bool res = mBuffer.ParseFromZeroCopyStream(&zero_copy_input)
            && input->eof();
    if (res)
        read(mode, filter);
    return res;
}

bool StockpileSerializer::unserialize_from_file(const std::string& file, DeserializeMode mode, const string& filter) {
    std::fstream input(file, std::ios::in | std::ios::binary);
    if (input.fail()) {
        WARN(log).print("failed to open file for reading: '%s'\n",
                file.c_str());
        return false;
    }
    return parse_from_istream(&input, mode, filter);
}

/**
 * Find an enum's value based off the string label.
 * @param traits the enum's trait struct
 * @param token the string value in key_table
 * @return the enum's value,  -1 if not found
 */
template <typename E>
static typename df::enum_traits<E>::base_type linear_index(df::enum_traits<E> traits, const std::string& token) {
    auto j = traits.first_item_value;
    auto limit = traits.last_item_value;
    // sometimes enums start at -1, which is bad news for array indexing
    if (j < 0) {
        j += abs(traits.first_item_value);
        limit += abs(traits.first_item_value);
    }
    for (; j <= limit; ++j) {
        if (token.compare(traits.key_table[j]) == 0)
            return j;
    }
    return -1;
}

static bool matches_filter(const string& filter, const string& name) {
    if (!filter.size())
        return true;
    DEBUG(log).print("searching for '%s' in '%s'\n", filter.c_str(), name.c_str());
    return std::search(name.begin(), name.end(), filter.begin(), filter.end(),
        [](unsigned char ch1, unsigned char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    ) != name.end();
}

static void set_flag(const char* name, const string& filter, bool all, char val, bool enabled, bool& elem) {
    if ((all || enabled) && matches_filter(filter, name)) {
        DEBUG(log).print("setting %s to %d\n", name, val);
        elem = val;
    }
}

static void set_filter_elem(const char* subcat, const string& filter, char val,
        const string& name, const string& id, char& elem) {
    if (matches_filter(filter, subcat + ((*subcat ? "/" : "") + name))) {
        DEBUG(log).print("setting %s (%s) to %d\n", name.c_str(), id.c_str(), val);
        elem = val;
    }
}

template<typename T_val, typename T_id>
static void set_filter_elem(const char* subcat, const string& filter, T_val val,
        const string& name, T_id id, T_val& elem) {
    if (matches_filter(filter, subcat + ((*subcat ? "/" : "") + name))) {
        DEBUG(log).print("setting %s (%d) to %d\n", name.c_str(), (int32_t)id, val);
        elem = val;
    }
}

/**
 * There are many repeated (un)serialization cases throughout the stockpile_settings structure,
 * so the most common cases have been generalized into generic functions using lambdas.
 *
 * The basic process to serialize a stockpile_settings structure is:
 * 1. loop through the list
 * 2. for every element that is TRUE:
 * 3.   map the specific stockpile_settings index into a general material, creature, etc index
 * 4.   verify that type is allowed in the list (e.g.,  no stone in gems stockpiles)
 * 5.   add it to the protobuf using FuncWriteExport
 *
 * The unserialization process is the same in reverse.
 */
static bool serialize_list_itemdef(FuncWriteExport add_value,
        std::vector<char> list,
        std::vector<df::itemdef*> items,
        item_type::item_type type) {
    bool all = true;
    for (size_t i = 0; i < list.size(); ++i) {
        if (!list.at(i)) {
            all = false;
            continue;
        }
        const df::itemdef* a = items.at(i);
        // skip procedurally generated items
        if (a->base_flags.is_set(df::itemdef_flags::GENERATED))
            continue;
        ItemTypeInfo ii;
        if (!ii.decode(type, i))
            continue;
        DEBUG(log).print("adding itemdef type %s\n", ii.getToken().c_str());
        add_value(ii.getToken());
    }
    return all;
}

static void unserialize_list_itemdef(const char* subcat, bool all, char val, const string& filter,
        FuncReadImport read_value, int32_t list_size, std::vector<char>& pile_list, item_type::item_type type) {
    int num_elems = Items::getSubtypeCount(type);
    pile_list.resize(num_elems, '\0');
    if (all) {
        for (auto idx = 0; idx < num_elems; ++idx) {
            ItemTypeInfo ii;
            ii.decode(type, idx);
            set_filter_elem(subcat, filter, val, ii.toString(), idx, pile_list.at(idx));
        }
        return;
    }

    for (auto i = 0; i < list_size; ++i) {
        string id = read_value(i);
        ItemTypeInfo ii;
        if (!ii.find(id))
            continue;
        if (ii.subtype < 0 || size_t(ii.subtype) >= pile_list.size()) {
            WARN(log).print("item type index invalid: %d\n", ii.subtype);
            continue;
        }
        set_filter_elem(subcat, filter, val, id, ii.subtype, pile_list.at(ii.subtype));
    }
}

static bool serialize_list_quality(FuncWriteExport add_value,
        const bool(&quality_list)[7]) {
    using df::enums::item_quality::item_quality;
    using quality_traits = df::enum_traits<item_quality>;

    bool all = true;
    for (size_t i = 0; i < 7; ++i) {
        if (!quality_list[i]) {
            all = false;
            continue;
        }
        const std::string f_type(quality_traits::key_table[i]);
        add_value(f_type);
        DEBUG(log).print("adding quality %s\n", f_type.c_str());
    }
    return all;
}

static void quality_clear(bool(&pile_list)[7]) {
    std::fill(pile_list, pile_list + 7, false);
}

static void unserialize_list_quality(const char* subcat, bool all, bool val, const string& filter,
        FuncReadImport read_value, int32_t list_size, bool(&pile_list)[7]) {
    if (all) {
        for (auto idx = 0; idx < 7; ++idx) {
            string id = ENUM_KEY_STR(item_quality, (df::item_quality)idx);
            set_filter_elem(subcat, filter, val, id, idx, pile_list[idx]);
        }
        return;
    }

    using df::enums::item_quality::item_quality;
    df::enum_traits<item_quality> quality_traits;
    for (int i = 0; i < list_size; ++i) {
        const std::string quality = read_value(i);
        df::enum_traits<item_quality>::base_type idx = linear_index(quality_traits, quality);
        if (idx < 0) {
            WARN(log).print("invalid quality token: %s\n", quality.c_str());
            continue;
        }
        set_filter_elem(subcat, filter, val, quality, idx, pile_list[idx]);
    }
}

static string other_mats_index(const std::map<int, std::string> other_mats,
        int idx) {
    auto it = other_mats.find(idx);
    if (it == other_mats.end())
        return std::string();
    return it->second;
}

static int other_mats_token(const std::map<int, std::string> other_mats,
        const std::string& token) {
    for (auto it = other_mats.begin(); it != other_mats.end(); ++it) {
        if (it->second == token)
            return it->first;
    }
    return -1;
}

static bool serialize_list_other_mats(
            const std::map<int, std::string> other_mats,
            FuncWriteExport add_value,
            std::vector<char> list) {
    bool all = true;
    for (size_t i = 0; i < list.size(); ++i) {
        if (!list.at(i)) {
            all = false;
            continue;
        }
        const std::string token = other_mats_index(other_mats, i);
        if (token.empty()) {
            WARN(log).print("invalid other material with index %zd\n", i);
            continue;
        }
        add_value(token);
        DEBUG(log).print("other mats %zd is %s\n", i, token.c_str());
    }
    return all;
}

static void unserialize_list_other_mats(const char* subcat, bool all, char val, const string& filter,
            const std::map<int, std::string> other_mats, FuncReadImport read_value, int32_t list_size, std::vector<char>& pile_list) {
    size_t num_elems = other_mats.size();
    pile_list.resize(num_elems, '\0');

    if (all) {
        for (auto & entry : other_mats)
            set_filter_elem(subcat, filter, val, entry.second, entry.first, pile_list.at(entry.first));
        return;
    }

    for (int i = 0; i < list_size; ++i) {
        const std::string token = read_value(i);
        size_t idx = other_mats_token(other_mats, token);
        if (idx < 0) {
            WARN(log).print("invalid other mat with token %s\n", token.c_str());
            continue;
        }
        if (idx >= num_elems) {
            WARN(log).print("other_mats index too large! idx[%zd] max_size[%zd]\n", idx, num_elems);
            continue;
        }
        set_filter_elem(subcat, filter, val, token, idx, pile_list.at(idx));
    }
}

static bool serialize_list_organic_mat(FuncWriteExport add_value,
        const std::vector<char>* list,
        organic_mat_category::organic_mat_category cat) {
    bool all = true;
    if (!list) {
        DEBUG(log).print("serialize_list_organic_mat: list null\n");
        return all;
    }
    for (size_t i = 0; i < list->size(); ++i) {
        if (!list->at(i)) {
            all = false;
            continue;
        }
        std::string token = OrganicMatLookup::food_token_by_idx(cat, i);
        if (token.empty()) {
            DEBUG(log).print("food mat invalid :(\n");
            continue;
        }
        DEBUG(log).print("organic_material %zd is %s\n", i, token.c_str());
        add_value(token);
    }
    return all;
}

static void unserialize_list_organic_mat(const char* subcat, bool all, char val, const string& filter,
        FuncReadImport read_value, size_t list_size, std::vector<char>& pile_list,
        organic_mat_category::organic_mat_category cat) {
    size_t num_elems = OrganicMatLookup::food_max_size(cat);
    pile_list.resize(num_elems, '\0');
    if (all) {
        for (size_t idx = 0; idx < num_elems; ++idx) {
            string token = OrganicMatLookup::food_token_by_idx(cat, idx);
            set_filter_elem(subcat, filter, val, token, idx, pile_list.at(idx));
        }
        return;
    }

    for (size_t i = 0; i < list_size; ++i) {
        const std::string token = read_value(i);
        int16_t idx = OrganicMatLookup::food_idx_by_token(cat, token);
        if (idx < 0 || size_t(idx) >= num_elems) {
            WARN(log).print("organic mat index too large! idx[%d] max_size[%zd]\n", idx, num_elems);
            continue;
        }
        set_filter_elem(subcat, filter, val, token, idx, pile_list.at(idx));
    }
}

static bool serialize_list_item_type(FuncItemAllowed is_allowed,
        FuncWriteExport add_value, const std::vector<char>& list) {
    using df::enums::item_type::item_type;
    using type_traits = df::enum_traits<item_type>;

    bool all = true;
    size_t num_item_types = list.size();
    DEBUG(log).print("item_type size = %zd size limit = %d typecasted: %zd\n",
            num_item_types, type_traits::last_item_value,
            (size_t)type_traits::last_item_value);
    for (size_t i = 0; i <= (size_t)type_traits::last_item_value; ++i) {
        if (i < num_item_types && !list.at(i)) {
            all = false;
            continue;
        }
        const item_type type = (item_type)((df::enum_traits<item_type>::base_type)i);
        std::string r_type(type_traits::key_table[i + 1]);
        if (!is_allowed(type))
            continue;
        add_value(r_type);
        DEBUG(log).print("item_type key_table[%zd] type[%d] is %s\n", i + 1, (int16_t)type, r_type.c_str());
    }
    return all;
}

static void unserialize_list_item_type(const char* subcat, bool all, char val, const string& filter,
        FuncItemAllowed is_allowed, FuncReadImport read_value, int32_t list_size, std::vector<char>& pile_list) {
    // TODO can we remove the hardcoded list size?
    size_t num_elems = 112;
    pile_list.resize(num_elems, '\0');
    for (size_t i = 0; i < num_elems; ++i) {
        if (!is_allowed((df::item_type)i))
            pile_list.at(i) = 1;
    }

    if (all) {
        for (size_t idx = 0; idx < num_elems; ++idx) {
            string id = ENUM_KEY_STR(item_type, (df::item_type)idx);
            set_filter_elem(subcat, filter, val, id, idx, pile_list.at(idx));
        }
        return;
    }

    using df::enums::item_type::item_type;
    df::enum_traits<item_type> type_traits;
    for (int i = 0; i < list_size; ++i) {
        const std::string token = read_value(i);
        // subtract one because item_type starts at -1
        const df::enum_traits<item_type>::base_type idx = linear_index(type_traits, token) - 1;
        if (!is_allowed((item_type)idx))
            continue;
        if (idx < 0 || size_t(idx) >= num_elems) {
            WARN(log).print("error item_type index too large! idx[%d] max_size[%zd]\n", idx, num_elems);
            continue;
        }
        set_filter_elem(subcat, filter, val, token, idx, pile_list.at(idx));
    }
}

static bool serialize_list_material(FuncMaterialAllowed is_allowed,
        FuncWriteExport add_value, const std::vector<char>& list) {
    bool all = true;
    MaterialInfo mi;
    for (size_t i = 0; i < list.size(); ++i) {
        if (!list.at(i)) {
            all = false;
            continue;
        }
        mi.decode(0, i);
        if (!is_allowed(mi))
            continue;
        DEBUG(log).print("adding material %s\n", mi.getToken().c_str());
        add_value(mi.getToken());
    }
    return all;
}

static void unserialize_list_material(const char* subcat, bool all, char val, const string& filter,
        FuncMaterialAllowed is_allowed, FuncReadImport read_value, int32_t list_size,
        std::vector<char>& pile_list) {
    // we initialize all disallowed values to 1
    // why? because that's how the memory is in DF before we muck with it.
    size_t num_elems = world->raws.inorganics.size();
    pile_list.resize(num_elems, 0);
    for (size_t i = 0; i < pile_list.size(); ++i) {
        MaterialInfo mi(0, i);
        if (!is_allowed(mi))
            pile_list.at(i) = 1;
    }

    if (all) {
        for (size_t idx = 0; idx < num_elems; ++idx) {
            MaterialInfo mi;
            mi.decode(0, idx);
            set_filter_elem(subcat, filter, val, mi.toString(), idx, pile_list.at(idx));
        }
        return;
    }

    for (auto i = 0; i < list_size; ++i) {
        string id = read_value(i);
        MaterialInfo mi;
        if (!mi.find(id) || !is_allowed(mi))
            continue;
        if (mi.index < 0 || size_t(mi.index) >= pile_list.size()) {
            WARN(log).print("material type index invalid: %d\n", mi.index);
            continue;
        }
        set_filter_elem(subcat, filter, val, id, mi.index, pile_list.at(mi.index));
    }
}

static bool serialize_list_creature(FuncWriteExport add_value, const std::vector<char>& list) {
    bool all = true;

    for (size_t i = 0; i < list.size(); ++i) {
        if (!list.at(i)) {
            all = false;
            continue;
        }
        df::creature_raw* r = find_creature(i);
        if (r->flags.is_set(creature_raw_flags::GENERATED)
                || r->creature_id == "EQUIPMENT_WAGON")
            continue;
        DEBUG(log).print("adding creature %s\n", r->creature_id.c_str());
        add_value(r->creature_id);
    }
    return all;
}

static void unserialize_list_creature(const char* subcat, bool all, char val, const string& filter,
        FuncReadImport read_value, int32_t list_size, std::vector<char>& pile_list) {
    size_t num_elems = world->raws.creatures.all.size();
    pile_list.resize(num_elems, '\0');
    if (all) {
        for (size_t idx = 0; idx < num_elems; ++idx) {
            auto r = find_creature(idx);
            set_filter_elem(subcat, filter, val, r->name[0], r->creature_id, pile_list.at(idx));
        }
        return;
    }

    for (auto i = 0; i < list_size; ++i) {
        string id = read_value(i);
        int idx = find_creature(id);
        if (idx < 0 || size_t(idx) >= num_elems) {
            WARN(log).print("animal index invalid: %d\n", idx);
            continue;
        }
        auto r = find_creature(idx);
        set_filter_elem(subcat, filter, val, r->name[0], r->creature_id, pile_list.at(idx));
    }
}

template<typename T_cat_set>
static void write_cat(const char *name, bool include_types, uint32_t cat_flags,
        enum df::stockpile_group_set::Mask cat_mask,
        std::function<T_cat_set*()> mutable_cat_fn,
        std::function<bool(T_cat_set*)> write_cat_fn) {
    if (!(cat_flags & cat_mask))
        return;

    T_cat_set* cat_set = mutable_cat_fn();

    if (!include_types) {
        DEBUG(log).print("including all for %s since only category is being recorded\n", name);
        cat_set->set_all(true);
        return;
    }

    if (write_cat_fn(cat_set)) {
        // all fields were set. clear them and use the "all" flag instead so "all" can be applied
        // to other worlds with other generated types
        DEBUG(log).print("including all for %s since all fields were enabled\n", name);
        cat_set->Clear();
        cat_set->set_all(true);
    }
}

void StockpileSerializer::write(uint32_t includedElements) {
    if (includedElements & INCLUDED_ELEMENTS_CONTAINERS)
        write_containers();
    if (includedElements & INCLUDED_ELEMENTS_GENERAL)
        write_general();

    if (!(includedElements & INCLUDED_ELEMENTS_CATEGORIES))
        return;

    DEBUG(log).print("GROUP SET %s\n",
            bitfield_to_string(mPile->settings.flags).c_str());

    bool include_types = 0 != (includedElements & INCLUDED_ELEMENTS_TYPES);

    write_cat<StockpileSettings_AmmoSet>("ammo", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_ammo,
        std::bind(&StockpileSettings::mutable_ammo, &mBuffer),
        std::bind(&StockpileSerializer::write_ammo, this, _1));
    write_cat<StockpileSettings_AnimalsSet>("animals", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_animals,
        std::bind(&StockpileSettings::mutable_animals, &mBuffer),
        std::bind(&StockpileSerializer::write_animals, this, _1));
    write_cat<StockpileSettings_ArmorSet>("armor", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_armor,
        std::bind(&StockpileSettings::mutable_armor, &mBuffer),
        std::bind(&StockpileSerializer::write_armor, this, _1));
    write_cat<StockpileSettings_BarsBlocksSet>("bars_blocks", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_bars_blocks,
        std::bind(&StockpileSettings::mutable_barsblocks, &mBuffer),
        std::bind(&StockpileSerializer::write_bars_blocks, this, _1));
    write_cat<StockpileSettings_ClothSet>("cloth", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_cloth,
        std::bind(&StockpileSettings::mutable_cloth, &mBuffer),
        std::bind(&StockpileSerializer::write_cloth, this, _1));
    write_cat<StockpileSettings_CoinSet>("coin", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_coins,
        std::bind(&StockpileSettings::mutable_coin, &mBuffer),
        std::bind(&StockpileSerializer::write_coins, this, _1));
    write_cat<StockpileSettings_FinishedGoodsSet>("finished_goods", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_finished_goods,
        std::bind(&StockpileSettings::mutable_finished_goods, &mBuffer),
        std::bind(&StockpileSerializer::write_finished_goods, this, _1));
    write_cat<StockpileSettings_FoodSet>("food", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_food,
        std::bind(&StockpileSettings::mutable_food, &mBuffer),
        std::bind(&StockpileSerializer::write_food, this, _1));
    write_cat<StockpileSettings_FurnitureSet>("furniture", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_furniture,
        std::bind(&StockpileSettings::mutable_furniture, &mBuffer),
        std::bind(&StockpileSerializer::write_furniture, this, _1));
    write_cat<StockpileSettings_GemsSet>("gems", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_gems,
        std::bind(&StockpileSettings::mutable_gems, &mBuffer),
        std::bind(&StockpileSerializer::write_gems, this, _1));
    write_cat<StockpileSettings_LeatherSet>("leather", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_leather,
        std::bind(&StockpileSettings::mutable_leather, &mBuffer),
        std::bind(&StockpileSerializer::write_leather, this, _1));
    write_cat<StockpileSettings_CorpsesSet>("corpses", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_corpses,
        std::bind(&StockpileSettings::mutable_corpses_v50, &mBuffer),
        std::bind(&StockpileSerializer::write_corpses, this, _1));
    write_cat<StockpileSettings_RefuseSet>("refuse", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_refuse,
        std::bind(&StockpileSettings::mutable_refuse, &mBuffer),
        std::bind(&StockpileSerializer::write_refuse, this, _1));
    write_cat<StockpileSettings_SheetSet>("sheet", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_sheet,
        std::bind(&StockpileSettings::mutable_sheet, &mBuffer),
        std::bind(&StockpileSerializer::write_sheet, this, _1));
    write_cat<StockpileSettings_StoneSet>("stone", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_stone,
        std::bind(&StockpileSettings::mutable_stone, &mBuffer),
        std::bind(&StockpileSerializer::write_stone, this, _1));
    write_cat<StockpileSettings_WeaponsSet>("weapons", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_weapons,
        std::bind(&StockpileSettings::mutable_weapons, &mBuffer),
        std::bind(&StockpileSerializer::write_weapons, this, _1));
    write_cat<StockpileSettings_WoodSet>("wood", include_types,
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_wood,
        std::bind(&StockpileSettings::mutable_wood, &mBuffer),
        std::bind(&StockpileSerializer::write_wood, this, _1));
}

void StockpileSerializer::read(DeserializeMode mode, const std::string& filter) {
    DEBUG(log).print("==READ==\n");
    read_containers(mode);
    read_general(mode);
    read_ammo(mode, filter);
    read_animals(mode, filter);
    read_armor(mode, filter);
    read_bars_blocks(mode, filter);
    read_cloth(mode, filter);
    read_coins(mode, filter);
    read_finished_goods(mode, filter);
    read_food(mode, filter);
    read_furniture(mode, filter);
    read_gems(mode, filter);
    read_leather(mode, filter);

    // support for old versions before corpses had a set
    if (mBuffer.has_corpses()) {
        StockpileSettings::CorpsesSet* corpses = mBuffer.mutable_corpses_v50();
        corpses->set_all(true);
    }
    read_corpses(mode, filter);

    read_refuse(mode, filter);
    read_sheet(mode, filter);
    read_stone(mode, filter);
    read_weapons(mode, filter);
    read_wood(mode, filter);
}

void StockpileSerializer::write_containers() {
    DEBUG(log).print("writing container settings\n");
    mBuffer.set_max_bins(mPile->max_bins);
    mBuffer.set_max_barrels(mPile->max_barrels);
    mBuffer.set_max_wheelbarrows(mPile->max_wheelbarrows);
}

template<typename T_elem, typename T_elem_ret>
static void read_elem(const char *name, DeserializeMode mode,
        std::function<bool()> has_elem_fn,
        std::function<T_elem_ret()> elem_fn,
        T_elem &setting) {
    if (!has_elem_fn())
        return;

    bool is_set = elem_fn() != 0;
    if (mode == DESERIALIZE_MODE_SET || is_set) {
        T_elem val = (mode == DESERIALIZE_MODE_DISABLE) ? 0 : elem_fn();
        DEBUG(log).print("setting %s to %d\n", name, val);
        setting = val;
    }
}

template<typename T_cat>
static void read_category(const char *name, DeserializeMode mode,
        std::function<bool()> has_cat_fn,
        std::function<const T_cat &()> cat_fn,
        uint32_t & cat_flags,
        enum df::stockpile_group_set::Mask cat_mask,
        std::function<void()> clear_fn,
        std::function<void(bool, char)> set_fn) {
    if (mode == DESERIALIZE_MODE_SET) {
        DEBUG(log).print("clearing %s\n", name);
        cat_flags &= ~cat_mask;
        clear_fn();
    }

    if (!has_cat_fn())
        return;

    if (mode == DESERIALIZE_MODE_DISABLE && !(cat_flags & cat_mask))
        return;

    if (mode == DESERIALIZE_MODE_SET || mode == DESERIALIZE_MODE_ENABLE)
        cat_flags |= cat_mask;

    bool all = cat_fn().all();
    char val = (mode == DESERIALIZE_MODE_DISABLE) ? (char)0 : (char)1;
    DEBUG(log).print("setting %s %s elements to %d\n",
            all ? "all" : "marked", name, val);
    set_fn(all, val);
}

void StockpileSerializer::read_containers(DeserializeMode mode) {
    read_elem<int16_t, int32_t>("max_bins", mode,
            std::bind(&StockpileSettings::has_max_bins, mBuffer),
            std::bind(&StockpileSettings::max_bins, mBuffer),
            mPile->max_bins);
    read_elem<int16_t, int32_t>("max_barrels", mode,
            std::bind(&StockpileSettings::has_max_barrels, mBuffer),
            std::bind(&StockpileSettings::max_barrels, mBuffer),
            mPile->max_barrels);
    read_elem<int16_t, int32_t>("max_wheelbarrows", mode,
            std::bind(&StockpileSettings::has_max_wheelbarrows, mBuffer),
            std::bind(&StockpileSettings::max_wheelbarrows, mBuffer),
            mPile->max_wheelbarrows);
}

void StockpileSerializer::write_general() {
    DEBUG(log).print("writing general settings\n");
    mBuffer.set_use_links_only(mPile->use_links_only);
    mBuffer.set_allow_inorganic(mPile->settings.allow_inorganic);
    mBuffer.set_allow_organic(mPile->settings.allow_organic);
}

void StockpileSerializer::read_general(DeserializeMode mode) {
    read_elem<int32_t, bool>("use_links_only", mode,
            std::bind(&StockpileSettings::has_use_links_only, mBuffer),
            std::bind(&StockpileSettings::use_links_only, mBuffer),
            mPile->use_links_only);
    read_elem<bool, bool>("allow_inorganic", mode,
            std::bind(&StockpileSettings::has_allow_inorganic, mBuffer),
            std::bind(&StockpileSettings::allow_inorganic, mBuffer),
            mPile->settings.allow_inorganic);
    read_elem<bool, bool>("allow_organic", mode,
            std::bind(&StockpileSettings::has_allow_organic, mBuffer),
            std::bind(&StockpileSettings::allow_organic, mBuffer),
            mPile->settings.allow_organic);
}

static bool ammo_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && mi.material->flags.is_set(material_flags::IS_METAL);
}

bool StockpileSerializer::write_ammo(StockpileSettings::AmmoSet* ammo) {
    bool all = serialize_list_itemdef(
        [&](const std::string& token) { ammo->add_type(token); },
        mPile->settings.ammo.type,
        std::vector<df::itemdef*>(world->raws.itemdefs.ammo.begin(), world->raws.itemdefs.ammo.end()),
        item_type::AMMO);

    all = serialize_list_material(
        ammo_mat_is_allowed,
        [&](const std::string& token) { ammo->add_mats(token); },
        mPile->settings.ammo.mats) && all;

    if (mPile->settings.ammo.other_mats.size() > 2) {
        WARN(log).print("ammo other materials > 2: %zd\n",
                mPile->settings.ammo.other_mats.size());
    }

    size_t num_other_mats = std::min(size_t(2),
            mPile->settings.ammo.other_mats.size());
    for (size_t i = 0; i < num_other_mats; ++i) {
        if (!mPile->settings.ammo.other_mats.at(i)) {
            all = false;
            continue;
        }
        const std::string token = i == 0 ? "WOOD" : "BONE";
        ammo->add_other_mats(token);
        DEBUG(log).print("other mats %zd is %s\n", i, token.c_str());
    }

    all = serialize_list_quality(
        [&](const std::string& token) { ammo->add_quality_core(token); },
        mPile->settings.ammo.quality_core) && all;

    all = serialize_list_quality(
        [&](const std::string& token) { ammo->add_quality_total(token); },
        mPile->settings.ammo.quality_total) && all;

    return all;
}

void StockpileSerializer::read_ammo(DeserializeMode mode, const std::string& filter) {
    auto & pammo = mPile->settings.ammo;
    read_category<StockpileSettings_AmmoSet>("ammo", mode,
        std::bind(&StockpileSettings::has_ammo, mBuffer),
        std::bind(&StockpileSettings::ammo, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_ammo,
        [&]() {
            pammo.type.clear();
            pammo.mats.clear();
            pammo.other_mats.clear();
            quality_clear(pammo.quality_core);
            quality_clear(pammo.quality_total);
        },
        [&](bool all, char val) {
            auto & bammo = mBuffer.ammo();

            unserialize_list_itemdef("type", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bammo.type(idx); },
                bammo.type_size(), pammo.type, item_type::AMMO);

            unserialize_list_material("mats", all, val, filter, ammo_mat_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bammo.mats(idx); },
                bammo.mats_size(), pammo.mats);

            pammo.other_mats.resize(2, '\0');
            if (all) {
                set_filter_elem("other", filter, val, "WOOD", 0, pammo.other_mats.at(0));
                set_filter_elem("other", filter, val, "BONE", 1, pammo.other_mats.at(1));
            } else {
                // TODO can we un-hardcode the values?
                for (int i = 0; i < bammo.other_mats_size(); ++i) {
                    const std::string id = bammo.other_mats(i);
                    const int32_t idx = id == "WOOD" ? 0 : id == "BONE" ? 1 : -1;
                    if (idx == -1)
                        continue;
                    set_filter_elem("other", filter, val, id, idx, pammo.other_mats.at(idx));
                }
            }

            unserialize_list_quality("core", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bammo.quality_core(idx); },
                bammo.quality_core_size(), pammo.quality_core);

            unserialize_list_quality("total", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bammo.quality_total(idx); },
                bammo.quality_total_size(), pammo.quality_total);
        });
}

bool StockpileSerializer::write_animals(StockpileSettings::AnimalsSet* animals) {
    auto & panimals = mPile->settings.animals;
    bool all = panimals.empty_cages && panimals.empty_traps;

    animals->set_empty_cages(panimals.empty_cages);
    animals->set_empty_traps(panimals.empty_traps);

    return serialize_list_creature(
        [&](const std::string& token) { animals->add_enabled(token); },
        panimals.enabled) && all;
}

void StockpileSerializer::read_animals(DeserializeMode mode, const std::string& filter) {
    auto & panimals = mPile->settings.animals;
    read_category<StockpileSettings_AnimalsSet>("animals", mode,
        std::bind(&StockpileSettings::has_animals, mBuffer),
        std::bind(&StockpileSettings::animals, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_animals,
        [&]() {
            panimals.empty_cages = false;
            panimals.empty_traps = false;
            panimals.enabled.clear();
        },
        [&](bool all, char val) {
            auto & banimals = mBuffer.animals();

            set_flag("cages", filter, all, val, banimals.empty_cages(), panimals.empty_cages);
            set_flag("traps", filter, all, val, banimals.empty_traps(), panimals.empty_traps);

            unserialize_list_creature("", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return banimals.enabled(idx); },
                banimals.enabled_size(), panimals.enabled);
        });
}

static bool armor_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && mi.material->flags.is_set(material_flags::IS_METAL);
}

bool StockpileSerializer::write_armor(StockpileSettings::ArmorSet* armor) {

    auto & parmor = mPile->settings.armor;
    bool all = parmor.unusable && parmor.usable;

    armor->set_unusable(parmor.unusable);
    armor->set_usable(parmor.usable);

    // armor type
    all = serialize_list_itemdef(
        [&](const std::string& token) { armor->add_body(token); },
        parmor.body,
        std::vector<df::itemdef*>(world->raws.itemdefs.armor.begin(), world->raws.itemdefs.armor.end()),
        item_type::ARMOR) && all;

    // helm type
    all = serialize_list_itemdef(
        [&](const std::string& token) { armor->add_head(token); },
        parmor.head,
        std::vector<df::itemdef*>(world->raws.itemdefs.helms.begin(), world->raws.itemdefs.helms.end()),
        item_type::HELM) && all;

    // shoes type
    all = serialize_list_itemdef(
        [&](const std::string& token) { armor->add_feet(token); },
        parmor.feet,
        std::vector<df::itemdef*>(world->raws.itemdefs.shoes.begin(), world->raws.itemdefs.shoes.end()),
        item_type::SHOES) && all;

    // gloves type
    all = serialize_list_itemdef(
        [&](const std::string& token) { armor->add_hands(token); },
        parmor.hands,
        std::vector<df::itemdef*>(world->raws.itemdefs.gloves.begin(), world->raws.itemdefs.gloves.end()),
        item_type::GLOVES) && all;

    // pant type
    all = serialize_list_itemdef(
        [&](const std::string& token) { armor->add_legs(token); },
        parmor.legs,
        std::vector<df::itemdef*>(world->raws.itemdefs.pants.begin(), world->raws.itemdefs.pants.end()),
        item_type::PANTS) && all;

    // shield type
    all = serialize_list_itemdef(
        [&](const std::string& token) { armor->add_shield(token); },
        parmor.shield,
        std::vector<df::itemdef*>(world->raws.itemdefs.shields.begin(), world->raws.itemdefs.shields.end()),
        item_type::SHIELD) && all;

    // materials
    all = serialize_list_material(
        armor_mat_is_allowed,
        [&](const std::string& token) { armor->add_mats(token); },
        parmor.mats) && all;

    // other mats
    all = serialize_list_other_mats(
        mOtherMatsWeaponsArmor.mats, [&](const std::string& token) { armor->add_other_mats(token); },
        parmor.other_mats) && all;

    // quality core
    all = serialize_list_quality([&](const std::string& token) { armor->add_quality_core(token); },
        parmor.quality_core) && all;

    // quality total
    all = serialize_list_quality([&](const std::string& token) { armor->add_quality_total(token); },
        parmor.quality_total) && all;

    return all;
}

void StockpileSerializer::read_armor(DeserializeMode mode, const std::string& filter) {
    auto & parmor = mPile->settings.armor;
    read_category<StockpileSettings_ArmorSet>("armor", mode,
        std::bind(&StockpileSettings::has_armor, mBuffer),
        std::bind(&StockpileSettings::armor, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_armor,
        [&]() {
            parmor.body.clear();
            parmor.head.clear();
            parmor.feet.clear();
            parmor.hands.clear();
            parmor.legs.clear();
            parmor.shield.clear();
            parmor.other_mats.clear();
            parmor.mats.clear();
            quality_clear(parmor.quality_core);
            quality_clear(parmor.quality_total);
        },
        [&](bool all, char val) {
            auto & barmor = mBuffer.armor();

            set_flag("nouse", filter, all, val, barmor.unusable(), parmor.unusable);
            set_flag("canuse", filter, all, val, barmor.usable(), parmor.usable);

            unserialize_list_itemdef("body", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return barmor.body(idx); },
                barmor.body_size(), parmor.body, item_type::ARMOR);

            unserialize_list_itemdef("head", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return barmor.head(idx); },
                barmor.head_size(), parmor.head, item_type::HELM);

            unserialize_list_itemdef("feet", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return barmor.feet(idx); },
                barmor.feet_size(), parmor.feet, item_type::SHOES);

            unserialize_list_itemdef("hands", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return barmor.hands(idx); },
                barmor.hands_size(), parmor.hands, item_type::GLOVES);

            unserialize_list_itemdef("legs", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return barmor.legs(idx); },
                barmor.legs_size(), parmor.legs, item_type::PANTS);

            unserialize_list_itemdef("shield", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return barmor.shield(idx); },
                barmor.shield_size(), parmor.shield, item_type::SHIELD);

            unserialize_list_material("mats", all, val, filter,
                armor_mat_is_allowed,
                [&](const size_t& idx) -> const std::string& { return barmor.mats(idx); },
                barmor.mats_size(), parmor.mats);

            unserialize_list_other_mats("other", all, val, filter,
                mOtherMatsWeaponsArmor.mats, [&](const size_t& idx) -> const std::string& { return barmor.other_mats(idx); },
                barmor.other_mats_size(), parmor.other_mats);

            unserialize_list_quality("core", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return barmor.quality_core(idx); },
                barmor.quality_core_size(), parmor.quality_core);

            unserialize_list_quality("total", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return barmor.quality_total(idx); },
                barmor.quality_total_size(), parmor.quality_total);
        });
}

static bool bars_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && mi.material->flags.is_set(material_flags::IS_METAL);
}

static bool blocks_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && (mi.material->flags.is_set(material_flags::IS_METAL) || mi.material->flags.is_set(material_flags::IS_STONE));
}

bool StockpileSerializer::write_bars_blocks(StockpileSettings::BarsBlocksSet* bars_blocks) {
    bool all = serialize_list_material(
        bars_mat_is_allowed,
        [&](const std::string& token) { bars_blocks->add_bars_mats(token); },
        mPile->settings.bars_blocks.bars_mats);

    all = serialize_list_material(
        blocks_mat_is_allowed,
        [&](const std::string& token) { bars_blocks->add_blocks_mats(token); },
        mPile->settings.bars_blocks.blocks_mats) && all;

    all = serialize_list_other_mats(
        mOtherMatsBars.mats, [&](const std::string& token) { bars_blocks->add_bars_other_mats(token); },
        mPile->settings.bars_blocks.bars_other_mats) && all;

    all = serialize_list_other_mats(
        mOtherMatsBlocks.mats, [&](const std::string& token) { bars_blocks->add_blocks_other_mats(token); },
        mPile->settings.bars_blocks.blocks_other_mats) && all;

    return all;
}

void StockpileSerializer::read_bars_blocks(DeserializeMode mode, const std::string& filter) {
    auto & pbarsblocks = mPile->settings.bars_blocks;
    read_category<StockpileSettings_BarsBlocksSet>("bars_blocks", mode,
        std::bind(&StockpileSettings::has_barsblocks, mBuffer),
        std::bind(&StockpileSettings::barsblocks, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_bars_blocks,
        [&]() {
            pbarsblocks.bars_mats.clear();
            pbarsblocks.bars_other_mats.clear();
            pbarsblocks.blocks_mats.clear();
            pbarsblocks.blocks_other_mats.clear();
        },
        [&](bool all, char val) {
            auto & bbarsblocks = mBuffer.barsblocks();

            unserialize_list_material("mats/bars", all, val, filter, bars_mat_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bbarsblocks.bars_mats(idx); },
                bbarsblocks.bars_mats_size(), pbarsblocks.bars_mats);

            unserialize_list_other_mats("other/bars", all, val, filter,
                mOtherMatsBars.mats,
                [&](const size_t& idx) -> const std::string& { return bbarsblocks.bars_other_mats(idx); },
                bbarsblocks.bars_other_mats_size(), pbarsblocks.bars_other_mats);

            unserialize_list_material("mats/blocks", all, val, filter,
                blocks_mat_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bbarsblocks.blocks_mats(idx); },
                bbarsblocks.blocks_mats_size(), pbarsblocks.blocks_mats);

            unserialize_list_other_mats("other/blocks", all, val, filter,
                mOtherMatsBlocks.mats,
                [&](const size_t& idx) -> const std::string& { return bbarsblocks.blocks_other_mats(idx); },
                bbarsblocks.blocks_other_mats_size(), pbarsblocks.blocks_other_mats);
        });
}

bool StockpileSerializer::write_cloth(StockpileSettings::ClothSet* cloth) {
    bool all = true;

    all = serialize_list_organic_mat(
        [&](const std::string& token) { cloth->add_thread_silk(token); },
        &mPile->settings.cloth.thread_silk, organic_mat_category::Silk) && all;

    all = serialize_list_organic_mat(
        [&](const std::string& token) { cloth->add_thread_plant(token); },
        &mPile->settings.cloth.thread_plant, organic_mat_category::PlantFiber) && all;

    all = serialize_list_organic_mat(
        [&](const std::string& token) { cloth->add_thread_yarn(token); },
        &mPile->settings.cloth.thread_yarn, organic_mat_category::Yarn) && all;

    all = serialize_list_organic_mat(
        [&](const std::string& token) { cloth->add_thread_metal(token); },
        &mPile->settings.cloth.thread_metal, organic_mat_category::MetalThread) && all;

    all = serialize_list_organic_mat(
        [&](const std::string& token) { cloth->add_cloth_silk(token); },
        &mPile->settings.cloth.cloth_silk, organic_mat_category::Silk) && all;

    all = serialize_list_organic_mat(
        [&](const std::string& token) { cloth->add_cloth_plant(token); },
        &mPile->settings.cloth.cloth_plant, organic_mat_category::PlantFiber) && all;

    all = serialize_list_organic_mat(
        [&](const std::string& token) { cloth->add_cloth_yarn(token); },
        &mPile->settings.cloth.cloth_yarn, organic_mat_category::Yarn) && all;

    all = serialize_list_organic_mat(
        [&](const std::string& token) { cloth->add_cloth_metal(token); },
        &mPile->settings.cloth.cloth_metal, organic_mat_category::MetalThread) && all;

    return all;
}

void StockpileSerializer::read_cloth(DeserializeMode mode, const std::string& filter) {
    auto & pcloth = mPile->settings.cloth;
    read_category<StockpileSettings_ClothSet>("cloth", mode,
        std::bind(&StockpileSettings::has_cloth, mBuffer),
        std::bind(&StockpileSettings::cloth, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_cloth,
        [&]() {
            pcloth.thread_silk.clear();
            pcloth.thread_yarn.clear();
            pcloth.thread_plant.clear();
            pcloth.thread_metal.clear();
            pcloth.cloth_silk.clear();
            pcloth.cloth_plant.clear();
            pcloth.cloth_yarn.clear();
            pcloth.cloth_metal.clear();
        },
        [&](bool all, char val) {
            auto & bcloth = mBuffer.cloth();

            unserialize_list_organic_mat("thread/silk", all, val, filter,
                [&](size_t idx) -> std::string { return bcloth.thread_silk(idx); },
                bcloth.thread_silk_size(), pcloth.thread_silk, organic_mat_category::Silk);

            unserialize_list_organic_mat("thread/plant", all, val, filter,
                [&](size_t idx) -> std::string { return bcloth.thread_plant(idx); },
                bcloth.thread_plant_size(), pcloth.thread_plant, organic_mat_category::PlantFiber);

            unserialize_list_organic_mat("thread/yarn", all, val, filter,
                [&](size_t idx) -> std::string { return bcloth.thread_yarn(idx); },
                bcloth.thread_yarn_size(), pcloth.thread_yarn, organic_mat_category::Yarn);

            unserialize_list_organic_mat("thread/metal", all, val, filter,
                [&](size_t idx) -> std::string { return bcloth.thread_metal(idx); },
                bcloth.thread_metal_size(), pcloth.thread_metal, organic_mat_category::MetalThread);

            unserialize_list_organic_mat("cloth/silk", all, val, filter,
                [&](size_t idx) -> std::string { return bcloth.cloth_silk(idx); },
                bcloth.cloth_silk_size(), pcloth.cloth_silk, organic_mat_category::Silk);

            unserialize_list_organic_mat("cloth/plant", all, val, filter,
                [&](size_t idx) -> std::string { return bcloth.cloth_plant(idx); },
                bcloth.cloth_plant_size(), pcloth.cloth_plant, organic_mat_category::PlantFiber);

            unserialize_list_organic_mat("cloth/yarn", all, val, filter,
                [&](size_t idx) -> std::string { return bcloth.cloth_yarn(idx); },
                bcloth.cloth_yarn_size(), pcloth.cloth_yarn, organic_mat_category::Yarn);

            unserialize_list_organic_mat("cloth/metal", all, val, filter,
                [&](size_t idx) -> std::string { return bcloth.cloth_metal(idx); },
                bcloth.cloth_metal_size(), pcloth.cloth_metal, organic_mat_category::MetalThread);
        });
}

static bool coins_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid();
}

bool StockpileSerializer::write_coins(StockpileSettings::CoinSet* coins) {
    return serialize_list_material(
        coins_mat_is_allowed,
        [&](const std::string& token) { coins->add_mats(token); },
        mPile->settings.coins.mats);
}

void StockpileSerializer::read_coins(DeserializeMode mode, const std::string& filter) {
    auto & pcoins = mPile->settings.coins;
    read_category<StockpileSettings_CoinSet>("coin", mode,
        std::bind(&StockpileSettings::has_coin, mBuffer),
        std::bind(&StockpileSettings::coin, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_coins,
        [&]() {
            pcoins.mats.clear();
        },
        [&](bool all, char val) {
            auto & bcoin = mBuffer.coin();

            unserialize_list_material("", all, val, filter, coins_mat_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bcoin.mats(idx); },
                bcoin.mats_size(), pcoins.mats);
        });
}

static bool finished_goods_type_is_allowed(item_type::item_type type) {
    switch (type) {
    case item_type::CHAIN:
    case item_type::FLASK:
    case item_type::GOBLET:
    case item_type::INSTRUMENT:
    case item_type::TOY:
    case item_type::ARMOR:
    case item_type::SHOES:
    case item_type::HELM:
    case item_type::GLOVES:
    case item_type::FIGURINE:
    case item_type::AMULET:
    case item_type::SCEPTER:
    case item_type::CROWN:
    case item_type::RING:
    case item_type::EARRING:
    case item_type::BRACELET:
    case item_type::GEM:
    case item_type::TOTEM:
    case item_type::PANTS:
    case item_type::BACKPACK:
    case item_type::QUIVER:
    case item_type::SPLINT:
    case item_type::CRUTCH:
    case item_type::TOOL:
    case item_type::BOOK:
        return true;
    default:
        return false;
    }
}

static bool finished_goods_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && (mi.material->flags.is_set(material_flags::IS_GEM) || mi.material->flags.is_set(material_flags::IS_METAL) || mi.material->flags.is_set(material_flags::IS_STONE));
}

bool StockpileSerializer::write_finished_goods(StockpileSettings::FinishedGoodsSet* finished_goods) {
    bool all = serialize_list_item_type(
        finished_goods_type_is_allowed,
        [&](const std::string& token) { finished_goods->add_type(token); },
        mPile->settings.finished_goods.type);

    all = serialize_list_material(
        finished_goods_mat_is_allowed,
        [&](const std::string& token) { finished_goods->add_mats(token); },
        mPile->settings.finished_goods.mats) && all;

    all = serialize_list_other_mats(
        mOtherMatsFinishedGoods.mats, [&](const std::string& token) { finished_goods->add_other_mats(token); },
        mPile->settings.finished_goods.other_mats) && all;

    all = serialize_list_quality([&](const std::string& token) { finished_goods->add_quality_core(token); },
        mPile->settings.finished_goods.quality_core) && all;

    all = serialize_list_quality([&](const std::string& token) { finished_goods->add_quality_total(token); },
        mPile->settings.finished_goods.quality_total) && all;

    return all;
}

void StockpileSerializer::read_finished_goods(DeserializeMode mode, const std::string& filter) {
    auto & pfinished_goods = mPile->settings.finished_goods;
    read_category<StockpileSettings_FinishedGoodsSet>("finished_goods", mode,
        std::bind(&StockpileSettings::has_finished_goods, mBuffer),
        std::bind(&StockpileSettings::finished_goods, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_finished_goods,
        [&]() {
            pfinished_goods.type.clear();
            pfinished_goods.other_mats.clear();
            pfinished_goods.mats.clear();
            quality_clear(pfinished_goods.quality_core);
            quality_clear(pfinished_goods.quality_total);
        },
        [&](bool all, char val) {
            auto & bfinished_goods = mBuffer.finished_goods();

            unserialize_list_item_type("type", all, val, filter, finished_goods_type_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bfinished_goods.type(idx); },
                bfinished_goods.type_size(), pfinished_goods.type);

            unserialize_list_material("mats", all, val, filter, finished_goods_mat_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bfinished_goods.mats(idx); },
                bfinished_goods.mats_size(), pfinished_goods.mats);

            unserialize_list_other_mats("other", all, val, filter, mOtherMatsFinishedGoods.mats,
                [&](const size_t& idx) -> const std::string& { return bfinished_goods.other_mats(idx); },
                bfinished_goods.other_mats_size(), pfinished_goods.other_mats);

            unserialize_list_quality("core", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bfinished_goods.quality_core(idx); },
                bfinished_goods.quality_core_size(), pfinished_goods.quality_core);

            unserialize_list_quality("total", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bfinished_goods.quality_total(idx); },
                bfinished_goods.quality_total_size(), pfinished_goods.quality_total);
        });
}

food_pair StockpileSerializer::food_map(organic_mat_category::organic_mat_category cat) {
    using df::enums::organic_mat_category::organic_mat_category;

    switch (cat) {
    case organic_mat_category::Meat:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_meat(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().meat(idx); };
        return food_pair("meat", setter, &mPile->settings.food.meat, getter, mBuffer.food().meat_size());
    }
    case organic_mat_category::Fish:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_fish(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().fish(idx); };
        return food_pair("fish", setter, &mPile->settings.food.fish, getter, mBuffer.food().fish_size());
    }
    case organic_mat_category::UnpreparedFish:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_unprepared_fish(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().unprepared_fish(idx); };
        return food_pair("unpreparedfish", setter, &mPile->settings.food.unprepared_fish, getter, mBuffer.food().unprepared_fish_size());
    }
    case organic_mat_category::Eggs:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_egg(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().egg(idx); };
        return food_pair("egg", setter, &mPile->settings.food.egg, getter, mBuffer.food().egg_size());
    }
    case organic_mat_category::Plants:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_plants(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().plants(idx); };
        return food_pair("plants", setter, &mPile->settings.food.plants, getter, mBuffer.food().plants_size());
    }
    case organic_mat_category::PlantDrink:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_drink_plant(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().drink_plant(idx); };
        return food_pair("drink/plant", setter, &mPile->settings.food.drink_plant, getter, mBuffer.food().drink_plant_size());
    }
    case organic_mat_category::CreatureDrink:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_drink_animal(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().drink_animal(idx); };
        return food_pair("drink/animal", setter, &mPile->settings.food.drink_animal, getter, mBuffer.food().drink_animal_size());
    }
    case organic_mat_category::PlantCheese:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_cheese_plant(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().cheese_plant(idx); };
        return food_pair("cheese/plant", setter, &mPile->settings.food.cheese_plant, getter, mBuffer.food().cheese_plant_size());
    }
    case organic_mat_category::CreatureCheese:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_cheese_animal(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().cheese_animal(idx); };
        return food_pair("cheese/animal", setter, &mPile->settings.food.cheese_animal, getter, mBuffer.food().cheese_animal_size());
    }
    case organic_mat_category::Seed:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_seeds(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().seeds(idx); };
        return food_pair("seeds", setter, &mPile->settings.food.seeds, getter, mBuffer.food().seeds_size());
    }
    case organic_mat_category::Leaf:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_leaves(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().leaves(idx); };
        return food_pair("leaves", setter, &mPile->settings.food.leaves, getter, mBuffer.food().leaves_size());
    }
    case organic_mat_category::PlantPowder:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_powder_plant(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().powder_plant(idx); };
        return food_pair("powder/plant", setter, &mPile->settings.food.powder_plant, getter, mBuffer.food().powder_plant_size());
    }
    case organic_mat_category::CreaturePowder:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_powder_creature(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().powder_creature(idx); };
        return food_pair("powder/animal", setter, &mPile->settings.food.powder_creature, getter, mBuffer.food().powder_creature_size());
    }
    case organic_mat_category::Glob:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_glob(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().glob(idx); };
        return food_pair("glob", setter, &mPile->settings.food.glob, getter, mBuffer.food().glob_size());
    }
    case organic_mat_category::PlantLiquid:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_liquid_plant(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().liquid_plant(idx); };
        return food_pair("liquid/plant", setter, &mPile->settings.food.liquid_plant, getter, mBuffer.food().liquid_plant_size());
    }
    case organic_mat_category::CreatureLiquid:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_liquid_animal(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().liquid_animal(idx); };
        return food_pair("liquid/animal", setter, &mPile->settings.food.liquid_animal, getter, mBuffer.food().liquid_animal_size());
    }
    case organic_mat_category::MiscLiquid:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_liquid_misc(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().liquid_misc(idx); };
        return food_pair("liquid/misc", setter, &mPile->settings.food.liquid_misc, getter, mBuffer.food().liquid_misc_size());
    }

    case organic_mat_category::Paste:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_glob_paste(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().glob_paste(idx); };
        return food_pair("paste", setter, &mPile->settings.food.glob_paste, getter, mBuffer.food().glob_paste_size());
    }
    case organic_mat_category::Pressed:
    {
        FuncWriteExport setter = [&](const std::string& id) {
            mBuffer.mutable_food()->add_glob_pressed(id);
        };
        FuncReadImport getter = [&](size_t idx) -> std::string { return mBuffer.food().glob_pressed(idx); };
        return food_pair("pressed", setter, &mPile->settings.food.glob_pressed, getter, mBuffer.food().glob_pressed_size());
    }
    case organic_mat_category::Leather:
    case organic_mat_category::Silk:
    case organic_mat_category::PlantFiber:
    case organic_mat_category::Bone:
    case organic_mat_category::Shell:
    case organic_mat_category::Wood:
    case organic_mat_category::Horn:
    case organic_mat_category::Pearl:
    case organic_mat_category::Tooth:
    case organic_mat_category::EdibleCheese:
    case organic_mat_category::AnyDrink:
    case organic_mat_category::EdiblePlant:
    case organic_mat_category::CookableLiquid:
    case organic_mat_category::CookablePowder:
    case organic_mat_category::CookableSeed:
    case organic_mat_category::CookableLeaf:
    case organic_mat_category::Yarn:
    case organic_mat_category::MetalThread:
    default:
        // not used in stockpile food menu
        break;
    }
    return food_pair();
}

bool StockpileSerializer::write_food(StockpileSettings::FoodSet* food) {
    auto & pfood = mPile->settings.food;
    bool all = pfood.prepared_meals;

    food->set_prepared_meals(pfood.prepared_meals);

    using df::enums::organic_mat_category::organic_mat_category;
    using traits = df::enum_traits<organic_mat_category>;
    for (int32_t mat_category = traits::first_item_value; mat_category < traits::last_item_value; ++mat_category) {
        food_pair p = food_map((organic_mat_category)mat_category);
        if (!p.valid)
            continue;
        all = serialize_list_organic_mat(p.set_value, p.stockpile_values,
                (organic_mat_category)mat_category) && all;
    }

    return all;
}

void StockpileSerializer::read_food(DeserializeMode mode, const std::string& filter) {
    using df::enums::organic_mat_category::organic_mat_category;
    using traits = df::enum_traits<organic_mat_category>;

    auto & pfood = mPile->settings.food;
    read_category<StockpileSettings_FoodSet>("food", mode,
        std::bind(&StockpileSettings::has_food, mBuffer),
        std::bind(&StockpileSettings::food, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_food,
        [&]() {
            pfood.prepared_meals = false;
            for (int32_t mat_category = traits::first_item_value; mat_category < traits::last_item_value; ++mat_category) {
                food_pair p = food_map((organic_mat_category)mat_category);
                if (!p.valid)
                    continue;
                p.stockpile_values->clear();
            }
        },
        [&](bool all, char val) {
            auto & bfood = mBuffer.food();

            set_flag("preparedmeals", filter, all, val, bfood.prepared_meals(), pfood.prepared_meals);

            for (int32_t mat_category = traits::first_item_value; mat_category < traits::last_item_value; ++mat_category) {
                food_pair p = food_map((organic_mat_category)mat_category);
                if (!p.valid)
                    continue;
                unserialize_list_organic_mat(p.name, all, val, filter,
                    p.get_value, p.serialized_count, *p.stockpile_values,
                    (organic_mat_category)mat_category);
            }
        });

}

static bool furniture_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && (mi.material->flags.is_set(material_flags::IS_METAL) || mi.material->flags.is_set(material_flags::IS_STONE));
}

bool StockpileSerializer::write_furniture(StockpileSettings::FurnitureSet* furniture) {
    using df::enums::furniture_type::furniture_type;
    using type_traits = df::enum_traits<furniture_type>;

    auto & pfurniture = mPile->settings.furniture;
    bool all = true;

    for (size_t i = 0; i < pfurniture.type.size(); ++i) {
        if (!pfurniture.type.at(i)) {
            all = false;
            continue;
        }
        std::string f_type(type_traits::key_table[i]);
        furniture->add_type(f_type);
        DEBUG(log).print("furniture_type %zd is %s\n", i, f_type.c_str());
    }

    all = serialize_list_material(
        furniture_mat_is_allowed,
        [&](const std::string& token) { furniture->add_mats(token); },
        pfurniture.mats) && all;
    all = serialize_list_other_mats(
        mOtherMatsFurniture.mats,
        [&](const std::string& token) { furniture->add_other_mats(token); },
        pfurniture.other_mats) && all;
    all = serialize_list_quality(
        [&](const std::string& token) { furniture->add_quality_core(token); },
        pfurniture.quality_core) && all;
    all = serialize_list_quality(
        [&](const std::string& token) { furniture->add_quality_total(token); },
        pfurniture.quality_total) && all;

    return all;
}

void StockpileSerializer::read_furniture(DeserializeMode mode, const std::string& filter) {
    auto & pfurniture = mPile->settings.furniture;
    read_category<StockpileSettings_FurnitureSet>("furniture", mode,
        std::bind(&StockpileSettings::has_furniture, mBuffer),
        std::bind(&StockpileSettings::furniture, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_furniture,
        [&]() {
            pfurniture.type.clear();
            pfurniture.other_mats.clear();
            pfurniture.mats.clear();
            quality_clear(pfurniture.quality_core);
            quality_clear(pfurniture.quality_total);
        },
        [&](bool all, char val) {
            auto & bfurniture = mBuffer.furniture();

            using df::enums::furniture_type::furniture_type;
            df::enum_traits<furniture_type> type_traits;
            size_t num_elems = type_traits.last_item_value + 1;
            pfurniture.type.resize(num_elems, '\0');

            if (all) {
                for (size_t idx = 0; idx < num_elems; ++idx) {
                    string id = ENUM_KEY_STR(furniture_type, (df::furniture_type)idx);
                    set_filter_elem("type", filter, val, id, idx, pfurniture.type.at(idx));
                }
            } else {
                for (int i = 0; i < bfurniture.type_size(); ++i) {
                    const std::string token = bfurniture.type(i);
                    df::enum_traits<furniture_type>::base_type idx = linear_index(type_traits, token);
                    if (idx < 0 || size_t(idx) >= pfurniture.type.size()) {
                        WARN(log).print("furniture type index invalid %s, idx=%d\n", token.c_str(), idx);
                        continue;
                    }
                    set_filter_elem("type", filter, val, token, idx, pfurniture.type.at(idx));
                }
            }

            unserialize_list_material("mats", all, val, filter, furniture_mat_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bfurniture.mats(idx); },
                bfurniture.mats_size(), pfurniture.mats);

            unserialize_list_other_mats("other", all, val, filter,
                mOtherMatsFurniture.mats, [&](const size_t& idx) -> const std::string& { return bfurniture.other_mats(idx); },
                bfurniture.other_mats_size(), pfurniture.other_mats);

            unserialize_list_quality("core", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bfurniture.quality_core(idx); },
                bfurniture.quality_core_size(), pfurniture.quality_core);

            unserialize_list_quality("total", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bfurniture.quality_total(idx); },
                bfurniture.quality_total_size(), pfurniture.quality_total);
        });
}

static bool gem_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && mi.material->flags.is_set(material_flags::IS_GEM);
}

static bool gem_cut_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && (mi.material->flags.is_set(material_flags::IS_GEM) || mi.material->flags.is_set(material_flags::IS_STONE));
}

static bool gem_other_mat_is_allowed(MaterialInfo& mi) {
    return mi.isValid() && (mi.getToken() == "GLASS_GREEN" || mi.getToken() == "GLASS_CLEAR" || mi.getToken() == "GLASS_CRYSTAL");
}

bool StockpileSerializer::write_gems(StockpileSettings::GemsSet* gems) {
    MaterialInfo mi;

    auto & pgems = mPile->settings.gems;

    bool all = serialize_list_material(
        gem_mat_is_allowed,
        [&](const std::string& token) { gems->add_rough_mats(token); },
        pgems.rough_mats);

    all = serialize_list_material(
        gem_cut_mat_is_allowed,
        [&](const std::string& token) { gems->add_cut_mats(token); },
        pgems.cut_mats) && all;

    for (size_t i = 0; i < pgems.rough_other_mats.size(); ++i) {
        if (!pgems.rough_other_mats.at(i)) {
            all = false;
            continue;
        }
        mi.decode(i, -1);
        if (!gem_other_mat_is_allowed(mi))
            continue;
        DEBUG(log).print("gem rough_other mat %zd is %s\n", i, mi.getToken().c_str());
        gems->add_rough_other_mats(mi.getToken());
    }

    for (size_t i = 0; i < pgems.cut_other_mats.size(); ++i) {
        if (!pgems.cut_other_mats.at(i)) {
            all = false;
            continue;
        }
        mi.decode(i, -1);
        if (!mi.isValid())
            mi.decode(0, i);
        if (!gem_other_mat_is_allowed(mi))
            continue;
        DEBUG(log).print("gem cut_other mat %zd is %s\n", i, mi.getToken().c_str());
        gems->add_cut_other_mats(mi.getToken());
    }

    return all;
}

void StockpileSerializer::read_gems(DeserializeMode mode, const std::string& filter) {
    auto & pgems = mPile->settings.gems;
    read_category<StockpileSettings_GemsSet>("gems", mode,
        std::bind(&StockpileSettings::has_gems, mBuffer),
        std::bind(&StockpileSettings::gems, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_gems,
        [&]() {
            pgems.cut_other_mats.clear();
            pgems.cut_mats.clear();
            pgems.rough_other_mats.clear();
            pgems.rough_mats.clear();
        },
        [&](bool all, char val) {
            auto & bgems = mBuffer.gems();

            unserialize_list_material("mats/rough", all, val, filter, gem_mat_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bgems.rough_mats(idx); },
                bgems.rough_mats_size(),pgems.rough_mats);

            unserialize_list_material("mats/cut", all, val, filter, gem_cut_mat_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bgems.cut_mats(idx); },
                bgems.cut_mats_size(), pgems.cut_mats);

            const size_t builtin_size = std::extent<decltype(world->raws.mat_table.builtin)>::value;
            pgems.rough_other_mats.resize(builtin_size, '\0');
            pgems.cut_other_mats.resize(builtin_size, '\0');
            if (all) {
                for (size_t idx = 0; idx < builtin_size; ++idx) {
                    MaterialInfo mi;
                    mi.decode(idx, -1);
                    if (gem_other_mat_is_allowed(mi))
                        set_filter_elem("other/rough", filter, val, mi.getToken(), idx, pgems.rough_other_mats.at(idx));
                    if (!mi.isValid())
                        mi.decode(0, idx);
                    if (gem_other_mat_is_allowed(mi))
                        set_filter_elem("other/cut", filter, val, mi.getToken(), idx, pgems.cut_other_mats.at(idx));
                }
                return;
            } else {
                MaterialInfo mi;
                for (size_t i = 0; i < builtin_size; ++i) {
                    string id = bgems.rough_other_mats(i);
                    if (mi.find(id) && mi.isValid() && size_t(mi.type) < builtin_size)
                        set_filter_elem("other/rough", filter, val, id, mi.type, pgems.rough_other_mats.at(mi.type));
                    id = bgems.cut_other_mats(i);
                    if (mi.find(id) && mi.isValid() && size_t(mi.type) < builtin_size)
                        set_filter_elem("other/cut", filter, val, id, mi.type, pgems.cut_other_mats.at(mi.type));
                }
            }
        });
}

bool StockpileSerializer::write_leather(StockpileSettings::LeatherSet* leather) {
    return serialize_list_organic_mat(
        [&](const std::string& id) { leather->add_mats(id); },
        &mPile->settings.leather.mats, organic_mat_category::Leather);
}

void StockpileSerializer::read_leather(DeserializeMode mode, const std::string& filter) {
    auto & pleather = mPile->settings.leather;
    read_category<StockpileSettings_LeatherSet>("leather", mode,
        std::bind(&StockpileSettings::has_leather, mBuffer),
        std::bind(&StockpileSettings::leather, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_leather,
        [&]() {
            pleather.mats.clear();
        },
        [&](bool all, char val) {
            auto & bleather = mBuffer.leather();

            unserialize_list_organic_mat("", all, val, filter,
                [&](size_t idx) -> std::string { return bleather.mats(idx); },
                bleather.mats_size(), pleather.mats, organic_mat_category::Leather);
        });
}

bool StockpileSerializer::write_corpses(StockpileSettings::CorpsesSet* corpses) {
    return serialize_list_creature(
        [&](const std::string& token) { corpses->add_corpses(token); },
        mPile->settings.corpses.corpses);
}

void StockpileSerializer::read_corpses(DeserializeMode mode, const std::string& filter) {
    auto & pcorpses = mPile->settings.corpses;
    read_category<StockpileSettings_CorpsesSet>("corpses", mode,
        std::bind(&StockpileSettings::has_corpses_v50, mBuffer),
        std::bind(&StockpileSettings::corpses_v50, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_corpses,
        [&]() {
            pcorpses.corpses.clear();
        },
        [&](bool all, char val) {
            auto & bcorpses = mBuffer.corpses_v50();
            unserialize_list_creature("", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bcorpses.corpses(idx); },
                bcorpses.corpses_size(), pcorpses.corpses);
        });
}

static bool refuse_type_is_allowed(item_type::item_type type) {
    if (type == item_type::NONE || type == item_type::BAR || type == item_type::SMALLGEM
            || type == item_type::BLOCKS || type == item_type::ROUGH || type == item_type::BOULDER
            || type == item_type::CORPSE || type == item_type::CORPSEPIECE || type == item_type::ROCK
            || type == item_type::ORTHOPEDIC_CAST)
        return false;
    return true;
}

bool StockpileSerializer::write_refuse(StockpileSettings::RefuseSet* refuse) {
    auto & prefuse = mPile->settings.refuse;
    bool all = prefuse.fresh_raw_hide && prefuse.rotten_raw_hide;

    refuse->set_fresh_raw_hide(prefuse.fresh_raw_hide);
    refuse->set_rotten_raw_hide(prefuse.rotten_raw_hide);

    all = serialize_list_item_type(refuse_type_is_allowed,
        [&](const std::string& token) { refuse->add_type(token); },
        prefuse.type) && all;

    all = serialize_list_creature(
        [&](const std::string& token) { refuse->add_corpses(token); },
        prefuse.corpses) && all;
    all = serialize_list_creature(
        [&](const std::string& token) { refuse->add_body_parts(token); },
        prefuse.body_parts) && all;
    all = serialize_list_creature(
        [&](const std::string& token) { refuse->add_skulls(token); },
        prefuse.skulls) && all;
    all = serialize_list_creature(
        [&](const std::string& token) { refuse->add_bones(token); },
        prefuse.bones) && all;
    all = serialize_list_creature(
        [&](const std::string& token) { refuse->add_hair(token); },
        prefuse.hair) && all;
    all = serialize_list_creature(
        [&](const std::string& token) { refuse->add_shells(token); },
        prefuse.shells) && all;
    all = serialize_list_creature(
        [&](const std::string& token) { refuse->add_teeth(token); },
        prefuse.teeth) && all;
    all = serialize_list_creature(
        [&](const std::string& token) { refuse->add_horns(token); },
        prefuse.horns) && all;

    return all;
}

void StockpileSerializer::read_refuse(DeserializeMode mode, const std::string& filter) {
    auto & prefuse = mPile->settings.refuse;
    read_category<StockpileSettings_RefuseSet>("refuse", mode,
        std::bind(&StockpileSettings::has_refuse, mBuffer),
        std::bind(&StockpileSettings::refuse, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_refuse,
        [&]() {
            prefuse.fresh_raw_hide = false;
            prefuse.rotten_raw_hide = false;
            prefuse.type.clear();
            prefuse.corpses.clear();
            prefuse.body_parts.clear();
            prefuse.skulls.clear();
            prefuse.bones.clear();
            prefuse.hair.clear();
            prefuse.shells.clear();
            prefuse.teeth.clear();
            prefuse.horns.clear();
        },
        [&](bool all, char val) {
            auto & brefuse = mBuffer.refuse();

            set_flag("rawhide/fresh", filter, all, val, brefuse.fresh_raw_hide(), prefuse.fresh_raw_hide);
            set_flag("rawhide/rotten", filter, all, val, brefuse.rotten_raw_hide(), prefuse.rotten_raw_hide);

            unserialize_list_item_type("type", all, val, filter, refuse_type_is_allowed,
                [&](const size_t& idx) -> const string& { return brefuse.type(idx); },
                brefuse.type_size(), prefuse.type);

            unserialize_list_creature("corpses", all, val, filter,
                [&](const size_t& idx) -> const string& { return brefuse.corpses(idx); },
                brefuse.corpses_size(), prefuse.corpses);
            unserialize_list_creature("bodyparts", all, val, filter,
                [&](const size_t& idx) -> const string& { return brefuse.body_parts(idx); },
                brefuse.body_parts_size(), prefuse.body_parts);
            unserialize_list_creature("skulls", all, val, filter,
                [&](const size_t& idx) -> const string& { return brefuse.skulls(idx); },
                brefuse.skulls_size(), prefuse.skulls);
            unserialize_list_creature("bones", all, val, filter,
                [&](const size_t& idx) -> const string& { return brefuse.bones(idx); },
                brefuse.bones_size(), prefuse.bones);
            unserialize_list_creature("hair", all, val, filter,
                [&](const size_t& idx) -> const string& { return brefuse.hair(idx); },
                brefuse.hair_size(), prefuse.hair);
            unserialize_list_creature("shells", all, val, filter,
                [&](const size_t& idx) -> const string& { return brefuse.shells(idx); },
                brefuse.shells_size(), prefuse.shells);
            unserialize_list_creature("teeth", all, val, filter,
                [&](const size_t& idx) -> const string& { return brefuse.teeth(idx); },
                brefuse.teeth_size(), prefuse.teeth);
            unserialize_list_creature("horns", all, val, filter,
                [&](const size_t& idx) -> const string& { return brefuse.horns(idx); },
                brefuse.horns_size(), prefuse.horns);
        });

}

bool StockpileSerializer::write_sheet(StockpileSettings::SheetSet* sheet) {
    bool all = serialize_list_organic_mat(
        [&](const std::string& token) { sheet->add_paper(token); },
        &mPile->settings.sheet.paper, organic_mat_category::Paper);

    all = serialize_list_organic_mat(
        [&](const std::string& token) { sheet->add_parchment(token); },
        &mPile->settings.sheet.parchment, organic_mat_category::Parchment) && all;

    return all;
}

void StockpileSerializer::read_sheet(DeserializeMode mode, const std::string& filter) {
    auto & psheet = mPile->settings.sheet;
    read_category<StockpileSettings_SheetSet>("sheet", mode,
        std::bind(&StockpileSettings::has_sheet, mBuffer),
        std::bind(&StockpileSettings::sheet, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_sheet,
        [&]() {
            psheet.paper.clear();
            psheet.parchment.clear();
        },
        [&](bool all, char val) {
            auto & bsheet = mBuffer.sheet();

            unserialize_list_organic_mat("paper", all, val, filter,
                [&](size_t idx) -> std::string { return bsheet.paper(idx); },
                bsheet.paper_size(), psheet.paper, organic_mat_category::Paper);

            unserialize_list_organic_mat("parchment", all, val, filter,
                [&](size_t idx) -> std::string { return bsheet.parchment(idx); },
                bsheet.parchment_size(), psheet.parchment, organic_mat_category::Parchment);
        });
}

static bool stone_is_allowed(const MaterialInfo& mi) {
    if (!mi.isValid())
        return false;
    const bool is_allowed_soil = mi.inorganic->flags.is_set(inorganic_flags::SOIL) && !mi.inorganic->flags.is_set(inorganic_flags::AQUIFER);
    const bool is_allowed_stone = mi.material->flags.is_set(material_flags::IS_STONE) && !mi.material->flags.is_set(material_flags::NO_STONE_STOCKPILE);
    return is_allowed_soil || is_allowed_stone;
}

bool StockpileSerializer::write_stone(StockpileSettings::StoneSet* stone) {
    return serialize_list_material(
        stone_is_allowed,
        [&](const std::string& token) { stone->add_mats(token); },
        mPile->settings.stone.mats);
}

void StockpileSerializer::read_stone(DeserializeMode mode, const std::string& filter) {
    auto & pstone = mPile->settings.stone;
    read_category<StockpileSettings_StoneSet>("stone", mode,
        std::bind(&StockpileSettings::has_stone, mBuffer),
        std::bind(&StockpileSettings::stone, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_stone,
        [&]() {
            pstone.mats.clear();
        },
        [&](bool all, char val) {
            auto & bstone = mBuffer.stone();

            unserialize_list_material("", all, val, filter, stone_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bstone.mats(idx); },
                bstone.mats_size(), pstone.mats);
        });
}

static bool weapons_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && (mi.material->flags.is_set(material_flags::IS_METAL) || mi.material->flags.is_set(material_flags::IS_STONE));
}

bool StockpileSerializer::write_weapons(StockpileSettings::WeaponsSet* weapons) {
    auto & pweapons = mPile->settings.weapons;
    bool all = pweapons.unusable && pweapons.usable;

    weapons->set_unusable(pweapons.unusable);
    weapons->set_usable(pweapons.usable);

    all = serialize_list_itemdef(
        [&](const std::string& token) { weapons->add_weapon_type(token); },
        pweapons.weapon_type,
        std::vector<df::itemdef*>(world->raws.itemdefs.weapons.begin(), world->raws.itemdefs.weapons.end()),
        item_type::WEAPON) && all;

    all = serialize_list_itemdef(
        [&](const std::string& token) { weapons->add_trapcomp_type(token); },
        pweapons.trapcomp_type,
        std::vector<df::itemdef*>(world->raws.itemdefs.trapcomps.begin(), world->raws.itemdefs.trapcomps.end()),
        item_type::TRAPCOMP) && all;

    all = serialize_list_material(
        weapons_mat_is_allowed,
        [&](const std::string& token) { weapons->add_mats(token); },
        pweapons.mats) && all;

    all = serialize_list_other_mats(
        mOtherMatsWeaponsArmor.mats,
        [&](const std::string& token) { weapons->add_other_mats(token); },
        pweapons.other_mats) && all;

    all = serialize_list_quality(
        [&](const std::string& token) { weapons->add_quality_core(token); },
        pweapons.quality_core) && all;

    all = serialize_list_quality(
        [&](const std::string& token) { weapons->add_quality_total(token); },
        pweapons.quality_total) && all;

    return all;
}

void StockpileSerializer::read_weapons(DeserializeMode mode, const std::string& filter) {
    auto & pweapons = mPile->settings.weapons;
    read_category<StockpileSettings_WeaponsSet>("weapons", mode,
        std::bind(&StockpileSettings::has_weapons, mBuffer),
        std::bind(&StockpileSettings::weapons, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_weapons,
        [&]() {
            pweapons.weapon_type.clear();
            pweapons.trapcomp_type.clear();
            pweapons.other_mats.clear();
            pweapons.mats.clear();
            quality_clear(pweapons.quality_core);
            quality_clear(pweapons.quality_total);
        },
        [&](bool all, char val) {
            auto & bweapons = mBuffer.weapons();

            set_flag("nouse", filter, all, val, bweapons.unusable(), pweapons.unusable);
            set_flag("canuse", filter, all, val, bweapons.usable(), pweapons.usable);

            unserialize_list_itemdef("type/weapon", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bweapons.weapon_type(idx); },
                bweapons.weapon_type_size(), pweapons.weapon_type, item_type::WEAPON);

            unserialize_list_itemdef("type/trapcomp", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bweapons.trapcomp_type(idx); },
                bweapons.trapcomp_type_size(), pweapons.trapcomp_type, item_type::TRAPCOMP);

            unserialize_list_material("mats", all, val, filter, weapons_mat_is_allowed,
                [&](const size_t& idx) -> const std::string& { return bweapons.mats(idx); },
                bweapons.mats_size(), pweapons.mats);

            unserialize_list_other_mats("other", all, val, filter, mOtherMatsWeaponsArmor.mats,
                [&](const size_t& idx) -> const std::string& { return bweapons.other_mats(idx); },
                bweapons.other_mats_size(), pweapons.other_mats);

            unserialize_list_quality("core", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bweapons.quality_core(idx); },
                bweapons.quality_core_size(), pweapons.quality_core);

            unserialize_list_quality("total", all, val, filter,
                [&](const size_t& idx) -> const std::string& { return bweapons.quality_total(idx); },
                bweapons.quality_total_size(), pweapons.quality_total);
        });
}

static bool wood_mat_is_allowed(const df::plant_raw* plant) {
    return plant && plant->flags.is_set(plant_raw_flags::TREE);
}

bool StockpileSerializer::write_wood(StockpileSettings::WoodSet* wood) {
    bool all = true;
    for (size_t i = 0; i < mPile->settings.wood.mats.size(); ++i) {
        if (!mPile->settings.wood.mats.at(i)) {
            all = false;
            continue;
        }
        const df::plant_raw* plant = find_plant(i);
        if (!wood_mat_is_allowed(plant))
            continue;
        wood->add_mats(plant->id);
        DEBUG(log).print("plant %zd is %s\n", i, plant->id.c_str());
    }
    return all;
}

void StockpileSerializer::read_wood(DeserializeMode mode, const std::string& filter) {
    auto & pwood = mPile->settings.wood;
    read_category<StockpileSettings_WoodSet>("wood", mode,
        std::bind(&StockpileSettings::has_wood, mBuffer),
        std::bind(&StockpileSettings::wood, mBuffer),
        mPile->settings.flags.whole,
        mPile->settings.flags.mask_wood,
        [&]() {
            pwood.mats.clear();
        },
        [&](bool all, char val) {
            auto & bwood = mBuffer.wood();

            size_t num_elems = world->raws.plants.all.size();
            pwood.mats.resize(num_elems, '\0');

            if (all) {
                for (size_t idx = 0; idx < num_elems; ++idx) {
                    string id = world->raws.plants.all[idx]->id;
                    set_filter_elem("", filter, val, id, idx, pwood.mats.at(idx));
                }
            } else {
                for (int i = 0; i < bwood.mats_size(); ++i) {
                    const std::string token = bwood.mats(i);
                    const size_t idx = find_plant(token);
                    if (idx < 0 || (size_t)idx >= num_elems) {
                        WARN(log).print("wood mat index invalid %s idx=%zd\n", token.c_str(), idx);
                        continue;
                    }
                    set_filter_elem("", filter, val, token, idx, pwood.mats.at(idx));
                }
            }
        });
}
