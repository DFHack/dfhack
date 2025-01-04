#include "PluginManager.h"

#include "RemoteServer.h"
#include "example.pb.h"  // see plugins/proto/example.proto

using std::vector;
using std::string;

using namespace DFHack;
using namespace dfproto;

DFHACK_PLUGIN("example_rpc");

DFhackCExport command_result plugin_init (color_ostream &out, std::vector<PluginCommand> &commands) {
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    return CR_OK;
}

static command_result RenameSquad(color_ostream &stream, const RenameSquadIn *in) {
    df::squad *squad = df::squad::find(in->squad_id());
    if (!squad)
        return CR_NOT_FOUND;

    if (in->has_nickname())
        Translation::setNickname(&squad->name, UTF2DF(in->nickname()));
    if (in->has_alias())
        squad->alias = UTF2DF(in->alias());

    return CR_OK;
}

static command_result RenameUnit(color_ostream &stream, const RenameUnitIn *in) {
    df::unit *unit = df::unit::find(in->unit_id());
    if (!unit)
        return CR_NOT_FOUND;

    if (in->has_nickname())
        Units::setNickname(unit, UTF2DF(in->nickname()));
    if (in->has_profession())
        unit->custom_profession = UTF2DF(in->profession());

    return CR_OK;
}

static command_result RenameBuilding(color_ostream &stream, const RenameBuildingIn *in) {
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

DFhackCExport RPCService *plugin_rpcconnect(color_ostream &out) {
    RPCService *svc = new RPCService();
    svc->addFunction("RenameSquad", RenameSquad);
    svc->addFunction("RenameUnit", RenameUnit);
    svc->addFunction("RenameBuilding", RenameBuilding);
    return svc;
}
