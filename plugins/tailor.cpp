/*
 * Tailor plugin. Automatically manages keeping your dorfs clothed.
 */

#include <algorithm>

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"

#include "modules/Materials.h"
#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/building.h"
#include "df/creature_raw.h"
#include "df/descriptor_color.h"
#include "df/historical_entity.h"
#include "df/item.h"
#include "df/item_flags.h"
#include "df/itemdef_armorst.h"
#include "df/itemdef_glovesst.h"
#include "df/itemdef_helmst.h"
#include "df/itemdef_pantsst.h"
#include "df/itemdef_shoesst.h"
#include "df/items_other_id.h"
#include "df/manager_order.h"
#include "df/material.h"
#include "df/plotinfost.h"
#include "df/reaction.h"
#include "df/reaction_reagent.h"
#include "df/reaction_reagent_itemst.h"
#include "df/reaction_product_itemst.h"
#include "df/unit.h"
#include "df/world.h"

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("tailor");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(standing_orders_use_dyed_cloth);
REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(tailor, control, DebugCategory::LINFO);
    DBG_DECLARE(tailor, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static const string CONFIG_KEY_2 = string(plugin_name) + "/config1";
static PersistentDataItem config;
static PersistentDataItem config2;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
    CONFIG_SILK_IDX = 1,
    CONFIG_CLOTH_IDX = 2,
    CONFIG_YARN_IDX = 3,
    CONFIG_LEATHER_IDX = 4,
    CONFIG_ADAMANTINE_IDX = 5,
    CONFIG_CONFISCATE = 6
};

enum Config2Values
{
    CONFIG_AUTOMATE_DYE = 0
};

static const int32_t CYCLE_TICKS = 1231; // one day
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

// ah, if only STL had a bimap
static const std::map<df::job_type, df::item_type> jobTypeMap = {
    { df::job_type::MakeArmor, df::item_type::ARMOR },
    { df::job_type::MakePants, df::item_type::PANTS },
    { df::job_type::MakeHelm, df::item_type::HELM },
    { df::job_type::MakeGloves, df::item_type::GLOVES },
    { df::job_type::MakeShoes, df::item_type::SHOES }
};

static const std::map<df::item_type, df::job_type> itemTypeMap = {
    { df::item_type::ARMOR, df::job_type::MakeArmor },
    { df::item_type::PANTS, df::job_type::MakePants },
    { df::item_type::HELM, df::job_type::MakeHelm },
    { df::item_type::GLOVES, df::job_type::MakeGloves },
    { df::item_type::SHOES, df::job_type::MakeShoes }
};

class MatType {
public:
    const std::string name;
    const df::job_material_category job_material;
    const df::armor_general_flags armor_flag;

    bool operator==(const MatType& m) const {
        return name == m.name;
    }

    // operator< is required to use this as a std::map key
    bool operator<(const MatType& m) const {
        return name < m.name;
    }

    MatType(std::string& n, df::job_material_category jm, df::armor_general_flags af)
        : name(n), job_material(jm), armor_flag(af) {};
    MatType(const char* n, df::job_material_category jm, df::armor_general_flags af)
        : name(std::string(n)), job_material(jm), armor_flag(af) {};
};

static const MatType
    M_SILK = MatType("silk", df::job_material_category::mask_silk, df::armor_general_flags::SOFT),
    M_CLOTH = MatType("cloth", df::job_material_category::mask_cloth, df::armor_general_flags::SOFT),
    M_YARN = MatType("yarn", df::job_material_category::mask_yarn, df::armor_general_flags::SOFT),
    M_LEATHER = MatType("leather", df::job_material_category::mask_leather, df::armor_general_flags::LEATHER),
    M_ADAMANTINE = MatType("adamantine", df::job_material_category::mask_strand, df::armor_general_flags::SOFT);

static const std::list<MatType> all_materials = { M_SILK, M_CLOTH, M_YARN, M_LEATHER, M_ADAMANTINE };
static const std::list<MatType> default_materials = { M_SILK, M_CLOTH, M_YARN, M_LEATHER }; // adamantine not included by default

static struct BadFlags {
    uint32_t whole;

    BadFlags() {
        df::item_flags flags;
        #define F(x) flags.bits.x = true;
        F(dump); F(forbid); F(garbage_collect);
        F(hostile); F(on_fire); F(rotten); F(trader);
        F(in_building); F(construction); F(owned);
        F(removed); F(encased); F(spider_web);
        #undef F
        whole = flags.whole;
    }
} badFlags;

class Tailor {

private:
    std::map<int, int> sizes; // this maps body size to races

    std::map<std::pair<df::item_type, int>, int> available;

    std::map<std::pair<df::item_type, int>, int> needed;

    std::map<std::tuple<df::job_type, int, int>, int> orders;

    std::map<MatType, int> supply;
    std::map<MatType, int> reserves;

    std::list<MatType> material_order = default_materials;

    int default_reserve = 10;

    bool confiscate = true;

    bool automate_dye = false;

public:
    void set_confiscate(bool f){ confiscate = f; }

    bool get_confiscate() { return confiscate; }

    void set_automate_dye(bool f) { automate_dye = f; }

    bool get_automate_dye() { return automate_dye; }

    void sync_material_order()
    {
        material_order.clear();
        for (size_t i = 0; i < all_materials.size(); ++i) {
            if (i == (size_t)config.get_int(CONFIG_SILK_IDX))
                material_order.push_back(M_SILK);
            else if (i == (size_t)config.get_int(CONFIG_CLOTH_IDX))
                material_order.push_back(M_CLOTH);
            else if (i == (size_t)config.get_int(CONFIG_YARN_IDX))
                material_order.push_back(M_YARN);
            else if (i == (size_t)config.get_int(CONFIG_LEATHER_IDX))
                material_order.push_back(M_LEATHER);
            else if (i == (size_t)config.get_int(CONFIG_ADAMANTINE_IDX))
                material_order.push_back(M_ADAMANTINE);
        }
        if (!material_order.size())
            std::copy(default_materials.begin(), default_materials.end(), std::back_inserter(material_order));
    }

    std::list<MatType> get_material_order() {
        return material_order;
    }

    void reset()
    {
        available.clear();
        needed.clear();
        sizes.clear();
        supply.clear();
        orders.clear();
    }

    void scan_clothing()
    {
        for (auto i : world->items.other[df::items_other_id::ANY_GOES_IN_CABINET])
        {
            if (i->flags.whole & badFlags.whole)
            {
                continue;
            }
            if (i->getWear() >= 1)
                continue;
            if (i->getMakerRace() < 0) // sometimes we get borked items with no valid maker race
                continue;

            df::item_type t = i->getType();
            int size = world->raws.creatures.all[i->getMakerRace()]->adultsize;

            available[std::make_pair(t, size)] += 1;
        }

        if (DBG_NAME(cycle).isEnabled(DebugCategory::LDEBUG))
        {
            for (auto& i : available)
            {
                df::item_type t;
                int size;
                std::tie(t, size) = i.first;
                DEBUG(cycle).print("tailor: %d %s of size %d found\n",
                    i.second, ENUM_KEY_STR(item_type, t).c_str(), size);
            }
        }
    }

    void scan_materials()
    {
        bool require_dyed = df::global::standing_orders_use_dyed_cloth ? (*df::global::standing_orders_use_dyed_cloth) : false;

        for (auto i : world->items.other[df::items_other_id::CLOTH])
        {
            if (i->flags.whole & badFlags.whole)
                continue;

            if (require_dyed && (!i->isDyed()))
            {
                // only count dyed
                std::string d;
                i->getItemDescription(&d, 0);
                TRACE(cycle).print("tailor: skipping undyed %s\n", DF2CONSOLE(d).c_str());
                continue;
            }
            MaterialInfo mat(i);
            int ss = i->getStackSize();

            if (mat.material)
            {
                if (mat.material->flags.is_set(df::material_flags::SILK))
                    supply[M_SILK] += ss;
                else if (mat.material->flags.is_set(df::material_flags::THREAD_PLANT))
                    supply[M_CLOTH] += ss;
                else if (mat.material->flags.is_set(df::material_flags::YARN))
                    supply[M_YARN] += ss;
                else if (mat.material->flags.is_set(df::material_flags::STOCKPILE_THREAD_METAL))
                    supply[M_ADAMANTINE] += ss;
                else
                {
                    std::string d;
                    i->getItemDescription(&d, 0);
                    DEBUG(cycle).print("tailor: weird cloth item found: %s (%d)\n", DF2CONSOLE(d).c_str(), i->id);
                }
            }
        }

        for (auto i : world->items.other[df::items_other_id::SKIN_TANNED])
        {
            if (i->flags.whole & badFlags.whole)
                continue;
            supply[M_LEATHER] += i->getStackSize();
        }

        DEBUG(cycle).print("tailor: available silk %d yarn %d cloth %d leather %d adamantine %d\n",
            supply[M_SILK], supply[M_YARN], supply[M_CLOTH], supply[M_LEATHER], supply[M_ADAMANTINE]);
    }

    void scan_replacements()
    {
        for (auto u : Units::citizensRange(world->units.active)) {
            if (Units::isBaby(u) ||
                !Units::casteFlagSet(u->race, u->caste, df::enums::caste_raw_flags::EQUIPS))
                continue; // skip units we don't control or that can't wear clothes

            std::set <df::item_type> equipped;
            std::set <df::item_type> ordered;
            std::deque<df::item*> damaged;

            for (auto inv : u->inventory)
            {
                if (inv->mode != df::inv_item_role_type::Worn)
                    continue;
                // skip non-clothing
                if (!inv->item->isClothing())
                    continue;
                if (inv->item->getWear() > 0)
                    damaged.push_back(inv->item);
                else
                    equipped.insert(inv->item->getType());
            }

            int usize = world->raws.creatures.all[u->race]->adultsize;
            sizes[usize] = u->race;

            for (auto w : damaged)
            {
                // skip armor
                if (w->getEffectiveArmorLevel() > 0)
                    continue;

                auto ty = w->getType();

                auto makerRace = w->getMakerRace();
                if (makerRace < 0 || makerRace >= (int16_t) world->raws.creatures.all.size())
                    continue;

                int isize = world->raws.creatures.all[makerRace]->adultsize;
                std::string description;
                w->getItemDescription(&description, 0);

                if (equipped.count(ty) == 0)
                {
                    if (ordered.count(ty) == 0)
                    {
                        DEBUG(cycle).print ("tailor: %s (size %d) worn by %s (size %d) needs replacement\n",
                                            DF2CONSOLE(description).c_str(), isize,
                                            DF2CONSOLE(Units::getReadableName(u)).c_str(), usize);
                        needed[std::make_pair(ty, usize)] += 1;
                        ordered.insert(ty);
                    }
                }

                if (confiscate && equipped.count(ty) > 0)
                {
                    if (w->flags.bits.owned)
                    {
                        bool confiscated = Items::setOwner(w, NULL);

                        INFO(cycle).print(
                            "tailor: %s %s from %s.\n",
                            (confiscated ? "confiscated" : "could not confiscate"),
                            DF2CONSOLE(description).c_str(),
                            DF2CONSOLE(Units::getReadableName(u)).c_str()
                        );
                    }

                    if (w->getWear() > 1)
                        w->flags.bits.dump = true;
                }

            }

            for (auto ty : std::set<df::item_type>{ df::item_type::ARMOR, df::item_type::PANTS, df::item_type::SHOES })
            {
                if (equipped.count(ty) == 0 && ordered.count(ty) == 0)
                {
                    TRACE(cycle).print("tailor: one %s of size %d needed to cover %s\n",
                        ENUM_KEY_STR(item_type, ty).c_str(),
                        usize,
                        DF2CONSOLE(Units::getReadableName(u)).c_str());
                    needed[std::make_pair(ty, usize)] += 1;
                }
            }
        }
    }

    void create_orders()
    {
        auto entity = world->entities.all[plotinfo->civ_id];

        for (auto& a : needed)
        {
            df::item_type ty = a.first.first;
            int size = a.first.second;
            int count = a.second;

            // decrease "need" by "available"
            count -= available[std::make_pair(ty, size)];

            if (count <= 0)
                continue;

            int sub = 0;
            std::vector<int16_t> v;

            switch (ty) {
            case df::item_type::ARMOR:  v = entity->resources.armor_type; break;
            case df::item_type::GLOVES: v = entity->resources.gloves_type; break;
            case df::item_type::HELM:   v = entity->resources.helm_type; break;
            case df::item_type::PANTS:  v = entity->resources.pants_type; break;
            case df::item_type::SHOES:  v = entity->resources.shoes_type; break;
            default: break;
            }

            for (auto vv : v) {
                bool isClothing = false;
                switch (ty) {
                case df::item_type::ARMOR:  isClothing = world->raws.itemdefs.armor[vv]->armorlevel == 0; break;
                case df::item_type::GLOVES: isClothing = world->raws.itemdefs.gloves[vv]->armorlevel == 0; break;
                case df::item_type::HELM:   isClothing = world->raws.itemdefs.helms[vv]->armorlevel == 0; break;
                case df::item_type::PANTS:  isClothing = world->raws.itemdefs.pants[vv]->armorlevel == 0; break;
                case df::item_type::SHOES:  isClothing = world->raws.itemdefs.shoes[vv]->armorlevel == 0; break;
                default: break;
                }
                if (isClothing)
                {
                    sub = vv;
                    break;
                }
            }

            auto jj = itemTypeMap.find(ty);
            if (jj != itemTypeMap.end())
            {
                const df::job_type j = jj->second;
                orders[std::make_tuple(j, sub, size)] += count;
                DEBUG(cycle).print("tailor: %s times %d of size %d ordered\n", ENUM_KEY_STR(job_type, j).c_str(), count, size);
            }
        }
    }

    void scan_existing_orders()
    {
        for (auto o : world->manager_orders.all)
        {
            auto f = jobTypeMap.find(o->job_type);
            if (f == jobTypeMap.end())
                continue;

            for (auto& m : all_materials)
            {
                if (o->material_category.whole == m.job_material.whole)
                {
                    supply[m] -= o->amount_left;
                    TRACE(cycle).print("tailor: supply of %s reduced by %d due to being required for an existing order\n",
                        DF2CONSOLE(m.name).c_str(), o->amount_left);
                }
            }
        }
    }

    using jiqt = decltype(df::reaction_reagent_itemst::quantity);

    int get_reaction_max_count(df::reaction* r, jiqt lim = std::numeric_limits<jiqt>::max())
    {
        jiqt max = lim;

        for (auto rr : r->reagents)
        {
            if (rr->getType() == df::reaction_reagent_type::item)
            {
                max = std::min(get_reagent_max_count(virtual_cast<df::reaction_reagent_itemst>(rr), max), max);
            }
        }

        return max;
    }

    int get_reagent_max_count(df::reaction_reagent_itemst* r, jiqt lim = std::numeric_limits<jiqt>::max())
    {
        df::reaction_reagent_itemst* t = df::allocate<df::reaction_reagent_itemst>();
        *t = *r;
        jiqt orig = t->quantity;

        std::vector<int32_t> tbc;
        std::vector<int32_t> toc;
        std::vector<std::string*> trc;
        std::vector<int32_t> tcc;

        auto test = [&] (jiqt m) {
            t->quantity = orig * m;
            return t->have_enough_from_precalc_info(NULL, &tbc, &toc, &trc, &tcc, -1);
            };

        jiqt max = lim / orig;

        jiqt lo = 0;
        jiqt hi = max;

        // exit fast when 0
        if (test(1))
        {
            lo = 1;

            while (hi > lo + 1)
            {
                // can't do (hi+lo)/2 because hi+lo may overflow!
                jiqt mult = hi / 2 + lo / 2;
                if (mult == lo) mult += 1;
                if (test(mult))
                    lo = mult;
                else
                    hi = mult;
            }
        }

        delete t;

        return lo;
    }

    int count_dyeables(df::item_type ty)
    {
        auto reagent = df::allocate<df::reaction_reagent_itemst>();
        reagent->item_subtype = -1;
        reagent->mat_type = -1;
        reagent->mat_index = -1;
        reagent->flags2.bits.dyeable = true;
        reagent->has_tool_use = df::tool_uses::NONE;
        if (ty == df::item_type::SKIN_TANNED)
        {
            reagent->quantity = 1;
            reagent->min_dimension = 1;
        }
        else
        {
            reagent->quantity = 10000;
            reagent->min_dimension = 10000;
        }
        reagent->item_type = ty;
        return get_reagent_max_count(reagent);
    }

    int count_dyes()
    {
        auto reagent = df::allocate<df::reaction_reagent_itemst>();
        reagent->item_type = df::item_type::NONE;
        reagent->item_subtype = -1;
        reagent->mat_type = -1;
        reagent->mat_index = -1;
        reagent->flags1.bits.unrotten = true;
        reagent->flags2.bits.dye = true;
        reagent->has_tool_use = df::tool_uses::NONE;
        reagent->quantity = 1;
        int count = get_reagent_max_count(reagent);
        delete reagent;
        return count;
    }

    using color_type = decltype(MaterialInfo::material->powder_dye);

    static auto product_is_dye (df::reaction_product* r) -> bool
    {
        if (r->getType() == df::reaction_product_type::item)
        {
            auto rr = virtual_cast<df::reaction_product_itemst> (r);
            auto mat = MaterialInfo(rr->mat_type, rr->mat_index);
            return mat.material && mat.material->flags.is_set(df::enums::material_flags::IS_DYE);
        }
        return false;
    };

    void order_dye_from_reaction(df::reaction* r, int c = 1)
    {
        std::string descr;
        auto dye = std::ranges::find_if(r->products, product_is_dye);
        assert(dye != r->products.end());
        auto pp = virtual_cast<df::reaction_product_itemst>(*dye);
        assert(pp != nullptr);

        pp->getDescription(&descr);

        get_or_create_order(c, df::job_type::CustomReaction, -1, -1, 0, r->code);
        INFO(cycle).print("tailor: ordered %d %s\n", c, DF2CONSOLE(descr).c_str());
    }

    void order_dye_cloth(int c = 1)
    {
        get_or_create_order(c, df::job_type::DyeCloth, -1, -1, 0);
    }

    void make_dyes(int count)
    {
        auto reaction_produces_dye = [] (df::reaction* r) {
            return std::ranges::any_of(r->products, product_is_dye);
            };

        for (auto r : std::ranges::views::filter(world->raws.reactions.reactions, reaction_produces_dye))
        {
            int max = get_reaction_max_count(r,1);

            if (max > 0)
            {
                order_dye_from_reaction(r, max);
            }

            count = std::max(0, count - max);
            if (count <= 0)
                break;
        }
    }

    int count_dye_cloth_orders()
    {
        auto f = [] (df::manager_order* o) {
            return o->job_type == df::job_type::DyeCloth;
            };

        int sum = 0;
        for (auto o : std::ranges::views::filter(world->manager_orders.all, f))
        {
            sum += o->amount_left;
        }
        return sum;
    }

    using oqt = decltype(df::manager_order::amount_total);

    static df::manager_order* get_or_create_order(oqt c, df::job_type ty, int16_t sub, int32_t hfid, df::job_material_category mcat, std::string custom_reaction = "")
    {
        auto f = [&] (df::manager_order* order) {
            return order->job_type == ty &&
                order->item_type == df::item_type::NONE &&
                order->item_subtype == sub &&
                order->mat_type == -1 &&
                order->mat_index == -1 &&
                order->specdata.race == hfid &&
                order->material_category.whole == mcat.whole &&
                order->frequency == df::workquota_frequency_type::OneTime &&
                order->reaction_name == custom_reaction;
            };

        auto orderIt = std::ranges::find_if(world->manager_orders.all, f);

        if (orderIt != world->manager_orders.all.end())
        {
            auto o = *orderIt;
            if (o->amount_left > 0)
            {
                o->amount_left = std::max(c, o->amount_left);
                o->amount_total = std::max(c, o->amount_total);
            }
            return o;
        }

        auto order = new df::manager_order;
        order->job_type = ty;
        order->item_type = df::item_type::NONE;
        order->item_subtype = sub;
        order->reaction_name = custom_reaction;
        order->specdata.race = hfid;
        order->material_category = mcat;
        order->mat_type = -1;
        order->mat_index = -1;
        order->amount_left = c;
        order->amount_total = c;
        order->status.bits.validated = false;
        order->status.bits.active = false;
        order->id = world->manager_orders.manager_order_next_id++;

        world->manager_orders.all.push_back(order);

        return order;
    }

    int place_orders()
    {
        int ordered = 0;
        int skipped = 0;
        auto entity = world->entities.all[plotinfo->civ_id];

        for (auto& o : orders)
        {
            df::job_type ty;
            int sub;
            int size;

            std::tie(ty, sub, size) = o.first;
            int count = o.second;

            if (sizes.count(size) == 0)
            {
                WARN(cycle).print("tailor: cannot determine race for clothing of size %d, skipped\n",
                    size);
                continue;
            }

            if (count > 0)
            {
                std::vector<int16_t> v;
                BitArray<df::armor_general_flags>* fl;
                std::string name_s, name_p;

                switch (ty) {

                case df::job_type::MakeArmor:
                    v = entity->resources.armor_type;
                    name_s = world->raws.itemdefs.armor[sub]->name;
                    name_p = world->raws.itemdefs.armor[sub]->name_plural;
                    fl = &world->raws.itemdefs.armor[sub]->props.flags;
                    break;
                case df::job_type::MakeGloves:
                    name_s = world->raws.itemdefs.gloves[sub]->name;
                    name_p = world->raws.itemdefs.gloves[sub]->name_plural;
                    v = entity->resources.gloves_type;
                    fl = &world->raws.itemdefs.gloves[sub]->props.flags;
                    break;
                case df::job_type::MakeHelm:
                    name_s = world->raws.itemdefs.helms[sub]->name;
                    name_p = world->raws.itemdefs.helms[sub]->name_plural;
                    v = entity->resources.helm_type;
                    fl = &world->raws.itemdefs.helms[sub]->props.flags;
                    break;
                case df::job_type::MakePants:
                    name_s = world->raws.itemdefs.pants[sub]->name;
                    name_p = world->raws.itemdefs.pants[sub]->name_plural;
                    v = entity->resources.pants_type;
                    fl = &world->raws.itemdefs.pants[sub]->props.flags;
                    break;
                case df::job_type::MakeShoes:
                    name_s = world->raws.itemdefs.shoes[sub]->name;
                    name_p = world->raws.itemdefs.shoes[sub]->name_plural;
                    v = entity->resources.shoes_type;
                    fl = &world->raws.itemdefs.shoes[sub]->props.flags;
                    break;
                default:
                    break;
                }

                bool can_make = std::find(v.begin(), v.end(), sub) != v.end();

                if (!can_make)
                {
                    INFO(cycle).print("tailor: civilization cannot make %s, skipped\n", DF2CONSOLE(name_p).c_str());
                    continue;
                }

                DEBUG(cycle).print("tailor: ordering %d %s\n", count, DF2CONSOLE(name_p).c_str());

                for (auto& m : material_order)
                {
                    if (count <= 0)
                        break;

                    auto r = reserves.find(m);
                    int res = (r == reserves.end()) ? default_reserve : r->second;

                    if (supply[m] > res && fl->is_set(m.armor_flag)) {
                        int c = count;
                        if (supply[m] < count + res)
                        {
                            c = supply[m] - res;
                            TRACE(cycle).print("tailor: order reduced from %d to %d to protect reserves of %s\n",
                                count, c, DF2CONSOLE(m.name).c_str());
                            skipped += (count - c);
                        }
                        supply[m] -= c;

                        auto order = get_or_create_order(c, ty, sub, sizes[size], m.job_material);

                        INFO(cycle).print("tailor: added order #%d for %d %s %s, sized for %s\n",
                            order->id,
                            c,
                            bitfield_to_string(order->material_category).c_str(),
                            DF2CONSOLE((c > 1) ? name_p : name_s).c_str(),
                            DF2CONSOLE(world->raws.creatures.all[order->specdata.hist_figure_id]->name[1]).c_str()
                        );

                        count -= c;
                        ordered += c;
                    }
                    else
                    {
                        skipped += count;
                        INFO(cycle).print("tailor: material %s skipped due to lack of reserves, %d available\n", DF2CONSOLE(m.name).c_str(), supply[m]);
                    }

                }
            }
        }

        if (skipped > 0)
        {
            INFO(cycle).print("tailor: %d item%s not ordered due to a lack of materials\n", skipped, skipped != 1 ? "s" : "");

            if (automate_dye)
            {
                int available_dyes = count_dyes();
                int available_dyeable_cloth = count_dyeables(df::item_type::CLOTH);

                int to_dye = std::max(0, std::min(skipped, std::min(count_dyes(), available_dyeable_cloth)) - count_dye_cloth_orders());
                if (to_dye > 0)
                {
                    INFO(cycle).print("tailor: dyeing %d cloth\n", to_dye);
                    order_dye_cloth(to_dye);
                }

                int dyes_to_make = std::max(0, skipped - available_dyes - to_dye);
                if (dyes_to_make > 0)
                {
                    INFO(cycle).print("tailor: ordering up to %d dyes\n", dyes_to_make);
                    make_dyes(dyes_to_make);
                }
            }
        }

        return ordered;
    }

    int do_cycle()
    {
        reset();
        scan_clothing();
        scan_materials();
        scan_replacements();
        scan_existing_orders();
        create_orders();
        return place_orders();
    }

};

static std::unique_ptr<Tailor> tailor_instance;

static command_result do_command(color_ostream &out, vector<string> &parameters);
static int do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);

    tailor_instance = std::make_unique<Tailor>();

    // provide a configuration interface for the plugin
    commands.push_back(PluginCommand(
        plugin_name,
        "Automatically keep your dwarves in fresh clothing.",
        do_command));

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_cycle(out);
    } else {
        DEBUG(control,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(control,out).print("shutting down %s\n", plugin_name);

    tailor_instance.release();

    return CR_OK;
}



DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        config.set_bool(CONFIG_CONFISCATE, true);
        config.set_bool(CONFIG_AUTOMATE_DYE, false);
    }
    // transition existing saves to CONFIG_CONFISCATE=true
    if (config.get_int(CONFIG_CONFISCATE) < 0) {
        DEBUG(control,out).print("found existing configuration with CONFIG_CONFISCATE unset, initializing to true\n");
        config.set_bool(CONFIG_CONFISCATE, true);
    }

    config2 = World::GetPersistentSiteData(CONFIG_KEY_2);
    // transition existing saves to CONFIG_AUTOMATE_DYE=false
    if (config2.get_int(CONFIG_AUTOMATE_DYE) < 0)
    {
        DEBUG(control, out).print("found existing configuration with CONFIG_AUTOMATE_DYE unset, initializing to false\n");
        config2.set_bool(CONFIG_AUTOMATE_DYE, false);
    }

    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    DEBUG(control,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");
    tailor_instance->set_confiscate(config.get_bool(CONFIG_CONFISCATE));
    tailor_instance->set_automate_dye(config2.get_bool(CONFIG_AUTOMATE_DYE));
    DEBUG(control,out).print("loading persisted confiscation state: %s\n",
                            tailor_instance->get_confiscate() ? "true" : "false");
    tailor_instance->sync_material_order();

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(control,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS) {
        int ordered = do_cycle(out);
        if (0 < ordered)
            out.print("tailor: ordered %d items of clothing\n", ordered);
    }
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.tailor", "parse_commandline", parameters, 1,
            [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

/////////////////////////////////////////////////////
// cycle logic
//

static int do_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    return tailor_instance->do_cycle();
}

/////////////////////////////////////////////////////
// Lua API
//

static void tailor_doCycle(color_ostream &out) {
    DEBUG(control,out).print("entering tailor_doCycle\n");
    out.print("ordered %d items of clothing\n", do_cycle(out));
}

// remember, these are ONE-based indices from Lua
static void tailor_setMaterialPreferences(color_ostream &out, int32_t silkIdx,
                        int32_t clothIdx, int32_t yarnIdx, int32_t leatherIdx,
                        int32_t adamantineIdx) {
    DEBUG(control,out).print("entering tailor_setMaterialPreferences\n");

    // it doesn't really matter if these are invalid. set_material_order will do
    // the right thing.
    config.set_int(CONFIG_SILK_IDX, silkIdx - 1);
    config.set_int(CONFIG_CLOTH_IDX, clothIdx - 1);
    config.set_int(CONFIG_YARN_IDX, yarnIdx - 1);
    config.set_int(CONFIG_LEATHER_IDX, leatherIdx - 1);
    config.set_int(CONFIG_ADAMANTINE_IDX, adamantineIdx - 1);

    tailor_instance->sync_material_order();
}

static void tailor_setConfiscate(color_ostream& out, bool enable)
{
    DEBUG(control,out).print("%s confiscation of tattered clothing \n", enable ? "enabling" : "disabling");
    config.set_bool(CONFIG_CONFISCATE, enable);
    tailor_instance->set_confiscate(enable);
}

static bool tailor_getConfiscate(color_ostream& out)
{
    return tailor_instance->get_confiscate();
}

static void tailor_setAutomateDye(color_ostream& out, bool enable)
{
    DEBUG(control, out).print("%s automation of dye\n", enable ? "enabling" : "disabling");
    config2.set_bool(CONFIG_AUTOMATE_DYE, enable);
    tailor_instance->set_automate_dye(enable);
}

static bool tailor_getAutomateDye(color_ostream& out)
{
    return tailor_instance->get_automate_dye();
}

static int tailor_getMaterialPreferences(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(control,*out).print("entering tailor_getMaterialPreferences\n");
    vector<string> names;
    for (const auto& m : tailor_instance->get_material_order())
        names.emplace_back(m.name);
    Lua::PushVector(L, names);
    return 1;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(tailor_doCycle),
    DFHACK_LUA_FUNCTION(tailor_setMaterialPreferences),
    DFHACK_LUA_FUNCTION(tailor_setConfiscate),
    DFHACK_LUA_FUNCTION(tailor_getConfiscate),
    DFHACK_LUA_FUNCTION(tailor_setAutomateDye),
    DFHACK_LUA_FUNCTION(tailor_getAutomateDye),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(tailor_getMaterialPreferences),
    DFHACK_LUA_END
};
