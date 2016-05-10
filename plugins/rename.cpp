#include "Core.h"
#include "Console.h"
#include <Error.h>
#include "Export.h"
#include "MiscUtils.h"
#include "PluginManager.h"
#include <LuaTools.h>
#include <VTableInterpose.h>

#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/Persistent.h"
#include "modules/Screen.h"
#include "modules/Translation.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/building_furnacest.h"
#include "df/building_civzonest.h"
#include "df/building_siegeenginest.h"
#include "df/building_stockpilest.h"
#include "df/building_trapst.h"
#include "df/building_workshopst.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/identity.h"
#include "df/language_name.h"
#include "df/squad.h"
#include "df/ui.h"
#include "df/ui_sidebar_menus.h"
#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

#include "RemoteServer.h"
#include "rename.pb.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using std::cerr;
using namespace DFHack;
using namespace df::enums;
using namespace dfproto;

DFHACK_PLUGIN("rename");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_sidebar_menus);
REQUIRE_GLOBAL(world);

static const int32_t persist_version=1;
static void save_config(color_ostream& out);
static void load_config(color_ostream& out);
static void on_presave_callback(color_ostream& out, void* nothing) {
    save_config(out);
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event);

static command_result rename(color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "rename", "Rename various things.", rename, false,
        "  rename squad <index> \"name\"\n"
        "  rename hotkey <index> \"name\"\n"
        "    (identified by ordinal index)\n"
        "  rename unit \"nickname\"\n"
        "  rename unit-profession \"custom profession\"\n"
        "    (a unit must be highlighted in the ui)\n"
        "  rename building \"nickname\"\n"
        "    (a building must be highlighted via 'q')\n"
    ));

    EventManager::EventHandler handler(on_presave_callback, 1);
    EventManager::registerListener(EventManager::EventType::PRESAVE, handler, plugin_self);

    if (Core::getInstance().isWorldLoaded())
        plugin_onstatechange(out, SC_WORLD_LOADED);

    return CR_OK;
}

static void init_buildings(bool enable);

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_WORLD_LOADED:
        load_config(out);
        init_buildings(true);
        break;
    case SC_WORLD_UNLOADED:
        init_buildings(false);
        break;
    default:
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if ( DFHack::Core::getInstance().isWorldLoaded() )
        save_config(out);
    return CR_OK;
}

/*
 * Building renaming - it needs some per-type hacking.
 */

#define KNOWN_BUILDINGS \
    BUILDING('p', building_stockpilest, "Stockpile") \
    BUILDING('w', building_workshopst, NULL) \
    BUILDING('e', building_furnacest, NULL) \
    BUILDING('T', building_trapst, NULL) \
    BUILDING('i', building_siegeenginest, NULL) \
    BUILDING('Z', building_civzonest, "Zone")

#define BUILDING(code, cname, tag) \
    struct cname##_hook : df::cname { \
        typedef df::cname interpose_base; \
        DEFINE_VMETHOD_INTERPOSE(void, getName, (std::string *buf)) { \
            if (!name.empty()) {\
                buf->clear(); \
                *buf += name; \
                *buf += " ("; \
                if (tag) *buf += (const char*)tag; \
                else { std::string tmp; INTERPOSE_NEXT(getName)(&tmp); *buf += tmp; } \
                *buf += ")"; \
                return; \
            } \
            else \
                INTERPOSE_NEXT(getName)(buf); \
        } \
    }; \
    IMPLEMENT_VMETHOD_INTERPOSE_PRIO(cname##_hook, getName, 100);
KNOWN_BUILDINGS
#undef BUILDING

struct dwarf_render_zone_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        if (ui->main.mode == ui_sidebar_mode::Zones &&
            ui_sidebar_menus && ui_sidebar_menus->zone.selected &&
            !ui_sidebar_menus->zone.selected->name.empty())
        {
            auto dims = Gui::getDwarfmodeViewDims();
            int width = dims.menu_x2 - dims.menu_x1 - 1;

            Screen::Pen pen(' ',COLOR_WHITE);
            Screen::fillRect(pen, dims.menu_x1, dims.y1+1, dims.menu_x2, dims.y1+1);

            std::string name;
            ui_sidebar_menus->zone.selected->getName(&name);
            Screen::paintString(pen, dims.menu_x1+1, dims.y1+1, name.substr(0, width));
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dwarf_render_zone_hook, render);

static char getBuildingCode(df::building *bld)
{
    CHECK_NULL_POINTER(bld);

#define BUILDING(code, cname, tag) \
    if (strict_virtual_cast<df::cname>(bld)) return code;
KNOWN_BUILDINGS
#undef BUILDING

    return 0;
}

static bool enable_building_rename(char code, bool enable)
{
    if (enable)
        is_enabled = true;

    if (code == 'Z')
        INTERPOSE_HOOK(dwarf_render_zone_hook, render).apply(enable);

    switch (code) {
#define BUILDING(code, cname, tag) \
    case code: return INTERPOSE_HOOK(cname##_hook, getName).apply(enable);
KNOWN_BUILDINGS
#undef BUILDING
    default:
        return false;
    }
}

static void disable_building_rename()
{
    is_enabled = false;
    INTERPOSE_HOOK(dwarf_render_zone_hook, render).remove();

#define BUILDING(code, cname, tag) \
    INTERPOSE_HOOK(cname##_hook, getName).remove();
KNOWN_BUILDINGS
#undef BUILDING
}

static bool is_enabled_building(char code)
{
    switch (code) {
#define BUILDING(code, cname, tag) \
    case code: return INTERPOSE_HOOK(cname##_hook, getName).is_applied();
KNOWN_BUILDINGS
#undef BUILDING
    default:
        return false;
    }
}

static void load_config(color_ostream& out) {
    Json::Value& p = Persistent::get("rename");
    int32_t version = p["version"].isInt() ? p["version"].asInt() : 0;
    if ( version == 0 ) {
        auto entry = World::GetPersistentData("rename/building_types");
        if ( entry.isValid() ) {
            std::string val = entry.val();
            for ( size_t i = 0; i < val.size(); ++i ) {
                enable_building_rename(val[i],true);
            }
            World::DeletePersistentData(entry);
        }
    } else if ( version == 1 ) {
        Json::Value& building_types_node = p["building_types"];
        if ( !building_types_node.isArray() )
            return;
        for ( int32_t i = 0; i < building_types_node.size(); ++i ) {
            Json::Value& type_node = building_types_node[i];
            string typestr = type_node.asString();
            enable_building_rename(typestr[0],true);
        }
    } else {
        cerr << __FILE__ << ":" << __LINE__ << ": Unrecognized version: " << version << endl;
        exit(1);
    }
}
static void save_config(color_ostream& out) {
    Json::Value& p = Persistent::get("rename");
    p.clear();
    p["version"] = persist_version;
    Json::Value& types_node = p["building_types"];
    const char* chars = "pweTiZ";
    const int32_t len = strlen(chars);
    for ( int32_t i = 0; i < len; i++ ) {
        if ( is_enabled_building(chars[i]) )
            types_node.append(std::string(&chars[i],1));
    }
}

static void init_buildings(bool enable)
{
    disable_building_rename();

    if (enable)
    {
        /*auto entry = World::GetPersistentData("rename/building_types");

        if (entry.isValid())
        {
            std::string val = entry.val();
            for (size_t i = 0; i < val.size(); i++)
                enable_building_rename(val[i], true);
        }*/
    }
}

static bool canRenameBuilding(df::building *bld)
{
    return getBuildingCode(bld) != 0;
}

static bool isRenamingBuilding(df::building *bld)
{
    return is_enabled_building(getBuildingCode(bld));
}

static bool renameBuilding(df::building *bld, std::string name)
{
    char code = getBuildingCode(bld);
    if (code == 0 && !name.empty())
        return false;

    if (!name.empty() && !is_enabled_building(code))
    {
        auto entry = World::GetPersistentData("rename/building_types", NULL);
        if (!entry.isValid())
            return false;

        if (!enable_building_rename(code, true))
            return false;

        entry.val().push_back(code);
    }

    bld->name = name;
    return true;
}

static df::squad *getSquadByIndex(unsigned idx)
{
    auto entity = df::historical_entity::find(ui->group_id);
    if (!entity)
        return NULL;

    if (idx >= entity->squads.size())
        return NULL;

    return df::squad::find(entity->squads[idx]);
}

static command_result RenameSquad(color_ostream &stream, const RenameSquadIn *in)
{
    df::squad *squad = df::squad::find(in->squad_id());
    if (!squad)
        return CR_NOT_FOUND;

    if (in->has_nickname())
        Translation::setNickname(&squad->name, UTF2DF(in->nickname()));
    if (in->has_alias())
        squad->alias = UTF2DF(in->alias());

    return CR_OK;
}

static command_result RenameUnit(color_ostream &stream, const RenameUnitIn *in)
{
    df::unit *unit = df::unit::find(in->unit_id());
    if (!unit)
        return CR_NOT_FOUND;

    if (in->has_nickname())
        Units::setNickname(unit, UTF2DF(in->nickname()));
    if (in->has_profession())
        unit->custom_profession = UTF2DF(in->profession());

    return CR_OK;
}

static command_result RenameBuilding(color_ostream &stream, const RenameBuildingIn *in)
{
    auto building = df::building::find(in->building_id());
    if (!building)
        return CR_NOT_FOUND;

    if (in->has_name())
    {
        if (!renameBuilding(building, in->name()))
            return CR_FAILURE;
    }

    return CR_OK;
}

DFhackCExport RPCService *plugin_rpcconnect(color_ostream &)
{
    RPCService *svc = new RPCService();
    svc->addFunction("RenameSquad", RenameSquad);
    svc->addFunction("RenameUnit", RenameUnit);
    svc->addFunction("RenameBuilding", RenameBuilding);
    return svc;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(canRenameBuilding),
    DFHACK_LUA_FUNCTION(isRenamingBuilding),
    DFHACK_LUA_FUNCTION(renameBuilding),
    DFHACK_LUA_END
};

static command_result rename(color_ostream &out, vector <string> &parameters)
{
    CoreSuspender suspend;

    string cmd;
    if (!parameters.empty())
        cmd = parameters[0];

    if (cmd == "squad")
    {
        if (parameters.size() != 3)
            return CR_WRONG_USAGE;

        int id = atoi(parameters[1].c_str());
        df::squad *squad = getSquadByIndex(id-1);

        if (!squad) {
            out.printerr("Couldn't find squad with index %d.\n", id);
            return CR_WRONG_USAGE;
        }

        squad->alias = parameters[2];
    }
    else if (cmd == "hotkey")
    {
        if (parameters.size() != 3)
            return CR_WRONG_USAGE;

        int id = atoi(parameters[1].c_str());
        if (id < 1 || id > 16) {
            out.printerr("Invalid hotkey index\n");
            return CR_WRONG_USAGE;
        }

        ui->main.hotkeys[id-1].name = parameters[2];
    }
    else if (cmd == "unit")
    {
        if (parameters.size() != 2)
            return CR_WRONG_USAGE;

        df::unit *unit = Gui::getSelectedUnit(out, true);
        if (!unit)
            return CR_WRONG_USAGE;

        Units::setNickname(unit, parameters[1]);
    }
    else if (cmd == "unit-profession")
    {
        if (parameters.size() != 2)
            return CR_WRONG_USAGE;

        df::unit *unit = Gui::getSelectedUnit(out, true);
        if (!unit)
            return CR_WRONG_USAGE;

        unit->custom_profession = parameters[1];
    }
    else if (cmd == "building")
    {
        if (parameters.size() != 2)
            return CR_WRONG_USAGE;

        df::building *bld = Gui::getSelectedBuilding(out, true);
        if (!bld)
            return CR_WRONG_USAGE;

        if (!renameBuilding(bld, parameters[1]))
        {
            out.printerr("This type of building is not supported.\n");
            return CR_FAILURE;
        }
    }
    else
    {
        if (!parameters.empty() && cmd != "?")
            out.printerr("Invalid command: %s\n", cmd.c_str());
        return CR_WRONG_USAGE;
    }

    return CR_OK;
}
