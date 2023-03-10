#include "Debug.h"
#include "PluginManager.h"
#include "StockpileUtils.h"
#include "StockpileSerializer.h"

#include "modules/Filesystem.h"
#include "modules/Gui.h"

using std::vector;
using std::string;

using namespace DFHack;

DFHACK_PLUGIN("stockpiles");

REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(stockpiles, log, DebugCategory::LINFO);
}

static command_result savestock(color_ostream& out, vector <string>& parameters);
static command_result loadstock(color_ostream& out, vector <string>& parameters);

DFhackCExport command_result plugin_init(color_ostream& out, std::vector <PluginCommand>& commands) {
    commands.push_back(PluginCommand(
        "savestock",
        "Save the active stockpile's settings to a file.",
        savestock,
        Gui::any_stockpile_hotkey));
    commands.push_back(PluginCommand(
        "loadstock",
        "Load and apply stockpile settings from a file.",
        loadstock,
        Gui::any_stockpile_hotkey));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream& out) {
    return CR_OK;
}

//  exporting
static command_result savestock(color_ostream& out, vector <string>& parameters) {
    df::building_stockpilest* sp = Gui::getSelectedStockpile(out, true);
    if (!sp) {
        out.printerr("Selected building isn't a stockpile.\n");
        return CR_WRONG_USAGE;
    }

    if (parameters.size() > 2) {
        out.printerr("Invalid parameters\n");
        return CR_WRONG_USAGE;
    }

    std::string file;
    for (size_t i = 0; i < parameters.size(); ++i) {
        const std::string o = parameters.at(i);
        if (!o.empty() && o[0] != '-') {
            file = o;
        }
    }
    if (file.empty()) {
        out.printerr("You must supply a valid filename.\n");
        return CR_WRONG_USAGE;
    }

    StockpileSerializer cereal(sp);

    if (!is_dfstockfile(file)) file += ".dfstock";
    try {
        if (!cereal.serialize_to_file(file)) {
            out.printerr("could not save to %s\n", file.c_str());
            return CR_FAILURE;
        }
    }
    catch (std::exception& e) {
        out.printerr("serialization failed: protobuf exception: %s\n", e.what());
        return CR_FAILURE;
    }

    return CR_OK;
}


// importing
static command_result loadstock(color_ostream& out, vector <string>& parameters) {
    df::building_stockpilest* sp = Gui::getSelectedStockpile(out, true);
    if (!sp) {
        out.printerr("Selected building isn't a stockpile.\n");
        return CR_WRONG_USAGE;
    }

    if (parameters.size() < 1 || parameters.size() > 2) {
        out.printerr("Invalid parameters\n");
        return CR_WRONG_USAGE;
    }

    std::string file;
    for (size_t i = 0; i < parameters.size(); ++i) {
        const std::string o = parameters.at(i);
        if (!o.empty() && o[0] != '-') {
            file = o;
        }
    }
    if (file.empty()) {
        out.printerr("ERROR: missing .dfstock file parameter\n");
        return DFHack::CR_WRONG_USAGE;
    }
    if (!is_dfstockfile(file))
        file += ".dfstock";
    if (!Filesystem::exists(file)) {
        out.printerr("ERROR: the .dfstock file doesn't exist: %s\n", file.c_str());
        return CR_WRONG_USAGE;
    }

    StockpileSerializer cereal(sp);
    try {
        if (!cereal.unserialize_from_file(file)) {
            out.printerr("unserialization failed: %s\n", file.c_str());
            return CR_FAILURE;
        }
    }
    catch (std::exception& e) {
        out.printerr("unserialization failed: protobuf exception: %s\n", e.what());
        return CR_FAILURE;
    }
    return CR_OK;
}
