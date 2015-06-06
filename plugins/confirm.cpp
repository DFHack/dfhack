#include <set>
#include <map>
#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "VTableInterpose.h"
#include "uicommon.h"
#include "df/viewscreen_tradegoodsst.h"

using namespace DFHack;

DFHACK_PLUGIN("confirm");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(gps);

typedef std::set<df::interface_key> ikey_set;
command_result df_confirm (color_ostream &out, std::vector <std::string> & parameters);

static std::multimap<std::string, VMethodInterposeLinkBase> hooks;

#define IMPLEMENT_CONFIRMATION_HOOKS(cls) \
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
IMPLEMENT_VMETHOD_INTERPOSE(cls##_hooks, feed); \
IMPLEMENT_VMETHOD_INTERPOSE(cls##_hooks, render); \
IMPLEMENT_VMETHOD_INTERPOSE(cls##_hooks, key_conflict);

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
        static std::vector<std::string> lines;
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
            std::string title = " " + get_title() + " ";
            Screen::paintString(Screen::Pen(' ', COLOR_BLACK, COLOR_GREY),
                (gps->dimx / 2) - (title.size() / 2), y1, title);
            int x = x1 + 2;
            OutputString(COLOR_LIGHTGREEN, x, y2, Screen::getKeyDisplay(df::interface_key::LEAVESCREEN));
            OutputString(COLOR_WHITE, x, y2, ": Cancel");
            x = x2 - 2 - 4 - Screen::getKeyDisplay(df::interface_key::SELECT).size();
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
    virtual std::string get_title() { return "Confirm"; }
    virtual std::string get_message() = 0;
    virtual UIColor get_color() { return COLOR_YELLOW; }
protected:
    cstate state;
    df::interface_key last_key;
};

class trade_seize_confirmation : public confirmation<df::viewscreen_tradegoodsst> {
public:
    virtual bool intercept_key (df::interface_key key) { return key == df::interface_key::TRADE_SEIZE; }
    virtual std::string get_id() { return "trade-seize"; }
    virtual std::string get_title() { return "Confirm seize"; }
    virtual std::string get_message() { return "Are you sure you want to sieze these goods?"; }
};
IMPLEMENT_CONFIRMATION_HOOKS(trade_seize_confirmation);

class trade_offer_confirmation : public confirmation<df::viewscreen_tradegoodsst> {
public:
    virtual bool intercept_key (df::interface_key key) { return key == df::interface_key::TRADE_OFFER; }
    virtual std::string get_id() { return "trade-offer"; }
    virtual std::string get_title() { return "Confirm offer"; }
    virtual std::string get_message() { return "Are you sure you want to offer these goods?\nYou will receive no payment."; }
};
IMPLEMENT_CONFIRMATION_HOOKS(trade_offer_confirmation);

#define CHOOK(cls) \
    HOOK_ACTION(cls, render) \
    HOOK_ACTION(cls, feed) \
    HOOK_ACTION(cls, key_conflict)

#define CHOOKS \
    CHOOK(trade_seize_confirmation) \
    CHOOK(trade_offer_confirmation)

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    std::vector<std::string> hook_names;
#define HOOK_ACTION(cls, method) hooks.insert(std::pair<std::string, VMethodInterposeLinkBase>(cls##_instance.get_id(), INTERPOSE_HOOK(cls##_hooks, method))); \
        if (std::find(hook_names.begin(), hook_names.end(), cls##_instance.get_id()) == hook_names.end()) hook_names.push_back(cls##_instance.get_id());
    CHOOKS
#undef HOOK_ACTION
    std::string help =
        "  confirmation enable|disable option|all ...\n"
        "Available options:\n  " + join_strings(", ", hook_names);
    commands.push_back(PluginCommand(
        "confirm",
        "Confirmation dialogs",
        df_confirm,
        false, //allow non-interactive use
        help.c_str()
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable)
{
    if (is_enabled != enable)
    {
#define HOOK_ACTION(cls, method) !INTERPOSE_HOOK(cls##_hooks, method).apply(enable) ||
        if (CHOOKS 0)
            return CR_FAILURE;
#undef HOOK_ACTION
        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
#define HOOK_ACTION(cls, method) INTERPOSE_HOOK(cls##_hooks, method).remove();
    CHOOKS;
#undef HOOK_ACTION
    return CR_OK;
}

void enable_conf (color_ostream &out, std::string name, bool state)
{
    bool found = false;
    for (auto it = hooks.begin(); it != hooks.end(); ++it)
    {
        if (it->first == name)
        {
            found = true;
            it->second.apply(state);
        }
    }
    if (!found)
        out.printerr("Unrecognized option: %s\n", name.c_str());
}

command_result df_confirm (color_ostream &out, std::vector <std::string> & parameters)
{
    CoreSuspender suspend;
    bool state = true;
    for (auto it = parameters.begin(); it != parameters.end(); ++it)
    {
        if (*it == "enable")
            state = true;
        else if (*it == "disable")
            state = false;
        else if (*it == "all")
        {
            for (auto it = hooks.begin(); it != hooks.end(); ++it)
            {
                it->second.apply(state);
            }
        }
        else
            enable_conf(out, *it, state);
    }
    return CR_OK;
}
