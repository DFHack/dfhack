/*
 * Tailor plugin. Automatically manages keeping your dorfs clothed.
 */

#include <algorithm>
#include <array>
#include <map>
#include <tuple>

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"

#include "modules/Items.h"
#include "modules/Materials.h"
#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/body_part_raw.h"
#include "df/building.h"
#include "df/caste_body_info.h"
#include "df/creature_raw.h"
#include "df/descriptor_color.h"
#include "df/glove_handedness.h"
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
static const string CONFIG_KEY_2 = string(plugin_name) + "/config_2";
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

// chirality classes used to track left and right glove needs (and stock)
// separately; achiral items always use CHIRAL_ANY
enum TailorChiral {
    CHIRAL_ANY = 0,
    CHIRAL_RIGHT = 1,
    CHIRAL_LEFT = 2,
    CHIRAL_OTHER = 3,   // gloves with no (or an invalid) handedness; nothing we
                        // can produce fills such a need
};

// one body part that wants a worn item, as determined by its flags using the
// same precedence DF uses to compute clothing needs
struct TailorSlot {
    int16_t body_part_id;
    df::item_type item_type;
    int16_t subtype = -1; // chosen clothing itemdef, -1 if none is usable
    int chiral = CHIRAL_ANY;
};

// the set of clothing slots for a body plan, plus the clothing item
// definitions chosen to fill them. this is expensive to compute, so it is
// cached per caste_body_info for the lifetime of the world
struct BodyClothingPlan {
    std::vector<TailorSlot> slots;
    std::map<int16_t, size_t> slot_by_part;
};

// the item type a body part wants worn on it. DF checks part flags in this
// order, so e.g. a part flagged both STANCE and GRASP (the "hands" of a
// creature that walks on its hands) wants shoes rather than gloves
static df::item_type slot_type_for_part(df::body_part_raw *part)
{
    auto &f = part->flags;
    if (f.is_set(df::body_part_raw_flags::LOWERBODY)) return df::item_type::PANTS;
    if (f.is_set(df::body_part_raw_flags::UPPERBODY)) return df::item_type::ARMOR;
    if (f.is_set(df::body_part_raw_flags::STANCE))    return df::item_type::SHOES;
    if (f.is_set(df::body_part_raw_flags::HEAD))      return df::item_type::HELM;
    if (f.is_set(df::body_part_raw_flags::GRASP))     return df::item_type::GLOVES;
    return df::item_type::NONE;
}

// faithful reimplementation of DF's bodyst::get_body_step_distance, which
// measures how many "steps" of non-torso body parts lie between a target part
// and the part a garment is worn on, walking up the body part parent
// (con_part_id) chain. DF compares the result to a clothing item
// definition's UBSTEP/LBSTEP values to decide coverage:
//
//   *main is set when the target is the worn part itself or a LOWERBODY part
//        directly attached to it (body armor always covers the lower body).
//   *ub_steps is set when the target does not lie on a stance limb chain
//        (e.g. arms), *lb_steps when it does (legs, stance tails, etc).
//
// The distance counts every non-UPPERBODY/LOWERBODY part on the chain from
// the target upward, stopping the count at the first torso part; every part
// on the chain between that torso part and the worn part must itself be a
// torso part, and the worn part must be an ancestor of the target, otherwise
// no distance is reported.
static void body_step_distance(df::caste_body_info *body, int16_t worn, int16_t target,
        bool *main, int16_t *ub_steps, int16_t *lb_steps)
{
    *main = false;
    *ub_steps = -1;
    *lb_steps = -1;

    auto &parts = body->body_parts;
    int n = (int)parts.size();
    if (worn < 0 || worn >= n || target < 0 || target >= n)
        return;

    auto is_torso = [&parts](int16_t i) {
        return parts[i]->flags.is_set(df::body_part_raw_flags::UPPERBODY) ||
               parts[i]->flags.is_set(df::body_part_raw_flags::LOWERBODY);
    };

    if (worn == target ||
        (parts[target]->con_part_id == worn &&
         parts[target]->flags.is_set(df::body_part_raw_flags::LOWERBODY))) {
        *main = true;
        return;
    }

    // a part is on a stance limb chain if it is itself a stance part or a LIMB
    // ancestor of a stance part
    bool on_stance_chain = false;
    for (int i = 0; i < n && !on_stance_chain; i++) {
        if (!parts[i]->flags.is_set(df::body_part_raw_flags::STANCE))
            continue;
        if (i == target) {
            on_stance_chain = true;
            break;
        }
        for (int16_t s = parts[i]->con_part_id; 0 <= s && s < n; s = parts[s]->con_part_id) {
            if (!parts[s]->flags.is_set(df::body_part_raw_flags::LIMB))
                break;
            if (s == target) {
                on_stance_chain = true;
                break;
            }
        }
    }

    int steps = 0;
    bool crossed_torso = false;
    int16_t cur = target;
    while (0 <= cur && cur < n) {
        if (crossed_torso) {
            if (!is_torso(cur))
                return;
        } else if (is_torso(cur)) {
            crossed_torso = true;
        } else {
            steps++;
        }
        if (cur == worn)
            break;
        cur = parts[cur]->con_part_id;
    }
    if (steps > 0 && cur == worn) {
        if (on_stance_chain)
            *lb_steps = steps;
        else
            *ub_steps = steps;
    }
}

// replicates the coverage decision made by DF's unitst::insulation_quantity:
// does a garment of item type 'ty' worn on body part 'worn' cover body part
// 'target'? the step parameters are the item definition's UBSTEP/LBSTEP/UPSTEP
// values as appropriate for the type
static bool covers_part(df::caste_body_info *body, df::item_type ty,
        int16_t ubstep, int16_t lbstep, int16_t upstep, int16_t worn, int16_t target)
{
    auto &parts = body->body_parts;
    int n = (int)parts.size();
    if (worn < 0 || worn >= n || target < 0 || target >= n)
        return false;

    switch (ty) {
    case df::item_type::HELM:
        // helms cover only the head they are worn on
        return target == worn;
    case df::item_type::GLOVES:
    case df::item_type::SHOES: {
        // gloves and shoes cover the part they are worn on plus up to UPSTEP
        // LIMB ancestors of that part
        if (target == worn)
            return true;
        int steps = 1;
        for (int16_t s = parts[worn]->con_part_id; 0 <= s && s < n; s = parts[s]->con_part_id, steps++) {
            if (!parts[s]->flags.is_set(df::body_part_raw_flags::LIMB))
                break;
            if (s == target)
                return steps <= upstep;
        }
        return false;
    }
    case df::item_type::ARMOR:
    case df::item_type::PANTS: {
        // body armor and pants never cover heads, grasp, or stance parts
        auto &tf = parts[target]->flags;
        if (tf.is_set(df::body_part_raw_flags::HEAD) ||
            tf.is_set(df::body_part_raw_flags::GRASP) ||
            tf.is_set(df::body_part_raw_flags::STANCE))
            return false;
        bool main;
        int16_t ub, lb;
        body_step_distance(body, worn, target, &main, &ub, &lb);
        if (main)
            return true;
        if (ty == df::item_type::ARMOR)
            return (0 <= ub && ub <= ubstep) || (0 <= lb && lb <= lbstep);
        // pants can only extend down stance limb chains, never along arms
        return 0 <= lb && lb <= lbstep;
    }
    default:
        return false;
    }
}

// the number of body parts an item of the given type/subtype would cover on
// the given body plan when worn on 'worn'
static int itemdef_coverage(df::caste_body_info *body, df::item_type ty, int16_t sub, int16_t worn)
{
    int16_t ubstep = 0, lbstep = 0, upstep = 0;
    switch (ty) {
    case df::item_type::ARMOR: {
        auto def = world->raws.itemdefs.armor[sub];
        if (!def)
            return 0;
        ubstep = def->ubstep;
        lbstep = def->lbstep;
        break;
    }
    case df::item_type::PANTS: {
        auto def = world->raws.itemdefs.pants[sub];
        if (!def)
            return 0;
        lbstep = def->lbstep;
        break;
    }
    case df::item_type::GLOVES: {
        auto def = world->raws.itemdefs.gloves[sub];
        if (!def)
            return 0;
        upstep = def->upstep;
        break;
    }
    case df::item_type::SHOES: {
        auto def = world->raws.itemdefs.shoes[sub];
        if (!def)
            return 0;
        upstep = def->upstep;
        break;
    }
    case df::item_type::HELM:   break;
    default: return 0;
    }
    int count = 0;
    for (int t = 0; t < (int)body->body_parts.size(); t++)
        if (covers_part(body, ty, ubstep, lbstep, upstep, worn, t))
            count++;
    return count;
}

static const std::vector<int16_t> &entity_itemdefs(df::historical_entity *e, df::item_type ty)
{
    static const std::vector<int16_t> empty;
    switch (ty) {
    case df::item_type::ARMOR:  return e->resources.armor_type;
    case df::item_type::GLOVES: return e->resources.gloves_type;
    case df::item_type::HELM:   return e->resources.helm_type;
    case df::item_type::PANTS:  return e->resources.pants_type;
    case df::item_type::SHOES:  return e->resources.shoes_type;
    default: return empty;
    }
}

// clothing is distinguished from real armor by armorlevel == 0
static bool itemdef_is_clothing(df::item_type ty, int16_t sub)
{
    switch (ty) {
    case df::item_type::ARMOR:
        return world->raws.itemdefs.armor[sub] &&
            world->raws.itemdefs.armor[sub]->armorlevel == 0;
    case df::item_type::GLOVES:
        return world->raws.itemdefs.gloves[sub] &&
            world->raws.itemdefs.gloves[sub]->armorlevel == 0;
    case df::item_type::HELM:
        return world->raws.itemdefs.helms[sub] &&
            world->raws.itemdefs.helms[sub]->armorlevel == 0;
    case df::item_type::PANTS:
        return world->raws.itemdefs.pants[sub] &&
            world->raws.itemdefs.pants[sub]->armorlevel == 0;
    case df::item_type::SHOES:
        return world->raws.itemdefs.shoes[sub] &&
            world->raws.itemdefs.shoes[sub]->armorlevel == 0;
    default: return false;
    }
}

// the chirality bucket a stock item belongs to
static int stock_chiral(df::item *i)
{
    if (i->getType() != df::item_type::GLOVES)
        return CHIRAL_ANY;
    uint32_t h = i->getGloveHandedness();
    if (h == (1u << df::glove_handedness::RIGHT))
        return CHIRAL_RIGHT;
    if (h == (1u << df::glove_handedness::LEFT))
        return CHIRAL_LEFT;
    return CHIRAL_OTHER;
}

// does this item fill the given clothing slot when worn on it? the item type
// must match, and gloves must have the handedness the slot requires
static bool item_fills_slot(df::item *item, const TailorSlot &slot)
{
    if (item->getType() != slot.item_type)
        return false;
    if (slot.item_type == df::item_type::GLOVES) {
        uint32_t h = item->getGloveHandedness();
        switch (slot.chiral) {
        case CHIRAL_RIGHT: return (h & (1u << df::glove_handedness::RIGHT)) != 0;
        case CHIRAL_LEFT:  return (h & (1u << df::glove_handedness::LEFT)) != 0;
        case CHIRAL_OTHER: return h == 0;
        default:           return false;
        }
    }
    return true;
}

class Tailor {

private:
    std::map<int, int> sizes; // this maps body size to races

    // (item type, size, chirality) -> count of unworn clothing items in stock
    std::map<std::tuple<df::item_type, int, int>, int> available;

    // (item type, itemdef subtype, size, chirality) -> count of items needed.
    // for shoes and gloves this counts individual items; each order produces
    // a pair, so order counts are derived from these numbers
    std::map<std::tuple<df::item_type, int, int, int>, int> needed;

    std::map<std::tuple<df::job_type, int, int>, int> orders;

    // cached clothing plans, keyed by the shared caste body plan pointer; see
    // BodyClothingPlan. body plans and entity clothing lists are fixed for the
    // lifetime of a loaded world
    std::map<df::caste_body_info*, BodyClothingPlan> plan_cache;

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

    void clear_plan_cache()
    {
        plan_cache.clear();
    }

    void reset()
    {
        available.clear();
        needed.clear();
        sizes.clear();
        supply.clear();
        orders.clear();
    }

    // choose the craftable clothing itemdef of the given type covering the
    // most body parts on this body plan when worn on 'worn'; -1 if none
    int best_subtype(df::item_type ty, const std::vector<int16_t> &candidates,
            df::caste_body_info *body, int16_t worn)
    {
        int best = -1;
        int best_coverage = -1;
        for (auto sub : candidates)
        {
            if (!itemdef_is_clothing(ty, sub))
                continue;
            int coverage = itemdef_coverage(body, ty, sub, worn);
            if (coverage > best_coverage)
            {
                best_coverage = coverage;
                best = sub;
            }
        }
        return best;
    }

    // compute (or fetch from the cache) the clothing plan for a body plan:
    // the list of clothing slots and the best-covering itemdef for each
    const BodyClothingPlan *get_plan(df::caste_body_info *body)
    {
        auto found = plan_cache.find(body);
        if (found != plan_cache.end())
            return &found->second;

        BodyClothingPlan plan;
        auto entity = world->entities.all[plotinfo->civ_id];
        int n = (int)body->body_parts.size();
        for (int i = 0; i < n; i++)
        {
            auto part = body->body_parts[i];
            df::item_type ty = slot_type_for_part(part);
            if (ty == df::item_type::NONE)
                continue;

            TailorSlot slot;
            slot.body_part_id = (int16_t)i;
            slot.item_type = ty;
            if (ty == df::item_type::GLOVES)
            {
                // gloves are made in pairs of one left and one right glove, so
                // only distinctly left/right grasp parts can wear them
                bool r = part->flags.is_set(df::body_part_raw_flags::RIGHT);
                bool l = part->flags.is_set(df::body_part_raw_flags::LEFT);
                slot.chiral = r && !l ? CHIRAL_RIGHT :
                              l && !r ? CHIRAL_LEFT : CHIRAL_OTHER;
            }
            if (slot.chiral != CHIRAL_OTHER)
                slot.subtype = best_subtype(ty, entity_itemdefs(entity, ty), body, slot.body_part_id);

            plan.slot_by_part[slot.body_part_id] = plan.slots.size();
            plan.slots.push_back(slot);
        }

        return &plan_cache.emplace(body, std::move(plan)).first->second;
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
            if (!i->isClothing())
                continue;
            if (i->getMakerRace() < 0) // sometimes we get borked items with no valid maker race
                continue;

            df::item_type t = i->getType();
            int size = world->raws.creatures.all[i->getMakerRace()]->adultsize;

            available[std::make_tuple(t, size, stock_chiral(i))] += 1;
        }

        if (DBG_NAME(cycle).isEnabled(DebugCategory::LDEBUG))
        {
            for (auto& i : available)
            {
                df::item_type t;
                int size, chiral;
                std::tie(t, size, chiral) = i.first;
                DEBUG(cycle).print("tailor: {} {} of size {} found\n",
                    i.second, ENUM_KEY_STR(item_type, t), size);
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
                TRACE(cycle).print("tailor: skipping undyed {}\n", DF2CONSOLE(d));
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
                    DEBUG(cycle).print("tailor: weird cloth item found: {} ({})\n", DF2CONSOLE(d), i->id);
                }
            }
        }

        for (auto i : world->items.other[df::items_other_id::SKIN_TANNED])
        {
            if (i->flags.whole & badFlags.whole)
                continue;
            supply[M_LEATHER] += i->getStackSize();
        }

        DEBUG(cycle).print("tailor: available silk {} yarn {} cloth {} leather {} adamantine {}\n",
            supply[M_SILK], supply[M_YARN], supply[M_CLOTH], supply[M_LEATHER], supply[M_ADAMANTINE]);
    }

    void confiscate_item(df::unit *u, df::item *w)
    {
        std::string description;
        w->getItemDescription(&description, 0);

        if (w->flags.bits.owned)
        {
            bool confiscated = Items::setOwner(w, NULL);

            INFO(cycle).print(
                "tailor: {} {} from {}.\n",
                (confiscated ? "confiscated" : "could not confiscate"),
                DF2CONSOLE(description),
                DF2CONSOLE(Units::getReadableName(u))
            );
        }

        if (w->getWear() > 1)
            w->flags.bits.dump = true;
    }

    void scan_replacements()
    {
        for (auto u : Units::citizensRange(world->units.active)) {
            if (Units::isBaby(u) ||
                !Units::casteFlagSet(u->race, u->caste, df::enums::caste_raw_flags::EQUIPS))
                continue; // skip units we don't control or that can't wear clothes

            auto body = u->body.body_plan;
            if (!body)
                continue;
            auto plan = get_plan(body);
            if (!plan)
                continue;

            int usize = world->raws.creatures.all[u->race]->adultsize;
            sizes[usize] = u->race;

            // count, per clothing slot, the intact and tattered items worn on
            // the slot's body part that actually fill the slot
            std::vector<int> good(plan->slots.size(), 0);
            std::map<size_t, std::vector<df::item*>> tattered;

            for (auto inv : u->inventory)
            {
                if (inv->mode != df::inv_item_role_type::Worn &&
                    inv->mode != df::inv_item_role_type::WrappedAround)
                    continue;
                // skip non-clothing
                if (!inv->item->isClothing())
                    continue;
                auto sit = plan->slot_by_part.find(inv->body_part_id);
                if (sit == plan->slot_by_part.end())
                    continue;
                if (!item_fills_slot(inv->item, plan->slots[sit->second]))
                    continue;
                if (inv->item->getWear() > 0)
                    tattered[sit->second].push_back(inv->item);
                else
                    good[sit->second] += 1;
            }

            auto &part_status = u->body.components.body_part_status;
            for (size_t i = 0; i < plan->slots.size(); i++)
            {
                const TailorSlot &slot = plan->slots[i];
                // missing parts need no clothing
                if (slot.body_part_id < (int)part_status.size() &&
                    part_status[slot.body_part_id].bits.missing)
                    continue;

                if (good[i] > 0)
                {
                    // the slot is adequately covered; any tattered clothing
                    // still worn on it is redundant and can be confiscated
                    if (confiscate)
                        for (auto w : tattered[i])
                            confiscate_item(u, w);
                    continue;
                }

                if (slot.subtype < 0)
                {
                    TRACE(cycle).print("tailor: {} has a {} part tailor cannot supply\n",
                        DF2CONSOLE(Units::getReadableName(u)),
                        ENUM_KEY_STR(item_type, slot.item_type));
                    continue;
                }

                TRACE(cycle).print("tailor: one {} of size {} needed to cover {} ({})\n",
                    ENUM_KEY_STR(item_type, slot.item_type),
                    usize,
                    DF2CONSOLE(Units::getReadableName(u)),
                    DF2CONSOLE(body->body_parts[slot.body_part_id]->token));
                needed[std::make_tuple(slot.item_type, slot.subtype, usize, slot.chiral)] += 1;
            }
        }
    }

    void create_orders()
    {
        // glove needs are tallied per chirality across all body plans sharing
        // a (subtype, size) pair; each job makes one left and one right glove,
        // so the number of jobs is the larger of the two needs
        std::map<std::pair<int, int>, std::array<int, 3>> glove_need;

        for (auto& a : needed)
        {
            df::item_type ty;
            int sub, size, chiral;
            std::tie(ty, sub, size, chiral) = a.first;

            // decrease "need" by "available"; stock of any subtype fills a
            // need of its type, so consume the shared pool as we go
            int &stock = available[std::make_tuple(ty, size, chiral)];
            int count = a.second - stock;
            stock = std::max(0, stock - a.second);

            if (count <= 0)
                continue;

            if (ty == df::item_type::SHOES)
            {
                // shoes are made in identical pairs, so two uncovered stance
                // parts can be shod with a single order
                int pairs = (count + 1) / 2;
                orders[std::make_tuple(df::job_type::MakeShoes, sub, size)] += pairs;
                DEBUG(cycle).print("tailor: {} shoes of size {} needed, ordering {} pairs\n",
                    count, size, pairs);
            }
            else if (ty == df::item_type::GLOVES)
            {
                glove_need[std::make_pair(sub, size)][chiral] += count;
            }
            else
            {
                auto jj = itemTypeMap.find(ty);
                if (jj != itemTypeMap.end())
                {
                    orders[std::make_tuple(jj->second, sub, size)] += count;
                    DEBUG(cycle).print("tailor: {} times {} of size {} ordered\n",
                        ENUM_KEY_STR(job_type, jj->second), count, size);
                }
            }
        }

        for (auto& g : glove_need)
        {
            int pairs = std::max(g.second[CHIRAL_LEFT], g.second[CHIRAL_RIGHT]);
            if (pairs <= 0)
                continue;
            orders[std::make_tuple(df::job_type::MakeGloves, g.first.first, g.first.second)] += pairs;
            DEBUG(cycle).print("tailor: {} left and {} right gloves of size {} needed, ordering {} pairs\n",
                g.second[CHIRAL_LEFT], g.second[CHIRAL_RIGHT], g.first.second, pairs);
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
                    TRACE(cycle).print("tailor: supply of {} reduced by {} due to being required for an existing order\n",
                        DF2CONSOLE(m.name), o->amount_left);
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

    using oqt = decltype(df::manager_order::amount_total);

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

    oqt order_dye_from_reaction(df::reaction* r, int c = 1)
    {
        std::string descr;
        auto dye = std::ranges::find_if(r->products, product_is_dye);
        assert(dye != r->products.end());
        auto pp = virtual_cast<df::reaction_product_itemst>(*dye);
        assert(pp != nullptr);

        pp->getDescription(&descr);

        auto [order, added] = ensure_order(c, df::job_type::CustomReaction, -1, -1, 0, r->code);
        if (added > 0)
        {
            INFO(cycle).print("tailor: ordered {} {}\n", added, DF2CONSOLE(descr));
        }
        // the order (existing or new) now covers the requested amount
        return order ? std::min<oqt>(c, order->amount_left) : 0;
    }

    oqt order_dye_cloth(int c = 1)
    {
        auto [order, added] = ensure_order(c, df::job_type::DyeCloth, -1, -1, 0);
        return order ? std::min<oqt>(c, order->amount_left) : 0;
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
                max = order_dye_from_reaction(r, max);
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

    static df::manager_order *find_order(df::job_type ty, int16_t sub, int32_t hfid, df::job_material_category mcat, const std::string &custom_reaction)
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
                order->reaction_name == custom_reaction &&
                order->amount_left > 0;
            };

        auto orderIt = std::ranges::find_if(world->manager_orders.all, f);
        return orderIt != world->manager_orders.all.end() ? *orderIt : nullptr;
    }

    // how much of the demand for this (job, subtype, race) is already covered
    // by pending manager orders; any material counts, since every such order
    // eventually produces an item that fills the need
    static oqt pending_coverage(oqt demand, df::job_type ty, int16_t sub, int32_t hfid)
    {
        oqt covered = 0;
        for (auto order : world->manager_orders.all)
        {
            if (order->job_type == ty &&
                order->item_type == df::item_type::NONE &&
                order->item_subtype == sub &&
                order->mat_type == -1 &&
                order->mat_index == -1 &&
                order->specdata.race == hfid &&
                order->frequency == df::workquota_frequency_type::OneTime &&
                order->reaction_name.empty())
            {
                covered += order->amount_left;
                if (covered >= demand)
                    break;
            }
        }
        return std::min(covered, demand);
    }

    // increase the matching manager order by `add` items, creating it with
    // `add` items if none exists; returns (order, items committed)
    static std::pair<df::manager_order*,oqt> add_to_order(oqt add, df::job_type ty, int16_t sub, int32_t hfid, df::job_material_category mcat, std::string custom_reaction = "")
    {
        if (auto order = find_order(ty, sub, hfid, mcat, custom_reaction))
        {
            order->amount_left += add;
            order->amount_total += add;
            return {order, add};
        }

        if (add <= 0)
            return {nullptr, 0};

        auto order = new df::manager_order;
        order->job_type = ty;
        order->item_type = df::item_type::NONE;
        order->item_subtype = sub;
        order->reaction_name = custom_reaction;
        order->specdata.race = hfid;
        order->material_category = mcat;
        order->mat_type = -1;
        order->mat_index = -1;
        order->amount_left = add;
        order->amount_total = add;
        order->status.bits.validated = false;
        order->status.bits.active = false;
        order->frequency = df::workquota_frequency_type::OneTime;
        order->id = world->manager_orders.manager_order_next_id++;

        world->manager_orders.all.push_back(order);

        return {order, add};
    }

    // ensure the matching order has at least `demand` items pending;
    // returns (order, additional items committed by this call)
    static std::pair<df::manager_order*,oqt> ensure_order(oqt demand, df::job_type ty, int16_t sub, int32_t hfid, df::job_material_category mcat, std::string custom_reaction = "")
    {
        auto order = find_order(ty, sub, hfid, mcat, custom_reaction);
        oqt add = std::max<oqt>(0, demand - (order ? order->amount_left : 0));
        return add_to_order(add, ty, sub, hfid, mcat, custom_reaction);
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
                WARN(cycle).print("tailor: cannot determine race for clothing of size {}, skipped\n",
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
                    INFO(cycle).print("tailor: civilization cannot make {}, skipped\n", DF2CONSOLE(name_p));
                    continue;
                }

                // pending orders for this item already count toward the need
                count -= pending_coverage(count, ty, sub, sizes[size]);
                if (count <= 0)
                    continue;

                DEBUG(cycle).print("tailor: ordering {} {}\n", count, DF2CONSOLE(name_p));

                for (auto& m : material_order)
                {
                    if (count <= 0)
                        break;

                    auto r = reserves.find(m);
                    int res = (r == reserves.end()) ? default_reserve : r->second;

                    if (supply[m] > res && fl->is_set(m.armor_flag)) {
                        int c = std::min(count, supply[m] - res);
                        if (c < count)
                        {
                            TRACE(cycle).print("tailor: order reduced from {} to {} to protect reserves of {}\n",
                                count, c, DF2CONSOLE(m.name));
                        }
                        supply[m] -= c;

                        auto [order,n] = add_to_order(c, ty, sub, sizes[size], m.job_material);

                        if (n > 0)
                        {
                            INFO(cycle).print("tailor: added order #{} for {} {} {}, sized for {}\n",
                                order->id,
                                n,
                                bitfield_to_string(order->material_category),
                                DF2CONSOLE((c > 1) ? name_p : name_s),
                                DF2CONSOLE(world->raws.creatures.all[order->specdata.hist_figure_id]->name[1]));

                            count -= n;
                            ordered += n;
                        }
                    }
                    else
                    {
                        DEBUG(cycle).print("tailor: material {} skipped due to lack of reserves, {} available\n", DF2CONSOLE(m.name), supply[m]);
                    }

                }

                skipped += count;
            }
        }

        if (skipped > 0)
        {
            INFO(cycle).print("tailor: {} item{} not ordered due to a lack of materials\n", skipped, skipped != 1 ? "s" : "");

            if (automate_dye)
            {
                int available_dyes = count_dyes();
                int available_dyeable_cloth = count_dyeables(df::item_type::CLOTH);
                int dye_cloth_orders = count_dye_cloth_orders();

                DEBUG(cycle).print("tailor: available dyes = {}, available dyeable cloth = {}, dye cloth orders = {}\n",
                    available_dyes, available_dyeable_cloth, dye_cloth_orders);
                int to_dye = std::min(skipped, std::min(available_dyes, available_dyeable_cloth) - dye_cloth_orders);
                DEBUG(cycle).print("tailor: to dye = {}\n", to_dye);
                if (to_dye > 0)
                {
                    int dyed = order_dye_cloth(to_dye);
                    if (dyed > 0)
                    {
                        INFO(cycle).print("tailor: dyeing {} cloth\n", to_dye);
                    }
                }

                int dyes_to_make = available_dyes - to_dye;
                if (dyes_to_make > 0)
                {
                    INFO(cycle).print("tailor: ordering up to {} dyes\n", dyes_to_make);
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
    DEBUG(control,out).print("initializing {}\n", plugin_name);

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
        out.printerr("Cannot enable {} without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control,out).print("{} from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_cycle(out);
    } else {
        DEBUG(control,out).print("{} from the API, but already {} ; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(control,out).print("shutting down {}\n", plugin_name);

    tailor_instance.release();

    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    cycle_timestamp = 0;
    if (tailor_instance)
        tailor_instance->clear_plan_cache();
    config = World::GetPersistentSiteData(CONFIG_KEY);
    config2 = World::GetPersistentSiteData(CONFIG_KEY_2);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        config.set_bool(CONFIG_CONFISCATE, true);
    }
    if (!config2.isValid())
    {
        DEBUG(control, out).print("no extended config found in this save; initializing\n");
        config2 = World::AddPersistentSiteData(CONFIG_KEY_2);
        config2.set_bool(CONFIG_AUTOMATE_DYE, false);
    }
    // transition existing saves to CONFIG_CONFISCATE=true
    if (config.get_int(CONFIG_CONFISCATE) < 0) {
        DEBUG(control,out).print("found existing configuration with CONFIG_CONFISCATE unset, initializing to true\n");
        config.set_bool(CONFIG_CONFISCATE, true);
    }

    // transition existing saves to CONFIG_AUTOMATE_DYE=false
    if (config2.get_int(CONFIG_AUTOMATE_DYE) < 0)
    {
        DEBUG(control, out).print("found existing configuration with CONFIG_AUTOMATE_DYE unset, initializing to false\n");
        config2.set_bool(CONFIG_AUTOMATE_DYE, false);
    }

    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    DEBUG(control,out).print("loading persisted enabled state: {}\n",
                            is_enabled ? "true" : "false");
    tailor_instance->set_confiscate(config.get_bool(CONFIG_CONFISCATE));
    DEBUG(control,out).print("loading persisted confiscation state: {}\n",
                            tailor_instance->get_confiscate() ? "true" : "false");
    tailor_instance->set_automate_dye(config2.get_bool(CONFIG_AUTOMATE_DYE));
    DEBUG(control, out).print("loading persisted dye automation state: {}\n",
                            tailor_instance->get_automate_dye() ? "true" : "false");

    tailor_instance->sync_material_order();

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(control,out).print("world unloaded; disabling {}\n",
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
            out.print("tailor: ordered {} items of clothing\n", ordered);
    }
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run {} without a loaded fort.\n", plugin_name);
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

    DEBUG(cycle,out).print("running {} cycle\n", plugin_name);

    return tailor_instance->do_cycle();
}

/////////////////////////////////////////////////////
// Lua API
//

static void tailor_doCycle(color_ostream &out) {
    DEBUG(control,out).print("entering tailor_doCycle\n");
    out.print("ordered {} items of clothing\n", do_cycle(out));
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
    DEBUG(control,out).print("{} confiscation of tattered clothing \n", enable ? "enabling" : "disabling");
    config.set_bool(CONFIG_CONFISCATE, enable);
    tailor_instance->set_confiscate(enable);
}

static bool tailor_getConfiscate(color_ostream& out)
{
    return tailor_instance->get_confiscate();
}

static void tailor_setAutomateDye(color_ostream& out, bool enable)
{
    DEBUG(control, out).print("{} automation of dye\n", enable ? "enabling" : "disabling");
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
