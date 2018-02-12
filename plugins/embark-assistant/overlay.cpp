#include <modules/Gui.h>

#include "df/coord2d.h"
#include "df/inorganic_raw.h"
#include "df/dfhack_material_category.h"
#include "df/interface_key.h"
#include "df/viewscreen.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/world.h"
#include "df/world_raws.h"

#include "finder_ui.h"
#include "help_ui.h"
#include "overlay.h"
#include "screen.h"

using df::global::world;

namespace embark_assist {
    namespace overlay {
        DFHack::Plugin *plugin_self;
        const Screen::Pen empty_pen = Screen::Pen('\0', COLOR_YELLOW, COLOR_BLACK, false);
        const Screen::Pen yellow_x_pen = Screen::Pen('X', COLOR_BLACK, COLOR_YELLOW, false);
        const Screen::Pen green_x_pen = Screen::Pen('X', COLOR_BLACK, COLOR_GREEN, false);

        struct display_strings {
            Screen::Pen pen;
            std::string text;
        };

        typedef Screen::Pen *pen_column;

        struct states {
            int blink_count = 0;
            bool show = true;

            bool matching = false;
            bool match_active = false;

            embark_update_callbacks embark_update;
            match_callbacks match_callback;
            clear_match_callbacks clear_match_callback;
            embark_assist::defs::find_callbacks find_callback;
            shutdown_callbacks shutdown_callback;

            Screen::Pen site_grid[16][16];
            uint8_t current_site_grid = 0;

            std::vector<display_strings> embark_info;

            Screen::Pen region_match_grid[16][16];

            pen_column *world_match_grid = nullptr;
            uint16_t match_count = 0;

            uint16_t max_inorganic;
        };

        static states *state = nullptr;

        //====================================================================

/*     //  Attempt to replicate the DF logic for sizing the right world map. This
       //  code seems to compute the values correctly, but the author hasn't been
       //  able to apply them at the same time as DF does to 100%.
       //  DF seems to round down on 0.5 values.
       df::coord2d  world_dimension_size(uint16_t available_screen, uint16_t map_size) {
            uint16_t result;

            for (uint16_t factor = 1; factor < 17; factor++) {
                result = map_size / factor;
                if ((map_size - result * factor) * 2 != factor) {
                    result = (map_size + factor / 2) / factor;
                }
                
                if (result <= available_screen) {
                    return {result, factor};
                }
            }
            return{16, 16};  //  Should never get here.
        }
*/
        //====================================================================

        class ViewscreenOverlay : public df::viewscreen_choose_start_sitest
        {
        public:
            typedef df::viewscreen_choose_start_sitest interpose_base;

            void send_key(const df::interface_key &key)
            {
                std::set< df::interface_key > keys;
                keys.insert(key);
                this->feed(&keys);
            }

            DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
            {
//                color_ostream_proxy out(Core::getInstance().getConsole());
                if (input->count(df::interface_key::CUSTOM_Q)) {
                    state->shutdown_callback();
                    return;

                }
                else if (input->count(df::interface_key::SETUP_LOCAL_X_MUP) ||
                    input->count(df::interface_key::SETUP_LOCAL_X_MDOWN) ||
                    input->count(df::interface_key::SETUP_LOCAL_Y_MUP) ||
                    input->count(df::interface_key::SETUP_LOCAL_Y_MDOWN) ||
                    input->count(df::interface_key::SETUP_LOCAL_X_UP) ||
                    input->count(df::interface_key::SETUP_LOCAL_X_DOWN) ||
                    input->count(df::interface_key::SETUP_LOCAL_Y_UP) ||
                    input->count(df::interface_key::SETUP_LOCAL_Y_DOWN)) {
                    INTERPOSE_NEXT(feed)(input);
                    state->embark_update();
                }
                else if (input->count(df::interface_key::CUSTOM_C)) {
                    state->match_active = false;
                    state->matching = false;
                    state->clear_match_callback();
                }
                else if (input->count(df::interface_key::CUSTOM_F)) {
                    if (!state->match_active && !state->matching) {
                        embark_assist::finder_ui::init(embark_assist::overlay::plugin_self, state->find_callback, state->max_inorganic);
                    }
                }
                else if (input->count(df::interface_key::CUSTOM_I)) {
                    embark_assist::help_ui::init(embark_assist::overlay::plugin_self);
                }
                else {
                    INTERPOSE_NEXT(feed)(input);
                }
            }

            //====================================================================

            DEFINE_VMETHOD_INTERPOSE(void, render, ())
            {
                INTERPOSE_NEXT(render)();
//                color_ostream_proxy out(Core::getInstance().getConsole());
                auto current_screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
                int16_t x = current_screen->location.region_pos.x;
                int16_t y = current_screen->location.region_pos.y;
                auto width = Screen::getWindowSize().x;
                auto height = Screen::getWindowSize().y;

                state->blink_count++;
                if (state->blink_count == 35) {
                    state->blink_count = 0;
                    state->show = !state->show;
                }

                if (state->matching) state->show = true;

                Screen::drawBorder("Embark Assistant");

                Screen::Pen pen_lr(' ', COLOR_LIGHTRED);
                Screen::Pen pen_w(' ', COLOR_WHITE);

                Screen::paintString(pen_lr, width - 28, 20, "i", false);
                Screen::paintString(pen_w, width - 27, 20, ":Embark Assistant Info", false);
                Screen::paintString(pen_lr, width - 28, 21, "f", false);
                Screen::paintString(pen_w, width - 27, 21, ":Find Embark ", false);
                Screen::paintString(pen_lr, width - 28, 22, "c", false);
                Screen::paintString(pen_w, width - 27, 22, ":Cancel/Clear Find", false);
                Screen::paintString(pen_lr, width - 28, 23, "q", false);
                Screen::paintString(pen_w, width - 27, 23, ":Quit Embark Assistant", false);
                Screen::paintString(pen_w, width - 28, 25, "Matching World Tiles", false);
                Screen::paintString(empty_pen, width - 7, 25, to_string(state->match_count), false);

                if (height > 25) {  //  Mask the vanilla DF find help as it's overridden.
                    Screen::paintString(pen_w, 50, height - 2, "                          ", false);
                }

                for (uint8_t i = 0; i < 16; i++) {
                    for (uint8_t k = 0; k < 16; k++) {
                        if (state->site_grid[i][k].ch) {
                            Screen::paintTile(state->site_grid[i][k], i + 1, k + 2);
                        }
                    }
                }

                for (auto i = 0; i < state->embark_info.size(); i++) {
                    embark_assist::screen::paintString(state->embark_info[i].pen, 1, i + 19, state->embark_info[i].text, false);
                }

                if (state->show) {
                    int16_t left_x = x - (width / 2 - 7 - 18 + 1) / 2;
                    int16_t right_x;
                    int16_t top_y = y - (height - 8 - 2 + 1) / 2;
                    int16_t bottom_y;

                    if (left_x < 0) { left_x = 0; }

                    if (top_y < 0) { top_y = 0; }
                    
                    right_x = left_x + width / 2 - 7 - 18;
                    bottom_y = top_y + height - 8 - 2;
                    
                    if (right_x >= world->worldgen.worldgen_parms.dim_x) {
                        right_x = world->worldgen.worldgen_parms.dim_x - 1;
                        left_x = right_x - (width / 2 - 7 - 18);
                    }

                    if (bottom_y >= world->worldgen.worldgen_parms.dim_y) {
                        bottom_y = world->worldgen.worldgen_parms.dim_y - 1;
                        top_y = bottom_y - (height - 8 - 2);
                    }

                    if (left_x < 0) { left_x = 0; }

                    if (top_y < 0) { top_y = 0; }


                    for (uint16_t i = left_x; i <= right_x; i++) {
                        for (uint16_t k = top_y; k <= bottom_y; k++) {
                            if (state->world_match_grid[i][k].ch) {
                                Screen::paintTile(state->world_match_grid[i][k], i - left_x + 18, k - top_y + 2);
                            }
                        }
                    }

                    for (uint8_t i = 0; i < 16; i++) {
                        for (uint8_t k = 0; k < 16; k++) {
                            if (state->region_match_grid[i][k].ch) {
                                Screen::paintTile(state->region_match_grid[i][k], i + 1, k + 2);
                            }
                        }
                    }
                    
/*                    //  Stuff for trying to replicate the DF right world map sizing logic. Close, but not there.
                    Screen::Pen pen(' ', COLOR_YELLOW);
                    //  Boundaries of the top level world map
                    Screen::paintString(pen, width / 2 - 5, 2, "X", false);  //  Marks UL corner of right world map. Constant
//                    Screen::paintString(pen, width - 30, 2, "X", false);     //  Marks UR corner of right world map area.
//                    Screen::paintString(pen, width / 2 - 5, height - 8, "X", false);  //  BL corner of right world map area.
//                    Screen::paintString(pen, width - 30, height - 8, "X", false);     //  BR corner of right world map area.
                    
                    uint16_t l_width = width - 30 - (width / 2 - 5) + 1;  //  Horizontal space available for right world map.
                    uint16_t l_height = height - 8 - 2 + 1;               //  Vertical space available for right world map.
                    df::coord2d size_factor_x = world_dimension_size(l_width, world->worldgen.worldgen_parms.dim_x);
                    df::coord2d size_factor_y = world_dimension_size(l_height, world->worldgen.worldgen_parms.dim_y);

                    Screen::paintString(pen, width / 2 - 5 + size_factor_x.x - 1, 2, "X", false);
                    Screen::paintString(pen, width / 2 - 5, 2 + size_factor_y.x - 1, "X", false);
                    Screen::paintString(pen, width / 2 - 5 + size_factor_x.x - 1, 2 + size_factor_y.x - 1, "X", false);
                    */
                }

                if (state->matching) {
                    embark_assist::overlay::state->match_callback();
                }
            }
        };

        IMPLEMENT_VMETHOD_INTERPOSE(embark_assist::overlay::ViewscreenOverlay, feed);
        IMPLEMENT_VMETHOD_INTERPOSE(embark_assist::overlay::ViewscreenOverlay, render);
    }
}

//====================================================================

bool embark_assist::overlay::setup(DFHack::Plugin *plugin_self,
    embark_update_callbacks embark_update_callback,
    match_callbacks match_callback,
    clear_match_callbacks clear_match_callback,
    embark_assist::defs::find_callbacks find_callback,
    shutdown_callbacks shutdown_callback,
    uint16_t max_inorganic)
{
//    color_ostream_proxy out(Core::getInstance().getConsole());
    state = new(states);

    embark_assist::overlay::plugin_self = plugin_self;
    embark_assist::overlay::state->embark_update = embark_update_callback;
    embark_assist::overlay::state->match_callback = match_callback;
    embark_assist::overlay::state->clear_match_callback = clear_match_callback;
    embark_assist::overlay::state->find_callback = find_callback;
    embark_assist::overlay::state->shutdown_callback = shutdown_callback;
    embark_assist::overlay::state->max_inorganic = max_inorganic;
    embark_assist::overlay::state->match_active = false;

    state->world_match_grid = new pen_column[world->worldgen.worldgen_parms.dim_x];
    if (!state->world_match_grid) {
        return false;  //  Out of memory
    }

    for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
        state->world_match_grid[i] = new Screen::Pen[world->worldgen.worldgen_parms.dim_y];
        if (!state->world_match_grid[i]) {  //  Out of memory.
            return false;
        }
    }

    clear_match_results();

    return INTERPOSE_HOOK(embark_assist::overlay::ViewscreenOverlay, feed).apply(true) &&
        INTERPOSE_HOOK(embark_assist::overlay::ViewscreenOverlay, render).apply(true);
}

//====================================================================

void embark_assist::overlay::set_sites(embark_assist::defs::site_lists *site_list) {
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            state->site_grid[i][k] = empty_pen;
        }
    }

    for (uint16_t i = 0; i < site_list->size(); i++) {
        state->site_grid[site_list->at(i).x][site_list->at(i).y].ch = site_list->at(i).type;
    }
}

//====================================================================

void embark_assist::overlay::initiate_match() {
    embark_assist::overlay::state->matching = true;
}

//====================================================================

void embark_assist::overlay::match_progress(uint16_t count, embark_assist::defs::match_results *match_results, bool done) {
//    color_ostream_proxy out(Core::getInstance().getConsole());
    state->matching = !done;
    state->match_count = count;
    for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
        for (uint16_t k = 0; k < world->worldgen.worldgen_parms.dim_y; k++) {
            if (match_results->at(i).at(k).preliminary_match) {
                state->world_match_grid[i][k] = yellow_x_pen;

            } else if (match_results->at(i).at(k).contains_match) {
                state->world_match_grid[i][k] = green_x_pen;
            }
            else {
                state->world_match_grid[i][k] = empty_pen;
            }
        }
    }
}

//====================================================================

void embark_assist::overlay::set_embark(embark_assist::defs::site_infos *site_info) {
    state->embark_info.clear();

    if (site_info->sand) {
        state->embark_info.push_back({ Screen::Pen(' ', COLOR_YELLOW), "Sand" });
    }

    if (site_info->clay) {
        state->embark_info.push_back({ Screen::Pen(' ', COLOR_RED), "Clay" });
    }

    state->embark_info.push_back({ Screen::Pen(' ', COLOR_BROWN), "Soil " + std::to_string(site_info->min_soil) + " - " + std::to_string(site_info->max_soil) });

    if (site_info->flat) {
        state->embark_info.push_back({ Screen::Pen(' ', COLOR_BROWN), "Flat" });
    }

    if (site_info->aquifer) {
        if (site_info->aquifer_full) {
            state->embark_info.push_back({ Screen::Pen(' ', COLOR_BLUE), "Aquifer" });
        
        }
        else {
            state->embark_info.push_back({ Screen::Pen(' ', COLOR_LIGHTBLUE), "Aquifer" });
        }
    }

    if (site_info->waterfall) {
        state->embark_info.push_back({ Screen::Pen(' ', COLOR_BLUE), "Waterfall" });
    }

    if (site_info->flux) {
        state->embark_info.push_back({ Screen::Pen(' ', COLOR_WHITE), "Flux" });
    }

    for (auto const& i : site_info->metals) {
        state->embark_info.push_back({ Screen::Pen(' ', COLOR_GREY), world->raws.inorganics[i]->id });
    }

    for (auto const& i : site_info->economics) {
        state->embark_info.push_back({ Screen::Pen(' ', COLOR_WHITE), world->raws.inorganics[i]->id });
    }
}

//====================================================================

void embark_assist::overlay::set_mid_level_tile_match(embark_assist::defs::mlt_matches mlt_matches) {
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            if (mlt_matches[i][k]) {
                state->region_match_grid[i][k] = green_x_pen;

            }
            else {
                state->region_match_grid[i][k] = empty_pen;
            }
        }
    }
}

//====================================================================

void embark_assist::overlay::clear_match_results() {
    for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
        for (uint16_t k = 0; k < world->worldgen.worldgen_parms.dim_y; k++) {
            state->world_match_grid[i][k] = empty_pen;
        }
    }

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t k = 0; k < 16; k++) {
            state->region_match_grid[i][k] = empty_pen;
        }
    }
}

//====================================================================

void embark_assist::overlay::shutdown() {
    if (state &&
        state->world_match_grid) {
        INTERPOSE_HOOK(ViewscreenOverlay, render).remove();
        INTERPOSE_HOOK(ViewscreenOverlay, feed).remove();

        for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
            delete[] state->world_match_grid[i];
        }

        delete[] state->world_match_grid;
    }

    if (state) {
        state->embark_info.clear();
        delete state;
        state = nullptr;
    }
}
