#include <dfhack/Core.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Items.h>
#include <dfhack/modules/Gui.h>
#include <vector>
#include <string>
#include <string.h>
#include <stdio.h> // sprintf()

using std::vector;
using std::string;
using namespace DFHack;

//////////////////////
// START item choosers
//////////////////////

class item_chooser
{
public:
    item_chooser(Core* _c, DFHack::Items* _Items) : c(_c), Items(_Items)
    {
    }

    virtual bool doPrint(DFHack::dfh_item *itm) = 0;

    virtual void postPrint(DFHack::dfh_item *itm)
    {
    }

protected:
    Core          *c;
    DFHack::Items *Items;
};

class choose_all : public item_chooser
{
public:
    choose_all(Core* _c, ::Items* _Items) : item_chooser(_c, _Items)
    {
    }

    virtual bool doPrint(DFHack::dfh_item *itm)
    {
        return (true);
    }

};

class choose_unknown : public item_chooser
{
public:
    choose_unknown(Core* _c, ::Items* _Items) : item_chooser(_c, _Items)
    {
    }

    virtual bool doPrint(DFHack::dfh_item *itm)
    {
        if (itm->base->unk1.size() > 0)
            return true;

        std::vector<std::string> refs;
        std::vector<int32_t>     values;
        if (Items->unknownRefs(*itm, refs, values))
            return true;

        t_itemflags &f = itm->base->flags;

        return (f.unk1 || f.unk2 || f.unk3 || f.unk4 || /*f.unk5 ||*/
                f.unk6 || f.unk7 ||
                // f.unk8 || f.unk9 || /* Too common */
                f.unk10 || f.unk11);
    }

    virtual void postPrint(DFHack::dfh_item *itm) 
    {
        std::vector<std::string> flags;

        t_itemflags &f = itm->base->flags;

        if (itm->base->unk1.size() > 0)
            c->con.print("      vec1: %p\n", itm->base->unk1[0]);

        std::vector<std::string> refs;
        std::vector<int32_t>     values;
        if (Items->unknownRefs(*itm, refs, values))
        {
            c->con.print("      refs: ");
            for (size_t i = 0; i < refs.size(); i++)
            {
                c->con.print("%s: %d", refs[i].c_str(), values[i]);
                if ( (i + 1) < refs.size() )
                    c->con.print(", ");
            }
            c->con.print("\n");
        }

        if (f.unk1) flags.push_back("unk1");
        if (f.unk2) flags.push_back("unk2");
        if (f.unk3) flags.push_back("unk3");
        if (f.unk4) flags.push_back("unk4");
        //if (f.unk5) flags.push_back("unk5");
        if (f.unk6) flags.push_back("unk6");
        if (f.unk7) flags.push_back("unk7");
        if (f.unk8) flags.push_back("unk8");
        if (f.unk9) flags.push_back("unk9");
        if (f.unk10) flags.push_back("unk10");
        if (f.unk11) flags.push_back("unk11");

        if (flags.size() > 0)
        {
            c->con.print("      flags: ");
            for (size_t i = 0; i < flags.size(); i++)
            {
                c->con.print("%s", flags[i].c_str());
                if ( (i + 1) < flags.size() )
                    c->con.print(", ");
            }
            c->con.print("\n");
        }
    }
};


class choose_cursor : public item_chooser
{
public:
    choose_cursor(Core* _c, ::Items* _Items, int32_t _x, int32_t _y, int32_t _z)
        : item_chooser(_c, _Items), x(_x), y(_y), z(_z)
    {
    }

    virtual bool doPrint(DFHack::dfh_item *itm)
    {
        return (itm->base->x == x && itm->base->y == y && itm->base->z == z
                && itm->base->flags.on_ground
                && !itm->base->flags.in_chest
                && !itm->base->flags.in_inventory
                && !itm->base->flags.in_building);
    }

protected:
    int32_t x, y, z;
};


//////////////////////
// END item choosers
//////////////////////

DFhackCExport command_result df_dumpitems (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "itemhacks";
}

DFhackCExport command_result plugin_init ( Core * c,
                                           std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("dumpitems",
               "Dump items\
\n              Options:\
\n                 unkown: Dump items that have anything unknown set",
               df_dumpitems));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result df_dumpitems (Core * c, vector <string> & parameters)
{
    c->Suspend();
    DFHack::Materials * Materials = c->getMaterials();
    Materials->ReadAllMaterials();

    DFHack::Gui * Gui = c->getGui();

    DFHack::Items * Items = c->getItems();
    Items->Start();

    int32_t x,y,z;
    Gui->getCursorCoords(x,y,z);

    std::vector<df_item *> p_items;
    Items->readItemVector(p_items);
    uint32_t size = p_items.size();

    item_chooser *chooser = NULL;

    if (x != -30000)
        chooser = new choose_cursor(c, Items, x, y, z);
    else if (parameters.size() == 0)
        chooser = new choose_all(c, Items);
    else if (parameters[0] == "unknown")
        chooser = new choose_unknown(c, Items);
    else
    {
        c->con.printerr("Invalid option: %s\n", parameters[0].c_str());
        Items->Finish();
        c->Resume();
        return CR_FAILURE;
    }

    for(size_t i = 0; i < size; i++)
    {
        DFHack::dfh_item itm;
        memset(&itm, 0, sizeof(DFHack::dfh_item));
        Items->readItem(p_items[i],itm);

        if (!chooser->doPrint(&itm))
            continue;

        // Print something useful, instead of (-30000,-30000,-30000), if
        // the item isn't on the ground.
        char location[80];
        if (itm.base->flags.in_chest)
            sprintf(location, "chest");
        else if (itm.base->flags.in_inventory)
            sprintf(location, "inventory");
        else if (itm.base->flags.in_building)
            sprintf(location, "building");
        else
            sprintf(location, "%d,%d,%d", itm.base->x, itm.base->y,
                    itm.base->z);

        c->con.print(
            "%5d: addr:0x%08x %6d %08x (%s) vptr:0x%08x [%d]\n"
            "       %d %s\n",
            i, itm.base, itm.base->id, itm.base->flags.whole,
            location,
            ((t_virtual *)itm.base)->vptr,
            itm.wear_level,
            itm.quantity,
            Items->getItemDescription(itm, Materials).c_str()
        );
        chooser->postPrint(&itm);
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

    Items->Finish();
    delete chooser;

    return CR_OK;
}
