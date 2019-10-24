#include <fstream>
#include <cerrno>

#include <json/json.h>

#include "DFHackVersion.h"
#include "ExportDatatypes.h"
#include "export/import.h"

#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"

using namespace DFHack;

command_result DFHack::export_datatypes(color_ostream &con,
                                        const std::vector<std::string> &args) {
    if (args.size() != 0) {
        con.print("usage: export-datatypes\n");
        return CR_WRONG_USAGE;
    }
    try {
        auto data = Export::load_df();
        auto filename = data.vinfo->getVersion() + '-'
            + DFHACK_GIT_DESCRIPTION + '-'
            + DFHACK_GIT_XML_DESCRIPTION + ".json";
        std::replace(filename.begin(), filename.end(), ' ', '-');
        std::ofstream out{filename};
        out << data.to_json();
        out.close();
        if (!out)
            Export::die(filename + ": " + strerror(errno));
        con.print("export-datatypes: wrote %s\n", filename.c_str());
        return CR_OK;
    } catch (const Export::Error &e) {
        con.printerr("export-datatypes: error: %s\n", e.what());
        return CR_FAILURE;
    }
}
