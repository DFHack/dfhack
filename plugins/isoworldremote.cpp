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

using namespace DFHack;
using namespace df::enums;
using namespace isoworldremote;


// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result isoWorldRemote (color_ostream &out, std::vector <std::string> & parameters);

void gather_embark_tile_layer(int EmbX, int EmbY, int EmbZ, EmbarkTileLayer * tile, MapExtras::MapCache * MP);
bool gather_embark_tile(int EmbX, int EmbY, EmbarkTile * tile, MapExtras::MapCache * MP);


// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - skeleton.plug.so or skeleton.plug.dll in this case
DFHACK_PLUGIN("skeleton");

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "isoworldremote", "Do nothing, look pretty.",
        isoWorldRemote, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  This command does nothing at all.\n"
        "Example:\n"
        "  isoworldremote\n"
        "    Does nothing.\n"
    ));
    return CR_OK;
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

// A command! It sits around and looks pretty. And it's nice and friendly.
command_result isoWorldRemote (color_ostream &out, std::vector <std::string> & parameters)
{
    // It's nice to print a help message you get invalid options
    // from the user instead of just acting strange.
    // This can be achieved by adding the extended help string to the
    // PluginCommand registration as show above, and then returning
    // CR_WRONG_USAGE from the function. The same string will also
    // be used by 'help your-command'.
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    // Commands are called from threads other than the DF one.
    // Suspend this thread until DF has time for us. If you
    // use CoreSuspender, it'll automatically resume DF when
    // execution leaves the current scope.
    CoreSuspender suspend;
    // Actually do something here. Yay.
    out.print("Doing a test...\n");
	MapExtras::MapCache MC;
	EmbarkTile test_tile;
	if(!gather_embark_tile(0,0, &test_tile, &MC))
		return CR_FAILURE;
	//test-write the file to check it.
	std::ofstream output_file("tile.p", std::ios_base::binary);
	output_file << test_tile.SerializeAsString();
	output_file.close();

	//load it again to verify.
	std::ifstream input_file("tile.p", std::ios_base::binary);
	std::string input_string( (std::istreambuf_iterator<char>(input_file) ),
                       (std::istreambuf_iterator<char>()    ) );
	EmbarkTile verify_tile;
	verify_tile.ParseFromString(input_string);
	//write contents to text file.
	std::ofstream debug_text("tile.txt", std::ios_base::trunc);
	debug_text << "world coords:" << verify_tile.world_x()<< "," << verify_tile.world_y()<< "," << verify_tile.world_z() << std::endl;
	for(int i = 0; i < verify_tile.tile_layer_size(); i++) {
		debug_text << "layer: " << i << std::endl;
		for(int j = 0; j < 48; j++) {
			debug_text << "	";
			for(int k = 0; k < 48; k++) {
				debug_text << verify_tile.tile_layer(i).mat_type_table(j*48+k) << ",";
			}
			debug_text << "	";
			for(int k = 0; k < 48; k++) {
				debug_text << std::setw(3) << verify_tile.tile_layer(i).mat_subtype_table(j*48+k) << ",";
			}
			debug_text << std::endl;
		}
		debug_text << std::endl;
	}
	//clean everything up.
	google::protobuf::ShutdownProtobufLibrary();
    // Give control back to DF.
    return CR_OK;
}

int coord_to_index_48(int x, int y) {
	return y*48+x;
}

bool gather_embark_tile(int EmbX, int EmbY, EmbarkTile * tile, MapExtras::MapCache * MP) {
	DFCoord base_coord;
	base_coord.x = EmbX;
	base_coord.y = EmbY;
	base_coord.z = 0;
	MapExtras::Block * b = MP->BlockAt(base_coord);
	if(!b) return 0;
	df::coord map_pos = b->getRaw()->map_pos;
	df::coord2d region_pos = b->getRaw()->region_pos;
	tile->set_world_x(region_pos.x*16+map_pos.x/3); //fixme: verify.
	tile->set_world_y(region_pos.y*16+map_pos.y/3);
	tile->set_world_z(map_pos.z);
	for(int z = 0; z < MP->maxZ(); z++)
	{
		EmbarkTileLayer * tile_layer = tile->add_tile_layer();
		gather_embark_tile_layer(EmbX, EmbY, z, tile_layer, MP);
	}
	return 1;
}


void gather_embark_tile_layer(int EmbX, int EmbY, int EmbZ, EmbarkTileLayer * tile, MapExtras::MapCache * MP)
{
	for(int i = tile->mat_type_table_size(); i < 2304; i++) { //This is needed so we have a full array to work with, otherwise the size isn't updated correctly.
		tile->add_mat_type_table(AIR);
		tile->add_mat_subtype_table(0);
	}
	for(int yy = 0; yy < 3; yy++) {
		for(int xx = 0; xx < 3; xx++) {
			DFCoord current_coord; 
			current_coord.x = EmbX+xx;
			current_coord.y = EmbY+yy;
			current_coord.z = EmbZ;
			MapExtras::Block * b = MP->BlockAt(current_coord);
			if(b && b->getRaw()) {
				for(int block_y=0; block_y<16; block_y++) {
					for(int block_x=0; block_x<16; block_x++) {
						df::coord2d block_coord;
						block_coord.x = block_x;
						block_coord.y = block_y;
						DFHack::t_matpair actual_mat = b->staticMaterialAt(block_coord);
						df::tiletype tile_type = b->staticTiletypeAt(block_coord);
						df::tile_designation designation = b->DesignationAt(block_coord);
						unsigned int array_index = coord_to_index_48(xx*16+block_x, yy*16+block_y);
						//make a new fake material at the given index
						if(tileMaterial(tile_type) == tiletype_material::FROZEN_LIQUID) { //Ice.
							tile->set_mat_type_table(array_index, BasicMaterial::LIQUID); //Ice is totally a liquid, shut up.
							tile->set_mat_subtype_table(array_index, 0);
						}
						else if(designation.bits.flow_size) { //Contains either water or lava.
							tile->set_mat_type_table(array_index, BasicMaterial::LIQUID); 
							if(designation.bits.liquid_type) //Magma
								tile->set_mat_subtype_table(array_index, 2);
							else //water
								tile->set_mat_subtype_table(array_index, 1);
						}
						else if(tileShapeBasic(tileShape(tile_type)) != tiletype_shape_basic::Open) {
							if(actual_mat.mat_type == builtin_mats::INORGANIC) { //inorganic
								tile->set_mat_type_table(array_index, BasicMaterial::INORGANIC); 
								tile->set_mat_subtype_table(array_index, actual_mat.mat_index);
							}
							else if(actual_mat.mat_type >= 419) { //Wooden constructions. Different from growing plants.
								tile->set_mat_type_table(array_index, BasicMaterial::WOOD); 
								tile->set_mat_subtype_table(array_index, actual_mat.mat_index);
							}
							else { //Unknown and unsupported stuff. Will just be drawn as grey.
								tile->set_mat_type_table(array_index, BasicMaterial::OTHER); 
								tile->set_mat_subtype_table(array_index, actual_mat.mat_type);
							}
						}
						else {
							tile->set_mat_type_table(array_index, BasicMaterial::AIR); 
						}
					}
				}
			}
		}
	}
}