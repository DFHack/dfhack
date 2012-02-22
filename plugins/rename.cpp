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

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::ui;
using df::global::world;

static command_result rename(Core * c, vector <string> & parameters);

DFHACK_PLUGIN("rename");

DFhackCExport command_result plugin_init (Core *c, std::vector <PluginCommand> &commands)
{
    commands.clear();
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

DFhackCExport command_result plugin_shutdown ( Core * c )
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

static command_result rename(Core * c, vector <string> &parameters)
{
    CoreSuspender suspend(c);

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
            c->con.printerr("Couldn't find squad with index %d.\n", id);
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
            c->con.printerr("Invalid hotkey index\n");
            return CR_WRONG_USAGE;
        }

        ui->main.hotkeys[id-1].name = parameters[2];
    }
    else if (cmd == "unit")
    {
        if (parameters.size() != 2)
            return CR_WRONG_USAGE;

        df::unit *unit = getSelectedUnit(c);
        if (!unit)
            return CR_WRONG_USAGE;

        // There are 3 copies of the name, and the one
        // in the unit is not the authoritative one.
        // This is the reason why military units often
        // lose nicknames set from Dwarf Therapist.
        set_nickname(&unit->name, parameters[1]);

        if (unit->status.current_soul)
            set_nickname(&unit->status.current_soul->name, parameters[1]);

        df::historical_figure *figure = df::historical_figure::find(unit->hist_figure_id);
        if (figure)
        {
            set_nickname(&figure->name, parameters[1]);

            // v0.34.01: added the vampire's assumed identity
            if (figure->info && figure->info->reputation)
            {
                auto identity = df::assumed_identity::find(figure->info->reputation->cur_identity);

                if (identity)
                    set_nickname(&identity->name, parameters[1]);
            }
        }
    }
    else if (cmd == "unit-profession")
    {
        if (parameters.size() != 2)
            return CR_WRONG_USAGE;

        df::unit *unit = getSelectedUnit(c);
        if (!unit)
            return CR_WRONG_USAGE;

        unit->custom_profession = parameters[1];
    }
    else
    {
        if (!parameters.empty() && cmd != "?")
            c->con.printerr("Invalid command: %s\n", cmd.c_str());
        return CR_WRONG_USAGE;
    }

    return CR_OK;
}
