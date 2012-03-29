#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Gui.h"

#include "DataDefs.h"
#include "df/ui.h"
#include "df/world.h"
#include "df/squad.h"
#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/assumed_identity.h"
#include "df/language_name.h"

#include "RemoteServer.h"
#include "rename.pb.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;
using namespace dfproto;

using df::global::ui;
using df::global::world;

static command_result rename(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("rename");

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (world && ui) {
        commands.push_back(PluginCommand(
            "rename", "Rename various things.", rename, false,
            "  rename squad <index> \"name\"\n"
            "  rename hotkey <index> \"name\"\n"
            "    (identified by ordinal index)\n"
            "  rename unit \"nickname\"\n"
            "  rename unit-profession \"custom profession\"\n"
            "    (a unit must be highlighted in the ui)\n"
        ));
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

static void set_nickname(df::language_name *name, std::string nick)
{
    if (!name->has_name)
    {
        *name = df::language_name();

        name->language = 0;
        name->has_name = true;
    }

    name->nickname = nick;
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

void setUnitNickname(df::unit *unit, const std::string &nick)
{
    // There are >=3 copies of the name, and the one
    // in the unit is not the authoritative one.
    // This is the reason why military units often
    // lose nicknames set from Dwarf Therapist.
    set_nickname(&unit->name, nick);

    if (unit->status.current_soul)
        set_nickname(&unit->status.current_soul->name, nick);

    df::historical_figure *figure = df::historical_figure::find(unit->hist_figure_id);
    if (figure)
    {
        set_nickname(&figure->name, nick);

        // v0.34.01: added the vampire's assumed identity
        if (figure->info && figure->info->reputation)
        {
            auto identity = df::assumed_identity::find(figure->info->reputation->cur_identity);

            if (identity)
            {
                auto id_hfig = df::historical_figure::find(identity->histfig_id);

                if (id_hfig)
                {
                    // Even DF doesn't do this bit, because it's apparently
                    // only used for demons masquerading as gods, so you
                    // can't ever change their nickname in-game.
                    set_nickname(&id_hfig->name, nick);
                }
                else
                    set_nickname(&identity->name, nick);
            }
        }
    }
}

static command_result RenameSquad(color_ostream &stream, const RenameSquadIn *in)
{
    df::squad *squad = df::squad::find(in->squad_id());
    if (!squad)
        return CR_NOT_FOUND;

    if (in->has_nickname())
        set_nickname(&squad->name, in->nickname());
    if (in->has_alias())
        squad->alias = in->alias();

    return CR_OK;
}

static command_result RenameUnit(color_ostream &stream, const RenameUnitIn *in)
{
    df::unit *unit = df::unit::find(in->unit_id());
    if (!unit)
        return CR_NOT_FOUND;

    if (in->has_nickname())
        setUnitNickname(unit, in->nickname());
    if (in->has_profession())
        unit->custom_profession = in->profession();

    return CR_OK;
}

DFhackCExport RPCService *plugin_rpcconnect(color_ostream &)
{
    RPCService *svc = new RPCService();
    svc->addFunction("RenameSquad", RenameSquad);
    svc->addFunction("RenameUnit", RenameUnit);
    return svc;
}

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

        df::unit *unit = Gui::getSelectedUnit(out);
        if (!unit)
            return CR_WRONG_USAGE;

        setUnitNickname(unit, parameters[1]);
    }
    else if (cmd == "unit-profession")
    {
        if (parameters.size() != 2)
            return CR_WRONG_USAGE;

        df::unit *unit = Gui::getSelectedUnit(out);
        if (!unit)
            return CR_WRONG_USAGE;

        unit->custom_profession = parameters[1];
    }
    else
    {
        if (!parameters.empty() && cmd != "?")
            out.printerr("Invalid command: %s\n", cmd.c_str());
        return CR_WRONG_USAGE;
    }

    return CR_OK;
}
