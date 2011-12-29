#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>

#include <dfhack/DataDefs.h>
#include <dfhack/df/world.h>
#include <dfhack/df/ui.h>
#include <dfhack/df/building_stockpilest.h>
#include <dfhack/df/selection_rect.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;
using df::global::selection_rect;

using df::building_stockpilest;

DFhackCExport command_result copystock(Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "stockpiles";
}

DFhackCExport command_result plugin_init (Core *c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    if (world && ui) {
        commands.push_back(PluginCommand("copystock", "Copy stockpile under cursor.", copystock));
    }
    std::cerr << "world: " << sizeof(df::world) << " ui: " << sizeof(df::ui)
              << " b_stock: " << sizeof(building_stockpilest) << endl;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

bool inSelectMode() {
    using namespace ui_sidebar_mode;

    switch (ui->main.mode) {
    case BuildingItems:
    case QueryBuilding:
        return true;
    default:
        return false;
    }
}

DFhackCExport command_result copystock(Core * c, vector <string> & parameters)
{
    CoreSuspender suspend(c);

    // For convenience: when used in the stockpiles mode, switch to 'q'
    if (ui->main.mode == ui_sidebar_mode::Stockpiles) {
        world->selected_building = NULL; // just in case it contains some kind of garbage
        ui->main.mode = ui_sidebar_mode::QueryBuilding;
        selection_rect->start_x = -30000;

        c->con << "Switched back to query building." << endl;
        return CR_OK;
    }

    if (!inSelectMode()) {
        c->con << "Cannot copy stockpile in mode " << ENUM_KEY_STR(ui_sidebar_mode, ui->main.mode) << endl;
        return CR_OK;
    }

    building_stockpilest *sp = virtual_cast<building_stockpilest>(world->selected_building);
    if (!sp) {
        c->con << "Selected building isn't a stockpile." << endl;
        return CR_OK;
    }

    ui->stockpile.custom_settings = sp->settings;
    ui->main.mode = ui_sidebar_mode::Stockpiles;
    world->selected_stockpile_type = stockpile_category::Custom;

    c->con << "Stockpile options copied." << endl;
    return CR_OK;
}
