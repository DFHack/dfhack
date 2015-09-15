#include "BitArray.h"
#include "df/building_farmplotst.h"
#include "df/plant_raw.h"

using namespace df::enums;

using df::global::ui;
using df::global::ui_building_item_cursor;
using df::global::world;

struct farm_select_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    df::building_farmplotst* getFarmPlot()
    {
        if (ui->main.mode != ui_sidebar_mode::QueryBuilding)
            return NULL;
        VIRTUAL_CAST_VAR(farm_plot, df::building_farmplotst, world->selected_building);
        return farm_plot;
    }

    bool isValidCrop (int32_t crop_id, int season, df::building_farmplotst* farm_plot)
    {
        // Adapted from autofarm
        using namespace df::enums::plant_raw_flags;
        // Possible to plant?
        df::plant_raw* raws = world->raws.plants.all[crop_id];
        if (raws->flags.is_set(SEED) && raws->flags.is_set((df::plant_raw_flags)season))
        {
            // Right depth?
            DFCoord cursor (farm_plot->centerx, farm_plot->centery, farm_plot->z);
            MapExtras::MapCache mc;
            MapExtras::Block * b = mc.BlockAt(cursor / 16);
            if (!b || !b->is_valid())
                return false;
            auto &block = *b->getRaw();
            df::tile_designation &des =
                block.designation[farm_plot->centerx % 16][farm_plot->centery % 16];
            if ((raws->underground_depth_min == 0 || raws->underground_depth_max == 0) != des.bits.subterranean)
            {
                return true;
            }
        }
        return false;
    }

    inline int32_t getSelectedCropId() { return ui->selected_farm_crops[*ui_building_item_cursor]; }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        df::building_farmplotst* farm_plot = getFarmPlot();
        if (farm_plot && ui->selected_farm_crops.size() > 0)
        {
            if (input->count(interface_key::SELECT_ALL))
            {
                int32_t crop_id = getSelectedCropId();
                for (int season = 0; season < 4; season++)
                {
                    if (isValidCrop(crop_id, season, farm_plot))
                    {
                        farm_plot->plant_id[season] = crop_id;
                    }
                }
            }
            else if (input->count(interface_key::DESELECT_ALL))
            {
                for (int season = 0; season < 4; season++)
                {
                    farm_plot->plant_id[season] = -1;
                }
            }
        }
        INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        auto farm_plot = getFarmPlot();
        if (!farm_plot || !ui->selected_farm_crops.size())
            return;
        if (farm_plot->getBuildStage() != farm_plot->getMaxBuildStage())
            return;
        auto dims = Gui::getDwarfmodeViewDims();
        int x = dims.menu_x1 + 1,
            y = dims.y2 - 5,
            left = x;
        OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(interface_key::SELECT_ALL));
        OutputString(COLOR_WHITE, x, y, ": All seasons", true, left);
        OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(interface_key::DESELECT_ALL));
        OutputString(COLOR_WHITE, x, y, ": Fallow all seasons", true, left);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(farm_select_hook, render);
IMPLEMENT_VMETHOD_INTERPOSE(farm_select_hook, feed);
