/*
 * Tailor plugin. Automatically manages keeping your dorfs clothed.
 * For best effect, place "tailor enable" in your dfhack.init configuration,
 * or set AUTOENABLE to true.
 */

#include "Core.h"
#include "DataDefs.h"
#include "PluginManager.h"

#include "df/creature_raw.h"
#include "df/global_objects.h"
#include "df/historical_entity.h"
#include "df/itemdef_armorst.h"
#include "df/itemdef_glovesst.h"
#include "df/itemdef_helmst.h"
#include "df/itemdef_pantsst.h"
#include "df/itemdef_shoesst.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/job_type.h"
#include "df/manager_order.h"
#include "df/ui.h"
#include "df/world.h"

#include "modules/Maps.h"
#include "modules/Units.h"
#include "modules/Translation.h"
#include "modules/World.h"

using namespace DFHack;
using namespace std;

using df::global::world;
using df::global::ui;

DFHACK_PLUGIN("tailor");
#define AUTOENABLE false
DFHACK_PLUGIN_IS_ENABLED(enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);

class Tailor {
    // ARMOR, SHOES, HELM, GLOVES, PANTS

    // ah, if only STL had a bimap

private:

    const map<df::job_type, df::item_type> jobTypeMap = {
        { df::job_type::MakeArmor, df::item_type::ARMOR },
        { df::job_type::MakePants, df::item_type::PANTS },
        { df::job_type::MakeHelm, df::item_type::HELM },
        { df::job_type::MakeGloves, df::item_type::GLOVES },
        { df::job_type::MakeShoes, df::item_type::SHOES }
    };

    const map<df::item_type, df::job_type> itemTypeMap = {
        { df::item_type::ARMOR, df::job_type::MakeArmor },
        { df::item_type::PANTS, df::job_type::MakePants },
        { df::item_type::HELM, df::job_type::MakeHelm },
        { df::item_type::GLOVES, df::job_type::MakeGloves },
        { df::item_type::SHOES, df::job_type::MakeShoes }
    };

#define F(x) df::item_flags::mask_##x
    const df::item_flags bad_flags = {
        (
        F(dump) | F(forbid) | F(garbage_collect) |
        F(hostile) | F(on_fire) | F(rotten) | F(trader) |
        F(in_building) | F(construction) | F(owned)
        )
    #undef F
    };

    class MatType {

    public:
        std::string name;
        df::job_material_category job_material;
        df::armor_general_flags armor_flag;

        bool operator==(const MatType& m) const
        {
            return name == m.name;
        }

        // operator< is required to use this as a std::map key
        bool operator<(const MatType& m) const
        {
            return name < m.name;
        }

        MatType(std::string& n, df::job_material_category jm, df::armor_general_flags af)
            : name(n), job_material(jm), armor_flag(af) {};
        MatType(const char* n, df::job_material_category jm, df::armor_general_flags af)
            : name(std::string(n)), job_material(jm), armor_flag(af) {};

    };

    const MatType
        M_SILK = MatType("silk", df::job_material_category::mask_silk, df::armor_general_flags::SOFT),
        M_CLOTH = MatType("cloth", df::job_material_category::mask_cloth, df::armor_general_flags::SOFT),
        M_YARN = MatType("yarn", df::job_material_category::mask_yarn, df::armor_general_flags::SOFT),
        M_LEATHER = MatType("leather", df::job_material_category::mask_leather, df::armor_general_flags::LEATHER);

    std::list<MatType> all_materials = { M_SILK, M_CLOTH, M_YARN, M_LEATHER };

    map<pair<df::item_type, int>, int> available; // key is item type & size
    map<pair<df::item_type, int>, int> needed;    // same
    map<pair<df::item_type, int>, int> queued;    // same

    map<int, int> sizes; // this maps body size to races

    map<tuple<df::job_type, int, int>, int> orders;  // key is item type, item subtype, size

    std::map<MatType, int> supply;

    color_ostream* out;

    std::list<MatType> material_order = { M_SILK, M_CLOTH, M_YARN, M_LEATHER };
    std::map<MatType, int> reserves;

    int default_reserve = 10;

    void reset()
    {
        available.clear();
        needed.clear();
        queued.clear();
        sizes.clear();
        orders.clear();
        supply.clear();
    }

    void scan_clothing()
    {
        for (auto i : world->items.other[df::items_other_id::ANY_GENERIC37]) // GENERIC37 is "clothing"
        {
            if (i->flags.whole & bad_flags.whole)
                continue;
            if (i->flags.bits.owned)
                continue;
            if (i->getWear() >= 1)
                continue;
            df::item_type t = i->getType();
            int size = world->raws.creatures.all[i->getMakerRace()]->adultsize;

            available[make_pair(t, size)] += 1;
        }
    }

    void scan_materials()
    {
        for (auto i : world->items.other[df::items_other_id::CLOTH])
        {
            if (i->flags.whole & bad_flags.whole)
                continue;
            if (!i->hasImprovements()) // only count dyed
                continue;
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
            }
        }

        for (auto i : world->items.other[df::items_other_id::SKIN_TANNED])
        {
            if (i->flags.whole & bad_flags.whole)
                continue;
            supply[M_LEATHER] += i->getStackSize();
        }

        out->print("tailor: available silk %d yarn %d cloth %d leather %d\n", supply[M_SILK], supply[M_YARN], supply[M_CLOTH], supply[M_LEATHER]);
    }

    void scan_replacements()
    {
        for (auto u : world->units.active)
        {
            if (!Units::isOwnCiv(u) ||
                !Units::isOwnGroup(u) ||
                !Units::isActive(u) ||
                Units::isBaby(u))
                continue; // skip units we don't control

            set <df::item_type> wearing;
            wearing.clear();

            deque<df::item*> worn;
            worn.clear();

            for (auto inv : u->inventory)
            {
                if (inv->mode != df::unit_inventory_item::Worn)
                    continue;
                if (inv->item->getWear() > 0)
                    worn.push_back(inv->item);
                else
                    wearing.insert(inv->item->getType());
            }

            int size = world->raws.creatures.all[u->race]->adultsize;
            sizes[size] = u->race;

            for (auto ty : set<df::item_type>{ df::item_type::ARMOR, df::item_type::PANTS, df::item_type::SHOES })
            {
                if (wearing.count(ty) == 0)
                    needed[make_pair(ty, size)] += 1;
            }

            for (auto w : worn)
            {
                auto ty = w->getType();
                auto o = itemTypeMap.at(ty);

                int size = world->raws.creatures.all[w->getMakerRace()]->adultsize;
                std::string description;
                w->getItemDescription(&description, 0);

                if (available[make_pair(ty, size)] > 0)
                {
                    if (w->flags.bits.owned)
                    {
                        bool confiscated = Items::setOwner(w, NULL);

                        out->print(
                            "tailor: %s %s from %s.\n",
                            (confiscated ? "confiscated" : "could not confiscate"),
                            description.c_str(),
                            Translation::TranslateName(&u->name, false).c_str()
                        );
                    }

                    if (wearing.count(ty) == 0)
                        available[make_pair(ty, size)] -= 1;

                    if (w->getWear() > 1)
                        w->flags.bits.dump = true;
                }
                else
                {
                    //                out->print("%s worn by %s needs replacement\n",
                    //                    description.c_str(),
                    //                    Translation::TranslateName(&u->name, false).c_str()
                    //                );
                    orders[make_tuple(o, w->getSubtype(), size)] += 1;
                }
            }
        }
    }

    void create_orders()
    {
        auto entity = world->entities.all[ui->civ_id];

        for (auto& a : needed)
        {
            df::item_type ty = a.first.first;
            int size = a.first.second;
            int count = a.second;

            int sub = 0;
            vector<int16_t> v;

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

            const df::job_type j = itemTypeMap.at(ty);
            orders[make_tuple(j, sub, size)] += count;
        }
    }

    void scan_existing_orders()
    {
        for (auto o : world->manager_orders)
        {
            auto f = jobTypeMap.find(o->job_type);
            if (f == jobTypeMap.end())
                continue;

            auto sub = o->item_subtype;
            int race = o->hist_figure_id;
            if (race == -1)
                continue; // -1 means that the race of the worker will determine the size made; we must ignore these jobs

            int size = world->raws.creatures.all[race]->adultsize;

            orders[make_tuple(o->job_type, sub, size)] -= o->amount_left;
        }

    }

    void place_orders()
    {
        auto entity = world->entities.all[ui->civ_id];

        for (auto& o : orders)
        {
            df::job_type ty;
            int sub;
            int size;

            tie(ty, sub, size) = o.first;
            int count = o.second;

            if (count > 0)
            {
                vector<int16_t> v;
                BitArray<df::armor_general_flags>* fl;
                string name_s, name_p;

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
                    out->print("tailor: civilization cannot make %s, skipped\n", name_p.c_str());
                    continue;
                }

                for (auto& m : material_order)
                {
                    if (count <= 0)
                        break;

                    auto r = reserves.find(m);
                    int res = (r == reserves.end()) ? default_reserve : r->second;

                    if (supply[m] > res && fl->is_set(m.armor_flag)) {
                        int c = count;
                        if (supply[m] < count + res)
                            c = supply[m] - res;
                        supply[m] -= c;

                        auto order = new df::manager_order;
                        order->job_type = ty;
                        order->item_type = df::item_type::NONE;
                        order->item_subtype = sub;
                        order->mat_type = -1;
                        order->mat_index = -1;
                        order->amount_left = c;
                        order->amount_total = c;
                        order->status.bits.validated = false;
                        order->status.bits.active = false;
                        order->id = world->manager_order_next_id++;
                        order->hist_figure_id = sizes[size];
                        order->material_category = m.job_material;

                        world->manager_orders.push_back(order);

                        out->print("tailor: added order #%d for %d %s %s, sized for %s\n",
                            order->id,
                            c,
                            bitfield_to_string(order->material_category).c_str(),
                            (c > 1) ? name_p.c_str() : name_s.c_str(),
                            world->raws.creatures.all[order->hist_figure_id]->name[1].c_str()
                        );

                        count -= c;
                    }
                }
            }
        }
    }

public:
    void do_scan(color_ostream& o)
    {
        out = &o;

        reset();

        // scan for useable clothing

        scan_clothing();

        // scan for clothing raw materials

        scan_materials();

        // scan for units who need replacement clothing

        scan_replacements();

        // create new orders

        create_orders();

        // scan existing orders and subtract

        scan_existing_orders();

        // place orders

        place_orders();
    }

public:
    command_result set_materials(color_ostream& out, vector<string>& parameters)
    {
        list<MatType> newmat;
        newmat.clear();

        for (auto m = parameters.begin() + 1; m != parameters.end(); m++)
        {
            auto nameMatch = [m](MatType& m1) { return *m == m1.name; };
            auto mm = std::find_if(all_materials.begin(), all_materials.end(), nameMatch);
            if (mm == all_materials.end())
            {
                out.print("tailor: material %s not recognized\n", m->c_str());
                return CR_WRONG_USAGE;
            }
            else {
                newmat.push_back(*mm);
            }
        }

        material_order = newmat;
        out.print("tailor: material list set to %s\n", get_material_list().c_str());

        return CR_OK;
    }

public:
    std::string get_material_list()
    {
        std::string s;
        for (const auto& m : material_order)
        {
            if (!s.empty()) s += ", ";
            s += m.name;
        }
        return s;
    }

public:
    void process(color_ostream& out)
    {
        bool found = false;

        for (df::job_list_link* link = &world->jobs.list; link != NULL; link = link->next)
        {
            if (link->item == NULL) continue;
            if (link->item->job_type == df::enums::job_type::UpdateStockpileRecords)
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            do_scan(out);
        }
    }
};

static std::unique_ptr<Tailor> tailor_instance;

#define DELTA_TICKS 600

DFhackCExport command_result plugin_onupdate(color_ostream& out)
{
    if (!enabled || !tailor_instance)
        return CR_OK;

    if (!Maps::IsValid())
        return CR_OK;

    if (DFHack::World::ReadPauseState())
        return CR_OK;

    if (world->frame_counter % DELTA_TICKS != 0)
        return CR_OK;

    {
        CoreSuspender suspend;
        tailor_instance->process(out);
    }

    return CR_OK;
}

static command_result tailor_cmd(color_ostream& out, vector <string>& parameters) {
    bool desired = enabled;
    if (parameters.size() == 1 && (parameters[0] == "enable" || parameters[0] == "on" || parameters[0] == "1"))
    {
        desired = true;
    }
    else if (parameters.size() == 1 && (parameters[0] == "disable" || parameters[0] == "off" || parameters[0] == "0"))
    {
        desired = false;
    }
    else if (parameters.size() == 1 && (parameters[0] == "usage" || parameters[0] == "help" || parameters[0] == "?"))
    {
        return CR_WRONG_USAGE;
    }
    else if (parameters.size() == 1 && parameters[0] == "test")
    {
        if (tailor_instance)
        {
            tailor_instance->do_scan(out);
            return CR_OK;
        }
        else
        {
            out.print("%s: not instantiated\n", plugin_name);
            return CR_FAILURE;
        }
    }
    else if (parameters.size() > 1 && parameters[0] == "materials")
    {
        if (tailor_instance)
        {
            return tailor_instance->set_materials(out, parameters);
        }
        else
        {
            out.print("%s: not instantiated\n", plugin_name);
            return CR_FAILURE;
        }
    }
    else if (parameters.size() == 1 && parameters[0] != "status")
    {
        return CR_WRONG_USAGE;
    }

    out.print("Tailor is %s %s.\n", (desired == enabled) ? "currently" : "now", desired ? "enabled" : "disabled");
    if (tailor_instance)
    {
        out.print("Material list is: %s\n", tailor_instance->get_material_list().c_str());
    }
    else
    {
        out.print("%s: not instantiated\n", plugin_name);
    }

    enabled = desired;

    return CR_OK;
}


DFhackCExport command_result plugin_onstatechange(color_ostream& out, state_change_event event)
{
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream& out, bool enable)
{
    enabled = enable;
    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream& out, std::vector <PluginCommand>& commands)
{
    tailor_instance = std::move(dts::make_unique<Tailor>());

    if (AUTOENABLE) {
        enabled = true;
    }

    commands.push_back(PluginCommand(
        plugin_name,
        "Automatically keep your dwarves in fresh clothing.",
        tailor_cmd));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream& out)
{
    tailor_instance.release();

    return plugin_enable(out, false);
}
