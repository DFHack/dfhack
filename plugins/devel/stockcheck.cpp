#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"

#include "modules/MapCache.h"
#include "modules/Items.h"

#include "df/world.h"
#include "df/plotinfost.h"
#include "df/building_stockpilest.h"
#include "df/general_ref.h"
#include "df/global_objects.h"
#include "df/item.h"
#include "df/map_block.h"
#include "df/unit.h"
#include "df/building.h"
#include "df/items_other_id.h"
#include "df/item_stockpile_ref.h"

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::plotinfo;
using df::global::selection_rect;

using df::building_stockpilest;

static command_result stockcheck(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("stockcheck");

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (world && plotinfo) {
        commands.push_back(
            PluginCommand("stockcheck", "Check for unprotected rottable items.",
                stockcheck, false,
                "Scan world for items that are susceptible to rot.  Currently just lists the items.\n"
            )
        );
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

struct StockpileInfo {
    building_stockpilest* sp;
    int size;
    int free;
    int x1, x2, y1, y2, z;

public:
    StockpileInfo(building_stockpilest *sp_) : sp(sp_)
    {
        MapExtras::MapCache mc;

        z = sp_->z;
        x1 = sp_->room.x;
        x2 = sp_->room.x + sp_->room.width;
        y1 = sp_->room.y;
        y2 = sp_->room.y + sp_->room.height;
        int e = 0;
        size = 0;
        free = 0;
        for (int y = y1; y < y2; y++)
            for (int x = x1; x < x2; x++)
                if (sp_->room.extents[e++] == 1)
                {
                    size++;
                    DFCoord cursor (x,y,z);
                    uint32_t tileX = x % 16;
                    uint32_t tileY = y % 16;
                    MapExtras::Block * b = mc.BlockAt(cursor/16);
                    if(b && b->is_valid())
                    {
                        auto &block = *b->getRaw();
                        df::tile_occupancy &occ = block.occupancy[tileX][tileY];
                        if (!occ.bits.item)
                            free++;
                    }
                }
    }

    bool isFull() { return free == 0; }

    bool canHold(df::item *i)
    {
        return false;
    }

    bool inStockpile(df::item *i)
    {
        df::item *container = Items::getContainer(i);
        if (container)
            return inStockpile(container);

        if (i->pos.z != z) return false;
        if (i->pos.x < x1 || i->pos.x >= x2 ||
            i->pos.y < y1 || i->pos.y >= y2) return false;
        int e = (i->pos.x - x1) + (i->pos.y - y1) * sp->room.width;
        return sp->room.extents[e] == 1;
    }

    int getId() { return sp->id; }
};

static command_result stockcheck(color_ostream &out, vector <string> & parameters)
{
    std::vector<StockpileInfo*> stockpiles;

    for (df::building *build : world->buildings.all)
    {
        auto type = build->getType();
        if (df::enums::building_type::Stockpile == type)
        {
            building_stockpilest *sp = virtual_cast<building_stockpilest>(build);
            StockpileInfo *spi = new StockpileInfo(sp);
            stockpiles.push_back(spi);
        }

    }

    std::vector<df::item*> &items = world->items.other[items_other_id::IN_PLAY];

    // Precompute a bitmask with the bad flags
    df::item_flags bad_flags;
    bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
    F(dump); F(forbid); F(garbage_collect);
    F(hostile); F(on_fire); F(rotten); F(trader);
    F(in_building); F(construction); F(artifact);
    F(spider_web); F(owned); F(in_job);
#undef F

    for (size_t i = 0; i < items.size(); i++)
    {
        df::item *item = items[i];
        if (item->flags.whole & bad_flags.whole)
            continue;

        // we really only care about MEAT, FISH, FISH_RAW, PLANT, CHEESE, FOOD, and EGG

        df::item_type typ = item->getType();
        if (typ != df::enums::item_type::MEAT &&
            typ != df::enums::item_type::FISH &&
            typ != df::enums::item_type::FISH_RAW &&
            typ != df::enums::item_type::PLANT &&
            typ != df::enums::item_type::CHEESE &&
            typ != df::enums::item_type::FOOD &&
            typ != df::enums::item_type::EGG)
            continue;

        df::item *container = 0;
        df::unit *holder = 0;
        df::building *building = 0;

        for (size_t i = 0; i < item->general_refs.size(); i++)
        {
            df::general_ref *ref = item->general_refs[i];

            switch (ref->getType())
            {
            case general_ref_type::CONTAINED_IN_ITEM:
                container = ref->getItem();
                break;

            case general_ref_type::UNIT_HOLDER:
                holder = ref->getUnit();
                break;

            case general_ref_type::BUILDING_HOLDER:
                building = ref->getBuilding();
                break;

            default:
                break;
            }
        }

        df::item *nextcontainer = container;
        df::item *lastcontainer = 0;

        while(nextcontainer) {
            df::item *thiscontainer = nextcontainer;
            nextcontainer = 0;
            for (size_t i = 0; i < thiscontainer->general_refs.size(); i++)
            {
                df::general_ref *ref = thiscontainer->general_refs[i];

                switch (ref->getType())
                {
                case general_ref_type::CONTAINED_IN_ITEM:
                    lastcontainer = nextcontainer = ref->getItem();
                    break;

                case general_ref_type::UNIT_HOLDER:
                    holder = ref->getUnit();
                    break;

                case general_ref_type::BUILDING_HOLDER:
                    building = ref->getBuilding();
                    break;

                default:
                    break;
                }
            }
        }

        if (holder)
            continue; // carried items do not rot as far as i know

        if (building) {
            df::building_type btype = building->getType();
            if (btype == df::enums::building_type::TradeDepot ||
                btype == df::enums::building_type::Wagon)
                continue; // items in trade depot or the embark wagon do not rot

            if (typ == df::enums::item_type::EGG && btype ==df::enums::building_type::NestBox)
                continue; // eggs in nest box do not rot
        }

        int canHoldCount = 0;
        StockpileInfo *current = 0;

        for (StockpileInfo *spi : stockpiles)
        {
            if (spi->canHold(item)) canHoldCount++;
            if (spi->inStockpile(item)) current=spi;
        }

        if (current)
            continue;

        std::string description;
        item->getItemDescription(&description, 0);
        out << " * " << description;

        if (container) {
            std::string containerDescription;
            container->getItemDescription(&containerDescription, 0);
            out << ", in container " << containerDescription;
            if (lastcontainer) {
                std::string lastcontainerDescription;
                lastcontainer->getItemDescription(&lastcontainerDescription, 0);
                out << ", in container " << lastcontainerDescription;
            }
        }

        if (holder) {
            out << ", carried";
        }

        if (building) {
            out << ", in building " << building->id << " (type=" << building->getType() << ")";
        }

        out << ", flags=" << std::hex << item->flags.whole << std::dec;
        out << endl;

    }

    return CR_OK;
}
