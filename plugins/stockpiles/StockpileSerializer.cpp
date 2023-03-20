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
        add_value(ii.getToken());
        DEBUG(log).print("itemdef type %zd is %s\n", i, ii.getToken().c_str());
    }
    return all;
}

static void unserialize_list_itemdef(FuncReadImport read_value,
        int32_t list_size,
        std::vector<char>* pile_list,
        item_type::item_type type) {
    pile_list->clear();
    pile_list->resize(Items::getSubtypeCount(type), '\0');
    for (int i = 0; i < list_size; ++i) {
        std::string token = read_value(i);
        ItemTypeInfo ii;
        if (!ii.find(token))
            continue;
        DEBUG(log).print("itemdef %d is %s\n", ii.subtype, token.c_str());
        if (size_t(ii.subtype) >= pile_list->size()) {
            WARN(log).print("itemdef index too large! idx[%d] max_size[%zd]\n", ii.subtype, pile_list->size());
            continue;
        }
        pile_list->at(ii.subtype) = 1;
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
        DEBUG(log).print("quality: %zd is %s\n", i, f_type.c_str());
    }
    return all;
}

static void quality_clear(bool(&pile_list)[7]) {
    std::fill(pile_list, pile_list + 7, false);
}

static void unserialize_list_quality(FuncReadImport read_value,
        int32_t list_size,
        bool(&pile_list)[7]) {
    quality_clear(pile_list);
    if (list_size > 0 && list_size <= 7) {
        using df::enums::item_quality::item_quality;
        df::enum_traits<item_quality> quality_traits;
        for (int i = 0; i < list_size; ++i) {
            const std::string quality = read_value(i);
            df::enum_traits<item_quality>::base_type idx = linear_index(quality_traits, quality);
            if (idx < 0) {
                WARN(log).print("invalid quality token: %s\n", quality.c_str());
                continue;
            }
            DEBUG(log).print("quality: %d is %s\n", idx, quality.c_str());
            pile_list[idx] = true;
        }
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

static void unserialize_list_other_mats(
            const std::map<int, std::string> other_mats,
            FuncReadImport read_value,
            int32_t list_size,
            std::vector<char>* pile_list) {
    pile_list->clear();
    pile_list->resize(other_mats.size(), '\0');
    for (int i = 0; i < list_size; ++i) {
        const std::string token = read_value(i);
        size_t idx = other_mats_token(other_mats, token);
        if (idx < 0) {
            WARN(log).print("invalid other mat with token %s\n", token.c_str());
            continue;
        }
        DEBUG(log).print("other_mats %zd is %s\n", idx, token.c_str());
        if (idx >= pile_list->size()) {
            WARN(log).print("other_mats index too large! idx[%zd] max_size[%zd]\n", idx, pile_list->size());
            continue;
        }
        pile_list->at(idx) = 1;
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

static void unserialize_list_organic_mat(FuncReadImport get_value,
        size_t list_size, std::vector<char>* pile_list,
        organic_mat_category::organic_mat_category cat) {
    pile_list->clear();
    pile_list->resize(OrganicMatLookup::food_max_size(cat), '\0');
    for (size_t i = 0; i < list_size; ++i) {
        std::string token = get_value(i);
        int16_t idx = OrganicMatLookup::food_idx_by_token(cat, token);
        DEBUG(log).print("   organic_material %d is %s\n", idx, token.c_str());
        if (size_t(idx) >= pile_list->size()) {
            WARN(log).print("organic mat index too large! idx[%d] max_size[%zd]\n", idx, pile_list->size());
            continue;
        }
        pile_list->at(idx) = 1;
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

static void unserialize_list_item_type(FuncItemAllowed is_allowed,
        FuncReadImport read_value, int32_t list_size,
        std::vector<char>* pile_list) {
    pile_list->clear();
    pile_list->resize(112, '\0'); // TODO remove hardcoded list size value
    for (size_t i = 0; i < pile_list->size(); ++i) {
        pile_list->at(i) = is_allowed((item_type::item_type)i) ? 0 : 1;
    }
    using df::enums::item_type::item_type;
    df::enum_traits<item_type> type_traits;
    for (int32_t i = 0; i < list_size; ++i) {
        const std::string token = read_value(i);
        // subtract one because item_type starts at -1
        const df::enum_traits<item_type>::base_type idx = linear_index(type_traits, token) - 1;
        const item_type type = (item_type)idx;
        if (!is_allowed(type))
            continue;
        DEBUG(log).print("item_type %d is %s\n", idx, token.c_str());
        if (size_t(idx) >= pile_list->size()) {
            WARN(log).print("error item_type index too large! idx[%d] max_size[%zd]\n", idx, pile_list->size());
            continue;
        }
        pile_list->at(idx) = 1;
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
        DEBUG(log).print("material %zd is %s\n", i, mi.getToken().c_str());
        add_value(mi.getToken());
    }
    return all;
}

static void unserialize_list_material(FuncMaterialAllowed is_allowed,
        FuncReadImport read_value, int32_t list_size,
        std::vector<char>* pile_list) {
    //  we initialize all possible (allowed) values to 0,
    //  then all other not-allowed values to 1
    //  why? because that's how the memory is in DF before
    //  we muck with it.
    std::set<int32_t> idx_set;
    pile_list->clear();
    pile_list->resize(world->raws.inorganics.size(), 0);
    for (size_t i = 0; i < pile_list->size(); ++i) {
        MaterialInfo mi(0, i);
        pile_list->at(i) = is_allowed(mi) ? 0 : 1;
    }
    for (int i = 0; i < list_size; ++i) {
        const std::string token = read_value(i);
        MaterialInfo mi;
        mi.find(token);
        if (!is_allowed(mi))
            continue;
        DEBUG(log).print("material %d is %s\n", mi.index, token.c_str());
        if (size_t(mi.index) >= pile_list->size()) {
            WARN(log).print("material index too large! idx[%d] max_size[%zd]\n", mi.index, pile_list->size());
            continue;
        }
        pile_list->at(mi.index) = 1;
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
        DEBUG(log).print("setting %s=%d\n", name, val);
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
    bool has_cat = has_cat_fn();
    bool all = has_cat && cat_fn().has_all() && cat_fn().all();
    bool just_disable = all && mode == DESERIALIZE_MODE_DISABLE;

    if (mode == DESERIALIZE_MODE_SET || just_disable) {
        DEBUG(log).print("clearing %s\n", name);
        cat_flags &= ~cat_mask;
        clear_fn();
    }

    if (!has_cat || just_disable)
        return;

    if (mode == DESERIALIZE_MODE_DISABLE && !(cat_flags & cat_mask))
        return;

    if (mode == DESERIALIZE_MODE_SET || mode == DESERIALIZE_MODE_ENABLE)
        cat_flags |= cat_mask;

    char val = (mode == DESERIALIZE_MODE_DISABLE) ? (char)0 : (char)1;
    DEBUG(log).print("setting %s %s elements to %d\n",
            all ? "all" : "marked", name, val);
    set_fn(all, val);
}

static void set_elem(bool all, char val, bool enabled, bool& elem) {
    if (all || enabled)
        elem = val;
}

static bool matches_filter(const std::string& filter, const std::string& name) {
    if (!filter.size())
        return true;
    return std::search(name.begin(), name.end(), filter.begin(), filter.end(),
        [](unsigned char ch1, unsigned char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    ) != name.end();
}

static void set_filter_elem(const std::string& filter, char val, df::creature_raw* r, char& elem) {
    if (matches_filter(filter, r->name[0])) {
        DEBUG(log).print("setting %s (%s) to %d\n", r->name[0].c_str(), r->creature_id.c_str(), val);
        elem = val;
    }
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
        [=](const std::string& token) { ammo->add_type(token); },
        mPile->settings.ammo.type,
        std::vector<df::itemdef*>(world->raws.itemdefs.ammo.begin(), world->raws.itemdefs.ammo.end()),
        item_type::AMMO);

    all = serialize_list_material(
        ammo_mat_is_allowed,
        [=](const std::string& token) { ammo->add_mats(token); },
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
        [=](const std::string& token) { ammo->add_quality_core(token); },
        mPile->settings.ammo.quality_core) && all;

    all = serialize_list_quality(
        [=](const std::string& token) { ammo->add_quality_total(token); },
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
            [&](bool force, char val) {
                auto & bammo = mBuffer.ammo();

                unserialize_list_itemdef(
                    [&](const size_t& idx) -> const std::string& { return bammo.type(idx); },
                    bammo.type_size(), &pammo.type, item_type::AMMO);

                unserialize_list_material(ammo_mat_is_allowed,
                    [&](const size_t& idx) -> const std::string& { return bammo.mats(idx); },
                    bammo.mats_size(), &pammo.mats);

                pammo.other_mats.clear();
                pammo.other_mats.resize(2, '\0');
                if (bammo.other_mats_size() > 0) {
                    // TODO remove hardcoded value
                    for (int i = 0; i < bammo.other_mats_size(); ++i) {
                        const std::string token = bammo.other_mats(i);
                        const int32_t idx = token == "WOOD" ? 0 : token == "BONE" ? 1
                            : -1;
                        DEBUG(log).print("other mats %d is %s\n", idx, token.c_str());
                        if (idx == -1)
                            continue;
                        pammo.other_mats.at(idx) = 1;
                    }
                }

                unserialize_list_quality([=](const size_t& idx) -> const std::string& { return bammo.quality_core(idx); },
                    bammo.quality_core_size(), pammo.quality_core);

                unserialize_list_quality([=](const size_t& idx) -> const std::string& { return bammo.quality_total(idx); },
                    bammo.quality_total_size(), pammo.quality_total);
            });
}

bool StockpileSerializer::write_animals(StockpileSettings::AnimalsSet* animals) {
    auto & panimals = mPile->settings.animals;
    bool all = panimals.empty_cages && panimals.empty_traps;

    animals->set_empty_cages(panimals.empty_cages);
    animals->set_empty_traps(panimals.empty_traps);
    for (size_t i = 0; i < panimals.enabled.size(); ++i) {
        if (!panimals.enabled.at(i)) {
            all = false;
            continue;
        }
        df::creature_raw* r = find_creature(i);
        if (r->flags.is_set(creature_raw_flags::GENERATED)
                || r->creature_id == "EQUIPMENT_WAGON")
            continue;
        DEBUG(log).print("saving creature %s\n", r->creature_id.c_str());
        animals->add_enabled(r->creature_id);
    }
    return all;
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
                set_elem(all, val, banimals.empty_cages(), panimals.empty_cages);
                set_elem(all, val, banimals.empty_traps(), panimals.empty_traps);

                size_t num_animals = world->raws.creatures.all.size();
                panimals.enabled.resize(num_animals, '\0');
                if (all) {
                    for (auto idx = 0; idx < num_animals; ++idx)
                        set_filter_elem(filter, val, find_creature(idx), panimals.enabled.at(idx));
                } else {
                    for (auto i = 0; i < banimals.enabled_size(); ++i) {
                        const std::string& id = banimals.enabled(i);
                        int idx = find_creature(id);
                        if (idx < 0 || size_t(idx) >= num_animals) {
                            WARN(log).print("animal index invalid: %d\n", idx);
                            continue;
                        }
                        set_filter_elem(filter, val, find_creature(idx), panimals.enabled.at(idx));
                    }
                }
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
        [=](const std::string& token) { armor->add_body(token); },
        parmor.body,
        std::vector<df::itemdef*>(world->raws.itemdefs.armor.begin(), world->raws.itemdefs.armor.end()),
        item_type::ARMOR) && all;

    // helm type
    all = serialize_list_itemdef(
        [=](const std::string& token) { armor->add_head(token); },
        parmor.head,
        std::vector<df::itemdef*>(world->raws.itemdefs.helms.begin(), world->raws.itemdefs.helms.end()),
        item_type::HELM) && all;

    // shoes type
    all = serialize_list_itemdef(
        [=](const std::string& token) { armor->add_feet(token); },
        parmor.feet,
        std::vector<df::itemdef*>(world->raws.itemdefs.shoes.begin(), world->raws.itemdefs.shoes.end()),
        item_type::SHOES) && all;

    // gloves type
    all = serialize_list_itemdef(
        [=](const std::string& token) { armor->add_hands(token); },
        parmor.hands,
        std::vector<df::itemdef*>(world->raws.itemdefs.gloves.begin(), world->raws.itemdefs.gloves.end()),
        item_type::GLOVES) && all;

    // pant type
    all = serialize_list_itemdef(
        [=](const std::string& token) { armor->add_legs(token); },
        parmor.legs,
        std::vector<df::itemdef*>(world->raws.itemdefs.pants.begin(), world->raws.itemdefs.pants.end()),
        item_type::PANTS) && all;

    // shield type
    all = serialize_list_itemdef(
        [=](const std::string& token) { armor->add_shield(token); },
        parmor.shield,
        std::vector<df::itemdef*>(world->raws.itemdefs.shields.begin(), world->raws.itemdefs.shields.end()),
        item_type::SHIELD) && all;

    // materials
    all = serialize_list_material(
        armor_mat_is_allowed,
        [=](const std::string& token) { armor->add_mats(token); },
        parmor.mats) && all;

    // other mats
    all = serialize_list_other_mats(
        mOtherMatsWeaponsArmor.mats, [=](const std::string& token) { armor->add_other_mats(token); },
        parmor.other_mats) && all;

    // quality core
    all = serialize_list_quality([=](const std::string& token) { armor->add_quality_core(token); },
        parmor.quality_core) && all;

    // quality total
    all = serialize_list_quality([=](const std::string& token) { armor->add_quality_total(token); },
        parmor.quality_total) && all;

    return all;
}

void StockpileSerializer::read_armor(DeserializeMode mode, const std::string& filter) {
    if (mBuffer.has_armor()) {
        mPile->settings.flags.bits.armor = 1;
        const StockpileSettings::ArmorSet armor = mBuffer.armor();
        DEBUG(log).print("armor:\n");

        bool unusable = armor.unusable();
        bool usable = armor.usable();
        DEBUG(log).print("unusable %d\n", unusable);
        DEBUG(log).print("usable %d\n", usable);
        mPile->settings.armor.unusable = unusable;
        mPile->settings.armor.usable = usable;

        unserialize_list_itemdef([=](const size_t& idx) -> const std::string& { return armor.body(idx); },
            armor.body_size(), &mPile->settings.armor.body, item_type::ARMOR);

        unserialize_list_itemdef([=](const size_t& idx) -> const std::string& { return armor.head(idx); },
            armor.head_size(), &mPile->settings.armor.head, item_type::HELM);

        unserialize_list_itemdef([=](const size_t& idx) -> const std::string& { return armor.feet(idx); },
            armor.feet_size(), &mPile->settings.armor.feet, item_type::SHOES);

        unserialize_list_itemdef([=](const size_t& idx) -> const std::string& { return armor.hands(idx); },
            armor.hands_size(), &mPile->settings.armor.hands, item_type::GLOVES);

        unserialize_list_itemdef([=](const size_t& idx) -> const std::string& { return armor.legs(idx); },
            armor.legs_size(), &mPile->settings.armor.legs, item_type::PANTS);

        unserialize_list_itemdef([=](const size_t& idx) -> const std::string& { return armor.shield(idx); },
            armor.shield_size(), &mPile->settings.armor.shield, item_type::SHIELD);

        unserialize_list_material(
            armor_mat_is_allowed,
            [=](const size_t& idx) -> const std::string& { return armor.mats(idx); },
            armor.mats_size(), &mPile->settings.armor.mats);

        unserialize_list_other_mats(
            mOtherMatsWeaponsArmor.mats, [=](const size_t& idx) -> const std::string& { return armor.other_mats(idx); },
            armor.other_mats_size(), &mPile->settings.armor.other_mats);

        unserialize_list_quality([=](const size_t& idx) -> const std::string& { return armor.quality_core(idx); },
            armor.quality_core_size(), mPile->settings.armor.quality_core);

        unserialize_list_quality([=](const size_t& idx) -> const std::string& { return armor.quality_total(idx); },
            armor.quality_total_size(), mPile->settings.armor.quality_total);
    }
    else {
        mPile->settings.flags.bits.armor = 0;
        mPile->settings.armor.body.clear();
        mPile->settings.armor.head.clear();
        mPile->settings.armor.feet.clear();
        mPile->settings.armor.hands.clear();
        mPile->settings.armor.legs.clear();
        mPile->settings.armor.shield.clear();
        mPile->settings.armor.other_mats.clear();
        mPile->settings.armor.mats.clear();
        quality_clear(mPile->settings.armor.quality_core);
        quality_clear(mPile->settings.armor.quality_total);
    }
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
        [=](const std::string& token) { bars_blocks->add_bars_mats(token); },
        mPile->settings.bars_blocks.bars_mats);

    all = serialize_list_material(
        blocks_mat_is_allowed,
        [=](const std::string& token) { bars_blocks->add_blocks_mats(token); },
        mPile->settings.bars_blocks.blocks_mats) && all;

    all = serialize_list_other_mats(
        mOtherMatsBars.mats, [=](const std::string& token) { bars_blocks->add_bars_other_mats(token); },
        mPile->settings.bars_blocks.bars_other_mats) && all;

    all = serialize_list_other_mats(
        mOtherMatsBlocks.mats, [=](const std::string& token) { bars_blocks->add_blocks_other_mats(token); },
        mPile->settings.bars_blocks.blocks_other_mats) && all;

    return all;
}

void StockpileSerializer::read_bars_blocks(DeserializeMode mode, const std::string& filter) {
    if (mBuffer.has_barsblocks()) {
        mPile->settings.flags.bits.bars_blocks = 1;
        const StockpileSettings::BarsBlocksSet bars_blocks = mBuffer.barsblocks();
        DEBUG(log).print("bars_blocks:\n");

        unserialize_list_material(
            bars_mat_is_allowed,
            [=](const size_t& idx) -> const std::string& { return bars_blocks.bars_mats(idx); },
            bars_blocks.bars_mats_size(), &mPile->settings.bars_blocks.bars_mats);

        unserialize_list_material(
            blocks_mat_is_allowed,
            [=](const size_t& idx) -> const std::string& { return bars_blocks.blocks_mats(idx); },
            bars_blocks.blocks_mats_size(), &mPile->settings.bars_blocks.blocks_mats);

        unserialize_list_other_mats(
            mOtherMatsBars.mats,
            [=](const size_t& idx) -> const std::string& { return bars_blocks.bars_other_mats(idx); },
            bars_blocks.bars_other_mats_size(), &mPile->settings.bars_blocks.bars_other_mats);

        unserialize_list_other_mats(
            mOtherMatsBlocks.mats,
            [=](const size_t& idx) -> const std::string& { return bars_blocks.blocks_other_mats(idx); },
            bars_blocks.blocks_other_mats_size(), &mPile->settings.bars_blocks.blocks_other_mats);
    }
    else {
        mPile->settings.flags.bits.bars_blocks = 0;
        mPile->settings.bars_blocks.bars_other_mats.clear();
        mPile->settings.bars_blocks.bars_mats.clear();
        mPile->settings.bars_blocks.blocks_other_mats.clear();
        mPile->settings.bars_blocks.blocks_mats.clear();
    }
}

bool StockpileSerializer::write_cloth(StockpileSettings::ClothSet* cloth) {
    bool all = true;

    all = serialize_list_organic_mat(
        [=](const std::string& token) { cloth->add_thread_silk(token); },
        &mPile->settings.cloth.thread_silk, organic_mat_category::Silk) && all;

    all = serialize_list_organic_mat(
        [=](const std::string& token) { cloth->add_thread_plant(token); },
        &mPile->settings.cloth.thread_plant, organic_mat_category::PlantFiber) && all;

    all = serialize_list_organic_mat(
        [=](const std::string& token) { cloth->add_thread_yarn(token); },
        &mPile->settings.cloth.thread_yarn, organic_mat_category::Yarn) && all;

    all = serialize_list_organic_mat(
        [=](const std::string& token) { cloth->add_thread_metal(token); },
        &mPile->settings.cloth.thread_metal, organic_mat_category::MetalThread) && all;

    all = serialize_list_organic_mat(
        [=](const std::string& token) { cloth->add_cloth_silk(token); },
        &mPile->settings.cloth.cloth_silk, organic_mat_category::Silk) && all;

    all = serialize_list_organic_mat(
        [=](const std::string& token) { cloth->add_cloth_plant(token); },
        &mPile->settings.cloth.cloth_plant, organic_mat_category::PlantFiber) && all;

    all = serialize_list_organic_mat(
        [=](const std::string& token) { cloth->add_cloth_yarn(token); },
        &mPile->settings.cloth.cloth_yarn, organic_mat_category::Yarn) && all;

    all = serialize_list_organic_mat(
        [=](const std::string& token) { cloth->add_cloth_metal(token); },
        &mPile->settings.cloth.cloth_metal, organic_mat_category::MetalThread) && all;

    return all;
}

void StockpileSerializer::read_cloth(DeserializeMode mode, const std::string& filter) {
    if (mBuffer.has_cloth()) {
        mPile->settings.flags.bits.cloth = 1;
        const StockpileSettings::ClothSet cloth = mBuffer.cloth();
        DEBUG(log).print("cloth:\n");

        unserialize_list_organic_mat([=](size_t idx) -> std::string { return cloth.thread_silk(idx); },
            cloth.thread_silk_size(), &mPile->settings.cloth.thread_silk, organic_mat_category::Silk);

        unserialize_list_organic_mat([=](size_t idx) -> std::string { return cloth.thread_plant(idx); },
            cloth.thread_plant_size(), &mPile->settings.cloth.thread_plant, organic_mat_category::PlantFiber);

        unserialize_list_organic_mat([=](size_t idx) -> std::string { return cloth.thread_yarn(idx); },
            cloth.thread_yarn_size(), &mPile->settings.cloth.thread_yarn, organic_mat_category::Yarn);

        unserialize_list_organic_mat([=](size_t idx) -> std::string { return cloth.thread_metal(idx); },
            cloth.thread_metal_size(), &mPile->settings.cloth.thread_metal, organic_mat_category::MetalThread);

        unserialize_list_organic_mat([=](size_t idx) -> std::string { return cloth.cloth_silk(idx); },
            cloth.cloth_silk_size(), &mPile->settings.cloth.cloth_silk, organic_mat_category::Silk);

        unserialize_list_organic_mat([=](size_t idx) -> std::string { return cloth.cloth_plant(idx); },
            cloth.cloth_plant_size(), &mPile->settings.cloth.cloth_plant, organic_mat_category::PlantFiber);

        unserialize_list_organic_mat([=](size_t idx) -> std::string { return cloth.cloth_yarn(idx); },
            cloth.cloth_yarn_size(), &mPile->settings.cloth.cloth_yarn, organic_mat_category::Yarn);

        unserialize_list_organic_mat([=](size_t idx) -> std::string { return cloth.cloth_metal(idx); },
            cloth.cloth_metal_size(), &mPile->settings.cloth.cloth_metal, organic_mat_category::MetalThread);
    }
    else {
        mPile->settings.cloth.thread_metal.clear();
        mPile->settings.cloth.thread_plant.clear();
        mPile->settings.cloth.thread_silk.clear();
        mPile->settings.cloth.thread_yarn.clear();
        mPile->settings.cloth.cloth_metal.clear();
        mPile->settings.cloth.cloth_plant.clear();
        mPile->settings.cloth.cloth_silk.clear();
        mPile->settings.cloth.cloth_yarn.clear();
        mPile->settings.flags.bits.cloth = 0;
    }
}

static bool coins_mat_is_allowed(const MaterialInfo& mi) {
    return mi.isValid();
}

bool StockpileSerializer::write_coins(StockpileSettings::CoinSet* coins) {
    return serialize_list_material(
        coins_mat_is_allowed,
        [=](const std::string& token) { coins->add_mats(token); },
        mPile->settings.coins.mats);
}

void StockpileSerializer::read_coins(DeserializeMode mode, const std::string& filter) {
    if (mBuffer.has_coin()) {
        mPile->settings.flags.bits.coins = 1;
        const StockpileSettings::CoinSet coins = mBuffer.coin();
        DEBUG(log).print("coins:\n");

        unserialize_list_material(
            coins_mat_is_allowed,
            [=](const size_t& idx) -> const std::string& { return coins.mats(idx); },
            coins.mats_size(), &mPile->settings.coins.mats);
    }
    else {
        mPile->settings.flags.bits.coins = 0;
        mPile->settings.coins.mats.clear();
    }
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
        [=](const std::string& token) { finished_goods->add_type(token); },
        mPile->settings.finished_goods.type);

    all = serialize_list_material(
        finished_goods_mat_is_allowed,
        [=](const std::string& token) { finished_goods->add_mats(token); },
        mPile->settings.finished_goods.mats) && all;

    all = serialize_list_other_mats(
        mOtherMatsFinishedGoods.mats, [=](const std::string& token) { finished_goods->add_other_mats(token); },
        mPile->settings.finished_goods.other_mats) && all;

    all = serialize_list_quality([=](const std::string& token) { finished_goods->add_quality_core(token); },
        mPile->settings.finished_goods.quality_core) && all;

    all = serialize_list_quality([=](const std::string& token) { finished_goods->add_quality_total(token); },
        mPile->settings.finished_goods.quality_total) && all;

    return all;
}

void StockpileSerializer::read_finished_goods(DeserializeMode mode, const std::string& filter) {
    if (mBuffer.has_finished_goods()) {
        mPile->settings.flags.bits.finished_goods = 1;
        const StockpileSettings::FinishedGoodsSet finished_goods = mBuffer.finished_goods();
        DEBUG(log).print("finished_goods:\n");

        unserialize_list_item_type(
            finished_goods_type_is_allowed,
            [=](const size_t& idx) -> const std::string& { return finished_goods.type(idx); },
            finished_goods.type_size(), &mPile->settings.finished_goods.type);

        unserialize_list_material(
            finished_goods_mat_is_allowed,
            [=](const size_t& idx) -> const std::string& { return finished_goods.mats(idx); },
            finished_goods.mats_size(), &mPile->settings.finished_goods.mats);

        unserialize_list_other_mats(
            mOtherMatsFinishedGoods.mats, [=](const size_t& idx) -> const std::string& { return finished_goods.other_mats(idx); },
            finished_goods.other_mats_size(), &mPile->settings.finished_goods.other_mats);

        unserialize_list_quality([=](const size_t& idx) -> const std::string& { return finished_goods.quality_core(idx); },
            finished_goods.quality_core_size(), mPile->settings.finished_goods.quality_core);

        unserialize_list_quality([=](const size_t& idx) -> const std::string& { return finished_goods.quality_total(idx); },
            finished_goods.quality_total_size(), mPile->settings.finished_goods.quality_total);
    }
    else {
        mPile->settings.flags.bits.finished_goods = 0;
        mPile->settings.finished_goods.type.clear();
        mPile->settings.finished_goods.other_mats.clear();
        mPile->settings.finished_goods.mats.clear();
        quality_clear(mPile->settings.finished_goods.quality_core);
        quality_clear(mPile->settings.finished_goods.quality_total);
    }
}

food_pair StockpileSerializer::food_map(organic_mat_category::organic_mat_category cat) {
    using df::enums::organic_mat_category::organic_mat_category;

    switch (cat) {
    case organic_mat_category::Meat:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_meat(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().meat(idx); };
        return food_pair(setter, &mPile->settings.food.meat, getter, mBuffer.food().meat_size());
    }
    case organic_mat_category::Fish:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_fish(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().fish(idx); };
        return food_pair(setter, &mPile->settings.food.fish, getter, mBuffer.food().fish_size());
    }
    case organic_mat_category::UnpreparedFish:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_unprepared_fish(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().unprepared_fish(idx); };
        return food_pair(setter, &mPile->settings.food.unprepared_fish, getter, mBuffer.food().unprepared_fish_size());
    }
    case organic_mat_category::Eggs:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_egg(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().egg(idx); };
        return food_pair(setter, &mPile->settings.food.egg, getter, mBuffer.food().egg_size());
    }
    case organic_mat_category::Plants:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_plants(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().plants(idx); };
        return food_pair(setter, &mPile->settings.food.plants, getter, mBuffer.food().plants_size());
    }
    case organic_mat_category::PlantDrink:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_drink_plant(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().drink_plant(idx); };
        return food_pair(setter, &mPile->settings.food.drink_plant, getter, mBuffer.food().drink_plant_size());
    }
    case organic_mat_category::CreatureDrink:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_drink_animal(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().drink_animal(idx); };
        return food_pair(setter, &mPile->settings.food.drink_animal, getter, mBuffer.food().drink_animal_size());
    }
    case organic_mat_category::PlantCheese:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_cheese_plant(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().cheese_plant(idx); };
        return food_pair(setter, &mPile->settings.food.cheese_plant, getter, mBuffer.food().cheese_plant_size());
    }
    case organic_mat_category::CreatureCheese:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_cheese_animal(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().cheese_animal(idx); };
        return food_pair(setter, &mPile->settings.food.cheese_animal, getter, mBuffer.food().cheese_animal_size());
    }
    case organic_mat_category::Seed:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_seeds(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().seeds(idx); };
        return food_pair(setter, &mPile->settings.food.seeds, getter, mBuffer.food().seeds_size());
    }
    case organic_mat_category::Leaf:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_leaves(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().leaves(idx); };
        return food_pair(setter, &mPile->settings.food.leaves, getter, mBuffer.food().leaves_size());
    }
    case organic_mat_category::PlantPowder:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_powder_plant(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().powder_plant(idx); };
        return food_pair(setter, &mPile->settings.food.powder_plant, getter, mBuffer.food().powder_plant_size());
    }
    case organic_mat_category::CreaturePowder:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_powder_creature(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().powder_creature(idx); };
        return food_pair(setter, &mPile->settings.food.powder_creature, getter, mBuffer.food().powder_creature_size());
    }
    case organic_mat_category::Glob:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_glob(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().glob(idx); };
        return food_pair(setter, &mPile->settings.food.glob, getter, mBuffer.food().glob_size());
    }
    case organic_mat_category::PlantLiquid:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_liquid_plant(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().liquid_plant(idx); };
        return food_pair(setter, &mPile->settings.food.liquid_plant, getter, mBuffer.food().liquid_plant_size());
    }
    case organic_mat_category::CreatureLiquid:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_liquid_animal(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().liquid_animal(idx); };
        return food_pair(setter, &mPile->settings.food.liquid_animal, getter, mBuffer.food().liquid_animal_size());
    }
    case organic_mat_category::MiscLiquid:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_liquid_misc(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().liquid_misc(idx); };
        return food_pair(setter, &mPile->settings.food.liquid_misc, getter, mBuffer.food().liquid_misc_size());
    }

    case organic_mat_category::Paste:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_glob_paste(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().glob_paste(idx); };
        return food_pair(setter, &mPile->settings.food.glob_paste, getter, mBuffer.food().glob_paste_size());
    }
    case organic_mat_category::Pressed:
    {
        FuncWriteExport setter = [=](const std::string& id) {
            mBuffer.mutable_food()->add_glob_pressed(id);
        };
        FuncReadImport getter = [=](size_t idx) -> std::string { return mBuffer.food().glob_pressed(idx); };
        return food_pair(setter, &mPile->settings.food.glob_pressed, getter, mBuffer.food().glob_pressed_size());
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
    DEBUG(log).print("food:\n");

    auto & pfood = mPile->settings.food;
    bool all = pfood.prepared_meals;

    food->set_prepared_meals(pfood.prepared_meals);

    using df::enums::organic_mat_category::organic_mat_category;
    using traits = df::enum_traits<organic_mat_category>;
    for (int32_t mat_category = traits::first_item_value; mat_category < traits::last_item_value; ++mat_category) {
        food_pair p = food_map((organic_mat_category)mat_category);
        if (!p.valid)
            continue;
        DEBUG(log).print("food: %s\n", traits::key_table[mat_category]);
        all = serialize_list_organic_mat(p.set_value, p.stockpile_values, (organic_mat_category)mat_category) && all;
    }

    return all;
}

void StockpileSerializer::read_food(DeserializeMode mode, const std::string& filter) {
    using df::enums::organic_mat_category::organic_mat_category;
    using traits = df::enum_traits<organic_mat_category>;
    if (mBuffer.has_food()) {
        mPile->settings.flags.bits.food = 1;
        const StockpileSettings::FoodSet food = mBuffer.food();
        DEBUG(log).print("food:\n");

        if (food.has_prepared_meals())
            mPile->settings.food.prepared_meals = food.prepared_meals();
        else
            mPile->settings.food.prepared_meals = true;

        DEBUG(log).print("prepared_meals: %d\n", mPile->settings.food.prepared_meals);

        for (int32_t mat_category = traits::first_item_value; mat_category < traits::last_item_value; ++mat_category) {
            food_pair p = food_map((organic_mat_category)mat_category);
            if (!p.valid)
                continue;
            unserialize_list_organic_mat(p.get_value, p.serialized_count, p.stockpile_values, (organic_mat_category)mat_category);
        }
    }
    else {
        for (int32_t mat_category = traits::first_item_value; mat_category < traits::last_item_value; ++mat_category) {
            food_pair p = food_map((organic_mat_category)mat_category);
            if (!p.valid)
                continue;
            p.stockpile_values->clear();
        }
        mPile->settings.flags.bits.food = 0;
        mPile->settings.food.prepared_meals = false;
    }
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
        [=](const std::string& token) { furniture->add_mats(token); },
        pfurniture.mats) && all;
    all = serialize_list_other_mats(
        mOtherMatsFurniture.mats,
        [=](const std::string& token) { furniture->add_other_mats(token); },
        pfurniture.other_mats) && all;
    all = serialize_list_quality(
        [=](const std::string& token) { furniture->add_quality_core(token); },
        pfurniture.quality_core) && all;
    all = serialize_list_quality(
        [=](const std::string& token) { furniture->add_quality_total(token); },
        pfurniture.quality_total) && all;

    return all;
}

void StockpileSerializer::read_furniture(DeserializeMode mode, const std::string& filter) {
    if (mBuffer.has_furniture()) {
        mPile->settings.flags.bits.furniture = 1;
        const StockpileSettings::FurnitureSet furniture = mBuffer.furniture();
        DEBUG(log).print("furniture:\n");

        // type
        using df::enums::furniture_type::furniture_type;
        df::enum_traits<furniture_type> type_traits;
        mPile->settings.furniture.type.clear();
        mPile->settings.furniture.type.resize(type_traits.last_item_value + 1, '\0');
        if (furniture.type_size() > 0) {
            for (int i = 0; i < furniture.type_size(); ++i) {
                const std::string type = furniture.type(i);
                df::enum_traits<furniture_type>::base_type idx = linear_index(type_traits, type);
                DEBUG(log).print("type %d is %s\n", idx, type.c_str());
                if (idx < 0 || size_t(idx) >= mPile->settings.furniture.type.size()) {
                    WARN(log).print("furniture type index invalid %s, idx=%d\n", type.c_str(), idx);
                    continue;
                }
                mPile->settings.furniture.type.at(idx) = 1;
            }
        }

        unserialize_list_material(
            furniture_mat_is_allowed,
            [=](const size_t& idx) -> const std::string& { return furniture.mats(idx); },
            furniture.mats_size(), &mPile->settings.furniture.mats);

        unserialize_list_other_mats(
            mOtherMatsFurniture.mats, [=](const size_t& idx) -> const std::string& { return furniture.other_mats(idx); },
            furniture.other_mats_size(), &mPile->settings.furniture.other_mats);

        unserialize_list_quality([=](const size_t& idx) -> const std::string& { return furniture.quality_core(idx); },
            furniture.quality_core_size(), mPile->settings.furniture.quality_core);

        unserialize_list_quality([=](const size_t& idx) -> const std::string& { return furniture.quality_total(idx); },
            furniture.quality_total_size(), mPile->settings.furniture.quality_total);
    }
    else {
        mPile->settings.flags.bits.furniture = 0;
        mPile->settings.furniture.type.clear();
        mPile->settings.furniture.other_mats.clear();
        mPile->settings.furniture.mats.clear();
        quality_clear(mPile->settings.furniture.quality_core);
        quality_clear(mPile->settings.furniture.quality_total);
    }
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
        [=](const std::string& token) { gems->add_rough_mats(token); },
        pgems.rough_mats);

    all = serialize_list_material(
        gem_cut_mat_is_allowed,
        [=](const std::string& token) { gems->add_cut_mats(token); },
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
    if (mBuffer.has_gems()) {
        mPile->settings.flags.bits.gems = 1;
        const StockpileSettings::GemsSet gems = mBuffer.gems();
        DEBUG(log).print("gems:\n");

        unserialize_list_material(
            gem_mat_is_allowed,
            [=](const size_t& idx) -> const std::string& { return gems.rough_mats(idx); },
            gems.rough_mats_size(), &mPile->settings.gems.rough_mats);

        unserialize_list_material(
            gem_cut_mat_is_allowed,
            [=](const size_t& idx) -> const std::string& { return gems.cut_mats(idx); },
            gems.cut_mats_size(), &mPile->settings.gems.cut_mats);

        const size_t builtin_size = std::extent<decltype(world->raws.mat_table.builtin)>::value;
        // rough other
        mPile->settings.gems.rough_other_mats.clear();
        mPile->settings.gems.rough_other_mats.resize(builtin_size, '\0');
        for (int i = 0; i < gems.rough_other_mats_size(); ++i) {
            const std::string token = gems.rough_other_mats(i);
            MaterialInfo mi;
            mi.find(token);
            if (!mi.isValid() || size_t(mi.type) >= builtin_size) {
                WARN(log).print("invalid gem mat %s idx=%d\n", token.c_str(), mi.type);
                continue;
            }
            DEBUG(log).print("rough_other mats %d is %s\n", mi.type, token.c_str());
            mPile->settings.gems.rough_other_mats.at(mi.type) = 1;
        }

        // cut other
        mPile->settings.gems.cut_other_mats.clear();
        mPile->settings.gems.cut_other_mats.resize(builtin_size, '\0');
        for (int i = 0; i < gems.cut_other_mats_size(); ++i) {
            const std::string token = gems.cut_other_mats(i);
            MaterialInfo mi;
            mi.find(token);
            if (!mi.isValid() || size_t(mi.type) >= builtin_size) {
                WARN(log).print("invalid gem mat %s idx=%d\n", token.c_str(), mi.type);
                continue;
            }
            DEBUG(log).print("cut_other mats %d is %s\n", mi.type, token.c_str());
            mPile->settings.gems.cut_other_mats.at(mi.type) = 1;
        }
    }
    else {
        mPile->settings.flags.bits.gems = 0;
        mPile->settings.gems.cut_other_mats.clear();
        mPile->settings.gems.cut_mats.clear();
        mPile->settings.gems.rough_other_mats.clear();
        mPile->settings.gems.rough_mats.clear();
    }
}

bool StockpileSerializer::write_leather(StockpileSettings::LeatherSet* leather) {
    return serialize_list_organic_mat(
        [=](const std::string& id) { leather->add_mats(id); },
        &mPile->settings.leather.mats, organic_mat_category::Leather);
}

void StockpileSerializer::read_leather(DeserializeMode mode, const std::string& filter) {
    if (mBuffer.has_leather()) {
        mPile->settings.flags.bits.leather = 1;
        const StockpileSettings::LeatherSet leather = mBuffer.leather();
        DEBUG(log).print("leather:\n");

        unserialize_list_organic_mat([=](size_t idx) -> std::string { return leather.mats(idx); },
            leather.mats_size(), &mPile->settings.leather.mats, organic_mat_category::Leather);
    }
    else {
        mPile->settings.flags.bits.leather = 0;
        mPile->settings.leather.mats.clear();
    }
}

bool StockpileSerializer::write_corpses(StockpileSettings::CorpsesSet* corpses) {
    bool all = true;

    return all;
}

void StockpileSerializer::read_corpses(DeserializeMode mode, const std::string& filter) {

}

static bool refuse_creature_is_allowed(const df::creature_raw* raw) {
    if (!raw)
        return false;
    // wagon and generated creatures not allowed,  except angels
    const bool is_wagon = raw->creature_id == "EQUIPMENT_WAGON";
    const bool is_generated = raw->flags.is_set(creature_raw_flags::GENERATED);
    const bool is_angel = is_generated && raw->creature_id.find("DIVINE_") != std::string::npos;
    return !is_wagon && !(is_generated && !is_angel);
}

static bool refuse_write_helper(std::function<void(const string&)> add_value, const vector<char>& list) {
    bool all = true;
    for (size_t i = 0; i < list.size(); ++i) {
        if (!list.at(i)) {
            all = false;
            continue;
        }
        df::creature_raw* r = find_creature(i);
        // skip forgotten beasts, titans, demons, and night creatures
        if (!refuse_creature_is_allowed(r))
            continue;
        DEBUG(log).print("creature %s %zd\n", r->creature_id.c_str(), i);
        add_value(r->creature_id);
    }
    return all;
}

static bool refuse_type_is_allowed(item_type::item_type type) {
    if (type == item_type::NONE || type == item_type::BAR || type == item_type::SMALLGEM || type == item_type::BLOCKS || type == item_type::ROUGH || type == item_type::BOULDER || type == item_type::CORPSE || type == item_type::CORPSEPIECE || type == item_type::ROCK || type == item_type::ORTHOPEDIC_CAST)
        return false;
    return true;
}

bool StockpileSerializer::write_refuse(StockpileSettings::RefuseSet* refuse) {
    auto & prefuse = mPile->settings.refuse;
    bool all = prefuse.fresh_raw_hide && prefuse.rotten_raw_hide;

    DEBUG(log).print("refuse:\n");
    refuse->set_fresh_raw_hide(prefuse.fresh_raw_hide);
    refuse->set_rotten_raw_hide(prefuse.rotten_raw_hide);

    DEBUG(log).print("getting types\n");
    all = serialize_list_item_type(
        refuse_type_is_allowed,
        [=](const std::string& token) {
            DEBUG(log).print("adding type: %s\n", token.c_str());
            refuse->add_type(token);
        },
        prefuse.type) && all;

    all = refuse_write_helper([=](const std::string& id) { refuse->add_corpses(id); },
        prefuse.corpses) && all;
    all = refuse_write_helper([=](const std::string& id) { refuse->add_body_parts(id); },
        prefuse.body_parts) && all;
    all = refuse_write_helper([=](const std::string& id) { refuse->add_skulls(id); },
        prefuse.skulls) && all;
    all = refuse_write_helper([=](const std::string& id) { refuse->add_bones(id); },
        prefuse.bones) && all;
    all = refuse_write_helper([=](const std::string& id) { refuse->add_hair(id); },
        prefuse.hair) && all;
    all = refuse_write_helper([=](const std::string& id) { refuse->add_shells(id); },
        prefuse.shells) && all;
    all = refuse_write_helper([=](const std::string& id) { refuse->add_teeth(id); },
        prefuse.teeth) && all;
    all = refuse_write_helper([=](const std::string& id) { refuse->add_horns(id); },
        prefuse.horns) && all;

    return all;
}

static void refuse_read_helper(std::function<std::string(const size_t&)> get_value, size_t list_size, std::vector<char>* pile_list) {
    pile_list->clear();
    pile_list->resize(world->raws.creatures.all.size(), '\0');
    if (list_size > 0) {
        for (size_t i = 0; i < list_size; ++i) {
            const std::string creature_id = get_value(i);
            const int idx = find_creature(creature_id);
            const df::creature_raw* creature = find_creature(idx);
            if (idx < 0 || !refuse_creature_is_allowed(creature) || size_t(idx) >= pile_list->size()) {
                WARN(log).print("invalid refuse creature %s, idx=%d\n", creature_id.c_str(), idx);
                continue;
            }
            DEBUG(log).print("creature %d is %s\n", idx, creature_id.c_str());
            pile_list->at(idx) = 1;
        }
    }
}

void StockpileSerializer::read_refuse(DeserializeMode mode, const std::string& filter) {
    if (mBuffer.has_refuse()) {
        mPile->settings.flags.bits.refuse = 1;
        const StockpileSettings::RefuseSet refuse = mBuffer.refuse();
        DEBUG(log).print("refuse:\n");
        DEBUG(log).print("  fresh hide %d\n", refuse.fresh_raw_hide());
        DEBUG(log).print("  rotten hide %d\n", refuse.rotten_raw_hide());
        mPile->settings.refuse.fresh_raw_hide = refuse.fresh_raw_hide();
        mPile->settings.refuse.rotten_raw_hide = refuse.rotten_raw_hide();

        unserialize_list_item_type(
            refuse_type_is_allowed,
            [=](const size_t& idx) -> const std::string& { return refuse.type(idx); },
            refuse.type_size(), &mPile->settings.refuse.type);

        DEBUG(log).print("  corpses\n");
        refuse_read_helper([=](const size_t& idx) -> const std::string& { return refuse.corpses(idx); },
            refuse.corpses_size(), &mPile->settings.refuse.corpses);

        DEBUG(log).print("  body_parts\n");
        refuse_read_helper([=](const size_t& idx) -> const std::string& { return refuse.body_parts(idx); },
            refuse.body_parts_size(), &mPile->settings.refuse.body_parts);

        DEBUG(log).print("  skulls\n");
        refuse_read_helper([=](const size_t& idx) -> const std::string& { return refuse.skulls(idx); },
            refuse.skulls_size(), &mPile->settings.refuse.skulls);

        DEBUG(log).print("  bones\n");
        refuse_read_helper([=](const size_t& idx) -> const std::string& { return refuse.bones(idx); },
            refuse.bones_size(), &mPile->settings.refuse.bones);

        DEBUG(log).print("  hair\n");
        refuse_read_helper([=](const size_t& idx) -> const std::string& { return refuse.hair(idx); },
            refuse.hair_size(), &mPile->settings.refuse.hair);

        DEBUG(log).print("  shells\n");
        refuse_read_helper([=](const size_t& idx) -> const std::string& { return refuse.shells(idx); },
            refuse.shells_size(), &mPile->settings.refuse.shells);

        DEBUG(log).print("  teeth\n");
        refuse_read_helper([=](const size_t& idx) -> const std::string& { return refuse.teeth(idx); },
            refuse.teeth_size(), &mPile->settings.refuse.teeth);

        DEBUG(log).print("  horns\n");
        refuse_read_helper([=](const size_t& idx) -> const std::string& { return refuse.horns(idx); },
            refuse.horns_size(), &mPile->settings.refuse.horns);
    }
    else {
        mPile->settings.flags.bits.refuse = 0;
        mPile->settings.refuse.type.clear();
        mPile->settings.refuse.corpses.clear();
        mPile->settings.refuse.body_parts.clear();
        mPile->settings.refuse.skulls.clear();
        mPile->settings.refuse.bones.clear();
        mPile->settings.refuse.hair.clear();
        mPile->settings.refuse.shells.clear();
        mPile->settings.refuse.teeth.clear();
        mPile->settings.refuse.horns.clear();
        mPile->settings.refuse.fresh_raw_hide = false;
        mPile->settings.refuse.rotten_raw_hide = false;
    }
}

bool StockpileSerializer::write_sheet(StockpileSettings::SheetSet* sheet) {
    bool all = true;

    return all;
}

void StockpileSerializer::read_sheet(DeserializeMode mode, const std::string& filter) {

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
        [=](const std::string& token) { stone->add_mats(token); },
        mPile->settings.stone.mats);
}

void StockpileSerializer::read_stone(DeserializeMode mode, const std::string& filter) {
    if (mBuffer.has_stone()) {
        mPile->settings.flags.bits.stone = 1;
        const StockpileSettings::StoneSet stone = mBuffer.stone();
        DEBUG(log).print("stone:\n");

        unserialize_list_material(
            stone_is_allowed,
            [=](const size_t& idx) -> const std::string& { return stone.mats(idx); },
            stone.mats_size(), &mPile->settings.stone.mats);
    }
    else {
        mPile->settings.flags.bits.stone = 0;
        mPile->settings.stone.mats.clear();
    }
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
        [=](const std::string& token) { weapons->add_weapon_type(token); },
        pweapons.weapon_type,
        std::vector<df::itemdef*>(world->raws.itemdefs.weapons.begin(), world->raws.itemdefs.weapons.end()),
        item_type::WEAPON) && all;

    all = serialize_list_itemdef(
        [=](const std::string& token) { weapons->add_trapcomp_type(token); },
        pweapons.trapcomp_type,
        std::vector<df::itemdef*>(world->raws.itemdefs.trapcomps.begin(), world->raws.itemdefs.trapcomps.end()),
        item_type::TRAPCOMP) && all;

    all = serialize_list_material(
        weapons_mat_is_allowed,
        [=](const std::string& token) { weapons->add_mats(token); },
        pweapons.mats) && all;

    all = serialize_list_other_mats(
        mOtherMatsWeaponsArmor.mats,
        [=](const std::string& token) { weapons->add_other_mats(token); },
        pweapons.other_mats) && all;

    all = serialize_list_quality(
        [=](const std::string& token) { weapons->add_quality_core(token); },
        pweapons.quality_core) && all;

    all = serialize_list_quality(
        [=](const std::string& token) { weapons->add_quality_total(token); },
        pweapons.quality_total) && all;

    return all;
}

void StockpileSerializer::read_weapons(DeserializeMode mode, const std::string& filter) {
    if (mBuffer.has_weapons()) {
        mPile->settings.flags.bits.weapons = 1;
        const StockpileSettings::WeaponsSet weapons = mBuffer.weapons();
        DEBUG(log).print("weapons: \n");

        bool unusable = weapons.unusable();
        bool usable = weapons.usable();
        DEBUG(log).print("unusable %d\n", unusable);
        DEBUG(log).print("usable %d\n", usable);
        mPile->settings.weapons.unusable = unusable;
        mPile->settings.weapons.usable = usable;

        unserialize_list_itemdef([=](const size_t& idx) -> const std::string& { return weapons.weapon_type(idx); },
            weapons.weapon_type_size(), &mPile->settings.weapons.weapon_type, item_type::WEAPON);

        unserialize_list_itemdef([=](const size_t& idx) -> const std::string& { return weapons.trapcomp_type(idx); },
            weapons.trapcomp_type_size(), &mPile->settings.weapons.trapcomp_type, item_type::TRAPCOMP);

        unserialize_list_material(
            weapons_mat_is_allowed,
            [=](const size_t& idx) -> const std::string& { return weapons.mats(idx); },
            weapons.mats_size(), &mPile->settings.weapons.mats);

        unserialize_list_other_mats(
            mOtherMatsWeaponsArmor.mats, [=](const size_t& idx) -> const std::string& { return weapons.other_mats(idx); },
            weapons.other_mats_size(), &mPile->settings.weapons.other_mats);

        unserialize_list_quality([=](const size_t& idx) -> const std::string& { return weapons.quality_core(idx); },
            weapons.quality_core_size(), mPile->settings.weapons.quality_core);

        unserialize_list_quality([=](const size_t& idx) -> const std::string& { return weapons.quality_total(idx); },
            weapons.quality_total_size(), mPile->settings.weapons.quality_total);
    }
    else {
        mPile->settings.flags.bits.weapons = 0;
        mPile->settings.weapons.weapon_type.clear();
        mPile->settings.weapons.trapcomp_type.clear();
        mPile->settings.weapons.other_mats.clear();
        mPile->settings.weapons.mats.clear();
        quality_clear(mPile->settings.weapons.quality_core);
        quality_clear(mPile->settings.weapons.quality_total);
    }
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
    if (mBuffer.has_wood()) {
        mPile->settings.flags.bits.wood = 1;
        const StockpileSettings::WoodSet wood = mBuffer.wood();
        DEBUG(log).print("wood: \n");

        mPile->settings.wood.mats.clear();
        mPile->settings.wood.mats.resize(world->raws.plants.all.size(), '\0');
        for (int i = 0; i < wood.mats_size(); ++i) {
            const std::string token = wood.mats(i);
            const size_t idx = find_plant(token);
            if (idx < 0 || idx >= mPile->settings.wood.mats.size()) {
                WARN(log).print("wood mat index invalid %s idx=%zd\n", token.c_str(), idx);
                continue;
            }
            DEBUG(log).print("plant %zd is %s\n", idx, token.c_str());
            mPile->settings.wood.mats.at(idx) = 1;
        }
    }
    else {
        mPile->settings.flags.bits.wood = 0;
        mPile->settings.wood.mats.clear();
    }
}
