#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Maps.h>
#include <dfhack/modules/Items.h>
#include <dfhack/modules/Creatures.h>
#include <dfhack/modules/Gui.h>

using namespace DFHack;

#include <vector>
#include <string>
#include <string.h>
using std::vector;
using std::string;

DFhackCExport command_result clean (Core * c, vector <string> & parameters);
DFhackCExport command_result spotclean (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "cleaners";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("clean","Removes contaminants from map tiles, items and creatures.",clean));
    commands.push_back(PluginCommand("spotclean","Cleans map tile under cursor.",spotclean));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

command_result cleanmap (Core * c, bool snow, bool mud)
{
    const uint32_t water_idx = 6;
    const uint32_t mud_idx = 12;
    vector<DFHack::t_spattervein *> splatter;
    DFHack::Maps *Mapz = c->getMaps();

    // init the map
    if(!Mapz->Start())
    {
        c->con << "Can't init map." << std::endl;
        c->Resume();
        return CR_FAILURE;
    }

    uint32_t x_max,y_max,z_max;
    Mapz->getSize(x_max,y_max,z_max);
    int num_blocks = 0;
    int blocks_total = 0;
    // walk the map
    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                df_block * block = Mapz->getBlock(x,y,z);
                if(block)
                {
                    blocks_total ++;
                    bool cleaned = false;
                    Mapz->SortBlockEvents(x,y,z,0,0,&splatter);
                    for(int i = 0; i < 16; i++)
                        for(int j = 0; j < 16; j++)
                        {
                            block->occupancy[i][j].bits.arrow_color = 0;
                            block->occupancy[i][j].bits.broken_arrows_variant = 0;
                        }
                    for(uint32_t i = 0; i < splatter.size(); i++)
                    {
                        DFHack::t_spattervein * vein = splatter[i];
                        // filter snow
                        if(!snow
                            && vein->mat1 == DFHack::Materials::WATER
                            && vein->matter_state == DFHack::state_powder)
                            continue;
                        // filter mud
                        if(!mud
                            && vein->mat1 == DFHack::Materials::MUD
                            && vein->matter_state == DFHack::state_solid)
                            continue;
                        Mapz->RemoveBlockEvent(x,y,z,(t_virtual *) vein);
                        cleaned = true;
                    }
                    num_blocks += cleaned;
                }
            }
        }
    }
    if(num_blocks)
        c->con.print("Cleaned %d of %d map blocks.\n", num_blocks, blocks_total);
    return CR_OK;
}

command_result cleanitems (Core * c)
{
    DFHack::Items * Items = c->getItems();

    vector <df_item*> p_items;
    if(!Items->readItemVector(p_items))
    {
        c->con.printerr("Can't access the item vector.\n");
        c->Resume();
        return CR_FAILURE;
    }
    std::size_t numItems = p_items.size();

    int cleaned_items = 0, cleaned_total = 0;
    for (std::size_t i = 0; i < numItems; i++)
    {
        df_item * itm = p_items[i];
        if(!itm->contaminants)
            continue;
        if (itm->contaminants->size())
        {
            for(int j = 0; j < itm->contaminants->size(); j++)
                delete itm->contaminants->at(j);
            cleaned_items++;
            cleaned_total += itm->contaminants->size();
            itm->contaminants->clear();
        }
    }
    if(cleaned_total)
        c->con.print("Removed %d contaminants from %d items.\n", cleaned_total, cleaned_items);
    return CR_OK;
}

command_result cleanunits (Core * c)
{
    DFHack::Creatures * Creatures = c->getCreatures();

    uint32_t num_creatures;
    if (!Creatures->Start(num_creatures))
    {
        c->con.printerr("Can't read unit list!\n");
        c->Resume();
        return CR_FAILURE;
    }
    int cleaned_units = 0, cleaned_total = 0;
    for (std::size_t i = 0; i < num_creatures; i++)
    {
        df_creature *unit = Creatures->creatures->at(i);
        int num = unit->contaminants.size();
        if (num)
        {
            for(int j = 0; j < unit->contaminants.size(); j++)
                delete unit->contaminants.at(j);
            cleaned_units++;
            cleaned_total += num;
            unit->contaminants.clear();
        }
    }
    if(cleaned_total)
        c->con.print("Removed %d contaminants from %d creatures.\n", cleaned_total, cleaned_units);
    return CR_OK;
}

DFhackCExport command_result spotclean (Core * c, vector <string> & parameters)
{
    c->Suspend();
    vector<DFHack::t_spattervein *> splatter;
    DFHack::Maps *Mapz = c->getMaps();
    DFHack::Gui *Gui = c->getGui();
    // init the map
    if(!Mapz->Start())
    {
        c->con.printerr("Can't init map.\n");
        c->Resume();
        return CR_FAILURE;
    }
    int32_t cursorX, cursorY, cursorZ;
    Gui->getCursorCoords(cursorX,cursorY,cursorZ);
    if(cursorX == -30000)
    {
        c->con.printerr("The cursor is not active.\n");
        c->Resume();
        return CR_FAILURE;
    }
    int32_t blockX = cursorX / 16, blockY = cursorY / 16;
    int32_t tileX = cursorX % 16, tileY = cursorY % 16;
    df_block *b = Mapz->getBlock(blockX,blockY,cursorZ);
    vector <t_spattervein *> spatters;
    Mapz->SortBlockEvents(blockX, blockY, cursorZ, 0,0, &spatters);
    for(int i = 0; i < spatters.size(); i++)
    {
        spatters[i]->intensity[tileX][tileY] = 0;
    }
    c->Resume();
    return CR_OK;
}

DFhackCExport command_result clean (Core * c, vector <string> & parameters)
{
    bool help = false;
    bool map = false;
    bool snow = false;
    bool mud = false;
    bool units = false;
    bool items = false;
    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "map")
            map = true;
        else if(parameters[i] == "units")
            units = true;
        else if(parameters[i] == "items")
            items = true;
        else if(parameters[i] == "all")
        {
            map = true;
            items = true;
            units = true;
        }
        if(parameters[i] == "snow")
            snow = true;
        else if(parameters[i] == "mud")
            mud = true;
        else if(parameters[i] == "help" ||parameters[i] == "?")
        {
            help = true;
        }
    }
    if(!map && !units && !items)
        help = true;
    if(help)
    {
        c->con.print("Removes contaminants from map tiles, items and creatures.\n"
        "Options:\n"
        "map        - clean the map tiles\n"
        "items      - clean all items\n"
        "units      - clean all creatures\n"
        "all        - clean everything.\n"
        "More options for 'map':\n"
        "snow       - also remove snow\n"
        "mud        - also remove mud\n"
        "Example: clean all mud snow\n"
        "This removes all spatter, including mud and snow from map tiles."
        );
        return CR_OK;
    }
    c->Suspend();
    if(map)
        cleanmap(c,snow,mud);
    if(units)
        cleanunits(c);
    if(items)
        cleanitems(c);
    c->Resume();
    return CR_OK;
}
