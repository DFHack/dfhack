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
#include "df/caste_raw.h"
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
using std::placeholders::_2;

namespace DFHack
{
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

StockpileSettingsSerializer::StockpileSettingsSerializer(df::stockpile_settings *settings)
    : mSettings(settings) { }

StockpileSettingsSerializer::~StockpileSettingsSerializer() { }

StockpileSerializer::StockpileSerializer(df::building_stockpilest* stockpile)
    : StockpileSettingsSerializer(&stockpile->settings), mPile(stockpile) { }

StockpileSerializer::~StockpileSerializer() { }

bool StockpileSettingsSerializer::serialize_to_ostream(color_ostream& out, std::ostream* output, uint32_t includedElements) {
    if (output->fail())
        return false;
    mBuffer.Clear();
    write(out, includedElements);
    {
        io::OstreamOutputStream zero_copy_output(output);
        if (!mBuffer.SerializeToZeroCopyStream(&zero_copy_output))
            return false;
    }
    return output->good();
}

bool StockpileSettingsSerializer::serialize_to_file(color_ostream& out, const string& file, uint32_t includedElements) {
    std::fstream output(file, std::ios::out | std::ios::binary | std::ios::trunc);
    if (output.fail()) {
        WARN(log, out).print("ERROR: failed to open file for writing: '%s'\n",
                file.c_str());
        return false;
    }
    return serialize_to_ostream(out, &output, includedElements);
}

bool StockpileSettingsSerializer::parse_from_istream(color_ostream &out, std::istream* input, DeserializeMode mode, const vector<string>& filters) {
    if (input->fail())
        return false;
    mBuffer.Clear();
    io::IstreamInputStream zero_copy_input(input);
    const bool res = mBuffer.ParseFromZeroCopyStream(&zero_copy_input)
            && input->eof();
    if (res)
        read(out, mode, filters);
    return res;
}

bool StockpileSettingsSerializer::unserialize_from_file(color_ostream &out, const string& file, DeserializeMode mode, const vector<string>& filters) {
    std::fstream input(file, std::ios::in | std::ios::binary);
    if (input.fail()) {
        WARN(log, out).print("failed to open file for reading: '%s'\n",
                file.c_str());
        return false;
    }
    return parse_from_istream(out, &input, mode, filters);
}

/**
 * Find an enum's value based off the string label.
 * @param traits the enum's trait struct
 * @param token the string value in key_table
 * @return the enum's value,  -1 if not found
 */
template <typename E>
static typename df::enum_traits<E>::base_type linear_index(df::enum_traits<E> traits, const string& token) {
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

static bool matches_filter(color_ostream& out, const vector<string>& filters, const string& name) {
    for (auto & filter : filters) {
        DEBUG(log, out).print("searching for '%s' in '%s'\n", filter.c_str(), name.c_str());
        if (std::search(name.begin(), name.end(), filter.begin(), filter.end(),
            [](unsigned char ch1, unsigned char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
        ) != name.end())
            return true;
    }
    return !filters.size();
}

static void set_flag(color_ostream& out, const char* name, const vector<string>& filters, bool all, char val, bool enabled, bool& elem) {
    if ((all || enabled) && matches_filter(out, filters, name)) {
        DEBUG(log, out).print("setting %s to %d\n", name, val);
        elem = val;
    }
}

static void set_filter_elem(color_ostream& out, const char* subcat, const vector<string>& filters, char val,
        const string& name, const string& id, char& elem) {
    if (matches_filter(out, filters, subcat + ((*subcat ? "/" : "") + name))) {
        DEBUG(log, out).print("setting %s (%s) to %d\n", name.c_str(), id.c_str(), val);
        elem = val;
    }
}

template<typename T_val, typename T_id>
static void set_filter_elem(color_ostream& out, const char* subcat, const vector<string>& filters, T_val val,
        const string& name, T_id id, T_val& elem) {
    if (matches_filter(out, filters, subcat + ((*subcat ? "/" : "") + name))) {
        DEBUG(log, out).print("setting %s (%d) to %d\n", name.c_str(), (int32_t)id, val);
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
static bool serialize_list_itemdef(color_ostream& out, FuncWriteExport add_value,
        vector<char> list,
        vector<df::itemdef*> items,
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
        DEBUG(log, out).print("adding itemdef type %s\n", ii.getToken().c_str());
        add_value(ii.getToken());
    }
    return all;
}

static void unserialize_list_itemdef(color_ostream& out, const char* subcat, bool all, char val, const vector<string>& filters,
        FuncReadImport read_value, int32_t list_size, vector<char>& pile_list, item_type::item_type type) {
    int num_elems = Items::getSubtypeCount(type);
    pile_list.resize(num_elems, '\0');
    if (all) {
        for (auto idx = 0; idx < num_elems; ++idx) {
            ItemTypeInfo ii;
            ii.decode(type, idx);
            set_filter_elem(out, subcat, filters, val, ii.toString(), idx, pile_list.at(idx));
        }
        return;
    }

    for (auto i = 0; i < list_size; ++i) {
        string id = read_value(i);
        ItemTypeInfo ii;
        if (!ii.find(id))
            continue;
        if (ii.subtype < 0 || size_t(ii.subtype) >= pile_list.size()) {
            WARN(log, out).print("item type index invalid: %d\n", ii.subtype);
            continue;
        }
        set_filter_elem(out, subcat, filters, val, id, ii.subtype, pile_list.at(ii.subtype));
    }
}

static bool serialize_list_quality(color_ostream& out, FuncWriteExport add_value,
        const bool(&quality_list)[7]) {
    using df::enums::item_quality::item_quality;
    using quality_traits = df::enum_traits<item_quality>;

    bool all = true;
    for (size_t i = 0; i < 7; ++i) {
        if (!quality_list[i]) {
            all = false;
            continue;
        }
        const string f_type(quality_traits::key_table[i]);
        add_value(f_type);
        DEBUG(log, out).print("adding quality %s\n", f_type.c_str());
    }
    return all;
}

static void quality_clear(bool(&pile_list)[7]) {
    std::fill(pile_list, pile_list + 7, false);
}

static void unserialize_list_quality(color_ostream& out, const char* subcat, bool all, bool val, const vector<string>& filters,
        FuncReadImport read_value, int32_t list_size, bool(&pile_list)[7]) {
    if (all) {
        for (auto idx = 0; idx < 7; ++idx) {
            string id = ENUM_KEY_STR(item_quality, (df::item_quality)idx);
            set_filter_elem(out, subcat, filters, val, id, idx, pile_list[idx]);
        }
        return;
    }

    using df::enums::item_quality::item_quality;
    df::enum_traits<item_quality> quality_traits;
    for (int i = 0; i < list_size; ++i) {
        const string quality = read_value(i);
        df::enum_traits<item_quality>::base_type idx = linear_index(quality_traits, quality);
        if (idx < 0) {
            WARN(log, out).print("invalid quality token: %s\n", quality.c_str());
            continue;
        }
        set_filter_elem(out, subcat, filters, val, quality, idx, pile_list[idx]);
    }
}

static string other_mats_index(const std::map<int, string> other_mats,
        int idx) {
    auto it = other_mats.find(idx);
    if (it == other_mats.end())
        return string();
    return it->second;
}

static int other_mats_token(const std::map<int, string> other_mats,
        const string& token) {
    for (auto it = other_mats.begin(); it != other_mats.end(); ++it) {
        if (it->second == token)
            return it->first;
    }
    return -1;
}

static bool serialize_list_other_mats(color_ostream& out,
            const std::map<int, string> other_mats,
            FuncWriteExport add_value,
            vector<char> list) {
    bool all = true;
    for (size_t i = 0; i < list.size(); ++i) {
        if (!list.at(i)) {
            all = false;
            continue;
        }
        const string token = other_mats_index(other_mats, i);
        if (token.empty()) {
            WARN(log, out).print("invalid other material with index %zd\n", i);
            continue;
        }
        add_value(token);
        DEBUG(log, out).print("other mats %zd is %s\n", i, token.c_str());
    }
    return all;
}

static void unserialize_list_other_mats(color_ostream& out, const char* subcat, bool all, char val, const vector<string>& filters,
            const std::map<int, string> other_mats, FuncReadImport read_value, int32_t list_size, vector<char>& pile_list) {
    size_t num_elems = other_mats.size();
    pile_list.resize(num_elems, '\0');

    if (all) {
        for (auto & entry : other_mats)
            set_filter_elem(out, subcat, filters, val, entry.second, entry.first, pile_list.at(entry.first));
        return;
    }

    for (int i = 0; i < list_size; ++i) {
        const string token = read_value(i);
        size_t idx = other_mats_token(other_mats, token);
        if (idx < 0) {
            WARN(log, out).print("invalid other mat with token %s\n", token.c_str());
            continue;
        }
        if (idx >= num_elems) {
            WARN(log, out).print("other_mats index too large! idx[%zd] max_size[%zd]\n", idx, num_elems);
            continue;
        }
        set_filter_elem(out, subcat, filters, val, token, idx, pile_list.at(idx));
    }
}

static bool serialize_list_organic_mat(color_ostream& out, FuncWriteExport add_value,
        const vector<char>* list,
        organic_mat_category::organic_mat_category cat) {
    bool all = true;
    if (!list) {
        DEBUG(log, out).print("serialize_list_organic_mat: list null\n");
        return all;
    }
    for (size_t i = 0; i < list->size(); ++i) {
        if (!list->at(i)) {
            all = false;
            continue;
        }
        string token = OrganicMatLookup::food_token_by_idx(out, cat, i);
        if (token.empty()) {
            DEBUG(log, out).print("food mat invalid :(\n");
            continue;
        }
        DEBUG(log, out).print("organic_material %zd is %s\n", i, token.c_str());
        add_value(token);
    }
    return all;
}

static void unserialize_list_organic_mat(color_ostream& out, const char* subcat, bool all, char val, const vector<string>& filters,
        FuncReadImport read_value, size_t list_size, vector<char>& pile_list,
        organic_mat_category::organic_mat_category cat) {
    size_t num_elems = OrganicMatLookup::food_max_size(cat);
    pile_list.resize(num_elems, '\0');
    if (all) {
        for (size_t idx = 0; idx < num_elems; ++idx) {
            string token = OrganicMatLookup::food_token_by_idx(out, cat, idx);
            set_filter_elem(out, subcat, filters, val, token, idx, pile_list.at(idx));
        }
        return;
    }

    for (size_t i = 0; i < list_size; ++i) {
        const string token = read_value(i);
        int16_t idx = OrganicMatLookup::food_idx_by_token(out, cat, token);
        if (idx < 0 || size_t(idx) >= num_elems) {
            WARN(log, out).print("organic mat index too large! idx[%d] max_size[%zd]\n", idx, num_elems);
            continue;
        }
        set_filter_elem(out, subcat, filters, val, token, idx, pile_list.at(idx));
    }
}

static bool serialize_list_item_type(color_ostream& out, FuncItemAllowed is_allowed,
        FuncWriteExport add_value, const vector<char>& list) {
    using df::enums::item_type::item_type;
    using type_traits = df::enum_traits<item_type>;

    bool all = true;
    size_t num_item_types = list.size();
    DEBUG(log, out).print("item_type size = %zd size limit = %d typecasted: %zd\n",
            num_item_types, type_traits::last_item_value,
            (size_t)type_traits::last_item_value);
    for (size_t i = 0; i <= (size_t)type_traits::last_item_value; ++i) {
        if (i < num_item_types && !list.at(i)) {
            all = false;
            continue;
        }
        const item_type type = (item_type)((df::enum_traits<item_type>::base_type)i);
        string r_type(type_traits::key_table[i + 1]);
        if (!is_allowed(type))
            continue;
        add_value(r_type);
        DEBUG(log, out).print("item_type key_table[%zd] type[%d] is %s\n", i + 1, (int16_t)type, r_type.c_str());
    }
    return all;
}

static void unserialize_list_item_type(color_ostream& out, const char* subcat, bool all, char val, const vector<string>& filters,
        FuncItemAllowed is_allowed, FuncReadImport read_value, int32_t list_size, vector<char>& pile_list) {
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
            set_filter_elem(out, subcat, filters, val, id, idx, pile_list.at(idx));
        }
        return;
    }

    using df::enums::item_type::item_type;
    df::enum_traits<item_type> type_traits;
    for (int i = 0; i < list_size; ++i) {
        const string token = read_value(i);
        // subtract one because item_type starts at -1
        const df::enum_traits<item_type>::base_type idx = linear_index(type_traits, token) - 1;
        if (!is_allowed((item_type)idx))
            continue;
        if (idx < 0 || size_t(idx) >= num_elems) {
            WARN(log, out).print("error item_type index too large! idx[%d] max_size[%zd]\n", idx, num_elems);
            continue;
        }
        set_filter_elem(out, subcat, filters, val, token, idx, pile_list.at(idx));
    }
}

static bool serialize_list_material(color_ostream& out, FuncMaterialAllowed is_allowed,
        FuncWriteExport add_value, const vector<char>& list) {
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
        DEBUG(log, out).print("adding material %s\n", mi.getToken().c_str());
        add_value(mi.getToken());
    }
    return all;
}

static void unserialize_list_material(color_ostream& out, const char* subcat, bool all, char val, const vector<string>& filters,
        FuncMaterialAllowed is_allowed, FuncReadImport read_value, int32_t list_size,
        vector<char>& pile_list) {
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
            set_filter_elem(out, subcat, filters, val, mi.toString(), idx, pile_list.at(idx));
        }
        return;
    }

    for (auto i = 0; i < list_size; ++i) {
        string id = read_value(i);
        MaterialInfo mi;
        if (!mi.find(id) || !is_allowed(mi))
            continue;
        if (mi.index < 0 || size_t(mi.index) >= pile_list.size()) {
            WARN(log, out).print("material type index invalid: %d\n", mi.index);
            continue;
        }
        set_filter_elem(out, subcat, filters, val, id, mi.index, pile_list.at(mi.index));
    }
}

static bool serialize_list_creature(color_ostream& out, FuncWriteExport add_value, const vector<char>& list) {
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
        DEBUG(log, out).print("adding creature %s\n", r->creature_id.c_str());
        add_value(r->creature_id);
    }
    return all;
}

static string get_filter_string(df::creature_raw *r) {
    if (!r->caste.size() ||
            (!r->caste[0]->flags.is_set(df::enums::caste_raw_flags::PET) &&
             !r->caste[0]->flags.is_set(df::enums::caste_raw_flags::PET_EXOTIC)))
        return r->name[0];
    return r->name[0] + "/tameable";
}

static void unserialize_list_creature(color_ostream& out, const char* subcat, bool all, char val, const vector<string>& filters,
        FuncReadImport read_value, int32_t list_size, vector<char>& pile_list) {
    size_t num_elems = world->raws.creatures.all.size();
    pile_list.resize(num_elems, '\0');
    if (all) {
        for (size_t idx = 0; idx < num_elems; ++idx) {
            auto r = find_creature(idx);
            set_filter_elem(out, subcat, filters, val, get_filter_string(r), r->creature_id, pile_list.at(idx));
        }
        return;
    }

    for (auto i = 0; i < list_size; ++i) {
        string id = read_value(i);
        int idx = find_creature(id);
        if (idx < 0 || size_t(idx) >= num_elems) {
            WARN(log, out).print("animal index invalid: %d\n", idx);
            continue;
        }
        auto r = find_creature(idx);
        set_filter_elem(out, subcat, filters, val, get_filter_string(r), r->creature_id, pile_list.at(idx));
    }
}

template<typename T_cat_set>
static void write_cat(color_ostream& out, const char* name, bool include_types, uint32_t cat_flags,
    enum df::stockpile_group_set::Mask cat_mask,
    std::function<T_cat_set* ()> mutable_cat_fn,
    std::function<bool(color_ostream&, T_cat_set*)> write_cat_fn) {

    if (!(cat_flags & cat_mask))
        return;

    T_cat_set* cat_set = mutable_cat_fn();

    if (!include_types) {
        DEBUG(log, out).print("including all for %s since only category is being recorded\n", name);
        cat_set->set_all(true);
        return;
    }

    if (write_cat_fn(out, cat_set)) {
        // all fields were set. clear them and use the "all" flag instead so "all" can be applied
        // to other worlds with other generated types
        DEBUG(log, out).print("including all for %s since all fields were enabled\n", name);
        cat_set->Clear();
        cat_set->set_all(true);
    }
}

void StockpileSettingsSerializer::write(color_ostream& out, uint32_t includedElements) {
    if (includedElements & INCLUDED_ELEMENTS_GENERAL)
        write_general(out);

    if (!(includedElements & INCLUDED_ELEMENTS_CATEGORIES))
        return;

    DEBUG(log, out).print("GROUP SET %s\n",
            bitfield_to_string(mSettings->flags).c_str());

    bool include_types = 0 != (includedElements & INCLUDED_ELEMENTS_TYPES);

    write_cat<StockpileSettings_AmmoSet>(out, "ammo", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_ammo,
        std::bind(&StockpileSettings::mutable_ammo, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_ammo, this, _1, _2));
    write_cat<StockpileSettings_AnimalsSet>(out, "animals", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_animals,
        std::bind(&StockpileSettings::mutable_animals, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_animals, this, _1, _2));
    write_cat<StockpileSettings_ArmorSet>(out, "armor", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_armor,
        std::bind(&StockpileSettings::mutable_armor, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_armor, this, _1, _2));
    write_cat<StockpileSettings_BarsBlocksSet>(out, "bars_blocks", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_bars_blocks,
        std::bind(&StockpileSettings::mutable_barsblocks, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_bars_blocks, this, _1, _2));
    write_cat<StockpileSettings_ClothSet>(out, "cloth", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_cloth,
        std::bind(&StockpileSettings::mutable_cloth, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_cloth, this, _1, _2));
    write_cat<StockpileSettings_CoinSet>(out, "coin", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_coins,
        std::bind(&StockpileSettings::mutable_coin, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_coins, this, _1, _2));
    write_cat<StockpileSettings_FinishedGoodsSet>(out, "finished_goods", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_finished_goods,
        std::bind(&StockpileSettings::mutable_finished_goods, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_finished_goods, this, _1, _2));
    write_cat<StockpileSettings_FoodSet>(out, "food", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_food,
        std::bind(&StockpileSettings::mutable_food, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_food, this, _1, _2));
    write_cat<StockpileSettings_FurnitureSet>(out, "furniture", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_furniture,
        std::bind(&StockpileSettings::mutable_furniture, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_furniture, this, _1, _2));
    write_cat<StockpileSettings_GemsSet>(out, "gems", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_gems,
        std::bind(&StockpileSettings::mutable_gems, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_gems, this, _1, _2));
    write_cat<StockpileSettings_LeatherSet>(out, "leather", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_leather,
        std::bind(&StockpileSettings::mutable_leather, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_leather, this, _1, _2));
    write_cat<StockpileSettings_CorpsesSet>(out, "corpses", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_corpses,
        std::bind(&StockpileSettings::mutable_corpses_v50, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_corpses, this, _1, _2));
    write_cat<StockpileSettings_RefuseSet>(out, "refuse", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_refuse,
        std::bind(&StockpileSettings::mutable_refuse, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_refuse, this, _1, _2));
    write_cat<StockpileSettings_SheetSet>(out, "sheet", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_sheet,
        std::bind(&StockpileSettings::mutable_sheet, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_sheet, this, _1, _2));
    write_cat<StockpileSettings_StoneSet>(out, "stone", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_stone,
        std::bind(&StockpileSettings::mutable_stone, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_stone, this, _1, _2));
    write_cat<StockpileSettings_WeaponsSet>(out, "weapons", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_weapons,
        std::bind(&StockpileSettings::mutable_weapons, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_weapons, this, _1, _2));
    write_cat<StockpileSettings_WoodSet>(out, "wood", include_types,
        mSettings->flags.whole,
        mSettings->flags.mask_wood,
        std::bind(&StockpileSettings::mutable_wood, &mBuffer),
        std::bind(&StockpileSettingsSerializer::write_wood, this, _1, _2));
}

void StockpileSerializer::write(color_ostream& out, uint32_t includedElements) {
    if (includedElements & INCLUDED_ELEMENTS_CONTAINERS)
        write_containers(out);

    StockpileSettingsSerializer::write(out, includedElements);
}

void StockpileSettingsSerializer::read(color_ostream &out, DeserializeMode mode, const vector<string>& filters) {
    DEBUG(log).print("==READ==\n");
    read_general(out, mode);
    read_ammo(out, mode, filters);
    read_animals(out, mode, filters);
    read_armor(out, mode, filters);
    read_bars_blocks(out, mode, filters);
    read_cloth(out, mode, filters);
    read_coins(out, mode, filters);
    read_finished_goods(out, mode, filters);
    read_food(out, mode, filters);
    read_furniture(out, mode, filters);
    read_gems(out, mode, filters);
    read_leather(out, mode, filters);

    // support for old versions before corpses had a set
    if (mBuffer.has_corpses()) {
        StockpileSettings::CorpsesSet* corpses = mBuffer.mutable_corpses_v50();
        corpses->set_all(true);
    }
    read_corpses(out, mode, filters);

    read_refuse(out, mode, filters);
    read_sheet(out, mode, filters);
    read_stone(out, mode, filters);
    read_weapons(out, mode, filters);
    read_wood(out, mode, filters);
}

void StockpileSerializer::read(color_ostream &out, DeserializeMode mode, const vector<string>& filters) {
    read_containers(out, mode);
    StockpileSettingsSerializer::read(out, mode, filters);
}

void StockpileSerializer::write_containers(color_ostream& out) {
    DEBUG(log, out).print("writing container settings\n");
    mBuffer.set_max_bins(mPile->max_bins);
    mBuffer.set_max_barrels(mPile->max_barrels);
    mBuffer.set_max_wheelbarrows(mPile->max_wheelbarrows);
}

template<typename T_elem, typename T_elem_ret>
static void read_elem(color_ostream& out, const char* name, DeserializeMode mode,
        std::function<bool()> has_elem_fn,
        std::function<T_elem_ret()> elem_fn,
        T_elem &setting) {
    if (!has_elem_fn())
        return;

    bool is_set = elem_fn() != 0;
    if (mode == DESERIALIZE_MODE_SET || is_set) {
        T_elem val = (mode == DESERIALIZE_MODE_DISABLE) ? 0 : elem_fn();
        DEBUG(log, out).print("setting %s to %d\n", name, val);
        setting = val;
    }
}

template<typename T_cat>
static void read_category(color_ostream& out, const char* name, DeserializeMode mode,
        std::function<bool()> has_cat_fn,
        std::function<const T_cat &()> cat_fn,
        uint32_t & cat_flags,
        enum df::stockpile_group_set::Mask cat_mask,
        std::function<void()> clear_fn,
        std::function<void(bool, char)> set_fn) {
    if (mode == DESERIALIZE_MODE_SET) {
        DEBUG(log, out).print("clearing %s\n", name);
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
    DEBUG(log, out).print("setting %s %s elements to %d\n",
            all ? "all" : "marked", name, val);
    set_fn(all, val);
}

void StockpileSerializer::read_containers(color_ostream& out, DeserializeMode mode) {
    read_elem<int16_t, int32_t>(out, "max_bins", mode,
            std::bind(&StockpileSettings::has_max_bins, mBuffer),
            std::bind(&StockpileSettings::max_bins, mBuffer),
            mPile->max_bins);
    read_elem<int16_t, int32_t>(out, "max_barrels", mode,
            std::bind(&StockpileSettings::has_max_barrels, mBuffer),
            std::bind(&StockpileSettings::max_barrels, mBuffer),
            mPile->max_barrels);
    read_elem<int16_t, int32_t>(out, "max_wheelbarrows", mode,
            std::bind(&StockpileSettings::has_max_wheelbarrows, mBuffer),
            std::bind(&StockpileSettings::max_wheelbarrows, mBuffer),
            mPile->max_wheelbarrows);
}

void StockpileSettingsSerializer::write_general(color_ostream& out) {
    DEBUG(log, out).print("writing general settings\n");
    mBuffer.set_allow_inorganic(mSettings->allow_inorganic);
    mBuffer.set_allow_organic(mSettings->allow_organic);
}

void StockpileSerializer::write_general(color_ostream& out) {
    StockpileSettingsSerializer::write_general(out);
    mBuffer.set_use_links_only(mPile->stockpile_flag.bits.use_links_only);
}

void StockpileSettingsSerializer::read_general(color_ostream& out, DeserializeMode mode) {
    read_elem<bool, bool>(out, "allow_inorganic", mode,
            std::bind(&StockpileSettings::has_allow_inorganic, mBuffer),
            std::bind(&StockpileSettings::allow_inorganic, mBuffer),
            mSettings->allow_inorganic);
    read_elem<bool, bool>(out, "allow_organic", mode,
            std::bind(&StockpileSettings::has_allow_organic, mBuffer),
            std::bind(&StockpileSettings::allow_organic, mBuffer),
            mSettings->allow_organic);
}

void StockpileSerializer::read_general(color_ostream& out, DeserializeMode mode) {
    StockpileSettingsSerializer::read_general(out, mode);
    if (!mBuffer.has_use_links_only())
        return;
    bool use_links_only = mBuffer.use_links_only();
    DEBUG(log, out).print("setting use_links_only to %d\n", use_links_only);
    mPile->stockpile_flag.bits.use_links_only = use_links_only;
}

static bool ammo_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && mi.material->flags.is_set(material_flags::IS_METAL);
}

bool StockpileSettingsSerializer::write_ammo(color_ostream& out, StockpileSettings::AmmoSet* ammo) {
    bool all = serialize_list_itemdef(out,
        [&](const string& token) { ammo->add_type(token); },
        mSettings->ammo.type,
        vector<df::itemdef*>(world->raws.itemdefs.ammo.begin(), world->raws.itemdefs.ammo.end()),
        item_type::AMMO);

    all = serialize_list_material(out,
        ammo_mat_is_allowed,
        [&](const string& token) { ammo->add_mats(token); },
        mSettings->ammo.mats) && all;

    if (mSettings->ammo.other_mats.size() > 2) {
        WARN(log, out).print("ammo other materials > 2: %zd\n",
            mSettings->ammo.other_mats.size());
    }

    size_t num_other_mats = std::min(size_t(2),
            mSettings->ammo.other_mats.size());
    for (size_t i = 0; i < num_other_mats; ++i) {
        if (!mSettings->ammo.other_mats.at(i)) {
            all = false;
            continue;
        }
        const string token = i == 0 ? "WOOD" : "BONE";
        ammo->add_other_mats(token);
        DEBUG(log, out).print("other mats %zd is %s\n", i, token.c_str());
    }

    all = serialize_list_quality(out,
        [&](const string& token) { ammo->add_quality_core(token); },
        mSettings->ammo.quality_core) && all;

    all = serialize_list_quality(out,
        [&](const string& token) { ammo->add_quality_total(token); },
        mSettings->ammo.quality_total) && all;

    return all;
}

void StockpileSettingsSerializer::read_ammo(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pammo = mSettings->ammo;
    read_category<StockpileSettings_AmmoSet>(out, "ammo", mode,
        std::bind(&StockpileSettings::has_ammo, mBuffer),
        std::bind(&StockpileSettings::ammo, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_ammo,
        [&]() {
            pammo.type.clear();
            pammo.mats.clear();
            pammo.other_mats.clear();
            quality_clear(pammo.quality_core);
            quality_clear(pammo.quality_total);
        },
        [&](bool all, char val) {
            auto & bammo = mBuffer.ammo();

            unserialize_list_itemdef(out, "type", all, val, filters,
                [&](const size_t& idx) -> const string& { return bammo.type(idx); },
                bammo.type_size(), pammo.type, item_type::AMMO);

            unserialize_list_material(out, "mats", all, val, filters, ammo_mat_is_allowed,
                [&](const size_t& idx) -> const string& { return bammo.mats(idx); },
                bammo.mats_size(), pammo.mats);

            pammo.other_mats.resize(2, '\0');
            if (all) {
                set_filter_elem(out, "other", filters, val, "WOOD", 0, pammo.other_mats.at(0));
                set_filter_elem(out, "other", filters, val, "BONE", 1, pammo.other_mats.at(1));
            } else {
                // TODO can we un-hardcode the values?
                for (int i = 0; i < bammo.other_mats_size(); ++i) {
                    const string id = bammo.other_mats(i);
                    const int32_t idx = id == "WOOD" ? 0 : id == "BONE" ? 1 : -1;
                    if (idx == -1)
                        continue;
                    set_filter_elem(out, "other", filters, val, id, idx, pammo.other_mats.at(idx));
                }
            }

            unserialize_list_quality(out, "core", all, val, filters,
                [&](const size_t& idx) -> const string& { return bammo.quality_core(idx); },
                bammo.quality_core_size(), pammo.quality_core);

            unserialize_list_quality(out, "total", all, val, filters,
                [&](const size_t& idx) -> const string& { return bammo.quality_total(idx); },
                bammo.quality_total_size(), pammo.quality_total);
        });
}

bool StockpileSettingsSerializer::write_animals(color_ostream& out, StockpileSettings::AnimalsSet* animals) {
    auto & panimals = mSettings->animals;
    bool all = panimals.empty_cages && panimals.empty_traps;

    animals->set_empty_cages(panimals.empty_cages);
    animals->set_empty_traps(panimals.empty_traps);

    return serialize_list_creature(out,
        [&](const string& token) { animals->add_enabled(token); },
        panimals.enabled) && all;
}

void StockpileSettingsSerializer::read_animals(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & panimals = mSettings->animals;
    read_category<StockpileSettings_AnimalsSet>(out, "animals", mode,
        std::bind(&StockpileSettings::has_animals, mBuffer),
        std::bind(&StockpileSettings::animals, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_animals,
        [&]() {
            panimals.empty_cages = false;
            panimals.empty_traps = false;
            panimals.enabled.clear();
        },
        [&](bool all, char val) {
            auto & banimals = mBuffer.animals();

            set_flag(out, "cages", filters, all, val, banimals.empty_cages(), panimals.empty_cages);
            set_flag(out, "traps", filters, all, val, banimals.empty_traps(), panimals.empty_traps);

            unserialize_list_creature(out, "", all, val, filters,
                [&](const size_t& idx) -> const string& { return banimals.enabled(idx); },
                banimals.enabled_size(), panimals.enabled);
        });
}

static bool armor_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && mi.material->flags.is_set(material_flags::IS_METAL);
}

bool StockpileSettingsSerializer::write_armor(color_ostream& out, StockpileSettings::ArmorSet* armor) {

    auto & parmor = mSettings->armor;
    bool all = parmor.unusable && parmor.usable;

    armor->set_unusable(parmor.unusable);
    armor->set_usable(parmor.usable);

    // armor type
    all = serialize_list_itemdef(out,
        [&](const string& token) { armor->add_body(token); },
        parmor.body,
        vector<df::itemdef*>(world->raws.itemdefs.armor.begin(), world->raws.itemdefs.armor.end()),
        item_type::ARMOR) && all;

    // helm type
    all = serialize_list_itemdef(out,
        [&](const string& token) { armor->add_head(token); },
        parmor.head,
        vector<df::itemdef*>(world->raws.itemdefs.helms.begin(), world->raws.itemdefs.helms.end()),
        item_type::HELM) && all;

    // shoes type
    all = serialize_list_itemdef(out,
        [&](const string& token) { armor->add_feet(token); },
        parmor.feet,
        vector<df::itemdef*>(world->raws.itemdefs.shoes.begin(), world->raws.itemdefs.shoes.end()),
        item_type::SHOES) && all;

    // gloves type
    all = serialize_list_itemdef(out,
        [&](const string& token) { armor->add_hands(token); },
        parmor.hands,
        vector<df::itemdef*>(world->raws.itemdefs.gloves.begin(), world->raws.itemdefs.gloves.end()),
        item_type::GLOVES) && all;

    // pant type
    all = serialize_list_itemdef(out,
        [&](const string& token) { armor->add_legs(token); },
        parmor.legs,
        vector<df::itemdef*>(world->raws.itemdefs.pants.begin(), world->raws.itemdefs.pants.end()),
        item_type::PANTS) && all;

    // shield type
    all = serialize_list_itemdef(out,
        [&](const string& token) { armor->add_shield(token); },
        parmor.shield,
        vector<df::itemdef*>(world->raws.itemdefs.shields.begin(), world->raws.itemdefs.shields.end()),
        item_type::SHIELD) && all;

    // materials
    all = serialize_list_material(out,
        armor_mat_is_allowed,
        [&](const string& token) { armor->add_mats(token); },
        parmor.mats) && all;

    // other mats
    all = serialize_list_other_mats(out,
        mOtherMatsWeaponsArmor.mats, [&](const string& token) { armor->add_other_mats(token); },
        parmor.other_mats) && all;

    // quality core
    all = serialize_list_quality(out, [&](const string& token) { armor->add_quality_core(token); },
        parmor.quality_core) && all;

    // quality total
    all = serialize_list_quality(out, [&](const string& token) { armor->add_quality_total(token); },
        parmor.quality_total) && all;

    return all;
}

void StockpileSettingsSerializer::read_armor(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & parmor = mSettings->armor;
    read_category<StockpileSettings_ArmorSet>(out, "armor", mode,
        std::bind(&StockpileSettings::has_armor, mBuffer),
        std::bind(&StockpileSettings::armor, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_armor,
        [&]() {
            parmor.unusable = false;
            parmor.usable = false;
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

            set_flag(out, "nouse", filters, all, val, barmor.unusable(), parmor.unusable);
            set_flag(out, "canuse", filters, all, val, barmor.usable(), parmor.usable);

            unserialize_list_itemdef(out, "body", all, val, filters,
                [&](const size_t& idx) -> const string& { return barmor.body(idx); },
                barmor.body_size(), parmor.body, item_type::ARMOR);

            unserialize_list_itemdef(out, "head", all, val, filters,
                [&](const size_t& idx) -> const string& { return barmor.head(idx); },
                barmor.head_size(), parmor.head, item_type::HELM);

            unserialize_list_itemdef(out, "feet", all, val, filters,
                [&](const size_t& idx) -> const string& { return barmor.feet(idx); },
                barmor.feet_size(), parmor.feet, item_type::SHOES);

            unserialize_list_itemdef(out, "hands", all, val, filters,
                [&](const size_t& idx) -> const string& { return barmor.hands(idx); },
                barmor.hands_size(), parmor.hands, item_type::GLOVES);

            unserialize_list_itemdef(out, "legs", all, val, filters,
                [&](const size_t& idx) -> const string& { return barmor.legs(idx); },
                barmor.legs_size(), parmor.legs, item_type::PANTS);

            unserialize_list_itemdef(out, "shield", all, val, filters,
                [&](const size_t& idx) -> const string& { return barmor.shield(idx); },
                barmor.shield_size(), parmor.shield, item_type::SHIELD);

            unserialize_list_material(out, "mats", all, val, filters,
                armor_mat_is_allowed,
                [&](const size_t& idx) -> const string& { return barmor.mats(idx); },
                barmor.mats_size(), parmor.mats);

            unserialize_list_other_mats(out, "other", all, val, filters,
                mOtherMatsWeaponsArmor.mats, [&](const size_t& idx) -> const string& { return barmor.other_mats(idx); },
                barmor.other_mats_size(), parmor.other_mats);

            unserialize_list_quality(out, "core", all, val, filters,
                [&](const size_t& idx) -> const string& { return barmor.quality_core(idx); },
                barmor.quality_core_size(), parmor.quality_core);

            unserialize_list_quality(out, "total", all, val, filters,
                [&](const size_t& idx) -> const string& { return barmor.quality_total(idx); },
                barmor.quality_total_size(), parmor.quality_total);
        });
}

static bool bars_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && mi.material->flags.is_set(material_flags::IS_METAL);
}

static bool blocks_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && (mi.material->flags.is_set(material_flags::IS_METAL) || mi.material->flags.is_set(material_flags::IS_STONE));
}

bool StockpileSettingsSerializer::write_bars_blocks(color_ostream& out, StockpileSettings::BarsBlocksSet* bars_blocks) {
    bool all = serialize_list_material(out,
        bars_mat_is_allowed,
        [&](const string& token) { bars_blocks->add_bars_mats(token); },
        mSettings->bars_blocks.bars_mats);

    all = serialize_list_material(out,
        blocks_mat_is_allowed,
        [&](const string& token) { bars_blocks->add_blocks_mats(token); },
        mSettings->bars_blocks.blocks_mats) && all;

    all = serialize_list_other_mats(out,
        mOtherMatsBars.mats, [&](const string& token) { bars_blocks->add_bars_other_mats(token); },
        mSettings->bars_blocks.bars_other_mats) && all;

    all = serialize_list_other_mats(out,
        mOtherMatsBlocks.mats, [&](const string& token) { bars_blocks->add_blocks_other_mats(token); },
        mSettings->bars_blocks.blocks_other_mats) && all;

    return all;
}

void StockpileSettingsSerializer::read_bars_blocks(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pbarsblocks = mSettings->bars_blocks;
    read_category<StockpileSettings_BarsBlocksSet>(out, "bars_blocks", mode,
        std::bind(&StockpileSettings::has_barsblocks, mBuffer),
        std::bind(&StockpileSettings::barsblocks, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_bars_blocks,
        [&]() {
            pbarsblocks.bars_mats.clear();
            pbarsblocks.bars_other_mats.clear();
            pbarsblocks.blocks_mats.clear();
            pbarsblocks.blocks_other_mats.clear();
        },
        [&](bool all, char val) {
            auto & bbarsblocks = mBuffer.barsblocks();

            unserialize_list_material(out, "mats/bars", all, val, filters, bars_mat_is_allowed,
                [&](const size_t& idx) -> const string& { return bbarsblocks.bars_mats(idx); },
                bbarsblocks.bars_mats_size(), pbarsblocks.bars_mats);

            unserialize_list_other_mats(out, "other/bars", all, val, filters,
                mOtherMatsBars.mats,
                [&](const size_t& idx) -> const string& { return bbarsblocks.bars_other_mats(idx); },
                bbarsblocks.bars_other_mats_size(), pbarsblocks.bars_other_mats);

            unserialize_list_material(out, "mats/blocks", all, val, filters,
                blocks_mat_is_allowed,
                [&](const size_t& idx) -> const string& { return bbarsblocks.blocks_mats(idx); },
                bbarsblocks.blocks_mats_size(), pbarsblocks.blocks_mats);

            unserialize_list_other_mats(out, "other/blocks", all, val, filters,
                mOtherMatsBlocks.mats,
                [&](const size_t& idx) -> const string& { return bbarsblocks.blocks_other_mats(idx); },
                bbarsblocks.blocks_other_mats_size(), pbarsblocks.blocks_other_mats);
        });
}

bool StockpileSettingsSerializer::write_cloth(color_ostream& out, StockpileSettings::ClothSet* cloth) {
    bool all = true;

    all = serialize_list_organic_mat(out,
        [&](const string& token) { cloth->add_thread_silk(token); },
        &mSettings->cloth.thread_silk, organic_mat_category::Silk) && all;

    all = serialize_list_organic_mat(out,
        [&](const string& token) { cloth->add_thread_plant(token); },
        &mSettings->cloth.thread_plant, organic_mat_category::PlantFiber) && all;

    all = serialize_list_organic_mat(out,
        [&](const string& token) { cloth->add_thread_yarn(token); },
        &mSettings->cloth.thread_yarn, organic_mat_category::Yarn) && all;

    all = serialize_list_organic_mat(out,
        [&](const string& token) { cloth->add_thread_metal(token); },
        &mSettings->cloth.thread_metal, organic_mat_category::MetalThread) && all;

    all = serialize_list_organic_mat(out,
        [&](const string& token) { cloth->add_cloth_silk(token); },
        &mSettings->cloth.cloth_silk, organic_mat_category::Silk) && all;

    all = serialize_list_organic_mat(out,
        [&](const string& token) { cloth->add_cloth_plant(token); },
        &mSettings->cloth.cloth_plant, organic_mat_category::PlantFiber) && all;

    all = serialize_list_organic_mat(out,
        [&](const string& token) { cloth->add_cloth_yarn(token); },
        &mSettings->cloth.cloth_yarn, organic_mat_category::Yarn) && all;

    all = serialize_list_organic_mat(out,
        [&](const string& token) { cloth->add_cloth_metal(token); },
        &mSettings->cloth.cloth_metal, organic_mat_category::MetalThread) && all;

    return all;
}

void StockpileSettingsSerializer::read_cloth(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pcloth = mSettings->cloth;
    read_category<StockpileSettings_ClothSet>(out, "cloth", mode,
        std::bind(&StockpileSettings::has_cloth, mBuffer),
        std::bind(&StockpileSettings::cloth, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_cloth,
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

            unserialize_list_organic_mat(out, "thread/silk", all, val, filters,
                [&](size_t idx) -> string { return bcloth.thread_silk(idx); },
                bcloth.thread_silk_size(), pcloth.thread_silk, organic_mat_category::Silk);

            unserialize_list_organic_mat(out, "thread/plant", all, val, filters,
                [&](size_t idx) -> string { return bcloth.thread_plant(idx); },
                bcloth.thread_plant_size(), pcloth.thread_plant, organic_mat_category::PlantFiber);

            unserialize_list_organic_mat(out, "thread/yarn", all, val, filters,
                [&](size_t idx) -> string { return bcloth.thread_yarn(idx); },
                bcloth.thread_yarn_size(), pcloth.thread_yarn, organic_mat_category::Yarn);

            unserialize_list_organic_mat(out, "thread/metal", all, val, filters,
                [&](size_t idx) -> string { return bcloth.thread_metal(idx); },
                bcloth.thread_metal_size(), pcloth.thread_metal, organic_mat_category::MetalThread);

            unserialize_list_organic_mat(out, "cloth/silk", all, val, filters,
                [&](size_t idx) -> string { return bcloth.cloth_silk(idx); },
                bcloth.cloth_silk_size(), pcloth.cloth_silk, organic_mat_category::Silk);

            unserialize_list_organic_mat(out, "cloth/plant", all, val, filters,
                [&](size_t idx) -> string { return bcloth.cloth_plant(idx); },
                bcloth.cloth_plant_size(), pcloth.cloth_plant, organic_mat_category::PlantFiber);

            unserialize_list_organic_mat(out, "cloth/yarn", all, val, filters,
                [&](size_t idx) -> string { return bcloth.cloth_yarn(idx); },
                bcloth.cloth_yarn_size(), pcloth.cloth_yarn, organic_mat_category::Yarn);

            unserialize_list_organic_mat(out, "cloth/metal", all, val, filters,
                [&](size_t idx) -> string { return bcloth.cloth_metal(idx); },
                bcloth.cloth_metal_size(), pcloth.cloth_metal, organic_mat_category::MetalThread);
        });
}

static bool coins_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid();
}

bool StockpileSettingsSerializer::write_coins(color_ostream& out, StockpileSettings::CoinSet* coins) {
    return serialize_list_material(out,
        coins_mat_is_allowed,
        [&](const string& token) { coins->add_mats(token); },
        mSettings->coins.mats);
}

void StockpileSettingsSerializer::read_coins(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pcoins = mSettings->coins;
    read_category<StockpileSettings_CoinSet>(out, "coin", mode,
        std::bind(&StockpileSettings::has_coin, mBuffer),
        std::bind(&StockpileSettings::coin, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_coins,
        [&]() {
            pcoins.mats.clear();
        },
        [&](bool all, char val) {
            auto & bcoin = mBuffer.coin();

            unserialize_list_material(out, "", all, val, filters, coins_mat_is_allowed,
                [&](const size_t& idx) -> const string& { return bcoin.mats(idx); },
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

bool StockpileSettingsSerializer::write_finished_goods(color_ostream& out, StockpileSettings::FinishedGoodsSet* finished_goods) {
    bool all = serialize_list_item_type(out,
        finished_goods_type_is_allowed,
        [&](const string& token) { finished_goods->add_type(token); },
        mSettings->finished_goods.type);

    all = serialize_list_material(out,
        finished_goods_mat_is_allowed,
        [&](const string& token) { finished_goods->add_mats(token); },
        mSettings->finished_goods.mats) && all;

    all = serialize_list_other_mats(out,
        mOtherMatsFinishedGoods.mats, [&](const string& token) { finished_goods->add_other_mats(token); },
        mSettings->finished_goods.other_mats) && all;

    all = serialize_list_quality(out,
        [&](const string& token) { finished_goods->add_quality_core(token); },
        mSettings->finished_goods.quality_core) && all;

    all = serialize_list_quality(out,
        [&](const string& token) { finished_goods->add_quality_total(token); },
        mSettings->finished_goods.quality_total) && all;

    return all;
}

void StockpileSettingsSerializer::read_finished_goods(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pfinished_goods = mSettings->finished_goods;
    read_category<StockpileSettings_FinishedGoodsSet>(out, "finished_goods", mode,
        std::bind(&StockpileSettings::has_finished_goods, mBuffer),
        std::bind(&StockpileSettings::finished_goods, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_finished_goods,
        [&]() {
            pfinished_goods.type.clear();
            pfinished_goods.other_mats.clear();
            pfinished_goods.mats.clear();
            quality_clear(pfinished_goods.quality_core);
            quality_clear(pfinished_goods.quality_total);
        },
        [&](bool all, char val) {
            auto & bfinished_goods = mBuffer.finished_goods();

            unserialize_list_item_type(out, "type", all, val, filters, finished_goods_type_is_allowed,
                [&](const size_t& idx) -> const string& { return bfinished_goods.type(idx); },
                bfinished_goods.type_size(), pfinished_goods.type);

            unserialize_list_material(out, "mats", all, val, filters, finished_goods_mat_is_allowed,
                [&](const size_t& idx) -> const string& { return bfinished_goods.mats(idx); },
                bfinished_goods.mats_size(), pfinished_goods.mats);

            unserialize_list_other_mats(out, "other", all, val, filters, mOtherMatsFinishedGoods.mats,
                [&](const size_t& idx) -> const string& { return bfinished_goods.other_mats(idx); },
                bfinished_goods.other_mats_size(), pfinished_goods.other_mats);

            unserialize_list_quality(out, "core", all, val, filters,
                [&](const size_t& idx) -> const string& { return bfinished_goods.quality_core(idx); },
                bfinished_goods.quality_core_size(), pfinished_goods.quality_core);

            unserialize_list_quality(out, "total", all, val, filters,
                [&](const size_t& idx) -> const string& { return bfinished_goods.quality_total(idx); },
                bfinished_goods.quality_total_size(), pfinished_goods.quality_total);
        });
}

food_pair StockpileSettingsSerializer::food_map(organic_mat_category::organic_mat_category cat) {
    using df::enums::organic_mat_category::organic_mat_category;

    switch (cat) {
    case organic_mat_category::Meat:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_meat(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().meat(idx); };
        return food_pair("meat", setter, &mSettings->food.meat, getter, mBuffer.food().meat_size());
    }
    case organic_mat_category::Fish:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_fish(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().fish(idx); };
        return food_pair("fish/prepared", setter, &mSettings->food.fish, getter, mBuffer.food().fish_size());
    }
    case organic_mat_category::UnpreparedFish:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_unprepared_fish(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().unprepared_fish(idx); };
        return food_pair("fish/unprepared", setter, &mSettings->food.unprepared_fish, getter, mBuffer.food().unprepared_fish_size());
    }
    case organic_mat_category::Eggs:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_egg(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().egg(idx); };
        return food_pair("egg", setter, &mSettings->food.egg, getter, mBuffer.food().egg_size());
    }
    case organic_mat_category::Plants:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_plants(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().plants(idx); };
        return food_pair("plants", setter, &mSettings->food.plants, getter, mBuffer.food().plants_size());
    }
    case organic_mat_category::PlantDrink:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_drink_plant(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().drink_plant(idx); };
        return food_pair("drink/plant", setter, &mSettings->food.drink_plant, getter, mBuffer.food().drink_plant_size());
    }
    case organic_mat_category::CreatureDrink:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_drink_animal(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().drink_animal(idx); };
        return food_pair("drink/animal", setter, &mSettings->food.drink_animal, getter, mBuffer.food().drink_animal_size());
    }
    case organic_mat_category::PlantCheese:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_cheese_plant(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().cheese_plant(idx); };
        return food_pair("cheese/plant", setter, &mSettings->food.cheese_plant, getter, mBuffer.food().cheese_plant_size());
    }
    case organic_mat_category::CreatureCheese:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_cheese_animal(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().cheese_animal(idx); };
        return food_pair("cheese/animal", setter, &mSettings->food.cheese_animal, getter, mBuffer.food().cheese_animal_size());
    }
    case organic_mat_category::Seed:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_seeds(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().seeds(idx); };
        return food_pair("seeds", setter, &mSettings->food.seeds, getter, mBuffer.food().seeds_size());
    }
    case organic_mat_category::PlantGrowth:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_leaves(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().leaves(idx); };
        return food_pair("leaves", setter, &mSettings->food.leaves, getter, mBuffer.food().leaves_size());
    }
    case organic_mat_category::PlantPowder:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_powder_plant(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().powder_plant(idx); };
        return food_pair("powder/plant", setter, &mSettings->food.powder_plant, getter, mBuffer.food().powder_plant_size());
    }
    case organic_mat_category::CreaturePowder:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_powder_creature(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().powder_creature(idx); };
        return food_pair("powder/animal", setter, &mSettings->food.powder_creature, getter, mBuffer.food().powder_creature_size());
    }
    case organic_mat_category::Glob:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_glob(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().glob(idx); };
        return food_pair("glob", setter, &mSettings->food.glob, getter, mBuffer.food().glob_size());
    }
    case organic_mat_category::PlantLiquid:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_liquid_plant(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().liquid_plant(idx); };
        return food_pair("liquid/plant", setter, &mSettings->food.liquid_plant, getter, mBuffer.food().liquid_plant_size());
    }
    case organic_mat_category::CreatureLiquid:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_liquid_animal(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().liquid_animal(idx); };
        return food_pair("liquid/animal", setter, &mSettings->food.liquid_animal, getter, mBuffer.food().liquid_animal_size());
    }
    case organic_mat_category::MiscLiquid:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_liquid_misc(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().liquid_misc(idx); };
        return food_pair("liquid/misc", setter, &mSettings->food.liquid_misc, getter, mBuffer.food().liquid_misc_size());
    }

    case organic_mat_category::Paste:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_glob_paste(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().glob_paste(idx); };
        return food_pair("paste", setter, &mSettings->food.glob_paste, getter, mBuffer.food().glob_paste_size());
    }
    case organic_mat_category::Pressed:
    {
        FuncWriteExport setter = [&](const string& id) {
            mBuffer.mutable_food()->add_glob_pressed(id);
        };
        FuncReadImport getter = [&](size_t idx) -> string { return mBuffer.food().glob_pressed(idx); };
        return food_pair("pressed", setter, &mSettings->food.glob_pressed, getter, mBuffer.food().glob_pressed_size());
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
    case organic_mat_category::CookablePlantGrowth:
    case organic_mat_category::Yarn:
    case organic_mat_category::MetalThread:
    default:
        // not used in stockpile food menu
        break;
    }
    return food_pair();
}

bool StockpileSettingsSerializer::write_food(color_ostream& out, StockpileSettings::FoodSet* food) {
    auto & pfood = mSettings->food;
    bool all = pfood.prepared_meals;

    food->set_prepared_meals(pfood.prepared_meals);

    using df::enums::organic_mat_category::organic_mat_category;
    using traits = df::enum_traits<organic_mat_category>;
    for (int32_t mat_category = traits::first_item_value; mat_category < traits::last_item_value; ++mat_category) {
        food_pair p = food_map((organic_mat_category)mat_category);
        if (!p.valid)
            continue;
        all = serialize_list_organic_mat(out, p.set_value, p.stockpile_values,
                (organic_mat_category)mat_category) && all;
    }

    return all;
}

void StockpileSettingsSerializer::read_food(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    using df::enums::organic_mat_category::organic_mat_category;
    using traits = df::enum_traits<organic_mat_category>;

    auto & pfood = mSettings->food;
    read_category<StockpileSettings_FoodSet>(out, "food", mode,
        std::bind(&StockpileSettings::has_food, mBuffer),
        std::bind(&StockpileSettings::food, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_food,
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

            set_flag(out, "preparedmeals", filters, all, val, bfood.prepared_meals(), pfood.prepared_meals);

            for (int32_t mat_category = traits::first_item_value; mat_category < traits::last_item_value; ++mat_category) {
                food_pair p = food_map((organic_mat_category)mat_category);
                if (!p.valid)
                    continue;
                unserialize_list_organic_mat(out, p.name, all, val, filters,
                    p.get_value, p.serialized_count, *p.stockpile_values,
                    (organic_mat_category)mat_category);
            }
        });
}

static bool furniture_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && (mi.material->flags.is_set(material_flags::IS_METAL) || mi.material->flags.is_set(material_flags::IS_STONE));
}

bool StockpileSettingsSerializer::write_furniture(color_ostream& out, StockpileSettings::FurnitureSet* furniture) {
    using df::enums::furniture_type::furniture_type;
    using type_traits = df::enum_traits<furniture_type>;

    auto & pfurniture = mSettings->furniture;
    bool all = true;

    for (size_t i = 0; i < pfurniture.type.size(); ++i) {
        if (!pfurniture.type.at(i)) {
            all = false;
            continue;
        }
        string f_type(type_traits::key_table[i]);
        furniture->add_type(f_type);
        DEBUG(log, out).print("furniture_type %zd is %s\n", i, f_type.c_str());
    }

    all = serialize_list_material(out,
        furniture_mat_is_allowed,
        [&](const string& token) { furniture->add_mats(token); },
        pfurniture.mats) && all;
    all = serialize_list_other_mats(out,
        mOtherMatsFurniture.mats,
        [&](const string& token) { furniture->add_other_mats(token); },
        pfurniture.other_mats) && all;
    all = serialize_list_quality(out,
        [&](const string& token) { furniture->add_quality_core(token); },
        pfurniture.quality_core) && all;
    all = serialize_list_quality(out,
        [&](const string& token) { furniture->add_quality_total(token); },
        pfurniture.quality_total) && all;

    return all;
}

void StockpileSettingsSerializer::read_furniture(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pfurniture = mSettings->furniture;
    read_category<StockpileSettings_FurnitureSet>(out, "furniture", mode,
        std::bind(&StockpileSettings::has_furniture, mBuffer),
        std::bind(&StockpileSettings::furniture, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_furniture,
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
                    set_filter_elem(out, "type", filters, val, id, idx, pfurniture.type.at(idx));
                }
            } else {
                for (int i = 0; i < bfurniture.type_size(); ++i) {
                    const string token = bfurniture.type(i);
                    df::enum_traits<furniture_type>::base_type idx = linear_index(type_traits, token);
                    if (idx < 0 || size_t(idx) >= pfurniture.type.size()) {
                        WARN(log, out).print("furniture type index invalid %s, idx=%d\n", token.c_str(), idx);
                        continue;
                    }
                    set_filter_elem(out, "type", filters, val, token, idx, pfurniture.type.at(idx));
                }
            }

            unserialize_list_material(out, "mats", all, val, filters, furniture_mat_is_allowed,
                [&](const size_t& idx) -> const string& { return bfurniture.mats(idx); },
                bfurniture.mats_size(), pfurniture.mats);

            unserialize_list_other_mats(out, "other", all, val, filters,
                mOtherMatsFurniture.mats, [&](const size_t& idx) -> const string& { return bfurniture.other_mats(idx); },
                bfurniture.other_mats_size(), pfurniture.other_mats);

            unserialize_list_quality(out, "core", all, val, filters,
                [&](const size_t& idx) -> const string& { return bfurniture.quality_core(idx); },
                bfurniture.quality_core_size(), pfurniture.quality_core);

            unserialize_list_quality(out, "total", all, val, filters,
                [&](const size_t& idx) -> const string& { return bfurniture.quality_total(idx); },
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

bool StockpileSettingsSerializer::write_gems(color_ostream& out, StockpileSettings::GemsSet* gems) {
    MaterialInfo mi;

    auto & pgems = mSettings->gems;

    bool all = serialize_list_material(out,
        gem_mat_is_allowed,
        [&](const string& token) { gems->add_rough_mats(token); },
        pgems.rough_mats);

    all = serialize_list_material(out,
        gem_cut_mat_is_allowed,
        [&](const string& token) { gems->add_cut_mats(token); },
        pgems.cut_mats) && all;

    for (size_t i = 0; i < pgems.rough_other_mats.size(); ++i) {
        if (!pgems.rough_other_mats.at(i)) {
            all = false;
            continue;
        }
        mi.decode(i, -1);
        if (!gem_other_mat_is_allowed(mi))
            continue;
        DEBUG(log, out).print("gem rough_other mat %zd is %s\n", i, mi.getToken().c_str());
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
        DEBUG(log, out).print("gem cut_other mat %zd is %s\n", i, mi.getToken().c_str());
        gems->add_cut_other_mats(mi.getToken());
    }

    return all;
}

void StockpileSettingsSerializer::read_gems(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pgems = mSettings->gems;
    read_category<StockpileSettings_GemsSet>(out, "gems", mode,
        std::bind(&StockpileSettings::has_gems, mBuffer),
        std::bind(&StockpileSettings::gems, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_gems,
        [&]() {
            pgems.cut_other_mats.clear();
            pgems.cut_mats.clear();
            pgems.rough_other_mats.clear();
            pgems.rough_mats.clear();
        },
        [&](bool all, char val) {
            auto & bgems = mBuffer.gems();

            unserialize_list_material(out, "mats/rough", all, val, filters, gem_mat_is_allowed,
                [&](const size_t& idx) -> const string& { return bgems.rough_mats(idx); },
                bgems.rough_mats_size(), pgems.rough_mats);

            unserialize_list_material(out, "mats/cut", all, val, filters, gem_cut_mat_is_allowed,
                [&](const size_t& idx) -> const string& { return bgems.cut_mats(idx); },
                bgems.cut_mats_size(), pgems.cut_mats);

            const size_t builtin_size = std::extent<decltype(world->raws.mat_table.builtin)>::value;
            pgems.rough_other_mats.resize(builtin_size, '\0');
            pgems.cut_other_mats.resize(builtin_size, '\0');
            if (all) {
                for (size_t idx = 0; idx < builtin_size; ++idx) {
                    MaterialInfo mi;
                    mi.decode(idx, -1);
                    if (gem_other_mat_is_allowed(mi))
                        set_filter_elem(out, "other/rough", filters, val, mi.getToken(), idx, pgems.rough_other_mats.at(idx));
                    if (!mi.isValid())
                        mi.decode(0, idx);
                    if (gem_other_mat_is_allowed(mi))
                        set_filter_elem(out, "other/cut", filters, val, mi.getToken(), idx, pgems.cut_other_mats.at(idx));
                }
                return;
            } else {
                MaterialInfo mi;
                for (int i = 0; i < (int)builtin_size; ++i) {
                    if (i < bgems.rough_other_mats_size()) {
                        string id = bgems.rough_other_mats(i);
                        if (mi.find(id) && mi.isValid() && size_t(mi.type) < builtin_size)
                            set_filter_elem(out, "other/rough", filters, val, id, mi.type, pgems.rough_other_mats.at(mi.type));
                    }
                    if (i < bgems.cut_other_mats_size()) {
                        string id = bgems.cut_other_mats(i);
                        if (mi.find(id) && mi.isValid() && size_t(mi.type) < builtin_size)
                            set_filter_elem(out, "other/cut", filters, val, id, mi.type, pgems.cut_other_mats.at(mi.type));
                    }
                }
            }
        });
}

bool StockpileSettingsSerializer::write_leather(color_ostream& out, StockpileSettings::LeatherSet* leather) {
    return serialize_list_organic_mat(out,
        [&](const string& id) { leather->add_mats(id); },
        &mSettings->leather.mats, organic_mat_category::Leather);
}

void StockpileSettingsSerializer::read_leather(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pleather = mSettings->leather;
    read_category<StockpileSettings_LeatherSet>(out, "leather", mode,
        std::bind(&StockpileSettings::has_leather, mBuffer),
        std::bind(&StockpileSettings::leather, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_leather,
        [&]() {
            pleather.mats.clear();
        },
        [&](bool all, char val) {
            auto & bleather = mBuffer.leather();

            unserialize_list_organic_mat(out, "", all, val, filters,
                [&](size_t idx) -> string { return bleather.mats(idx); },
                bleather.mats_size(), pleather.mats, organic_mat_category::Leather);
        });
}

bool StockpileSettingsSerializer::write_corpses(color_ostream& out, StockpileSettings::CorpsesSet* corpses) {
    return serialize_list_creature(out,
        [&](const string& token) { corpses->add_corpses(token); },
        mSettings->corpses.corpses);
}

void StockpileSettingsSerializer::read_corpses(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pcorpses = mSettings->corpses;
    read_category<StockpileSettings_CorpsesSet>(out, "corpses", mode,
        std::bind(&StockpileSettings::has_corpses_v50, mBuffer),
        std::bind(&StockpileSettings::corpses_v50, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_corpses,
        [&]() {
            pcorpses.corpses.clear();
        },
        [&](bool all, char val) {
            auto & bcorpses = mBuffer.corpses_v50();
            unserialize_list_creature(out, "", all, val, filters,
                [&](const size_t& idx) -> const string& { return bcorpses.corpses(idx); },
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

bool StockpileSettingsSerializer::write_refuse(color_ostream& out, StockpileSettings::RefuseSet* refuse) {
    auto & prefuse = mSettings->refuse;
    bool all = prefuse.fresh_raw_hide && prefuse.rotten_raw_hide;

    refuse->set_fresh_raw_hide(prefuse.fresh_raw_hide);
    refuse->set_rotten_raw_hide(prefuse.rotten_raw_hide);

    all = serialize_list_item_type(out, refuse_type_is_allowed,
        [&](const string& token) { refuse->add_type(token); },
        prefuse.type) && all;

    all = serialize_list_creature(out,
        [&](const string& token) { refuse->add_corpses(token); },
        prefuse.corpses) && all;
    all = serialize_list_creature(out,
        [&](const string& token) { refuse->add_body_parts(token); },
        prefuse.body_parts) && all;
    all = serialize_list_creature(out,
        [&](const string& token) { refuse->add_skulls(token); },
        prefuse.skulls) && all;
    all = serialize_list_creature(out,
        [&](const string& token) { refuse->add_bones(token); },
        prefuse.bones) && all;
    all = serialize_list_creature(out,
        [&](const string& token) { refuse->add_hair(token); },
        prefuse.hair) && all;
    all = serialize_list_creature(out,
        [&](const string& token) { refuse->add_shells(token); },
        prefuse.shells) && all;
    all = serialize_list_creature(out,
        [&](const string& token) { refuse->add_teeth(token); },
        prefuse.teeth) && all;
    all = serialize_list_creature(out,
        [&](const string& token) { refuse->add_horns(token); },
        prefuse.horns) && all;

    return all;
}

void StockpileSettingsSerializer::read_refuse(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & prefuse = mSettings->refuse;
    read_category<StockpileSettings_RefuseSet>(out, "refuse", mode,
        std::bind(&StockpileSettings::has_refuse, mBuffer),
        std::bind(&StockpileSettings::refuse, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_refuse,
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

            set_flag(out, "rawhide/fresh", filters, all, val, brefuse.fresh_raw_hide(), prefuse.fresh_raw_hide);
            set_flag(out, "rawhide/rotten", filters, all, val, brefuse.rotten_raw_hide(), prefuse.rotten_raw_hide);

            unserialize_list_item_type(out, "type", all, val, filters, refuse_type_is_allowed,
                [&](const size_t& idx) -> const string& { return brefuse.type(idx); },
                brefuse.type_size(), prefuse.type);

            unserialize_list_creature(out, "corpses", all, val, filters,
                [&](const size_t& idx) -> const string& { return brefuse.corpses(idx); },
                brefuse.corpses_size(), prefuse.corpses);
            unserialize_list_creature(out, "bodyparts", all, val, filters,
                [&](const size_t& idx) -> const string& { return brefuse.body_parts(idx); },
                brefuse.body_parts_size(), prefuse.body_parts);
            unserialize_list_creature(out, "skulls", all, val, filters,
                [&](const size_t& idx) -> const string& { return brefuse.skulls(idx); },
                brefuse.skulls_size(), prefuse.skulls);
            unserialize_list_creature(out, "bones", all, val, filters,
                [&](const size_t& idx) -> const string& { return brefuse.bones(idx); },
                brefuse.bones_size(), prefuse.bones);
            unserialize_list_creature(out, "hair", all, val, filters,
                [&](const size_t& idx) -> const string& { return brefuse.hair(idx); },
                brefuse.hair_size(), prefuse.hair);
            unserialize_list_creature(out, "shells", all, val, filters,
                [&](const size_t& idx) -> const string& { return brefuse.shells(idx); },
                brefuse.shells_size(), prefuse.shells);
            unserialize_list_creature(out, "teeth", all, val, filters,
                [&](const size_t& idx) -> const string& { return brefuse.teeth(idx); },
                brefuse.teeth_size(), prefuse.teeth);
            unserialize_list_creature(out, "horns", all, val, filters,
                [&](const size_t& idx) -> const string& { return brefuse.horns(idx); },
                brefuse.horns_size(), prefuse.horns);
        });

}

bool StockpileSettingsSerializer::write_sheet(color_ostream& out, StockpileSettings::SheetSet* sheet) {
    bool all = serialize_list_organic_mat(out,
        [&](const string& token) { sheet->add_paper(token); },
        &mSettings->sheet.paper, organic_mat_category::Paper);

    all = serialize_list_organic_mat(out,
        [&](const string& token) { sheet->add_parchment(token); },
        &mSettings->sheet.parchment, organic_mat_category::Parchment) && all;

    return all;
}

void StockpileSettingsSerializer::read_sheet(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & psheet = mSettings->sheet;
    read_category<StockpileSettings_SheetSet>(out, "sheet", mode,
        std::bind(&StockpileSettings::has_sheet, mBuffer),
        std::bind(&StockpileSettings::sheet, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_sheet,
        [&]() {
            psheet.paper.clear();
            psheet.parchment.clear();
        },
        [&](bool all, char val) {
            auto & bsheet = mBuffer.sheet();

            unserialize_list_organic_mat(out, "paper", all, val, filters,
                [&](size_t idx) -> string { return bsheet.paper(idx); },
                bsheet.paper_size(), psheet.paper, organic_mat_category::Paper);

            unserialize_list_organic_mat(out, "parchment", all, val, filters,
                [&](size_t idx) -> string { return bsheet.parchment(idx); },
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

bool StockpileSettingsSerializer::write_stone(color_ostream& out, StockpileSettings::StoneSet* stone) {
    return serialize_list_material(out,
        stone_is_allowed,
        [&](const string& token) { stone->add_mats(token); },
        mSettings->stone.mats);
}

void StockpileSettingsSerializer::read_stone(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pstone = mSettings->stone;
    read_category<StockpileSettings_StoneSet>(out, "stone", mode,
        std::bind(&StockpileSettings::has_stone, mBuffer),
        std::bind(&StockpileSettings::stone, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_stone,
        [&]() {
            pstone.mats.clear();
        },
        [&](bool all, char val) {
            auto & bstone = mBuffer.stone();

            unserialize_list_material(out, "", all, val, filters, stone_is_allowed,
                [&](const size_t& idx) -> const string& { return bstone.mats(idx); },
                bstone.mats_size(), pstone.mats);
        });
}

static bool weapons_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid() && mi.material && (mi.material->flags.is_set(material_flags::IS_METAL) || mi.material->flags.is_set(material_flags::IS_STONE));
}

bool StockpileSettingsSerializer::write_weapons(color_ostream& out, StockpileSettings::WeaponsSet* weapons) {
    auto & pweapons = mSettings->weapons;
    bool all = pweapons.unusable && pweapons.usable;

    weapons->set_unusable(pweapons.unusable);
    weapons->set_usable(pweapons.usable);

    all = serialize_list_itemdef(out,
        [&](const string& token) { weapons->add_weapon_type(token); },
        pweapons.weapon_type,
        vector<df::itemdef*>(world->raws.itemdefs.weapons.begin(), world->raws.itemdefs.weapons.end()),
        item_type::WEAPON) && all;

    all = serialize_list_itemdef(out,
        [&](const string& token) { weapons->add_trapcomp_type(token); },
        pweapons.trapcomp_type,
        vector<df::itemdef*>(world->raws.itemdefs.trapcomps.begin(), world->raws.itemdefs.trapcomps.end()),
        item_type::TRAPCOMP) && all;

    all = serialize_list_material(out,
        weapons_mat_is_allowed,
        [&](const string& token) { weapons->add_mats(token); },
        pweapons.mats) && all;

    all = serialize_list_other_mats(out,
        mOtherMatsWeaponsArmor.mats,
        [&](const string& token) { weapons->add_other_mats(token); },
        pweapons.other_mats) && all;

    all = serialize_list_quality(out,
        [&](const string& token) { weapons->add_quality_core(token); },
        pweapons.quality_core) && all;

    all = serialize_list_quality(out,
        [&](const string& token) { weapons->add_quality_total(token); },
        pweapons.quality_total) && all;

    return all;
}

void StockpileSettingsSerializer::read_weapons(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pweapons = mSettings->weapons;
    read_category<StockpileSettings_WeaponsSet>(out, "weapons", mode,
        std::bind(&StockpileSettings::has_weapons, mBuffer),
        std::bind(&StockpileSettings::weapons, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_weapons,
        [&]() {
            pweapons.unusable = false;
            pweapons.usable = false;
            pweapons.weapon_type.clear();
            pweapons.trapcomp_type.clear();
            pweapons.other_mats.clear();
            pweapons.mats.clear();
            quality_clear(pweapons.quality_core);
            quality_clear(pweapons.quality_total);
        },
        [&](bool all, char val) {
            auto & bweapons = mBuffer.weapons();

            set_flag(out, "nouse", filters, all, val, bweapons.unusable(), pweapons.unusable);
            set_flag(out, "canuse", filters, all, val, bweapons.usable(), pweapons.usable);

            unserialize_list_itemdef(out, "type/weapon", all, val, filters,
                [&](const size_t& idx) -> const string& { return bweapons.weapon_type(idx); },
                bweapons.weapon_type_size(), pweapons.weapon_type, item_type::WEAPON);

            unserialize_list_itemdef(out, "type/trapcomp", all, val, filters,
                [&](const size_t& idx) -> const string& { return bweapons.trapcomp_type(idx); },
                bweapons.trapcomp_type_size(), pweapons.trapcomp_type, item_type::TRAPCOMP);

            unserialize_list_material(out, "mats", all, val, filters, weapons_mat_is_allowed,
                [&](const size_t& idx) -> const string& { return bweapons.mats(idx); },
                bweapons.mats_size(), pweapons.mats);

            unserialize_list_other_mats(out, "other", all, val, filters, mOtherMatsWeaponsArmor.mats,
                [&](const size_t& idx) -> const string& { return bweapons.other_mats(idx); },
                bweapons.other_mats_size(), pweapons.other_mats);

            unserialize_list_quality(out, "core", all, val, filters,
                [&](const size_t& idx) -> const string& { return bweapons.quality_core(idx); },
                bweapons.quality_core_size(), pweapons.quality_core);

            unserialize_list_quality(out, "total", all, val, filters,
                [&](const size_t& idx) -> const string& { return bweapons.quality_total(idx); },
                bweapons.quality_total_size(), pweapons.quality_total);
        });
}

static bool wood_mat_is_allowed(const df::plant_raw* plant) {
    return plant && plant->flags.is_set(plant_raw_flags::TREE);
}

bool StockpileSettingsSerializer::write_wood(color_ostream& out, StockpileSettings::WoodSet* wood) {
    bool all = true;
    for (size_t i = 0; i < mSettings->wood.mats.size(); ++i) {
        if (!mSettings->wood.mats.at(i)) {
            all = false;
            continue;
        }
        const df::plant_raw* plant = find_plant(i);
        if (!wood_mat_is_allowed(plant))
            continue;
        wood->add_mats(plant->id);
        DEBUG(log, out).print("plant %zd is %s\n", i, plant->id.c_str());
    }
    return all;
}

void StockpileSettingsSerializer::read_wood(color_ostream& out, DeserializeMode mode, const vector<string>& filters) {
    auto & pwood = mSettings->wood;
    read_category<StockpileSettings_WoodSet>(out, "wood", mode,
        std::bind(&StockpileSettings::has_wood, mBuffer),
        std::bind(&StockpileSettings::wood, mBuffer),
        mSettings->flags.whole,
        mSettings->flags.mask_wood,
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
                    set_filter_elem(out, "", filters, val, id, idx, pwood.mats.at(idx));
                }
            } else {
                for (int i = 0; i < bwood.mats_size(); ++i) {
                    const string token = bwood.mats(i);
                    const size_t idx = find_plant(token);
                    if (idx < 0 || (size_t)idx >= num_elems) {
                        WARN(log, out).print("wood mat index invalid %s idx=%zd\n", token.c_str(), idx);
                        continue;
                    }
                    set_filter_elem(out, "", filters, val, token, idx, pwood.mats.at(idx));
                }
            }
        });
}
