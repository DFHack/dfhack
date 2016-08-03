#include "df/building_stockpilest.h"
#include "df/viewscreen_dwarfmodest.h"

using namespace DFHack;
using namespace df::enums;

using df::global::world;

static bool in_wheelbarrow_entry;
static std::string wheelbarrow_entry;

struct max_wheelbarrow_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    int wheelbarrow_count()
    {
        int ret = 0;
        std::stringstream tmp(wheelbarrow_entry);
        tmp >> ret;
        return ret;
    }

    df::building_stockpilest* getStockpile()
    {
        if (ui->main.mode != ui_sidebar_mode::QueryBuilding)
            return NULL;
        return virtual_cast<df::building_stockpilest>(world->selected_building);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        df::building_stockpilest* stockpile = getStockpile();
        if (stockpile && in_wheelbarrow_entry)
        {
            auto dims = Gui::getDwarfmodeViewDims();
            Screen::paintString(Screen::Pen(' ', COLOR_LIGHTCYAN),
                dims.menu_x1 + 22, dims.y1 + 6, wheelbarrow_entry + "_  ");
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key>* input))
    {
        df::building_stockpilest* stockpile = getStockpile();
        bool handled = false;
        if (stockpile)
        {
            auto dims = Gui::getDwarfmodeViewDims();
            handled = true;
            if (!in_wheelbarrow_entry &&
                input->count(df::interface_key::BUILDJOB_STOCKPILE_WHEELBARROW))
            {
                in_wheelbarrow_entry = true;
                std::stringstream tmp;
                tmp << stockpile->max_wheelbarrows;
                tmp >> wheelbarrow_entry;
            }
            else if (in_wheelbarrow_entry)
            {
                if (input->count(df::interface_key::SELECT) ||
                    input->count(df::interface_key::LEAVESCREEN) ||
                    input->count(df::interface_key::LEAVESCREEN_ALL) ||
                    input->count(df::interface_key::BUILDJOB_STOCKPILE_WHEELBARROW))
                {
                    in_wheelbarrow_entry = false;
                    stockpile->max_wheelbarrows = std::min(wheelbarrow_count(),
                        Buildings::countExtentTiles(&stockpile->room));
                }
                else if (input->count(df::interface_key::STRING_A000) &&
                    wheelbarrow_entry.size())
                {
                    wheelbarrow_entry.resize(wheelbarrow_entry.size() - 1);
                }
                else
                {
                    for (auto iter = input->begin(); iter != input->end(); ++iter)
                    {
                        df::interface_key key = *iter;
                        if (key >= Screen::charToKey('0') && key <= Screen::charToKey('9') &&
                            wheelbarrow_entry.size() < 3)
                        {
                            wheelbarrow_entry.push_back(Screen::keyToChar(key));
                        }
                    }
                }
            }
            else
                handled = false;
        }
        if (!handled)
            INTERPOSE_NEXT(feed)(input);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(max_wheelbarrow_hook, render);
IMPLEMENT_VMETHOD_INTERPOSE(max_wheelbarrow_hook, feed);
