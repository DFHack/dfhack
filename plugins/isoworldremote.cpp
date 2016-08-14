// This is a generic plugin that does nothing useful apart from acting as an example... of a plugin that does nothing :D

// some headers required for a plugin. Nothing special, just the basics.
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

// DF data structure definition headers
#include "DataDefs.h"
#include "df/world.h"
#include "df/map_block.h"
#include "df/builtin_mats.h"
#include "df/tile_designation.h"

//DFhack specific headers
#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/Materials.h"

//Needed for writing the protobuff stuff to a file.
#include <fstream>
#include <string>
#include <iomanip>

#include "isoworldremote.pb.h"

#include "RemoteServer.h"

using namespace DFHack;
using namespace df::enums;
using namespace isoworldremote;

DFHACK_PLUGIN("isoworldremote");
REQUIRE_GLOBAL(gamemode);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(cur_year);
REQUIRE_GLOBAL(cur_season);

// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result isoWorldRemote (color_ostream &out, std::vector <std::string> & parameters);

static command_result GetEmbarkTile(color_ostream &stream, const TileRequest *in, EmbarkTile *out);
static command_result GetEmbarkInfo(color_ostream &stream, const MapRequest *in, MapReply *out);
static command_result GetRawNames(color_ostream &stream, const MapRequest *in, RawNames *out);

bool gather_embark_tile_layer(int EmbX, int EmbY, int EmbZ, EmbarkTileLayer * tile, MapExtras::MapCache * MP);
bool gather_embark_tile(int EmbX, int EmbY, EmbarkTile * tile, MapExtras::MapCache * MP);

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    //// Fill the command list with your commands.
    //commands.push_back(PluginCommand(
    //    "isoworldremote", "Dump north-west embark tile to text file for debug purposes.",
    //    isoWorldRemote, false, /* true means that the command can't be used from non-interactive user interface */
    //    // Extended help string. Used by CR_WRONG_USAGE and the help command:
    //    "  This command does nothing at all.\n"
    //    "Example:\n"
    //    "  isoworldremote\n"
    //    "    Does nothing.\n"
    //));
    return CR_OK;
}

DFhackCExport RPCService *plugin_rpcconnect(color_ostream &)
{
    RPCService *svc = new RPCService();
    svc->addFunction("GetEmbarkTile", GetEmbarkTile);
    svc->addFunction("GetEmbarkInfo", GetEmbarkInfo);
    svc->addFunction("GetRawNames", GetRawNames);
    return svc;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    // You *MUST* kill all threads you created before this returns.
    // If everything fails, just return CR_FAILURE. Your plugin will be
    // in a zombie state, but things won't crash.
    return CR_OK;
}

// Called to notify the plugin about important state changes.
// Invoked with DF suspended, and always before the matching plugin_onupdate.
// More event codes may be added in the future.
/*
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_GAME_LOADED:
        // initialize from the world just loaded
        break;
    case SC_GAME_UNLOADED:
        // cleanup
        break;
    default:
        break;
    }
    return CR_OK;
}
*/

// Whatever you put here will be done in each game step. Don't abuse it.
// It's optional, so you can just comment it out like this if you don't need it.
/*
DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    // whetever. You don't need to suspend DF execution here.
    return CR_OK;
}
*/

//// A command! It sits around and looks pretty. And it's nice and friendly.
//command_result isoWorldRemote (color_ostream &out, std::vector <std::string> & parameters)
//{
//    // It's nice to print a help message you get invalid options
//    // from the user instead of just acting strange.
//    // This can be achieved by adding the extended help string to the
//    // PluginCommand registration as show above, and then returning
//    // CR_WRONG_USAGE from the function. The same string will also
//    // be used by 'help your-command'.
//    if (!parameters.empty())
//        return CR_WRONG_USAGE;
//    // Commands are called from threads other than the DF one.
//    // Suspend this thread until DF has time for us. If you
//    // use CoreSuspender, it'll automatically resume DF when
//    // execution leaves the current scope.
//    CoreSuspender suspend;
//    // Actually do something here. Yay.
//    out.print("Doing a test...\n");
//    MapExtras::MapCache MC;
//    EmbarkTile test_tile;
//    if(!gather_embark_tile(0,0, &test_tile, &MC))
//        return CR_FAILURE;
//    //test-write the file to check it.
//    std::ofstream output_file("tile.p", std::ios_base::binary);
//    output_file << test_tile.SerializeAsString();
//    output_file.close();
//
//    //load it again to verify.
//    std::ifstream input_file("tile.p", std::ios_base::binary);
//    std::string input_string( (std::istreambuf_iterator<char>(input_file) ),
//                       (std::istreambuf_iterator<char>()    ) );
//    EmbarkTile verify_tile;
//    verify_tile.ParseFromString(input_string);
//    //write contents to text file.
//    std::ofstream debug_text("tile.txt", std::ios_base::trunc);
//    debug_text << "world coords:" << verify_tile.world_x()<< "," << verify_tile.world_y()<< "," << verify_tile.world_z() << std::endl;
//    for(int i = 0; i < verify_tile.tile_layer_size(); i++) {
//        debug_text << "layer: " << i << std::endl;
//        for(int j = 0; j < 48; j++) {
//            debug_text << "    ";
//            for(int k = 0; k < 48; k++) {
//                debug_text << verify_tile.tile_layer(i).mat_type_table(j*48+k) << ",";
//            }
//            debug_text << "    ";
//            for(int k = 0; k < 48; k++) {
//                debug_text << std::setw(3) << verify_tile.tile_layer(i).mat_subtype_table(j*48+k) << ",";
//            }
//            debug_text << std::endl;
//        }
//        debug_text << std::endl;
//    }
//    // Give control back to DF.
//    return CR_OK;
//}

static command_result GetEmbarkTile(color_ostream &stream, const TileRequest *in, EmbarkTile *out)
{
    MapExtras::MapCache MC;
    gather_embark_tile(in->want_x() * 3, in->want_y() * 3, out, &MC);
    MC.trash();
    return CR_OK;
}

static command_result GetEmbarkInfo(color_ostream &stream, const MapRequest *in, MapReply *out)
{
    if(!Core::getInstance().isWorldLoaded()) {
        out->set_available(false);
        return CR_OK;
    }
    if(!Core::getInstance().isMapLoaded()) {
        out->set_available(false);
        return CR_OK;
    }
    if(!gamemode) {
        out->set_available(false);
        return CR_OK;
    }
    if((*gamemode != game_mode::ADVENTURE) && (*gamemode != game_mode::DWARF)) {
        out->set_available(false);
        return CR_OK;
    }
    if(!DFHack::Maps::IsValid()) {
        out->set_available(false);
        return CR_OK;
    }
    if (in->has_save_folder()) { //If no save folder is given, it means we don't care.
        if (!(in->save_folder() == world->cur_savegame.save_dir || in->save_folder() == "ANY")) { //isoworld has a different map loaded, don't bother trying to load tiles for it, we don't have them.
            out->set_available(false);
            return CR_OK;
        }
    }
    out->set_available(true);
    out->set_current_year(*cur_year);
    out->set_current_season(*cur_season);
    out->set_region_x(world->map.region_x);
    out->set_region_y(world->map.region_y);
    out->set_region_size_x(world->map.x_count_block / 3);
    out->set_region_size_y(world->map.y_count_block / 3);
    return CR_OK;
}

int coord_to_index_48(int x, int y) {
    return y*48+x;
}

bool gather_embark_tile(int EmbX, int EmbY, EmbarkTile * tile, MapExtras::MapCache * MP) {
    tile->set_is_valid(false);
    tile->set_world_x(world->map.region_x + (EmbX/3));
    tile->set_world_y(world->map.region_y + (EmbY/3));
    tile->set_world_z(world->map.region_z + 1); //adding one because floors get shifted one downwards.
    tile->set_current_year(*cur_year);
    tile->set_current_season(*cur_season);
    int num_valid_layers = 0;
    for(int z = 0; z < MP->maxZ(); z++)
    {
        EmbarkTileLayer * tile_layer = tile->add_tile_layer();
        num_valid_layers += gather_embark_tile_layer(EmbX, EmbY, z, tile_layer, MP);
    }
    if(num_valid_layers > 0)
        tile->set_is_valid(true);
    return 1;
}


bool gather_embark_tile_layer(int EmbX, int EmbY, int EmbZ, EmbarkTileLayer * tile, MapExtras::MapCache * MP)
{
    for(int i = tile->mat_type_table_size(); i < 2304; i++) { //This is needed so we have a full array to work with, otherwise the size isn't updated correctly.
        tile->add_mat_type_table(AIR);
        tile->add_mat_subtype_table(0);
    }
    int num_valid_blocks = 0;
    for(int yy = 0; yy < 3; yy++) {
        for(int xx = 0; xx < 3; xx++) {
            DFCoord current_coord, upper_coord;
            current_coord.x = EmbX+xx;
            current_coord.y = EmbY+yy;
            current_coord.z = EmbZ;
            upper_coord = current_coord;
            upper_coord.z += 1;
            MapExtras::Block * b = MP->BlockAt(current_coord);
            MapExtras::Block * b_upper = MP->BlockAt(upper_coord);
            if(b && b->getRaw()) {
                for(int block_y=0; block_y<16; block_y++) {
                    for(int block_x=0; block_x<16; block_x++) {
                        df::coord2d block_coord;
                        block_coord.x = block_x;
                        block_coord.y = block_y;
                        df::tiletype tile_type = b->tiletypeAt(block_coord);
                        df::tiletype upper_tile = df::tiletype::Void;
                        if(b_upper && b_upper->getRaw()) {
                            upper_tile = b_upper->tiletypeAt(block_coord);
                        }
                        df::tile_designation designation = b->DesignationAt(block_coord);
                        DFHack::t_matpair actual_mat;
                        if(tileShapeBasic(tileShape(upper_tile)) == tiletype_shape_basic::Floor && (tileMaterial(tile_type) != tiletype_material::FROZEN_LIQUID) && (tileMaterial(tile_type) != tiletype_material::BROOK)) { //if the upper tile is a floor, use that material instead. Unless it's ice.
                            actual_mat = b_upper->staticMaterialAt(block_coord);
                        }
                        else {
                            actual_mat = b->staticMaterialAt(block_coord);
                        }
                        if(((tileMaterial(tile_type) == tiletype_material::FROZEN_LIQUID) || (tileMaterial(tile_type) == tiletype_material::BROOK)) && (tileShapeBasic(tileShape(tile_type)) == tiletype_shape_basic::Floor)) {
                        tile_type = tiletype::OpenSpace;
                        }
                        unsigned int array_index = coord_to_index_48(xx*16+block_x, yy*16+block_y);
                        //make a new fake material at the given index
                        if(tileMaterial(tile_type) == tiletype_material::FROZEN_LIQUID && !((tileShapeBasic(tileShape(upper_tile)) == tiletype_shape_basic::Floor) && (tileMaterial(upper_tile) != tiletype_material::FROZEN_LIQUID))) { //Ice.
                            tile->set_mat_type_table(array_index, BasicMaterial::LIQUID); //Ice is totally a liquid, shut up.
                            tile->set_mat_subtype_table(array_index, LiquidType::ICE);
                            num_valid_blocks++;
                        }
                        else if(designation.bits.flow_size && (tileShapeBasic(tileShape(upper_tile)) != tiletype_shape_basic::Floor)) { //Contains either water or lava.
                            tile->set_mat_type_table(array_index, BasicMaterial::LIQUID);
                            if(designation.bits.liquid_type) //Magma
                                tile->set_mat_subtype_table(array_index, LiquidType::MAGMA);
                            else //water
                                tile->set_mat_subtype_table(array_index, LiquidType::WATER);
                            num_valid_blocks++;
                        }
                        else if(((tileShapeBasic(tileShape(tile_type)) != tiletype_shape_basic::Open) ||
                            (tileShapeBasic(tileShape(upper_tile)) == tiletype_shape_basic::Floor)) &&
                            ((tileShapeBasic(tileShape(tile_type)) != tiletype_shape_basic::Floor) ||
                            (tileShapeBasic(tileShape(upper_tile)) == tiletype_shape_basic::Floor))) { //if the upper tile is a floor, we don't skip, otherwise we do.
                                if(actual_mat.mat_type == builtin_mats::INORGANIC) { //inorganic
                                    tile->set_mat_type_table(array_index, BasicMaterial::INORGANIC);
                                    tile->set_mat_subtype_table(array_index, actual_mat.mat_index);
                                }
                                else if(actual_mat.mat_type == 419) { //Growing plants
                                    tile->set_mat_type_table(array_index, BasicMaterial::PLANT);
                                    tile->set_mat_subtype_table(array_index, actual_mat.mat_index);
                                }
                                else if(actual_mat.mat_type >= 420) { //Wooden constructions. Different from growing plants.
                                    tile->set_mat_type_table(array_index, BasicMaterial::WOOD);
                                    tile->set_mat_subtype_table(array_index, actual_mat.mat_index);
                                }
                                else { //Unknown and unsupported stuff. Will just be drawn as grey.
                                    tile->set_mat_type_table(array_index, BasicMaterial::OTHER);
                                    tile->set_mat_subtype_table(array_index, actual_mat.mat_type);
                                }
                                num_valid_blocks++;
                        }
                        else {
                            tile->set_mat_type_table(array_index, BasicMaterial::AIR);
                        }
                    }
                }
            }
        }
    }
    return (num_valid_blocks >0);
}

static command_result GetRawNames(color_ostream &stream, const MapRequest *in, RawNames *out){
    if(!Core::getInstance().isWorldLoaded()) {
        out->set_available(false);
        return CR_OK;
    }
    if(!Core::getInstance().isMapLoaded()) {
        out->set_available(false);
        return CR_OK;
    }
    if(!gamemode) {
        out->set_available(false);
        return CR_OK;
    }
    if((*gamemode != game_mode::ADVENTURE) && (*gamemode != game_mode::DWARF)) {
        out->set_available(false);
        return CR_OK;
    }
    if(!DFHack::Maps::IsValid()) {
        out->set_available(false);
        return CR_OK;
    }
    if (in->has_save_folder()) { //If no save folder is given, it means we don't care.
        if (!(in->save_folder() == world->cur_savegame.save_dir || in->save_folder() == "ANY")) { //isoworld has a different map loaded, don't bother trying to load tiles for it, we don't have them.
            out->set_available(false);
            return CR_OK;
        }
    }
    out->set_available(true);
    for(int i = 0; i < world->raws.inorganics.size(); i++){
        out->add_inorganic(world->raws.inorganics[i]->id);
    }

    for(int i = 0; i < world->raws.plants.all.size(); i++){
        out->add_organic(world->raws.plants.all[i]->id);
    }
    return CR_OK;
}
