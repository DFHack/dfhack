#include <dfhack/Core.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Items.h>
#include <dfhack/modules/Gui.h>
#include <vector>
#include <string>
#include <string.h>

using std::vector;
using std::string;
using namespace DFHack;

DFhackCExport command_result df_dumpitems (Core * c, vector <string> & parameters);
DFhackCExport command_result df_itscanvec1 (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "itemhacks";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("dumpitems", "Dump items...", df_dumpitems));
    commands.push_back(PluginCommand("itscanvec1", "Dump items that have the first vector valid.", df_itscanvec1));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result df_itscanvec1 (Core * c, vector <string> & parameters)
{
    c->Suspend();
    DFHack::Items * Items = c->getItems();
    Items->Start();
    std::vector<t_item *> p_items;
    Items->readItemVector(p_items);
    for(int i = 0; i < p_items.size();i++)
    {
        t_item * itm = p_items[i];
        if(itm->unk2.size())
        {
            c->con.print("Found %x, size %d\n",itm,itm->unk2.size());
        }
    }
    c->Resume();
    return CR_OK;
}

DFhackCExport command_result df_dumpitems (Core * c, vector <string> & parameters)
{
    c->Suspend();
    bool print_hex = false;
    if(parameters.size() && parameters[0] == "hex")
        print_hex = true;
    DFHack::Materials * Materials = c->getMaterials();
    Materials->ReadAllMaterials();

    DFHack::Gui * Gui = c->getGui();

    DFHack::Items * Items = c->getItems();
    Items->Start();

    int32_t x,y,z;
    Gui->getCursorCoords(x,y,z);

    std::vector<t_item *> p_items;
    Items->readItemVector(p_items);
    uint32_t size = p_items.size();

    for(size_t i = 0; i < size; i++)
    {
        DFHack::dfh_item itm;
        memset(&itm, 0, sizeof(DFHack::dfh_item));
        Items->readItem(p_items[i],itm);

        if (x != -30000
            && !(itm.base->x == x && itm.base->y == y && itm.base->z == z
                 && itm.base->flags.on_ground
                 && !itm.base->flags.in_chest
                 && !itm.base->flags.in_inventory
                 && !itm.base->flags.in_building))
            continue;

        c->con.print(
            "%5d: addr:0x%08x %6d %08x (%d,%d,%d) vptr:0x%08x [%d] *%d %s - %s\n",
            i, itm.base, itm.base->id, itm.base->flags.whole,
            itm.base->x, itm.base->y, itm.base->z,
            itm.base->vptr,
            itm.wear_level,
            itm.quantity,
            Items->getItemClass(itm.matdesc.itemType).c_str(),
            Items->getItemDescription(itm, Materials).c_str()
        );
    /*
        if (print_hex)
            hexdump(DF,p_items[i],0x300);
*/
        /*
        if (print_acc)
            c->con << Items->dumpAccessors(itm) << endl;
        */
/*
        if (print_refs) {
            DFHack::DfVector<uint32_t> p_refs(p, itm.origin + ref_vector);
            for (size_t j = 0; j < p_refs.size(); j++) {
                uint32_t vptr = p->readDWord(p_refs[j]);
                uint32_t val = p->readDWord(p_refs[j]+4);
                c->con.print("\t-> %d \t%s\n", int(val), p->readClassName(vptr).c_str());
            }
        }
        */
    }
    c->Resume();
    return CR_OK;
}
