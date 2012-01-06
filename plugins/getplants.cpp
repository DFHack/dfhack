// Designate all matching plants for gathering/cutting

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Maps.h>
#include <modules/Materials.h>
#include <modules/Vegetation.h>
#include <TileTypes.h>

using namespace std;
using namespace DFHack;

DFhackCExport command_result df_getplants (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
	return "getplants";
}

DFhackCExport command_result plugin_init ( Core * c, vector <PluginCommand> &commands)
{
	commands.clear();
	commands.push_back(PluginCommand("getplants", "Cut down all of the specified trees or gather all of the specified shrubs", df_getplants));
	return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
	return CR_OK;
}

DFhackCExport command_result df_getplants (Core * c, vector <string> & parameters)
{
	uint32_t x_max,y_max,z_max;
	designations40d designations;
	tiletypes40d tiles;
	t_blockflags blockflags;
	string plantMatStr = "";
	set<int> plantIDs;
	set<string> plantNames;
	bool deselect = false, exclude = false, treesonly = false, shrubsonly = false;

	bool dirty = false;
	int count = 0;
	for (size_t i = 0; i < parameters.size(); i++)
	{
		if(parameters[i] == "help" || parameters[i] == "?")
		{
			c->con.print("Specify the types of trees to cut down and/or shrubs to gather by their plant names, separated by spaces.\n"
				"Options:\n"
				"\t-t - Select trees only (exclude shrubs)\n"
				"\t-s - Select shrubs only (exclude trees)\n"
				"\t-c - Clear designations instead of setting them\n"
				"\t-x - Apply selected action to all plants except those specified\n"
				"Specifying both -t and -s will have no effect.\n"
				"If no plant IDs are specified, all valid plant IDs will be listed.\n"
				);
			return CR_OK;
		}
		else if(parameters[i] == "-t")
			treesonly = true;
		else if(parameters[i] == "-s")
			shrubsonly = true;
		else if(parameters[i] == "-c")
			deselect = true;
		else if(parameters[i] == "-x")
			exclude = true;
		else	plantNames.insert(parameters[i]);
	}
	c->Suspend();

	Materials *mats = c->getMaterials();
	for (vector<df_plant_type *>::const_iterator it = mats->df_organic->begin(); it != mats->df_organic->end(); it++)
	{
		df_plant_type &plant = **it;
		if (plantNames.find(plant.ID) != plantNames.end())
		{
			plantNames.erase(plant.ID);
			plantIDs.insert(it - mats->df_organic->begin());
		}
	}
	if (plantNames.size() > 0)
	{
		c->con.printerr("Invalid plant ID(s):");
		for (set<string>::const_iterator it = plantNames.begin(); it != plantNames.end(); it++)
			c->con.printerr(" %s", it->c_str());
		c->con.printerr("\n");
		c->Resume();
		return CR_FAILURE;
	}

	if (plantIDs.size() == 0)
	{
		c->con.print("Valid plant IDs:\n");
		for (vector<df_plant_type *>::const_iterator it = mats->df_organic->begin(); it != mats->df_organic->end(); it++)
		{
			df_plant_type &plant = **it;
			if (plant.flags.is_set(PLANT_GRASS))
				continue;
			c->con.print("* (%s) %s - %s\n", plant.flags.is_set(PLANT_TREE) ? "tree" : "shrub", plant.ID.c_str(), plant.name.c_str());
		}
		c->Resume();
		return CR_OK;
	}

	Maps *maps = c->getMaps();

	// init the map
	if (!maps->Start())
	{
		c->con.printerr("Can't init map.\n");
		c->Resume();
		return CR_FAILURE;
	}

	maps->getSize(x_max,y_max,z_max);
	// walk the map
	for (uint32_t x = 0; x < x_max; x++)
	{
		for (uint32_t y = 0; y < y_max; y++)
		{
			for (uint32_t z = 0; z < z_max; z++)
			{
				if (maps->getBlock(x,y,z))
				{
					dirty = false;
					maps->ReadDesignations(x,y,z, &designations);
					maps->ReadTileTypes(x,y,z, &tiles);
					maps->ReadBlockFlags(x,y,z, blockflags);

					vector<df_plant *> *plants;
					if (maps->ReadVegetation(x, y, z, plants))
					{
						for (vector<df_plant *>::const_iterator it = plants->begin(); it != plants->end(); it++)
						{
							const df_plant &plant = **it;
							uint32_t tx = plant.x % 16;
							uint32_t ty = plant.y % 16;
							if (plantIDs.find(plant.material) != plantIDs.end())
							{
								if (exclude)
									continue;
							}
							else
							{
								if (!exclude)
									continue;
							}

							TileShape shape = tileShape(tiles[tx][ty]);
							if (plant.is_shrub && (treesonly || shape != SHRUB_OK))
								continue;
							if (!plant.is_shrub && (shrubsonly || (shape != TREE_OK && shape != TREE_DEAD)))
								continue;
							if (designations[tx][ty].bits.hidden)
								continue;
							if (deselect && designations[tx][ty].bits.dig != designation_no)
							{
								designations[tx][ty].bits.dig = designation_no;
								dirty = true;
								++count;
							}
							if (!deselect && designations[tx][ty].bits.dig != designation_default)
							{
								designations[tx][ty].bits.dig = designation_default;
								dirty = true;
								++count;
							}
						}
					}
					// If anything was changed, write it all.
					if (dirty)
					{
						blockflags.bits.designated = 1;
						maps->WriteDesignations(x,y,z, &designations);
						maps->WriteBlockFlags(x,y,z, blockflags);
						dirty = false;
					}
				}
			}
		}
	}
	c->Resume();
	if (count)
		c->con.print("Updated %d plant designations.\n", count);
	return CR_OK;
}
