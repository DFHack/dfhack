#pragma once

#include "modules/Materials.h"

#include "df/itemdef.h"
#include "df/organic_mat_category.h"

#include "proto/stockpiles.pb.h"

#include <functional>

namespace df
{
    struct building_stockpilest;
}

enum IncludedElements {
    INCLUDED_ELEMENTS_NONE = 0x00,
    INCLUDED_ELEMENTS_CONTAINERS = 0x01,
    INCLUDED_ELEMENTS_GENERAL = 0x02,
    INCLUDED_ELEMENTS_CATEGORIES = 0x04,
    INCLUDED_ELEMENTS_TYPES = 0x08,
};

enum DeserializeMode {
    DESERIALIZE_MODE_SET = 0,
    DESERIALIZE_MODE_ENABLE = 1,
    DESERIALIZE_MODE_DISABLE = 2,
};

//  read the token from the serialized list during import
typedef std::function<std::string(const size_t&)> FuncReadImport;
//  add the token to the serialized list during export
typedef std::function<void(const std::string&)> FuncWriteExport;
//  are item's of item_type allowed?
typedef std::function<bool(df::enums::item_type::item_type)> FuncItemAllowed;
//  is this material allowed?
typedef std::function<bool(const DFHack::MaterialInfo&)> FuncMaterialAllowed;

// convenient struct for parsing food stockpile items
struct food_pair {
    // exporting
    FuncWriteExport set_value;
    std::vector<char>* stockpile_values;
    // importing
    FuncReadImport get_value;
    size_t serialized_count;
    bool valid;

    food_pair(FuncWriteExport s, std::vector<char>* sp_v, FuncReadImport g, size_t count)
        : set_value(s), stockpile_values(sp_v), get_value(g), serialized_count(count), valid(true) { }
    food_pair(): valid(false) { }
};

/**
 * Class for serializing the stockpile_settings structure into a Google protobuf
 */
class StockpileSerializer {
public:
    /**
     * @param stockpile stockpile to read or write settings to
     */
    StockpileSerializer(df::building_stockpilest* stockpile);

    ~StockpileSerializer();

    /**
     * Since we depend on protobuf-lite, not the full lib, we copy this function from
     * protobuf message.cc
     */
    bool serialize_to_ostream(std::ostream* output, uint32_t includedElements);

    /**
     * Will serialize stockpile settings to a file (overwrites existing files)
     * @return success/failure
     */
    bool serialize_to_file(const std::string& file, uint32_t includedElements);

    /**
     * Again, copied from message.cc
     */
    bool parse_from_istream(std::istream* input, DeserializeMode mode, const std::string& filter);

    /**
     * Read stockpile settings from file
     */
    bool unserialize_from_file(const std::string& file, DeserializeMode mode, const std::string& filter);

private:
    df::building_stockpilest* mPile;
    dfstockpiles::StockpileSettings mBuffer;

    // read memory structures and serialize to protobuf
    void write(uint32_t includedElements);

    // parse serialized data into ui indices
    void read(DeserializeMode mode, const std::string& filter);

    void write_containers();
    void read_containers(DeserializeMode mode);
    void write_general();
    void read_general(DeserializeMode mode);

    bool write_ammo(dfstockpiles::StockpileSettings::AmmoSet* ammo);
    void read_ammo(DeserializeMode mode, const std::string& filter);
    bool write_animals(dfstockpiles::StockpileSettings::AnimalsSet* animals);
    void read_animals(DeserializeMode mode, const std::string& filter);
    bool write_armor(dfstockpiles::StockpileSettings::ArmorSet* armor);
    void read_armor(DeserializeMode mode, const std::string& filter);
    bool write_bars_blocks(dfstockpiles::StockpileSettings::BarsBlocksSet* bars_blocks);
    void read_bars_blocks(DeserializeMode mode, const std::string& filter);
    bool write_cloth(dfstockpiles::StockpileSettings::ClothSet* cloth);
    void read_cloth(DeserializeMode mode, const std::string& filter);
    bool write_coins(dfstockpiles::StockpileSettings::CoinSet* coins);
    void read_coins(DeserializeMode mode, const std::string& filter);
    bool write_finished_goods(dfstockpiles::StockpileSettings::FinishedGoodsSet* finished_goods);
    void read_finished_goods(DeserializeMode mode, const std::string& filter);
    food_pair food_map(df::enums::organic_mat_category::organic_mat_category cat);
    bool write_food(dfstockpiles::StockpileSettings::FoodSet* food);
    void read_food(DeserializeMode mode, const std::string& filter);
    bool write_furniture(dfstockpiles::StockpileSettings::FurnitureSet* furniture);
    void read_furniture(DeserializeMode mode, const std::string& filter);
    bool write_gems(dfstockpiles::StockpileSettings::GemsSet* gems);
    void read_gems(DeserializeMode mode, const std::string& filter);
    bool write_leather(dfstockpiles::StockpileSettings::LeatherSet* leather);
    void read_leather(DeserializeMode mode, const std::string& filter);
    bool write_corpses(dfstockpiles::StockpileSettings::CorpsesSet* corpses);
    void read_corpses(DeserializeMode mode, const std::string& filter);
    bool write_refuse(dfstockpiles::StockpileSettings::RefuseSet* refuse);
    void read_refuse(DeserializeMode mode, const std::string& filter);
    bool write_sheet(dfstockpiles::StockpileSettings::SheetSet* sheet);
    void read_sheet(DeserializeMode mode, const std::string& filter);
    bool write_stone(dfstockpiles::StockpileSettings::StoneSet* stone);
    void read_stone(DeserializeMode mode, const std::string& filter);
    bool write_weapons(dfstockpiles::StockpileSettings::WeaponsSet* weapons);
    void read_weapons(DeserializeMode mode, const std::string& filter);
    bool write_wood(dfstockpiles::StockpileSettings::WoodSet* wood);
    void read_wood(DeserializeMode mode, const std::string& filter);
};
