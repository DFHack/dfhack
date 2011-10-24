// This does not work with Linux Dwarf Fortress
// With thanks to peterix for DFHack and Quietust for information http://www.bay12forums.com/smf/index.php?topic=91166.msg2605147#msg2605147

#include <map>
#include <string>
#include <vector>
#include "dfhack/Console.h"
#include "dfhack/Core.h"
#include "dfhack/Export.h"
#include "dfhack/PluginManager.h"
#include "dfhack/Process.h"
#include "dfhack/modules/Items.h"
#include "dfhack/modules/Materials.h"

typedef int32_t t_materialIndex;
typedef int16_t t_materialSubindex, t_itemType, t_itemSubtype;
typedef int8_t t_exclusionType;

const unsigned int seedLimit = 400; // a limit on the limits which can be placed on seeds
const t_itemType seedType = 52; // from df.magmawiki.com/index.php/Item_token
const t_itemType plantType = 53; // from df.magmawiki.com/index.php/Item_token
const t_itemSubtype organicSubtype = -1; // seems to fixed
const t_exclusionType cookingExclusion = 1; // seems to be fixed
const t_itemType limitType = 0; // used to store limit as an entry in the exclusion list
const t_itemSubtype limitSubtype = 0; // used to store limit as an entry in the exclusion list
const t_exclusionType limitExclusion = 4; // used to store limit as an entry in the exclusion list
const int buffer = 20; // seed number buffer - 20 is reasonable
bool running = false; // whether seedwatch is counting the seeds or not

std::map<std::string, std::string> abbreviations; // abbreviations for the standard plants

bool ignoreSeeds(DFHack::t_itemflags& f) // seeds with the following flags should not be counted
{
	return
		f.dump ||
		f.forbid ||
		f.garbage_colect || 
		f.hidden || 
		f.hostile || 
		f.on_fire || 
		f.rotten || 
		f.trader ||
		f.in_building ||
		f.in_job;
};
void updateCountAndSubindices(DFHack::Core& core, std::map<t_materialIndex, unsigned int>& seedCount, std::map<t_materialIndex, t_materialSubindex>& seedSubindices, std::map<t_materialIndex, t_materialSubindex>& plantSubindices) // fills seedCount, seedSubindices and plantSubindices
{
	DFHack::Items& itemsModule = *core.getItems();
	itemsModule.Start();
	std::vector<DFHack::t_item*> items;
	itemsModule.readItemVector(items);

	DFHack::dfh_item item;
	std::size_t size = items.size();
	for(std::size_t i = 0; i < size; ++i)
	{
		itemsModule.readItem(items[i], item);
		t_materialIndex materialIndex = item.matdesc.index;
		switch(item.matdesc.itemType)
		{
		case seedType:
			seedSubindices[materialIndex] = item.matdesc.subIndex;
			if(!ignoreSeeds(item.base->flags)) ++seedCount[materialIndex];
			break;
		case plantType:
			plantSubindices[materialIndex] = item.matdesc.subIndex;
			break;
		}
	}

	itemsModule.Finish();
};
void printHelp(DFHack::Core& core) // prints help
{
	core.con.print("\ne.g. #seedwatch MUSHROOM_HELMET_PLUMP 30\n");
	core.con.print("This is how to add an item to the watch list.\n");
	core.con.print("MUSHROOM_HELMET_PLUMP is the plant token for plump helmets (found in raws)\n");
	core.con.print("The number of available plump helmet seeds is counted each tick\n");
	core.con.print("If it falls below 30, plump helmets and plump helmet seeds will be excluded from cookery.\n");
	core.con.print("%i is the buffer, therefore if the number rises above 30 + %i, then cooking will be allowed.\n", buffer, buffer);
	if(!abbreviations.empty())
	{
		core.con.print("You can use these abbreviations for the plant tokens:\n");
		for(std::map<std::string, std::string>::const_iterator i = abbreviations.begin(); i != abbreviations.end(); ++i)
		{
			core.con.print("%s -> %s\n", i->first.c_str(), i->second.c_str());
		}
	}
	core.con.print("e.g. #seedwatch ph 30\nis the same as #seedwatch MUSHROOM_HELMET_PLUMP 30\n\n");
	core.con.print("e.g. #seedwatch all 30\nAdds all which are in the abbreviation list to the watch list, the limit being 30.\n\n");
	core.con.print("e.g. #seedwatch MUSHROOM_HELMET_PLUMP\nRemoves MUSHROOM_HELMET_PLUMP from the watch list.\n\n");
	core.con.print("#seedwatch all\nClears the watch list.\n\n");
	core.con.print("#seedwatch start\nStart seedwatch watch.\n\n");
	core.con.print("#seedwatch stop\nStop seedwatch watch.\n\n");
	core.con.print("#seedwatch info\nDisplay whether seedwatch is watching, and the watch list.\n\n");
	core.con.print("#seedwatch clear\nClears the watch list.\n");
};
std::string searchAbbreviations(std::string in) // searches abbreviations, returns expansion if so, returns original if not
{
	if(abbreviations.count(in) > 0) return abbreviations[in];
	return in;
};

class t_kitchenExclusions // helps to organise access to the cooking exclusions
{
public:
	t_kitchenExclusions(DFHack::Core& core_) // constructor
		: core(core_)
		, itemTypes         (*((std::vector<t_itemType        >*)(start(core_)       ))) // using the kludge in start
		, itemSubtypes      (*((std::vector<t_itemSubtype     >*)(start(core_) + 0x10))) // further kludge offset
		, materialSubindices(*((std::vector<t_materialSubindex>*)(start(core_) + 0x20))) // further kludge offset
		, materialIndices   (*((std::vector<t_materialIndex   >*)(start(core_) + 0x30))) // further kludge offset
		, exclusionTypes    (*((std::vector<t_exclusionType   >*)(start(core_) + 0x40))) // further kludge offset
	{
	};
	void print() // print the exclusion list, with the material index also translated into its token (for organics) - for debug really
	{
		core.con.print("Kitchen Exclusions\n");
		DFHack::Materials& materialsModule= *core.getMaterials();
		materialsModule.ReadOrganicMaterials();
		for(std::size_t i = 0; i < size(); ++i)
		{
			core.con.print("%2u: IT:%2i IS:%i MI:%2i MS:%3i ET:%i %s\n", i, itemTypes[i], itemSubtypes[i], materialIndices[i], materialSubindices[i], exclusionTypes[i], materialsModule.organic[materialIndices[i]].id.c_str());
		}
		core.con.print("\n");
	};
	void allowCookery(t_materialIndex materialIndex) // remove this material from the exclusion list if it is in it
	{
		bool match = false;
		do
		{
			match = false;
			std::size_t matchIndex = 0;
			for(std::size_t i = 0; i < size(); ++i)
			{
				if(materialIndices[i] == materialIndex && (itemTypes[i] == seedType || itemTypes[i] == plantType) && exclusionTypes[i] == cookingExclusion)
				{
					match = true;
					matchIndex = i;
				}
			}
			if(match)
			{
				itemTypes.erase(itemTypes.begin() + matchIndex);
				itemSubtypes.erase(itemSubtypes.begin() + matchIndex);
				materialIndices.erase(materialIndices.begin() + matchIndex);
				materialSubindices.erase(materialSubindices.begin() + matchIndex);
				exclusionTypes.erase(exclusionTypes.begin() + matchIndex);
			}
		} while(match);
	};
	void denyCookery(t_materialIndex materialIndex, std::map<t_materialIndex, t_materialSubindex>& seedSubindices, std::map<t_materialIndex, t_materialSubindex>& plantSubindices) // add this material to the exclusion list, if it is not already in it
	{
		if(seedSubindices.count(materialIndex) > 0)
		{
			bool alreadyIn = false;
			for(std::size_t i = 0; i < size(); ++i)
			{
				if(materialIndices[i] == materialIndex && itemTypes[i] == seedType && exclusionTypes[i] == cookingExclusion)
				{
					alreadyIn = true;
				}
			}
			if(!alreadyIn)
			{
				itemTypes.push_back(seedType);
				itemSubtypes.push_back(organicSubtype);
				materialIndices.push_back(materialIndex);
				materialSubindices.push_back(seedSubindices[materialIndex]);
				exclusionTypes.push_back(cookingExclusion);
			}
		}
		if(plantSubindices.count(materialIndex) > 0)
		{
			bool alreadyIn = false;
			for(std::size_t i = 0; i < size(); ++i)
			{
				if(materialIndices[i] == materialIndex && itemTypes[i] == plantType && exclusionTypes[i] == cookingExclusion)
				{
					alreadyIn = true;
				}
			}
			if(!alreadyIn)
			{
				itemTypes.push_back(plantType);
				itemSubtypes.push_back(organicSubtype);
				materialIndices.push_back(materialIndex);
				materialSubindices.push_back(plantSubindices[materialIndex]);
				exclusionTypes.push_back(cookingExclusion);
			}
		}
	};
	void fillWatchMap(std::map<t_materialIndex, unsigned int>& watchMap) // fills a map with info from the limit info storage entries in the exclusion list
	{
		watchMap.clear();
		for(std::size_t i = 0; i < size(); ++i)
		{
			if(itemTypes[i] == limitType && itemSubtypes[i] == limitSubtype && exclusionTypes[i] == limitExclusion)
			{
				watchMap[materialIndices[i]] = (unsigned int) materialSubindices[i];
			}
		}
	};
	void removeLimit(t_materialIndex materialIndex) // removes a limit info storage entry from the exclusion list if it is in it
	{
		bool match = false;
		do
		{
			match = false;
			std::size_t matchIndex = 0;
			for(std::size_t i = 0; i < size(); ++i)
			{
				if(itemTypes[i] == limitType && itemSubtypes[i] == limitSubtype && materialIndices[i] == materialIndex && exclusionTypes[i] == limitExclusion)
				{
					match = true;
					matchIndex = i;
				}
			}
			if(match)
			{
				itemTypes.erase(itemTypes.begin() + matchIndex);
				itemSubtypes.erase(itemSubtypes.begin() + matchIndex);
				materialIndices.erase(materialIndices.begin() + matchIndex);
				materialSubindices.erase(materialSubindices.begin() + matchIndex);
				exclusionTypes.erase(exclusionTypes.begin() + matchIndex);
			}
		} while(match);
	};
	void setLimit(t_materialIndex materialIndex, unsigned int limit) // add a limit info storage item to the exclusion list, or alters an existing one
	{
		removeLimit(materialIndex);
		if(limit > seedLimit) limit = seedLimit;

		itemTypes.push_back(limitType);
		itemSubtypes.push_back(limitSubtype);
		materialIndices.push_back(materialIndex);
		materialSubindices.push_back((t_materialSubindex) (limit < seedLimit) ? limit : seedLimit);
		exclusionTypes.push_back(limitExclusion);
	};
	void clearLimits() // clears all limit info storage items from the exclusion list
	{
		bool match = false;
		do
		{
			match = false;
			std::size_t matchIndex;
			for(std::size_t i = 0; i < size(); ++i)
			{
				if(itemTypes[i] == limitType && itemSubtypes[i] == limitSubtype && exclusionTypes[i] == limitExclusion)
				{
					match = true;
					matchIndex = i;
				}
			}
			if(match)
			{
				itemTypes.erase(itemTypes.begin() + matchIndex);
				itemSubtypes.erase(itemSubtypes.begin() + matchIndex);
				materialIndices.erase(materialIndices.begin() + matchIndex);
				materialSubindices.erase(materialSubindices.begin() + matchIndex);
				exclusionTypes.erase(exclusionTypes.begin() + matchIndex);
			}
		} while(match);
	};
private:
	DFHack::Core& core;
	std::vector<t_itemType>& itemTypes; // the item types vector of the kitchen exclusion list
	std::vector<t_itemSubtype>& itemSubtypes; // the item subtype vector of the kitchen exclusion list
	std::vector<t_materialSubindex>& materialSubindices; // the material subindex vector of the kitchen exclusion list
	std::vector<t_materialIndex>& materialIndices; // the material index vector of the kitchen exclusion list
	std::vector<t_exclusionType>& exclusionTypes; // the exclusion type vector of the kitchen excluions list
	std::size_t size() // the size of the exclusions vectors (they are all the same size - if not, there is a problem!)
	{
		return itemTypes.size();
	};
	static uint32_t start(const DFHack::Core& core) // the kludge for the exclusion position in memory
	{
		return core.p->getBase() + 0x014F0BB4 - 0x400000;
	};
};

DFhackCExport DFHack::command_result df_seedwatch(DFHack::Core* pCore, std::vector<std::string>& parameters)
{
	DFHack::Core& core = *pCore;
	if(!core.isValid()) return DFHack::CR_FAILURE;
	core.Suspend();

	DFHack::Materials& materialsModule = *core.getMaterials();
	materialsModule.ReadOrganicMaterials();
	std::vector<DFHack::t_matgloss>& organics = materialsModule.organic;

	std::map<std::string, t_materialIndex> materialsReverser;
	for(std::size_t i = 0; i < organics.size(); ++i)
	{
		materialsReverser[organics[i].id] = i;
	}

	std::string par;
	int limit;
	switch(parameters.size())
	{
	case 0:
		printHelp(core);
		break;
	case 1:
		par = parameters[0];
		if(par == "help") printHelp(core);
		else if(par == "?") printHelp(core);
		else if(par == "start")
		{
			running = true;
			core.con.print("seedwatch supervision started.\n");
		}
		else if(par == "stop")
		{
			running = false;
			core.con.print("seedwatch supervision stopped.\n");
		}
		else if(par == "clear")
		{
			t_kitchenExclusions kitchenExclusions(core);
			kitchenExclusions.clearLimits();
			core.con.print("seedwatch watchlist cleared\n");
		}
		else if(par == "info")
		{
			core.con.print("seedwatch Info:\n");
			if(running) core.con.print("seedwatch is supervising.  Use 'seedwatch stop' to stop supervision.\n");
			else core.con.print("seedwatch is not supervising.  Use 'seedwatch start' to start supervision.\n");
			t_kitchenExclusions kitchenExclusions(core);
			std::map<t_materialIndex, unsigned int> watchMap;
			kitchenExclusions.fillWatchMap(watchMap);
			if(watchMap.empty()) core.con.print("The watch list is empty.\n");
			else
			{
				core.con.print("The watch list is:\n");
				for(std::map<t_materialIndex, unsigned int>::const_iterator i = watchMap.begin(); i != watchMap.end(); ++i)
				{
					core.con.print("%s : %u\n", organics[i->first].id.c_str(), i->second);
				}
			}
			/*kitchenExclusions.print();*/
		}
		else
		{
			std::string token = searchAbbreviations(par);
			if(materialsReverser.count(token) > 0)
			{
				t_kitchenExclusions kitchenExclusions(core);
				kitchenExclusions.removeLimit(materialsReverser[token]);
				core.con.print("%s is not being watched\n", token.c_str());
			}
			else
			{
				core.con.print("%s has not been found as a material.\n", token.c_str());
			}
		}
		break;
	case 2:
		limit = atoi(parameters[1].c_str());
		if(limit < 0) limit = 0;
		if(parameters[0] == "all")
		{
			for(std::map<std::string, std::string>::const_iterator i = abbreviations.begin(); i != abbreviations.end(); ++i)
			{
				t_kitchenExclusions kitchenExclusions(core);
				if(materialsReverser.count(i->second) > 0) kitchenExclusions.setLimit(materialsReverser[i->second], limit);
			}
		}
		else
		{
			std::string token = searchAbbreviations(parameters[0]);
			if(materialsReverser.count(token) > 0)
			{
				t_kitchenExclusions kitchenExclusions(core);
				kitchenExclusions.setLimit(materialsReverser[token], limit);
				core.con.print("%s is being watched.\n", token.c_str());
			}
			else
			{
				core.con.print("%s has not been found as a material.\n", token.c_str());
			}
		}
		break;
	default:
		printHelp(core);
		break;
	}

	core.Resume();
	return DFHack::CR_OK;
}

DFhackCExport const char* plugin_name(void)
{
	return "seedwatch";
}

DFhackCExport DFHack::command_result plugin_init(DFHack::Core* pCore, std::vector<DFHack::PluginCommand>& commands)
{
	commands.clear();
	commands.push_back(DFHack::PluginCommand("seedwatch", "Switches cookery based on quantity of seeds, to keep reserves", df_seedwatch));
	// fill in the abbreviations map, with abbreviations for the standard plants
	abbreviations["bs"] = "SLIVER_BARB";
	abbreviations["bt"] = "TUBER_BLOATED";
	abbreviations["bw"] = "WEED_BLADE";
	abbreviations["cw"] = "GRASS_WHEAT_CAVE";
	abbreviations["dc"] = "MUSHROOM_CUP_DIMPLE";
	abbreviations["fb"] = "BERRIES_FISHER";
	abbreviations["hr"] = "ROOT_HIDE";
	abbreviations["kb"] = "BULB_KOBOLD";
	abbreviations["lg"] = "GRASS_LONGLAND";
	abbreviations["mr"] = "ROOT_MUCK";
	abbreviations["pb"] = "BERRIES_PRICKLE";
	abbreviations["ph"] = "MUSHROOM_HELMET_PLUMP";
	abbreviations["pt"] = "GRASS_TAIL_PIG";
	abbreviations["qb"] = "BUSH_QUARRY";
	abbreviations["rr"] = "REED_ROPE";
	abbreviations["rw"] = "WEED_RAT";
	abbreviations["sb"] = "BERRY_SUN";
	abbreviations["sp"] = "POD_SWEET";
	abbreviations["vh"] = "HERB_VALLEY";
	abbreviations["ws"] = "BERRIES_STRAW_WILD";
	abbreviations["wv"] = "VINE_WHIP";
	return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result plugin_onupdate(DFHack::Core* pCore)
{
	if(running)
	{
		DFHack::Core& core = *pCore;
		std::map<t_materialIndex, unsigned int> seedCount; // the number of seeds
		std::map<t_materialIndex, t_materialSubindex> seedSubindices; // holds information needed to add entries to the cooking exclusions
		std::map<t_materialIndex, t_materialSubindex> plantSubindices; // holds information need to add entries to the cooking exclusions
		updateCountAndSubindices(core, seedCount, seedSubindices, plantSubindices);
		t_kitchenExclusions kitchenExclusions(core);
		std::map<t_materialIndex, unsigned int> watchMap;
		kitchenExclusions.fillWatchMap(watchMap);
		for(std::map<t_materialIndex, unsigned int>::const_iterator i = watchMap.begin(); i != watchMap.end(); ++i)
		{
			if(seedCount[i->first] <= i->second)
			{
				kitchenExclusions.denyCookery(i->first, seedSubindices, plantSubindices);
			}
			else if(i->second + buffer < seedCount[i->first])
			{
				kitchenExclusions.allowCookery(i->first);
			}
		}
	}	
	return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result plugin_shutdown(DFHack::Core* pCore)
{
	return DFHack::CR_OK;
}
