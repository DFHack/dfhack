#pragma once

#include "modules/Materials.h"

#include "df/itemdef.h"
#include "df/organic_mat_category.h"
#include "df/stockpile_settings.h"

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
    const char * name;
    // exporting
    FuncWriteExport set_value;
    std::vector<char>* stockpile_values;
    // importing
    FuncReadImport get_value;
    size_t serialized_count;
    bool valid;

    food_pair(const char * n, FuncWriteExport s, std::vector<char>* sp_v, FuncReadImport g, size_t count)
        : name(n), set_value(s), stockpile_values(sp_v), get_value(g), serialized_count(count),
          valid(true) { }
    food_pair(): valid(false) { }
};

/**
 * Class for serializing the stockpile_settings structure into a Google protobuf
 */
class StockpileSettingsSerializer {
public:
    /**
     * @param settings settings to read or write to
     */
    StockpileSettingsSerializer(df::stockpile_settings *settings);

    ~StockpileSettingsSerializer();

    /**
     * Since we depend on protobuf-lite, not the full lib, we copy this function from
     * protobuf message.cc
     */
    bool serialize_to_ostream(DFHack::color_ostream& out, std::ostream* output, uint32_t includedElements);

    /**
     * Will serialize stockpile settings to a file (overwrites existing files)
     * @return success/failure
     */
    bool serialize_to_file(DFHack::color_ostream& out, const std::string& file, uint32_t includedElements);

    /**
     * Again, copied from message.cc
     */
    bool parse_from_istream(DFHack::color_ostream &out, std::istream* input, DeserializeMode mode, const std::vector<std::string>& filters);

    /**
     * Read stockpile settings from file
     */
    bool unserialize_from_file(DFHack::color_ostream &out, const std::string& file, DeserializeMode mode, const std::vector<std::string>& filters);

protected:
    dfstockpiles::StockpileSettings mBuffer;

    // read memory structures and serialize to protobuf
    virtual void write(DFHack::color_ostream& out, uint32_t includedElements);

    // parse serialized data into ui indices
    virtual void read(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);

    virtual void write_general(DFHack::color_ostream& out);
    virtual void read_general(DFHack::color_ostream& out, DeserializeMode mode);

private:
    df::stockpile_settings *mSettings;

    bool write_ammo(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::AmmoSet* ammo);
    void read_ammo(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_animals(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::AnimalsSet* animals);
    void read_animals(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_armor(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::ArmorSet* armor);
    void read_armor(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_bars_blocks(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::BarsBlocksSet* bars_blocks);
    void read_bars_blocks(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_cloth(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::ClothSet* cloth);
    void read_cloth(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_coins(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::CoinSet* coins);
    void read_coins(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_finished_goods(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::FinishedGoodsSet* finished_goods);
    void read_finished_goods(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    food_pair food_map(df::enums::organic_mat_category::organic_mat_category cat);
    bool write_food(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::FoodSet* food);
    void read_food(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_furniture(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::FurnitureSet* furniture);
    void read_furniture(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_gems(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::GemsSet* gems);
    void read_gems(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_leather(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::LeatherSet* leather);
    void read_leather(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_corpses(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::CorpsesSet* corpses);
    void read_corpses(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_refuse(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::RefuseSet* refuse);
    void read_refuse(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_sheet(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::SheetSet* sheet);
    void read_sheet(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_stone(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::StoneSet* stone);
    void read_stone(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_weapons(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::WeaponsSet* weapons);
    void read_weapons(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
    bool write_wood(DFHack::color_ostream& out, dfstockpiles::StockpileSettings::WoodSet* wood);
    void read_wood(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);
};

/**
 * Class for serializing a stockpile into a Google protobuf
 */
class StockpileSerializer : public StockpileSettingsSerializer {
public:
    /**
     * @param stockpile stockpile to read or write settings to
     */
    StockpileSerializer(df::building_stockpilest* stockpile);

    ~StockpileSerializer();

protected:
    // read memory structures and serialize to protobuf
    virtual void write(DFHack::color_ostream& out, uint32_t includedElements);

    // parse serialized data into ui indices
    virtual void read(DFHack::color_ostream& out, DeserializeMode mode, const std::vector<std::string>& filters);

    virtual void write_general(DFHack::color_ostream& out);
    virtual void read_general(DFHack::color_ostream& out, DeserializeMode mode);

private:
    df::building_stockpilest* mPile;

    void write_containers(DFHack::color_ostream& out);
    void read_containers(DFHack::color_ostream& out, DeserializeMode mode);
};
