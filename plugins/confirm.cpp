#include <set>
#include <map>
#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "VTableInterpose.h"
#include "uicommon.h"
#include "modules/Gui.h"

#include "df/building_tradedepotst.h"
#include "df/general_ref.h"
#include "df/general_ref_contained_in_itemst.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_layer_militaryst.h"
#include "df/viewscreen_tradegoodsst.h"

using namespace DFHack;
using namespace df::enums;
using std::string;
using std::vector;

DFHACK_PLUGIN("confirm");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(ui);

typedef std::set<df::interface_key> ikey_set;
command_result df_confirm (color_ostream &out, vector <string> & parameters);

struct conf_wrapper;
static std::map<std::string, conf_wrapper*> confirmations;

template <typename VT, typename FT>
inline bool in_vector (std::vector<VT> &vec, FT item)
{
    return std::find(vec.begin(), vec.end(), item) != vec.end();
}

namespace trade {
    static bool goods_selected (const std::vector<char> &selected)
    {
        for (auto it = selected.begin(); it != selected.end(); ++it)
            if (*it)
                return true;
        return false;
    }
    inline bool trader_goods_selected (df::viewscreen_tradegoodsst *screen)
    {
        return goods_selected(screen->trader_selected);
    }
    inline bool broker_goods_selected (df::viewscreen_tradegoodsst *screen)
    {
        return goods_selected(screen->broker_selected);
    }

    static bool goods_all_selected(const std::vector<char> &selected, const std::vector<df::item*> &items)  \
    {
        for (size_t i = 0; i < selected.size(); ++i)
        {
            if (!selected[i])
            {
                // check to see if item is in a container
                // (if the container is not selected, it will be detected separately)
                std::vector<df::general_ref*> &refs = items[i]->general_refs;
                bool in_container = false;
                for (auto it = refs.begin(); it != refs.end(); ++it)
                {
                    if (virtual_cast<df::general_ref_contained_in_itemst>(*it))
                    {
                        in_container = true;
                        break;
                    }
                }
                if (!in_container)
                    return false;
            }
        }
        return true;
    }
    inline bool trader_goods_all_selected(df::viewscreen_tradegoodsst *screen)
    {
        return goods_all_selected(screen->trader_selected, screen->trader_items);
    }
    inline bool broker_goods_all_selected(df::viewscreen_tradegoodsst *screen)
    {
        return goods_all_selected(screen->broker_selected, screen->broker_items);
    }
}

template <class T>
class confirmation {
public:
    enum cstate { INACTIVE, ACTIVE, SELECTED };
    typedef T screen_type;
    screen_type *screen;
    bool feed (ikey_set *input) {
        if (state == INACTIVE)
        {
            for (auto it = input->begin(); it != input->end(); ++it)
            {
                if (intercept_key(*it))
                {
                    last_key = *it;
                    state = ACTIVE;
                    return true;
                }
            }
            return false;
        }
        else if (state == ACTIVE)
        {
            if (input->count(df::interface_key::LEAVESCREEN))
                state = INACTIVE;
            else if (input->count(df::interface_key::SELECT))
                state = SELECTED;
            return true;
        }
        return false;
    }
    bool key_conflict (df::interface_key key)
    {
        if (key == df::interface_key::SELECT || key == df::interface_key::LEAVESCREEN)
            return false;
        return state == ACTIVE;
    }
    void render() {
        static vector<string> lines;
        Screen::Pen corner_ul = Screen::Pen((char)201, COLOR_GREY, COLOR_BLACK);
        Screen::Pen corner_ur = Screen::Pen((char)187, COLOR_GREY, COLOR_BLACK);
        Screen::Pen corner_dl = Screen::Pen((char)200, COLOR_GREY, COLOR_BLACK);
        Screen::Pen corner_dr = Screen::Pen((char)188, COLOR_GREY, COLOR_BLACK);
        Screen::Pen border_ud = Screen::Pen((char)205, COLOR_GREY, COLOR_BLACK);
        Screen::Pen border_lr = Screen::Pen((char)186, COLOR_GREY, COLOR_BLACK);
        if (state == ACTIVE)
        {
            split_string(&lines, get_message(), "\n");
            size_t max_length = 30;
            for (auto it = lines.begin(); it != lines.end(); ++it)
                max_length = std::max(max_length, it->size());
            int width = max_length + 4;
            int height = lines.size() + 4;
            int x1 = (gps->dimx / 2) - (width / 2);
            int x2 = x1 + width - 1;
            int y1 = (gps->dimy / 2) - (height / 2);
            int y2 = y1 + height - 1;
            for (int x = x1; x <= x2; x++)
            {
                Screen::paintTile(border_ud, x, y1);
                Screen::paintTile(border_ud, x, y2);
            }
            for (int y = y1; y <= y2; y++)
            {
                Screen::paintTile(border_lr, x1, y);
                Screen::paintTile(border_lr, x2, y);
            }
            Screen::paintTile(corner_ul, x1, y1);
            Screen::paintTile(corner_ur, x2, y1);
            Screen::paintTile(corner_dl, x1, y2);
            Screen::paintTile(corner_dr, x2, y2);
            string title = " " + get_title() + " ";
            Screen::paintString(Screen::Pen(' ', COLOR_DARKGREY, COLOR_BLACK),
                x2 - 6, y1, "DFHack");
            Screen::paintString(Screen::Pen(' ', COLOR_BLACK, COLOR_GREY),
                (gps->dimx / 2) - (title.size() / 2), y1, title);
            int x = x1 + 2;
            OutputString(COLOR_LIGHTGREEN, x, y2, Screen::getKeyDisplay(df::interface_key::LEAVESCREEN));
            OutputString(COLOR_WHITE, x, y2, ": Cancel");
            x = x2 - 2 - 3 - Screen::getKeyDisplay(df::interface_key::SELECT).size();
            OutputString(COLOR_LIGHTGREEN, x, y2, Screen::getKeyDisplay(df::interface_key::SELECT));
            OutputString(COLOR_WHITE, x, y2, ": Ok");
            Screen::fillRect(Screen::Pen(' ', COLOR_BLACK, COLOR_BLACK), x1 + 1, y1 + 1, x2 - 1, y2 - 1);
            for (size_t i = 0; i < lines.size(); i++)
            {
                Screen::paintString(Screen::Pen(' ', get_color(), COLOR_BLACK), x1 + 2, y1 + 2 + i, lines[i]);
            }
        }
        else if (state == SELECTED)
        {
            ikey_set tmp;
            tmp.insert(last_key);
            screen->feed(&tmp);
            state = INACTIVE;
        }
    }
    virtual bool intercept_key (df::interface_key key) = 0;
    virtual string get_id() = 0;
    virtual string get_title() { return "Confirm"; }
    virtual string get_message() = 0;
    virtual UIColor get_color() { return COLOR_YELLOW; }
protected:
    cstate state;
    df::interface_key last_key;
};

struct conf_wrapper {
private:
    bool enabled;
    std::set<VMethodInterposeLinkBase*> hooks;
public:
    conf_wrapper()
        :enabled(false)
    {}
    void add_hook(VMethodInterposeLinkBase *hook)
    {
        if (!hooks.count(hook))
            hooks.insert(hook);
    }
    bool apply (bool state) {
        if (state == enabled)
            return true;
        for (auto h = hooks.begin(); h != hooks.end(); ++h)
        {
            if (!(**h).apply(state))
                return false;
        }
        enabled = state;
        return true;
    }
    inline bool is_enabled() { return enabled; }
};

template<typename T>
int conf_register(confirmation<T> *c, ...)
{
    conf_wrapper *w = new conf_wrapper;
    confirmations[c->get_id()] = w;
    va_list args;
    va_start(args, c);
    while (VMethodInterposeLinkBase *hook = va_arg(args, VMethodInterposeLinkBase*))
        w->add_hook(hook);
    va_end(args);
    return 0;
}

#define IMPLEMENT_CONFIRMATION_HOOKS(cls) IMPLEMENT_CONFIRMATION_HOOKS_PRIO(cls, 0)
#define IMPLEMENT_CONFIRMATION_HOOKS_PRIO(cls, prio) \
static cls cls##_instance; \
struct cls##_hooks : cls::screen_type { \
    typedef cls::screen_type interpose_base; \
    DEFINE_VMETHOD_INTERPOSE(void, feed, (ikey_set *input)) \
    { \
        cls##_instance.screen = this; \
        if (!cls##_instance.feed(input)) \
            INTERPOSE_NEXT(feed)(input); \
    } \
    DEFINE_VMETHOD_INTERPOSE(void, render, ()) \
    { \
        cls##_instance.screen = this; \
        INTERPOSE_NEXT(render)(); \
        cls##_instance.render(); \
    } \
    DEFINE_VMETHOD_INTERPOSE(bool, key_conflict, (df::interface_key key)) \
    { \
        return cls##_instance.key_conflict(key) || INTERPOSE_NEXT(key_conflict)(key); \
    } \
}; \
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(cls##_hooks, feed, prio); \
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(cls##_hooks, render, prio); \
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(cls##_hooks, key_conflict, prio); \
static int conf_register_##cls = conf_register(&cls##_instance, \
    &INTERPOSE_HOOK(cls##_hooks, feed), \
    &INTERPOSE_HOOK(cls##_hooks, render), \
    &INTERPOSE_HOOK(cls##_hooks, key_conflict), \
    NULL);

class trade_confirmation : public confirmation<df::viewscreen_tradegoodsst> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        return key == df::interface_key::TRADE_TRADE;
    }
    virtual string get_id() { return "trade"; }
    virtual string get_title() { return "Confirm trade"; }
    virtual string get_message()
    {
        if (trade::trader_goods_selected(screen) && trade::broker_goods_selected(screen))
            return "Are you sure you want to trade the selected goods?";
        else if (trade::trader_goods_selected(screen))
            return "You are not giving any items. This is likely\n"
                "to irritate the merchants.\n"
                "Attempt to trade anyway?";
        else if (trade::broker_goods_selected(screen))
            return "You are not receiving any items. You may want to\n"
                "offer these items instead or choose items to receive.\n"
                "Attempt to trade anyway?";
        else
            return "No items are selected. This is likely\n"
                "to irritate the merchants.\n"
                "Attempt to trade anyway?";
    }
};
IMPLEMENT_CONFIRMATION_HOOKS(trade_confirmation);

class trade_cancel_confirmation : public confirmation<df::viewscreen_tradegoodsst> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        return key == df::interface_key::LEAVESCREEN &&
            (trade::trader_goods_selected(screen) || trade::broker_goods_selected(screen));
    }
    virtual string get_id() { return "trade-cancel"; }
    virtual string get_title() { return "Cancel trade"; }
    virtual string get_message() { return "Are you sure you want leave this screen?\nSelected items will not be saved."; }
};
IMPLEMENT_CONFIRMATION_HOOKS_PRIO(trade_cancel_confirmation, -1);

class trade_seize_confirmation : public confirmation<df::viewscreen_tradegoodsst> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        return trade::trader_goods_selected(screen) && key == df::interface_key::TRADE_SEIZE;
    }
    virtual string get_id() { return "trade-seize"; }
    virtual string get_title() { return "Confirm seize"; }
    virtual string get_message() { return "Are you sure you want to sieze these goods?"; }
};
IMPLEMENT_CONFIRMATION_HOOKS(trade_seize_confirmation);

class trade_offer_confirmation : public confirmation<df::viewscreen_tradegoodsst> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        return trade::broker_goods_selected(screen) && key == df::interface_key::TRADE_OFFER;
    }
    virtual string get_id() { return "trade-offer"; }
    virtual string get_title() { return "Confirm offer"; }
    virtual string get_message() { return "Are you sure you want to offer these goods?\nYou will receive no payment."; }
};
IMPLEMENT_CONFIRMATION_HOOKS(trade_offer_confirmation);

class trade_select_all_confirmation : public confirmation<df::viewscreen_tradegoodsst> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        if (key == df::interface_key::SEC_SELECT)
        {
            if (screen->in_right_pane && trade::broker_goods_selected(screen) && !trade::broker_goods_all_selected(screen))
                return true;
            else if (!screen->in_right_pane && trade::trader_goods_selected(screen) && !trade::trader_goods_all_selected(screen))
                return true;
        }
        return false;
    }
    virtual string get_id() { return "trade-select-all"; }
    virtual string get_title() { return "Confirm selection"; }
    virtual string get_message()
    {
        return "Selecting all goods will overwrite your current selection\n"
            "and cannot be undone. Continue?";
    }
};
IMPLEMENT_CONFIRMATION_HOOKS(trade_select_all_confirmation);

class hauling_route_delete_confirmation : public confirmation<df::viewscreen_dwarfmodest> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        if (ui->main.mode == ui_sidebar_mode::Hauling &&
            ui->hauling.view_routes.size() &&
            !ui->hauling.in_name &&
            !ui->hauling.in_stop &&
            !ui->hauling.in_assign_vehicle
        )
            return key == df::interface_key::D_HAULING_REMOVE;
        return false;
    }
    virtual string get_id() { return "haul-delete"; }
    virtual string get_title() { return "Confirm deletion"; }
    virtual string get_message()
    {
        std::string type = (ui->hauling.view_stops[ui->hauling.cursor_top]) ? "stop" : "route";
        return std::string("Are you sure you want to delete this ") + type + "?";
    }
};
IMPLEMENT_CONFIRMATION_HOOKS(hauling_route_delete_confirmation);

class depot_remove_confirmation : public confirmation<df::viewscreen_dwarfmodest> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        df::building_tradedepotst *depot = virtual_cast<df::building_tradedepotst>(Gui::getAnyBuilding(screen));
        if (depot && key == df::interface_key::DESTROYBUILDING)
        {
            for (auto it = ui->caravans.begin(); it != ui->caravans.end(); ++it)
            {
                if ((**it).time_remaining)
                    return true;
            }
        }
        return false;
    }
    virtual string get_id() { return "depot-remove"; }
    virtual string get_title() { return "Confirm depot removal"; }
    virtual string get_message()
    {
        return "Are you sure you want to remove this depot?\n"
            "Merchants are present and will lose profits.";
    }
};
IMPLEMENT_CONFIRMATION_HOOKS(depot_remove_confirmation);

class squad_disband_confirmation : public confirmation<df::viewscreen_layer_militaryst> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        return screen->page == df::viewscreen_layer_militaryst::T_page::Positions &&
            screen->num_squads &&
            !screen->in_rename_alert &&
            key == df::interface_key::D_MILITARY_DISBAND_SQUAD;
    }
    virtual string get_id() { return "squad-disband"; }
    virtual string get_title() { return "Disband squad"; }
    virtual string get_message() { return "Are you sure you want to disband this squad?"; }
};
IMPLEMENT_CONFIRMATION_HOOKS(squad_disband_confirmation);

class uniform_delete_confirmation : public confirmation<df::viewscreen_layer_militaryst> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        return screen->page == df::viewscreen_layer_militaryst::T_page::Uniforms &&
            !screen->equip.uniforms.empty() &&
            !screen->equip.in_name_uniform &&
            key == df::interface_key::D_MILITARY_DELETE_UNIFORM;
    }
    virtual string get_id() { return "uniform-delete"; }
    virtual string get_title() { return "Delete uniform"; }
    virtual string get_message() { return "Are you sure you want to delete this uniform?"; }
};
IMPLEMENT_CONFIRMATION_HOOKS(uniform_delete_confirmation);

class note_delete_confirmation : public confirmation<df::viewscreen_dwarfmodest> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        return ui->main.mode == ui_sidebar_mode::NotesPoints && key == df::interface_key::D_NOTE_DELETE;
    }
    virtual string get_id() { return "note-delete"; }
    virtual string get_title() { return "Delete note"; }
    virtual string get_message() { return "Are you sure you want to delete this note?"; }
};
IMPLEMENT_CONFIRMATION_HOOKS(note_delete_confirmation);

class route_delete_confirmation : public confirmation<df::viewscreen_dwarfmodest> {
public:
    virtual bool intercept_key (df::interface_key key)
    {
        return ui->main.mode == ui_sidebar_mode::NotesRoutes && key == df::interface_key::D_NOTE_ROUTE_DELETE;
    }
    virtual string get_id() { return "route-delete"; }
    virtual string get_title() { return "Delete route"; }
    virtual string get_message() { return "Are you sure you want to delete this route?"; }
};
IMPLEMENT_CONFIRMATION_HOOKS(route_delete_confirmation);

DFhackCExport command_result plugin_init (color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "confirm",
        "Confirmation dialogs",
        df_confirm,
        false, //allow non-interactive use

        "  confirmation enable|disable option|all ...\n"
        "  confirmation help|status\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_enable (color_ostream &out, bool enable)
{
    if (is_enabled != enable)
    {
        for (auto c = confirmations.begin(); c != confirmations.end(); ++c)
        {
            if (!c->second->apply(enable))
                return CR_FAILURE;
        }
        is_enabled = enable;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    if (plugin_enable(out, false) != CR_OK)
        return CR_FAILURE;
    return CR_OK;
}

void enable_conf (color_ostream &out, string name, bool state)
{
    bool found = false;
    for (auto it = confirmations.begin(); it != confirmations.end(); ++it)
    {
        if (it->first == name)
        {
            found = true;
            it->second->apply(state);
        }
    }
    if (!found)
        out.printerr("Unrecognized option: %s\n", name.c_str());
}

command_result df_confirm (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;
    bool state = true;
    if (in_vector(parameters, "help") || in_vector(parameters, "status"))
    {
        out << "Available options: \n";
        for (auto it = confirmations.begin(); it != confirmations.end(); ++it)
        {
            out << "  " << it->first << ": " << (it->second->is_enabled() ? "enabled" : "disabled") << std::endl;
        }
        return CR_OK;
    }
    for (auto it = parameters.begin(); it != parameters.end(); ++it)
    {
        if (*it == "enable")
            state = true;
        else if (*it == "disable")
            state = false;
        else if (*it == "all")
        {
            for (auto it = confirmations.begin(); it != confirmations.end(); ++it)
            {
                it->second->apply(state);
            }
        }
        else
            enable_conf(out, *it, state);
    }
    return CR_OK;
}
