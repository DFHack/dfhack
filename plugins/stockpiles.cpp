#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "MiscUtils.h"

#include "modules/Materials.h"
#include "modules/Items.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/inorganic_raw.h"
#include "df/organic_mat_category.h"
#include "df/furniture_type.h"
#include "df/item_quality.h"
#include "df/item_type.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/material.h"
#include "df/inorganic_raw.h"
#include "df/plant_raw.h"
#include "df/stockpile_group_set.h"

#include "df/ui.h"
#include "df/building_stockpilest.h"
#include "df/global_objects.h"
#include "df/viewscreen_dwarfmodest.h"
#include <df/itemdef_ammost.h>


#include "stockpiles.pb.h"

#include <google/protobuf/text_format.h>

#include <functional>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;
using namespace google::protobuf;
using namespace dfstockpiles;

using df::global::world;
using df::global::ui;
using df::global::selection_rect;

using df::building_stockpilest;
using std::placeholders::_1;

static command_result copystock ( color_ostream &out, vector <string> & parameters );
static bool copystock_guard ( df::viewscreen *top );

static command_result savestock ( color_ostream &out, vector <string> & parameters );
static bool savestock_guard ( df::viewscreen *top );

DFHACK_PLUGIN ( "stockpiles" );

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands )
{
    if ( world && ui ) {
        commands.push_back (
            PluginCommand (
                "copystock", "Copy stockpile under cursor.",
                copystock, copystock_guard,
                "  - In 'q' or 't' mode: select a stockpile and invoke in order\n"
                "    to switch to the 'p' stockpile creation mode, and initialize\n"
                "    the custom settings from the selected stockpile.\n"
                "  - In 'p': invoke in order to switch back to 'q'.\n"
            )
        );
        commands.push_back (
            PluginCommand (
                "savestock", "Export the stockpile under cursor.",
                savestock, savestock_guard,
                "  - In 'q' or 't' mode: select a stockpile and invoke in order\n"
                "    to switch to the 'p' stockpile creation mode, and initialize\n"
                "    the custom settings from the selected stockpile.\n"
                "  - In 'p': invoke in order to switch back to 'q'.\n"
            )
        );
    }
    std::cerr << "world: " << sizeof ( df::world ) << " ui: " << sizeof ( df::ui )
              << " b_stock: " << sizeof ( building_stockpilest ) << endl;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

static bool copystock_guard ( df::viewscreen *top )
{
    using namespace ui_sidebar_mode;

    if ( !Gui::dwarfmode_hotkey ( top ) )
        return false;

    switch ( ui->main.mode ) {
    case Stockpiles:
        return true;
    case BuildingItems:
    case QueryBuilding:
        return !!virtual_cast<building_stockpilest> ( world->selected_building );
    default:
        return false;
    }
}

static command_result copystock ( color_ostream &out, vector <string> & parameters )
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED

    // For convenience: when used in the stockpiles mode, switch to 'q'
    if ( ui->main.mode == ui_sidebar_mode::Stockpiles ) {
        world->selected_building = NULL; // just in case it contains some kind of garbage
        ui->main.mode = ui_sidebar_mode::QueryBuilding;
        selection_rect->start_x = -30000;

        out << "Switched back to query building." << endl;
        return CR_OK;
    }

    building_stockpilest *sp = virtual_cast<building_stockpilest> ( world->selected_building );
    if ( !sp ) {
        out.printerr ( "Selected building isn't a stockpile.\n" );
        return CR_WRONG_USAGE;
    }

    ui->stockpile.custom_settings = sp->settings;
    ui->main.mode = ui_sidebar_mode::Stockpiles;
    world->selected_stockpile_type = stockpile_category::Custom;

    out << "Stockpile options copied." << endl;
    return CR_OK;
}


static bool savestock_guard ( df::viewscreen *top )
{
    using namespace ui_sidebar_mode;

    if ( !Gui::dwarfmode_hotkey ( top ) )
        return false;

    switch ( ui->main.mode ) {
    case Stockpiles:
        return true;
    case BuildingItems:
    case QueryBuilding:
        return !!virtual_cast<building_stockpilest> ( world->selected_building );
    default:
        return false;
    }
}



/**
 * Retrieve creature raw from index
 */
static df::creature_raw* find_creature ( int32_t idx )
{
    return world->raws.creatures.all[idx];
}

/**
 * Retrieve creature index from id string
 */
static int16_t find_creature ( const std::string &creature_id )
{
    return linear_index ( world->raws.creatures.all, &df::creature_raw::creature_id, creature_id );
}

typedef std::pair<int16_t, int32_t> FoodMatPair;
typedef std::map<FoodMatPair, size_t> FoodMatMap;

/**
 * Helper class for performing organic_index/organic_types <---> material array index lookups
 */
class FoodLookup
{
public:
    struct FoodMat {
        MaterialInfo material;
        df::creature_raw *creature;
        df::caste_raw * caste;
        FoodMat() : material ( -1 ), creature ( 0 ), caste ( 0 ) {}
    };
    static void food_mat_by_idx ( color_ostream &out, organic_mat_category::organic_mat_category mat_category, std::vector<int16_t>::size_type food_idx, FoodMat & food_mat ) {
        out << "food_lookup: food_idx(" << food_idx << ") ";
        df::world_raws &raws = world->raws;
        df::special_mat_table table = raws.mat_table;
        int32_t main_idx = table.organic_indexes[mat_category][food_idx];
        int16_t type = table.organic_types[mat_category][food_idx];
        if ( mat_category == organic_mat_category::Fish ||
                mat_category == organic_mat_category::UnpreparedFish ||
                mat_category == organic_mat_category::Eggs ) {
            food_mat.creature = raws.creatures.all[type];
            food_mat.caste = food_mat.creature->caste[main_idx];
            out << " special creature type(" << type << ") caste("<< main_idx <<")" <<endl;
        } else {
            food_mat.material.decode ( type, main_idx );
            out << " type(" << type << ") index("<< main_idx <<")" <<endl;
        }
    }
    static std::string food_token_by_idx ( color_ostream &out, organic_mat_category::organic_mat_category mat_category, std::vector<int16_t>::size_type idx ) {
        FoodMat food_mat;
        food_mat_by_idx ( out, mat_category, idx, food_mat );
        if ( food_mat.material.isValid() ) {
            return food_mat.material.getToken();
        } else if ( food_mat.creature ) {
            return food_mat.creature->creature_id + ":" + food_mat.caste->caste_id;
        }
        return std::string();
    }

    static void food_build_map ( color_ostream &out ) {
        if ( index_built )
            return;
        df::world_raws &raws = world->raws;
        df::special_mat_table table = raws.mat_table;
        using df::enums::organic_mat_category::organic_mat_category;
        df::enum_traits<organic_mat_category> traits;
        for ( int32_t mat_category = traits.first_item_value; mat_category <traits.last_item_value; ++mat_category ) {
            for ( size_t i = 0; i < table.organic_indexes[mat_category].size(); ++i ) {
                int16_t type = table.organic_types[mat_category].at ( i );
                int32_t index = table.organic_indexes[mat_category].at ( i );
                food_index[mat_category].insert ( std::make_pair ( std::make_pair ( type,index ), i ) ); // wtf.. only in c++
            }
        }
        index_built = true;
    }

    static int food_idx_by_token ( color_ostream &out, organic_mat_category::organic_mat_category mat_category, const std::string & token ) {
        int16_t food_idx = -1;
        df::world_raws &raws = world->raws;
        df::special_mat_table table = raws.mat_table;
        out << "food_idx_by_token: ";
        if ( mat_category == organic_mat_category::Fish ||
                mat_category == organic_mat_category::UnpreparedFish ||
                mat_category == organic_mat_category::Eggs ) {
            std::vector<string> tokens;
            split_string ( &tokens, token, ":" );
            if ( tokens.size() != 2 ) {
                out << "creature " << "invalid CREATURE:CASTE token: " << token << endl;
            } else {
                int16_t creature_idx = find_creature ( tokens[0] );
                if ( creature_idx >= 0 ) {
                    food_idx = linear_index ( table.organic_types[mat_category], creature_idx );
                    out << "creature " << token << " creature_idx(" << creature_idx << ") food_idx("<< food_idx << ")" << endl;
                } else out << " creature invalid token " << tokens[0];
            }
        } else {
            if ( !index_built ) {
                food_build_map ( out );
            }
            MaterialInfo mat_info = food_mat_by_token ( out, mat_category, token );
            int16_t type = mat_info.type;
            int32_t index = mat_info.index;
            int16_t food_idx2 = -1;
            auto it = food_index[mat_category].find ( std::make_pair ( type, index ) );
            if ( it != food_index[mat_category].end() ) {
                out << "matinfo: " << token << " type(" << mat_info.type << ") idx("  << mat_info.index << ") food_idx(" << it->second << ")" << endl;
            } else {
                out << "matinfo: " << token << " type(" << mat_info.type << ") idx("  << mat_info.index << ") food_idx not found :(";
            }
        }
        return food_idx;
    }

    static MaterialInfo food_mat_by_token ( color_ostream &out, organic_mat_category::organic_mat_category mat_category, const std::string & token ) {
        MaterialInfo mat_info;
        mat_info.find ( token );
        return mat_info;
    }

    static bool index_built;
    static std::vector<FoodMatMap> food_index;
private:
    FoodLookup() {}
};

bool FoodLookup::index_built = false;
std::vector<FoodMatMap> FoodLookup::food_index = std::vector<FoodMatMap> ( 37 );

class StockpileSerializer
{
public:
    StockpileSerializer ( color_ostream &out, building_stockpilest const * stockpile )
        : mOut ( &out )
        , mPile ( stockpile ) {
        furniture_setup_other_mats();
        bars_blocks_setup_other_mats();
    }
    StockpileSettings write() {
// 	*mOut << "GROUP SET " << bitfield_to_string(mPile->settings.flags) << endl;
        write_general();
        if ( mPile->settings.flags.bits.animals )
            write_animals();
        if ( mPile->settings.flags.bits.food )
            write_food();
        if ( mPile->settings.flags.bits.furniture )
            write_furniture();
        if ( mPile->settings.flags.bits.refuse )
            write_refuse();
        if ( mPile->settings.flags.bits.stone )
            write_stone();
        if ( mPile->settings.flags.bits.ammo )
            write_ammo();
        if ( mPile->settings.flags.bits.coins )
            write_coins();
        if ( mPile->settings.flags.bits.bars_blocks )
            write_bars_blocks();
        if ( mPile->settings.flags.bits.gems )
            write_gems();
        if ( mPile->settings.flags.bits.finished_goods )
            write_finished_goods();
        if ( mPile->settings.flags.bits.leather )
            write_leather();
        if ( mPile->settings.flags.bits.cloth )
            write_cloth();
        if ( mPile->settings.flags.bits.wood )
            write_wood();
        if ( mPile->settings.flags.bits.weapons )
            write_weapons();
        if ( mPile->settings.flags.bits.armor )
            write_armor();

        std::string str;
        TextFormat::PrintToString ( mBuffer, &str );
        *mOut << "serialized: " << str << endl;
        return mBuffer;
    }

    void read ( const StockpileSettings & settings ) {
        *mOut << endl << "==READ==" << endl;
        mBuffer = settings;
        read_general();
        read_animals();
        read_food();
        read_furniture();
        read_refuse();
        read_stone();
        read_ammo();
        read_coins();
        read_bars_blocks();
        read_gems();
        read_finished_goods();
        read_leather();
        read_cloth();
        read_wood();
        read_weapons();
        read_armor();
    }

    ~StockpileSerializer() {}
private:
    color_ostream * mOut;
    building_stockpilest const * mPile;
    StockpileSettings mBuffer;
    std::map<int, std::string> mOtherMatsFurniture;;
    std::map<int, std::string> mOtherMatsBars;
    std::map<int, std::string> mOtherMatsBlocks;
    /**
     * Find an enum's value based off the string label.
     * @param traits the enum's trait struct
     * @param token the string value in key_table
     * @return the enum's value
     */
    template<typename E>
    static typename df::enum_traits<E>::base_type linear_index ( color_ostream& out, df::enum_traits<E> traits, const std::string &token ) {
        auto j = traits.first_item_value;
        auto limit = traits.last_item_value;
        // sometimes enums start at -1, which is bad news for array indexing
        if ( j < 0 ) {
            j += abs ( traits.first_item_value );
            limit += abs ( traits.first_item_value );
        }
        for ( ; j <= limit; ++j ) {
//             out << " linear_index("<< token <<") = table["<<j<<"/"<<limit<<"]: " <<traits.key_table[j] << endl;
            if ( token.compare ( traits.key_table[j] ) == 0 )
                return j;
        }
        return -1;
    }


    //  read the token from the serailized list during import
    typedef std::function<std::string ( const size_t& ) > FuncReadImport;
    //  add the token to the serialized list during export
    typedef std::function<void ( const string & ) > FuncWriteExport;
    //  are item's of item_type allowed?
    typedef std::function<bool ( item_type::item_type ) > FuncItemAllowed;
    typedef std::function<bool ( const MaterialInfo &) > FuncMaterialAllowed;
    struct food_pair {
        // exporting
        FuncWriteExport set_value;
        vector<char> stockpile_values;
        // importing
        FuncReadImport get_value;
        size_t serialized_count;

        food_pair ( FuncWriteExport s, const vector<char>& sp_v, FuncReadImport g, size_t count )
            : set_value ( s )
            , stockpile_values ( sp_v )
            , get_value ( g )
            ,serialized_count ( count )
        {}
        food_pair() {}
    };

    void serialize_list_item_type ( FuncItemAllowed is_allowed,  FuncWriteExport add_value,  std::vector<char> list ) {
        using df::enums::item_type::item_type;
        df::enum_traits<item_type> type_traits;
        *mOut <<  "item_type size = " <<  list.size() <<  " size limit = " <<  type_traits.last_item_value <<  " typecasted:  " << ( size_t ) type_traits.last_item_value <<  endl;
        for ( size_t i = 0; i <= ( size_t ) type_traits.last_item_value; ++i ) {
            if ( list.at ( i ) ) {
                const item_type type = ( item_type ) ( ( df::enum_traits<item_type>::base_type ) i );
                std::string r_type ( type_traits.key_table[i+1] );
                if ( !is_allowed ( type ) ) continue;
                add_value ( r_type );
                *mOut << "item_type key_table[" << i+1 << "] type[" << ( int16_t ) type <<  "] is " << r_type <<endl;
            }
        }
    }
    void unserialize_list_item_type ( FuncItemAllowed is_allowed, FuncReadImport read_value,  int32_t list_size ) {
        using df::enums::item_type::item_type;
        df::enum_traits<item_type> type_traits;
        for ( int32_t i = 0; i < list_size; ++i ) {
            const std::string token = read_value ( i );
            // subtract one because item_type starts at -1
            const df::enum_traits<item_type>::base_type idx = linear_index ( *mOut, type_traits, token ) - 1;
            const item_type type = ( item_type ) idx;
            if ( !is_allowed ( type ) ) continue;
            *mOut << "   item_type " << idx << " is " << token << endl;
        }
    }
    void serialize_list_material ( FuncMaterialAllowed is_allowed,  FuncWriteExport add_value,  std::vector<char> list ) {
	MaterialInfo mi;
        for ( size_t i = 0; i < list.size(); ++i ) {
            if ( list.at ( i ) ) {
                mi.decode ( 0, i );
                if ( !is_allowed( mi ) ) continue;
                *mOut << "   material" << i << " is " << mi.getToken() << endl;
                add_value(mi.getToken());
            }
        }
    }


    void unserialize_list_material ( FuncMaterialAllowed is_allowed, FuncReadImport read_value,  int32_t list_size ) {
        for ( int i = 0; i < list_size; ++i ) {
            const std::string token = read_value ( i );
            MaterialInfo mi;
            mi.find ( token );
            if ( !is_allowed ( mi ) ) continue;
            *mOut << "   material " << mi.index << " is " << token << endl;
        }
    }

    void write_general() {
        mBuffer.set_max_bins ( mPile->max_barrels );
        mBuffer.set_max_wheelbarrows ( mPile->max_wheelbarrows );
        mBuffer.set_use_links_only ( mPile->use_links_only );
        mBuffer.set_unknown1 ( mPile->settings.unk1 );
        *mOut <<  "unknown is " <<  mPile->settings.unk1 <<  endl;
    }
    void read_general() {
        int bins = mBuffer.max_bins();
        *mOut << "Max bins: " << bins <<endl;
    }
    void write_animals() {
        dfstockpiles::StockpileSettings::AnimalsSet animals;
        animals.set_empty_cages ( mPile->settings.animals.empty_cages );
        animals.set_empty_traps ( mPile->settings.animals.empty_traps );
        for ( size_t i = 0; i < mPile->settings.animals.enabled.size(); ++i ) {
            if ( mPile->settings.animals.enabled.at ( i ) == 1 ) {
                df::creature_raw* r = find_creature ( i );
                *mOut << "creature "<< r->creature_id << " " << i << endl;
                mBuffer.mutable_animals()->add_enabled ( r->creature_id );
            }
        }
    }

    void read_animals() {
        if ( mBuffer.has_animals() ) {
// 	    mPile->settings.flags.bits.animals = true;
            *mOut << "animals:" <<endl;
            mBuffer.animals().empty_cages();
            mBuffer.animals().empty_traps();
            if ( mBuffer.animals().enabled_size() > 0 ) {
                for ( auto i = 0; i < mBuffer.animals().enabled().size(); ++i ) {
                    std::string id = mBuffer.animals().enabled ( i );
                    int idx = find_creature ( id );
                    *mOut << id << " " << idx << endl;
                }
            }
        }
    }

    food_pair food_map ( organic_mat_category::organic_mat_category cat ) {
        using df::enums::organic_mat_category::organic_mat_category;
        using namespace std::placeholders;
        switch ( cat ) {
        case organic_mat_category::Meat: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_meat ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().meat ( idx ); };
            return food_pair ( setter, mPile->settings.food.meat, getter, mBuffer.food().meat_size() );
        }
        case organic_mat_category::Fish: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_fish ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().fish ( idx ); };
            return food_pair ( setter, mPile->settings.food.fish, getter, mBuffer.food().fish_size() );
        }
        case organic_mat_category::UnpreparedFish: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_unprepared_fish ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().unprepared_fish ( idx ); };
            return food_pair ( setter, mPile->settings.food.unprepared_fish, getter, mBuffer.food().unprepared_fish_size() );
        }
        case organic_mat_category::Eggs: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_egg ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().egg ( idx ); };
            return food_pair ( setter, mPile->settings.food.egg, getter, mBuffer.food().egg_size() );
        }
        case organic_mat_category::Plants: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_plants ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().plants ( idx ); };
            return food_pair ( setter, mPile->settings.food.plants, getter, mBuffer.food().plants_size() );
        }
        case organic_mat_category::PlantDrink: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_drink_plant ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().drink_plant ( idx ); };
            return food_pair ( setter, mPile->settings.food.drink_plant, getter, mBuffer.food().drink_plant_size() );
        }
        case organic_mat_category::CreatureDrink: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_drink_animal ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().drink_animal ( idx ); };
            return food_pair ( setter, mPile->settings.food.drink_animal, getter, mBuffer.food().drink_animal_size() );
        }
        case organic_mat_category::PlantCheese: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_cheese_plant ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().cheese_plant ( idx ); };
            return food_pair ( setter, mPile->settings.food.cheese_plant, getter, mBuffer.food().cheese_plant_size() );
        }
        case organic_mat_category::CreatureCheese: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_cheese_animal ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().cheese_animal ( idx ); };
            return food_pair ( setter, mPile->settings.food.cheese_animal, getter, mBuffer.food().cheese_animal_size() );
        }
        case organic_mat_category::Seed: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_seeds ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().seeds ( idx ); };
            return food_pair ( setter, mPile->settings.food.seeds, getter, mBuffer.food().seeds_size() );
        }
        case organic_mat_category::Leaf: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_leaves ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().leaves ( idx ); };
            return food_pair ( setter, mPile->settings.food.leaves, getter, mBuffer.food().leaves_size() );
        }
        case organic_mat_category::PlantPowder: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_powder_plant ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().powder_plant ( idx ); };
            return food_pair ( setter, mPile->settings.food.powder_plant, getter, mBuffer.food().powder_plant_size() );
        }
        case organic_mat_category::CreaturePowder: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_powder_creature ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().powder_creature ( idx ); };
            return food_pair ( setter, mPile->settings.food.powder_creature, getter, mBuffer.food().powder_creature_size() );
        }
        case organic_mat_category::Glob: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_glob ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().glob ( idx ); };
            return food_pair ( setter, mPile->settings.food.glob, getter, mBuffer.food().glob_size() );
        }
        case organic_mat_category::PlantLiquid: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_liquid_plant ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().liquid_plant ( idx ); };
            return food_pair ( setter, mPile->settings.food.liquid_plant, getter, mBuffer.food().liquid_plant_size() );
        }
        case organic_mat_category::CreatureLiquid: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_liquid_animal ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().liquid_animal ( idx ); };
            return food_pair ( setter, mPile->settings.food.liquid_animal, getter, mBuffer.food().liquid_animal_size() );
        }
        case organic_mat_category::MiscLiquid: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_liquid_misc ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().liquid_misc ( idx ); };
            return food_pair ( setter, mPile->settings.food.liquid_misc, getter, mBuffer.food().liquid_misc_size() );
        }

        case organic_mat_category::Paste: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_glob_paste ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().glob_paste ( idx ); };
            return food_pair ( setter, mPile->settings.food.glob_paste, getter, mBuffer.food().glob_paste_size() );
        }
        case organic_mat_category::Pressed: {
            FuncWriteExport setter = [=] ( const std::string &id ) {
                mBuffer.mutable_food()->add_glob_pressed ( id );
            };
            FuncReadImport getter = [=] ( size_t idx ) -> std::string { return mBuffer.food().glob_pressed ( idx ); };
            return food_pair ( setter, mPile->settings.food.glob_pressed, getter, mBuffer.food().glob_pressed_size() );
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
            // not used in stockpile food menu
            break;
        }
        return food_pair();
    }
    void food_write_helper ( std::function<void ( const string & ) > add_value, const vector<char> & list, organic_mat_category::organic_mat_category cat ) {
        for ( size_t i = 0; i < list.size(); ++i ) {
            if ( list.at ( i ) ) {
                std::string token = FoodLookup::food_token_by_idx ( *mOut, cat, i );
                if ( !token.empty() ) {
                    add_value ( token );
                } else {
                    *mOut << "food mat invalid :(" << endl;
                }
            }
        }
    }

    void write_food() {
        StockpileSettings::FoodSet *food = mBuffer.mutable_food();
        food->set_prepared_meals ( mPile->settings.food.prepared_meals );
        food_pair p = food_map ( organic_mat_category::Meat );
        food_write_helper ( p.set_value, p.stockpile_values, organic_mat_category::Meat );

        using df::enums::organic_mat_category::organic_mat_category;
        df::enum_traits<organic_mat_category> traits;
        for ( int32_t mat_category = traits.first_item_value; mat_category <traits.last_item_value; ++mat_category ) {
            food_pair p = food_map ( ( organic_mat_category ) mat_category );
            food_write_helper ( p.set_value, p.stockpile_values, ( organic_mat_category ) mat_category );
        }
    }


    void food_read_helper ( FuncReadImport get_value, size_t list_size, organic_mat_category::organic_mat_category cat ) {
        if ( list_size > 0 ) {
            for ( size_t i = 0; i < list_size; ++i ) {
                std::string token = get_value ( i );
                int idx = FoodLookup::food_idx_by_token ( *mOut, cat, token );
                //mPile->settings.food.meat.at(idx) = (char) 1;
            }
        }
    }
    void read_food() {
        if ( mBuffer.has_food() ) {
            const StockpileSettings::FoodSet food = mBuffer.food();
            *mOut << "food:" <<endl;

            using df::enums::organic_mat_category::organic_mat_category;
            df::enum_traits<organic_mat_category> traits;
            for ( int32_t mat_category = traits.first_item_value; mat_category <traits.last_item_value; ++mat_category ) {
                food_pair p = food_map ( ( organic_mat_category ) mat_category );
                food_read_helper ( p.get_value, p.serialized_count, ( organic_mat_category ) mat_category );
            }
        }
    }

    void furniture_setup_other_mats() {
        mOtherMatsFurniture.insert ( std::make_pair ( 0,"WOOD" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 1,"PLANT_CLOTH" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 2,"BONE" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 3,"TOOTH" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 4,"HORN" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 5,"PEARL" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 6,"SHELL" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 7,"LEATHER" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 8,"SILK" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 9,"AMBER" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 10,"CORAL" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 11,"GREEN_GLASS" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 12,"CLEAR_GLASS" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 13,"CRYSTAL_GLASS" ) );
        mOtherMatsFurniture.insert ( std::make_pair ( 14,"YARN" ) );
    }

    std::string furn_other_mats ( int idx ) {
        auto it = mOtherMatsFurniture.find ( idx );
        if ( it == mOtherMatsFurniture.end() )
            return std::string();
        return it->second;
    }

    int furn_other_mats ( const std::string & token ) {
        for ( auto it = mOtherMatsFurniture.begin(); it != mOtherMatsFurniture.end(); ++it ) {
            if ( it->second == token )
                return it->first;
        }
        return -1;
    }

    void write_furniture() {
        StockpileSettings::FurnitureSet *furniture= mBuffer.mutable_furniture();
        furniture->set_sand_bags ( mPile->settings.furniture.sand_bags );

        // FURNITURE type
        using df::enums::furniture_type::furniture_type;
        df::enum_traits<furniture_type> type_traits;
        for ( size_t i = 0; i < mPile->settings.furniture.type.size(); ++i ) {
            if ( mPile->settings.furniture.type.at ( i ) ) {
                std::string f_type ( type_traits.key_table[i] );
                furniture->add_type ( f_type );
                *mOut << "furniture_type " << i << " is " << f_type <<endl;
            }
        }
        // metal, stone/clay materials
        MaterialInfo mi;
        for ( size_t i = 0; i < mPile->settings.furniture.mats.size(); ++i ) {
            if ( mPile->settings.furniture.mats.at ( i ) ) {
                mi.decode ( 0, i );
                if ( !furniture_mat_is_allowed ( mi ) ) continue;
                *mOut << "furniture mat: " << mi.getToken() << endl;
                furniture->add_mats ( mi.getToken() );
            }
        }
        // other mats
        for ( size_t i = 0; i < mPile->settings.furniture.other_mats.size(); ++i ) {
            if ( mPile->settings.furniture.other_mats.at ( i ) ) {
                const std::string token = furn_other_mats ( i );
                if ( token.empty() ) {
                    *mOut << " invalid other material with index " << i << endl;
                    continue;
                }
                furniture->add_other_mats ( token );
                *mOut << "  other mats " << i << " is " << token << endl;
            }
        }
        // quality core
        using df::enums::item_quality::item_quality;
        df::enum_traits<item_quality> quality_traits;
        size_t core_size = std::extent< decltype ( mPile->settings.furniture.quality_core ) >::value;
        for ( size_t i = 0; i < core_size; ++i ) {
            if ( mPile->settings.furniture.quality_core[i] ) {
                const std::string f_type ( quality_traits.key_table[i] );
                furniture->add_quality_core ( f_type );
                *mOut << "quality_core " << i << " is " << f_type <<endl;
            }
        }
        // quality total
        size_t total_size = std::extent< decltype ( mPile->settings.furniture.quality_total ) >::value;
        for ( size_t i = 0; i < total_size; ++i ) {
            if ( mPile->settings.furniture.quality_total[i] ) {
                const std::string f_type ( quality_traits.key_table[i] );
                furniture->add_quality_total ( f_type );
                *mOut << "quality_total " << i << " is " << f_type <<endl;
            }
        }

    }

    /* skip gems and non hard things
    should be roughly (<range>:<step): 32-158:1,242-262:1,264-282:2,283-310:1
    */
    bool furniture_mat_is_allowed ( const MaterialInfo &mi ) {
        if ( !mi.isValid() ) return false;
        return !mi.material->flags.is_set ( material_flags::IS_GEM ) &&  mi.material->flags.is_set ( material_flags::ITEMS_HARD );
    }

    void read_furniture() {
        if ( mBuffer.has_furniture() ) {
            const StockpileSettings::FurnitureSet furniture = mBuffer.furniture();
            *mOut << "furniture:" <<endl;

            // type
            using df::enums::furniture_type::furniture_type;
            df::enum_traits<furniture_type> type_traits;
            for ( int i = 0; i < furniture.type_size(); ++i ) {
                const std::string type = furniture.type ( i );
                df::enum_traits<furniture_type>::base_type idx = linear_index ( *mOut, type_traits, type );
                *mOut << "   type " << idx << " is " << type << endl;
            }
            // metal, stone/clay materials
            for ( int i = 0; i < furniture.mats_size(); ++i ) {
                const std::string token = furniture.mats ( i );
                MaterialInfo mi;
                mi.find ( token );
                if ( !furniture_mat_is_allowed ( mi ) ) continue;
                *mOut << "   mats " << mi.index << " is " << token << endl;
            }
            // other materials
            for ( int i = 0; i < furniture.other_mats_size(); ++i ) {
                const std::string token = furniture.other_mats ( i );
                int16_t idx = furn_other_mats ( token );
                if ( idx < 0 ) {
                    *mOut << "invalid other mat with token " << token;
                    continue;
                }
                *mOut << "  other_mats" << idx << " is " << token << endl;
            }
            // core quality
            using df::enums::item_quality::item_quality;
            df::enum_traits<item_quality> quality_traits;
            for ( int i = 0; i < furniture.quality_core_size(); ++i ) {
                const std::string quality = furniture.quality_core ( i );
                df::enum_traits<item_quality>::base_type idx = linear_index ( *mOut, quality_traits, quality );
                if ( idx < 0 ) {
                    *mOut << " invalid quality core token " << quality << endl;
                    continue;
                }
                *mOut << "   quality_core" << idx << " is " << quality << endl;
            }
            // total quality
            for ( int i = 0; i < furniture.quality_total_size(); ++i ) {
                const std::string quality = furniture.quality_total ( i );
                df::enum_traits<item_quality>::base_type idx = linear_index ( *mOut, quality_traits, quality );
                if ( idx < 0 ) {
                    *mOut << " invalid quality total token " << quality << endl;
                    continue;
                }
                *mOut << "   quality_total" << idx << " is " << quality << endl;
            }
        }
    }

    bool refuse_creature_is_allowed ( const df::creature_raw *raw ) {
        if ( !raw ) return false;
        // wagon and generated creatures not allowed,  except angels
        const bool is_wagon = raw->creature_id == "EQUIPMENT_WAGON";
        const bool is_generated = raw->flags.is_set ( creature_raw_flags::GENERATED );
        const bool is_angel = is_generated && raw->creature_id.find ( "DIVINE_" ) != std::string::npos;
        return !is_wagon && ! ( is_generated && !is_angel );
    }

    void refuse_write_helper ( std::function<void ( const string & ) > add_value, const vector<char> & list ) {
        for ( size_t i = 0; i < list.size(); ++i ) {
            if ( list.at ( i ) == 1 ) {
                df::creature_raw* r = find_creature ( i );
                // skip forgotten beasts, titans, demons, and night creatures
                if ( !refuse_creature_is_allowed ( r ) ) continue;
                *mOut << "creature "<< r->creature_id << " " << i << endl;
                add_value ( r->creature_id );
            }
        }
    }

    bool refuse_type_is_allowed ( item_type::item_type type ) {
        if ( type ==  item_type::NONE
                || type ==  item_type::BAR
                || type ==  item_type::SMALLGEM
                || type ==  item_type::BLOCKS
                || type ==  item_type::ROUGH
                || type ==  item_type::BOULDER
                || type ==  item_type::CORPSE
                || type ==  item_type::CORPSEPIECE
                || type ==  item_type::ROCK
                || type ==  item_type::ORTHOPEDIC_CAST
           ) return false;
        return true;
    }


    void write_refuse() {
        StockpileSettings::RefuseSet *refuse = mBuffer.mutable_refuse();
        refuse->set_fresh_raw_hide ( mPile->settings.refuse.fresh_raw_hide );
        refuse->set_rotten_raw_hide ( mPile->settings.refuse.rotten_raw_hide );

        // type
        FuncItemAllowed filter = std::bind ( &StockpileSerializer::refuse_type_is_allowed, this,  _1 );
        serialize_list_item_type ( filter, [=] ( const std::string &token ) {
            refuse->add_type ( token );
        },  mPile->settings.refuse.type );

//         using df::enums::item_type::item_type;
//         df::enum_traits<item_type> type_traits;
//         *mOut <<  "refuse type size = " <<  mPile->settings.refuse.type.size() <<  " size limit = " <<  type_traits.last_item_value <<  " typecasted:  " << ( size_t ) type_traits.last_item_value <<  endl;
//         for ( size_t i = 0; i <= ( size_t ) type_traits.last_item_value; ++i ) {
//             if ( mPile->settings.refuse.type.at ( i ) ) {
//                 const item_type type = ( item_type ) ( ( df::enum_traits<item_type>::base_type ) i );
//                 std::string r_type ( type_traits.key_table[i+1] );
//                 if ( !refuse_type_is_allowed ( type ) ) continue;
//                 refuse->add_type ( r_type );
//                 *mOut << "refuse type key_table[" << i+1 << "] type[" << ( int16_t ) type <<  "] is " << r_type <<endl;
//             }
//         }
        // corpses
        refuse_write_helper ( [=] ( const std::string &id ) {
            refuse->add_corpses ( id );
        }, mPile->settings.refuse.corpses );
        // body_parts
        refuse_write_helper ( [=] ( const std::string &id ) {
            refuse->add_body_parts ( id );
        }, mPile->settings.refuse.body_parts );
        // skulls
        refuse_write_helper ( [=] ( const std::string &id ) {
            refuse->add_skulls ( id );
        }, mPile->settings.refuse.skulls );
        // bones
        refuse_write_helper ( [=] ( const std::string &id ) {
            refuse->add_bones ( id );
        }, mPile->settings.refuse.bones );
        // hair
        refuse_write_helper ( [=] ( const std::string &id ) {
            refuse->add_hair ( id );
        }, mPile->settings.refuse.hair );
        // shells
        refuse_write_helper ( [=] ( const std::string &id ) {
            refuse->add_shells ( id );
        }, mPile->settings.refuse.shells );
        // teeth
        refuse_write_helper ( [=] ( const std::string &id ) {
            refuse->add_teeth ( id );
        }, mPile->settings.refuse.teeth );
        // horns
        refuse_write_helper ( [=] ( const std::string &id ) {
            refuse->add_horns ( id );
        }, mPile->settings.refuse.horns );
    }

    void refuse_read_helper ( std::function<std::string ( const size_t& ) > get_value, size_t list_size ) {
        if ( list_size > 0 ) {
            for ( size_t i = 0; i < list_size; ++i ) {
                const std::string creature_id = get_value ( i );
                const int idx = find_creature ( creature_id );
                const df::creature_raw* creature = find_creature ( idx );
                if ( idx < 0 ||  !refuse_creature_is_allowed ( creature ) ) {
                    *mOut << "invalid refuse creature: " << creature_id << endl;
                    continue;
                }
                *mOut << "      creature " << idx << " is " << creature_id << endl;
            }
        }
    }



    void read_refuse() {
        if ( mBuffer.has_refuse() ) {
            const StockpileSettings::RefuseSet refuse = mBuffer.refuse();
            *mOut << "refuse: " <<endl;
            *mOut <<  "  fresh hide " <<  refuse.fresh_raw_hide() << endl;
            *mOut <<  "  rotten hide " << refuse.rotten_raw_hide() << endl;

            // type
            FuncItemAllowed filter = std::bind ( &StockpileSerializer::refuse_type_is_allowed, this,  _1 );
            unserialize_list_item_type ( filter, [=] ( const size_t & idx ) -> const std::string& {
                return refuse.type ( idx );
            },  refuse.type_size() );

            // corpses
            *mOut << "  corpses" << endl;
            refuse_read_helper ( [=] ( const size_t & idx ) -> const std::string& {
                return refuse.corpses ( idx );
            }, refuse.corpses_size() );
            // body_parts
            *mOut << "  body_parts" << endl;
            refuse_read_helper ( [=] ( const size_t & idx ) -> const std::string& {
                return refuse.body_parts ( idx );
            }, refuse.body_parts_size() );
            // skulls
            *mOut << "  skulls" << endl;
            refuse_read_helper ( [=] ( const size_t & idx ) -> const std::string& {
                return refuse.skulls ( idx );
            }, refuse.skulls_size() );
            // bones
            *mOut << "  bones" << endl;
            refuse_read_helper ( [=] ( const size_t & idx ) -> const std::string& {
                return refuse.bones ( idx );
            }, refuse.bones_size() );
            // hair
            *mOut << "  hair" << endl;
            refuse_read_helper ( [=] ( const size_t & idx ) -> const std::string& {
                return refuse.hair ( idx );
            }, refuse.hair_size() );
            // shells
            *mOut << "  shells" << endl;
            refuse_read_helper ( [=] ( const size_t & idx ) -> const std::string& {
                return refuse.shells ( idx );
            }, refuse.shells_size() );
            // teeth
            *mOut << "  teeth" << endl;
            refuse_read_helper ( [=] ( const size_t & idx ) -> const std::string& {
                return refuse.teeth ( idx );
            }, refuse.teeth_size() );
            // horns
            *mOut << "  horns" << endl;
            refuse_read_helper ( [=] ( const size_t & idx ) -> const std::string& {
                return refuse.horns ( idx );
            }, refuse.horns_size() );
        }
    }

    bool stone_is_allowed ( const MaterialInfo &mi ) {
        if ( !mi.isValid() ) return false;
        const bool is_allowed_soil = mi.inorganic->flags.is_set ( inorganic_flags::SOIL ) && !mi.inorganic->flags.is_set ( inorganic_flags::AQUIFER );
        const bool is_allowed_stone = mi.material->flags.is_set ( material_flags::IS_STONE ) && !mi.material->flags.is_set ( material_flags::NO_STONE_STOCKPILE );
        return is_allowed_soil ||  is_allowed_stone;
    }

    void write_stone() {
        StockpileSettings::StoneSet *stone= mBuffer.mutable_stone();
        MaterialInfo mi;
        for ( size_t i = 0; i < mPile->settings.stone.mats.size(); ++i ) {
            if ( mPile->settings.stone.mats.at ( i ) ) {
                mi.decode ( 0, i );
                if ( !stone_is_allowed ( mi ) ) continue;
                *mOut << "stone mat: " <<  i <<  " is " <<  mi.getToken() << endl;
                stone->add_mats ( mi.getToken() );
            }
        }
    }

    void read_stone() {
        if ( mBuffer.has_stone() ) {
            const StockpileSettings::StoneSet stone = mBuffer.stone();
            *mOut << "stone: " <<endl;
            for ( int i = 0; i < stone.mats_size(); ++i ) {
                const std::string token = stone.mats ( i );
                MaterialInfo mi;
                mi.find ( token );
                if ( !stone_is_allowed ( mi ) ) continue;
                *mOut << "   mats " << mi.index << " is " << token << endl;
            }
        }
    }

    bool ammo_mat_is_allowed ( const MaterialInfo &mi ) {
        return mi.isValid() && mi.material && mi.material->flags.is_set ( material_flags::IS_METAL );
    }

    void write_ammo() {
        StockpileSettings::AmmoSet *ammo = mBuffer.mutable_ammo();

        //  type
        for ( size_t i = 0; i < mPile->settings.ammo.type.size(); ++i ) {
            if ( mPile->settings.ammo.type.at ( i ) ) {
                const df::itemdef_ammost *a = world->raws.itemdefs.ammo.at ( i );
                // skip procedurally generated ammo
                if ( a->base_flags.is_set ( 0 ) ) continue;
                ItemTypeInfo ii;
                ii.decode ( item_type::AMMO,  i );
                if ( !ii.isValid() ) continue;
                ammo->add_type ( ii.getToken() );
                *mOut <<  "  " <<  i <<  " is " <<  ii.getToken() << endl;
            }
        }
        // metal
        MaterialInfo mi;
        for ( size_t i = 0; i < mPile->settings.ammo.mats.size(); ++i ) {
            if ( mPile->settings.ammo.mats.at ( i ) ) {
                mi.decode ( 0, i );
                if ( !ammo_mat_is_allowed ( mi ) ) continue;
                ammo->add_mats ( mi.getToken() );
            }
        }
        *mOut <<  "after metal" <<  endl;
        // other mats
        if ( mPile->settings.ammo.other_mats.size() > 2 ) {
            *mOut << "WARNING: ammo other materials > 2! " <<  mPile->settings.ammo.other_mats.size() <<  endl;
        }
        *mOut <<  "after other check" <<  endl;
        for ( size_t i = 0; i < std::min ( size_t ( 2 ), mPile->settings.ammo.other_mats.size() ); ++i ) {
            if ( !mPile->settings.ammo.other_mats.at ( i ) )
                continue;
            const std::string token = i ==  0  ?  "WOOD"  :  "BONE";
            ammo->add_other_mats ( token );
            *mOut << "  other mats " << i << " is " << token << endl;
        }
        *mOut <<  "after other" <<  endl;
        // quality core
        using df::enums::item_quality::item_quality;
        df::enum_traits<item_quality> quality_traits;
        size_t core_size = std::extent< decltype ( mPile->settings.ammo.quality_core ) >::value;
        for ( size_t i = 0; i < core_size; ++i ) {
            if ( mPile->settings.ammo.quality_core[i] ) {
                const std::string f_type ( quality_traits.key_table[i] );
                ammo->add_quality_core ( f_type );
                *mOut << "quality_core " << i << " is " << f_type <<endl;
            }
        }
        *mOut <<  "after core" <<  endl;
        // quality total
        size_t total_size = std::extent< decltype ( mPile->settings.ammo.quality_total ) >::value;
        for ( size_t i = 0; i < total_size; ++i ) {
            if ( mPile->settings.ammo.quality_total[i] ) {
                const std::string f_type ( quality_traits.key_table[i] );
                ammo->add_quality_total ( f_type );
                *mOut << "quality_total " << i << " is " << f_type <<endl;
            }
        }
        *mOut <<  "after total" <<  endl;

    }

    void read_ammo() {
        if ( mBuffer.has_ammo() ) {
            const StockpileSettings::AmmoSet ammo = mBuffer.ammo();
            *mOut << "ammo: " <<endl;
            for ( int i = 0; i < ammo.type_size(); ++i ) {
                std::string token = ammo.type ( i );
                ItemTypeInfo ii;
                if ( !ii.find ( token ) ) continue;
                *mOut <<  "  " <<  token << endl;
            }
            //  metals
            for ( int i = 0; i < ammo.mats_size(); ++i ) {
                const std::string token = ammo.mats ( i );
                MaterialInfo mi;
                mi.find ( token );
                if ( !ammo_mat_is_allowed ( mi ) ) continue;
                *mOut << "   mats " << mi.index << " is " << token << endl;
            }

            //  others
            for ( int i = 0; i < ammo.other_mats_size(); ++i ) {
                const std::string token = ammo.other_mats ( i );
                const int32_t idx = token ==  "WOOD"  ?  0 :  token ==  "BONE"  ? 1 : -1;
                *mOut << "   other mats " << idx << " is " << token << endl;
            }
            // core quality
            using df::enums::item_quality::item_quality;
            df::enum_traits<item_quality> quality_traits;
            for ( int i = 0; i < ammo.quality_core_size(); ++i ) {
                const std::string quality = ammo.quality_core ( i );
                df::enum_traits<item_quality>::base_type idx = linear_index ( *mOut, quality_traits, quality );
                if ( idx < 0 ) {
                    *mOut << " invalid quality core token " << quality << endl;
                    continue;
                }
                *mOut << "   quality_core" << idx << " is " << quality << endl;
            }
            // total quality
            for ( int i = 0; i < ammo.quality_total_size(); ++i ) {
                const std::string quality = ammo.quality_total ( i );
                df::enum_traits<item_quality>::base_type idx = linear_index ( *mOut, quality_traits, quality );
                if ( idx < 0 ) {
                    *mOut << " invalid quality total token " << quality << endl;
                    continue;
                }
                *mOut << "   quality_total" << idx << " is " << quality << endl;
            }
        }
    }

    bool coins_mat_is_allowed ( const MaterialInfo &mi ) {
        return mi.isValid();
    }

    void write_coins() {
        StockpileSettings::CoinSet *coins = mBuffer.mutable_coin();
        MaterialInfo mi;
        for ( size_t i = 0; i < mPile->settings.coins.mats.size(); ++i ) {
            if ( mPile->settings.coins.mats.at ( i ) ) {
                mi.decode ( 0, i );
                if ( !coins_mat_is_allowed ( mi ) ) continue;
                *mOut << "   coin mat" << i << " is " << mi.getToken() << endl;
                coins->add_mats ( mi.getToken() );
            }
        }

    }

    void read_coins() {
        if ( mBuffer.has_coin() ) {
            const StockpileSettings::CoinSet coins = mBuffer.coin();
            *mOut << "coins: " <<endl;
            for ( int i = 0; i < coins.mats_size(); ++i ) {
                const std::string token = coins.mats ( i );
                MaterialInfo mi;
                mi.find ( token );
                if ( !coins_mat_is_allowed ( mi ) ) continue;
                *mOut << "   mats " << mi.index << " is " << token << endl;
            }
        }
    }

    void bars_blocks_setup_other_mats() {
        mOtherMatsBars.insert ( std::make_pair ( 0,"COAL" ) );
        mOtherMatsBars.insert ( std::make_pair ( 1,"POTASH" ) );
        mOtherMatsBars.insert ( std::make_pair ( 2,"ASH" ) );
        mOtherMatsBars.insert ( std::make_pair ( 3,"PEARLASH" ) );
        mOtherMatsBars.insert ( std::make_pair ( 4,"SOAP" ) );

        mOtherMatsBlocks.insert ( std::make_pair ( 0,"GREEN_GLASS" ) );
        mOtherMatsBlocks.insert ( std::make_pair ( 1,"CLEAR_GLASS" ) );
        mOtherMatsBlocks.insert ( std::make_pair ( 2,"CRYSTAL_GLASS" ) );
        mOtherMatsBlocks.insert ( std::make_pair ( 3,"WOOD" ) );
    }

    std::string bars_other_mats ( int idx ) {
        auto it = mOtherMatsBars.find ( idx );
        if ( it == mOtherMatsBars.end() )
            return std::string();
        return it->second;
    }

    int bars_other_mats ( const std::string & token ) {
        for ( auto it = mOtherMatsBars.begin(); it != mOtherMatsBars.end(); ++it ) {
            if ( it->second == token )
                return it->first;
        }
        return -1;
    }

    std::string blocks_other_mats ( int idx ) {
        auto it = mOtherMatsBlocks.find ( idx );
        if ( it == mOtherMatsBlocks.end() )
            return std::string();
        return it->second;
    }

    int blocks_other_mats ( const std::string & token ) {
        for ( auto it = mOtherMatsBlocks.begin(); it != mOtherMatsBlocks.end(); ++it ) {
            if ( it->second == token )
                return it->first;
        }
        return -1;
    }

    bool bars_mat_is_allowed ( const MaterialInfo &mi ) {
        return mi.isValid() && mi.material && mi.material->flags.is_set ( material_flags::IS_METAL );
    }

    bool blocks_mat_is_allowed ( const MaterialInfo &mi ) {
        return mi.isValid() && mi.material && !mi.material->flags.is_set ( material_flags::IS_GEM ) &&  mi.material->flags.is_set ( material_flags::ITEMS_HARD );
    }


    void write_bars_blocks() {
        StockpileSettings::BarsBlocksSet *bars_blocks = mBuffer.mutable_barsblocks();
        MaterialInfo mi;
        //  bars mats
//         for ( size_t i = 0; i < mPile->settings.bars_blocks.bars_mats.size(); ++i ) {
//             if ( mPile->settings.bars_blocks.bars_mats.at ( i ) ) {
//                 mi.decode ( 0, i );
//                 if ( !bars_mat_is_allowed ( mi ) ) continue;
//                 *mOut << "   bars mat" << i << " is " << mi.getToken() << endl;
//             }
//         }

	FuncMaterialAllowed filter = std::bind ( &StockpileSerializer::bars_mat_is_allowed, this,  _1 );
        serialize_list_material( filter, [=] ( const std::string &token ) {
	    bars_blocks->add_bars_mats ( token );
        },  mPile->settings.bars_blocks.bars_mats);

        //  blocks mats
	filter = std::bind ( &StockpileSerializer::blocks_mat_is_allowed, this,  _1 );
        serialize_list_material ( filter, [=] ( const std::string &token ) {
            bars_blocks->add_blocks_mats ( token );
        },  mPile->settings.bars_blocks.blocks_mats);
//         for ( size_t i = 0; i < mPile->settings.bars_blocks.blocks_mats.size(); ++i ) {
//             if ( mPile->settings.bars_blocks.blocks_mats.at ( i ) ) {
//                 mi.decode ( 0, i );
//                 if ( !blocks_mat_is_allowed ( mi ) ) continue;
//                 *mOut << "   blocks mat" << i << " is " << mi.getToken() << endl;
//                 bars_blocks->add_blocks_mats ( mi.getToken() );
//             }
//         }

        //  bars other mats
        for ( size_t i = 0; i < mPile->settings.bars_blocks.bars_other_mats.size(); ++i ) {
            if ( mPile->settings.bars_blocks.bars_other_mats.at ( i ) ) {
                const std::string token = bars_other_mats ( i );
                if ( token.empty() ) {
                    *mOut << " invalid other bar material with index " << i << endl;
                    continue;
                }
                bars_blocks->add_bars_other_mats ( token );
                *mOut << "  bars other mats " << i << " is " << token << endl;
            }
        }
        //  blocks other mats
        for ( size_t i = 0; i < mPile->settings.bars_blocks.blocks_other_mats.size(); ++i ) {
            if ( mPile->settings.bars_blocks.blocks_other_mats.at ( i ) ) {
                const std::string token = blocks_other_mats ( i );
                if ( token.empty() ) {
                    *mOut << " invalid other block material with index " << i << endl;
                    continue;
                }
                bars_blocks->add_blocks_other_mats ( token );
                *mOut << "  blocks other mats " << i << " is " << token << endl;
            }
        }

    }

    void read_bars_blocks() {
        if ( mBuffer.has_barsblocks() ) {
            const StockpileSettings::BarsBlocksSet bars_blocks = mBuffer.barsblocks();
            *mOut << "bars_blocks: " <<endl;
            // bars
            FuncMaterialAllowed filter = std::bind ( &StockpileSerializer::bars_mat_is_allowed, this,  _1 );
            unserialize_list_material ( filter, [=] ( const size_t & idx ) -> const std::string& {
                return bars_blocks.bars_mats( idx );
            },  bars_blocks.bars_mats_size());
//             for ( int i = 0; i < bars_blocks.bars_mats_size(); ++i ) {
//                 const std::string token = bars_blocks.bars_mats ( i );
//                 MaterialInfo mi;
//                 mi.find ( token );
//                 if ( !bars_mat_is_allowed ( mi ) ) continue;
//                 *mOut << "   bars mats " << mi.index << " is " << token << endl;
//             }
            //  blocks
            filter = std::bind ( &StockpileSerializer::blocks_mat_is_allowed, this,  _1 );
            unserialize_list_material ( filter, [=] ( const size_t & idx ) -> const std::string& {
                return bars_blocks.blocks_mats( idx );
            },  bars_blocks.blocks_mats_size());
//             for ( int i = 0; i < bars_blocks.blocks_mats_size(); ++i ) {
//                 const std::string token = bars_blocks.blocks_mats ( i );
//                 MaterialInfo mi;
//                 mi.find ( token );
//                 if ( !blocks_mat_is_allowed ( mi ) ) continue;
//                 *mOut << "   blocks mats " << mi.index << " is " << token << endl;
//             }
            //  bars other mats
            for ( int i = 0; i < bars_blocks.bars_other_mats_size(); ++i ) {
                const std::string token = bars_blocks.bars_other_mats ( i );
                int16_t idx = bars_other_mats ( token );
                if ( idx < 0 ) {
                    *mOut << "invalid other bars mat with token " << token;
                    continue;
                }
                *mOut << "  bars other mats " << idx << " is " << token << endl;
            }
            //  blocks other mats
            for ( int i = 0; i < bars_blocks.blocks_other_mats_size(); ++i ) {
                const std::string token = bars_blocks.blocks_other_mats ( i );
                int16_t idx = blocks_other_mats ( token );
                if ( idx < 0 ) {
                    *mOut << "invalid other blocks mat with token " << token;
                    continue;
                }
                *mOut << "  blocks other mats " << idx << " is " << token << endl;
            }
        }
    }

    bool gem_mat_is_allowed ( const MaterialInfo &mi ) {
        return mi.isValid() && mi.material && mi.material->flags.is_set ( material_flags::IS_GEM );
    }
    bool gem_cut_mat_is_allowed ( const MaterialInfo &mi ) {
        return mi.isValid() && mi.material && ( mi.material->flags.is_set ( material_flags::IS_GEM ) || mi.material->flags.is_set ( material_flags::IS_STONE ) ) ;
    }

    void write_gems() {
        StockpileSettings::GemsSet *gems = mBuffer.mutable_gems();
        MaterialInfo mi;
        // rough mats
        for ( size_t i = 0; i < mPile->settings.gems.rough_mats.size(); ++i ) {
            if ( mPile->settings.gems.rough_mats.at ( i ) ) {
                mi.decode ( 0, i );
                if ( !gem_mat_is_allowed ( mi ) ) continue;
                *mOut << "   gem rough mat" << i << " is " << mi.getToken() << endl;
                gems->add_rough_mats ( mi.getToken() );
            }
        }
        // cut mats
        for ( size_t i = 0; i < mPile->settings.gems.cut_mats.size(); ++i ) {
            if ( mPile->settings.gems.cut_mats.at ( i ) ) {
                mi.decode ( 0, i );
                if ( !gem_cut_mat_is_allowed ( mi ) ) continue;
                *mOut << "   gem cut mat" << i << " is " << mi.getToken() << endl;
                gems->add_cut_mats ( mi.getToken() );
            }
        }
        //  rough other
        for ( size_t i = 0; i < mPile->settings.gems.rough_other_mats.size(); ++i ) {
            if ( mPile->settings.gems.rough_other_mats.at ( i ) ) {
                mi.decode ( i, -1 );
                if ( !mi.isValid() ) continue;
                *mOut << "   gem rough_other mat" << i << " is " << mi.getToken() << endl;
                gems->add_rough_other_mats ( mi.getToken() );
            }
        }
        //  cut other
        for ( size_t i = 0; i < mPile->settings.gems.cut_other_mats.size(); ++i ) {
            if ( mPile->settings.gems.cut_other_mats.at ( i ) ) {
                mi.decode ( i, -1 );
                if ( !mi.isValid() ) mi.decode ( 0, i );
                if ( !mi.isValid() ) continue;
                *mOut << "   gem cut_other mat" << i << " is " << mi.getToken() << endl;
                gems->add_cut_other_mats ( mi.getToken() );
            }
        }
    }
    void read_gems() {
        if ( mBuffer.has_gems() ) {
            const StockpileSettings::GemsSet gems = mBuffer.gems();
            *mOut << "gems: " <<endl;
            // rough
            for ( int i = 0; i < gems.rough_mats_size(); ++i ) {
                const std::string token = gems.rough_mats ( i );
                MaterialInfo mi;
                mi.find ( token );
                if ( !gem_mat_is_allowed ( mi ) ) continue;
                *mOut << "   rough mats " << mi.index << " is " << token << endl;
            }
            // cut
            for ( int i = 0; i < gems.cut_mats_size(); ++i ) {
                const std::string token = gems.cut_mats ( i );
                MaterialInfo mi;
                mi.find ( token );
                if ( !gem_cut_mat_is_allowed ( mi ) ) continue;
                *mOut << "   cut mats " << mi.index << " is " << token << endl;
            }
            // rough other
            for ( int i = 0; i < gems.rough_other_mats_size(); ++i ) {
                const std::string token = gems.rough_other_mats ( i );
                MaterialInfo mi;
                mi.find ( token );
                if ( !mi.isValid() ) continue;
                *mOut << "   rough_other mats " << mi.type << " is " << token << endl;
            }
            // cut other
            for ( int i = 0; i < gems.cut_other_mats_size(); ++i ) {
                const std::string token = gems.cut_other_mats ( i );
                MaterialInfo mi;
                mi.find ( token );
                if ( !mi.isValid() ) continue;
                *mOut << "   cut_other mats " << mi.type << " is " << token << endl;
            }
        }
    }

    bool finished_goods_type_is_allowed ( item_type::item_type type ) {
        switch ( type ) {
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

    bool finished_goods_mat_is_allowed(const MaterialInfo &mat) {
	return mat.isValid();
    }

    void write_finished_goods() {
        StockpileSettings::FinishedGoodsSet *finished_goods = mBuffer.mutable_finished_goods();

        // type
        FuncItemAllowed filter = std::bind ( &StockpileSerializer::finished_goods_type_is_allowed, this,  _1 );
        serialize_list_item_type ( filter, [=] ( const std::string &token ) {
            finished_goods->add_type ( token );
        },  mPile->settings.finished_goods.type );

        // materials
	FuncMaterialAllowed mat_filter = std::bind ( &StockpileSerializer::finished_goods_mat_is_allowed, this,  _1 );
        serialize_list_material ( mat_filter, [=] ( const std::string &token ) {
            finished_goods->add_mats ( token );
        },  mPile->settings.finished_goods.mats );

    }

    void read_finished_goods() {
        if ( mBuffer.has_finished_goods() ) {
            const StockpileSettings::FinishedGoodsSet finished_goods = mBuffer.finished_goods();
            *mOut << "finished_goods: " <<endl;

            // type
            FuncItemAllowed filter = std::bind ( &StockpileSerializer::finished_goods_type_is_allowed, this,  _1 );
            unserialize_list_item_type ( filter, [=] ( const size_t & idx ) -> const std::string& {
                return finished_goods.type ( idx );
            },  finished_goods.type_size() );

            // materials
	    FuncMaterialAllowed mat_filter = std::bind ( &StockpileSerializer::finished_goods_mat_is_allowed, this,  _1 );
            unserialize_list_material( mat_filter, [=] ( const size_t & idx ) -> const std::string& {
                return finished_goods.mats( idx );
            },  finished_goods.mats_size() );

        }
    }

    void write_leather() {}
    void read_leather() {}

    void write_cloth() {}
    void read_cloth() {}

    void write_wood() {}
    void read_wood() {}

    void write_weapons() {}
    void read_weapons() {}

    void write_armor() {}
    void read_armor() {}
};

static command_result savestock ( color_ostream &out, vector <string> & parameters )
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    building_stockpilest *sp = virtual_cast<building_stockpilest> ( world->selected_building );
    if ( !sp ) {
        out.printerr ( "Selected building isn't a stockpile.\n" );
        return CR_WRONG_USAGE;
    }

    //  for testing
    StockpileSerializer cereal ( out, sp );
    StockpileSettings s = cereal.write();
    StockpileSerializer cereal2 ( out, sp );
    cereal2.read ( s );

    return CR_OK;
}

