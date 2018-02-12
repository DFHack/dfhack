#include "Core.h"
#include <Console.h>

#include <modules/Gui.h>

#include "Types.h"

#include "df/biome_type.h"
#include "df/inorganic_raw.h"
#include "df/material_flags.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/world.h"
#include "df/world_region_type.h"
#include "df/world_raws.h"

#include "embark-assistant.h"
#include "finder_ui.h"
#include "screen.h"

using df::global::world;

namespace embark_assist {
    namespace finder_ui {

        enum class fields : int8_t {
            x_dim,
            y_dim,
            savagery_calm,
            savagery_medium,
            savagery_savage,
            good,
            neutral,
            evil,
            aquifer,
            min_river,
            max_river,
            waterfall,
            flat,
            clay,
            sand,
            flux,
            soil_min,
            soil_min_everywhere,
            soil_max,
            evil_weather,
            reanimation,
            thralling,
            biome_count_min,
            biome_count_max,
            region_type_1,
            region_type_2,
            region_type_3,
            biome_1,
            biome_2,
            biome_3,
            metal_1,
            metal_2,
            metal_3,
            economic_1,
            economic_2,
            economic_3,
            mineral_1,
            mineral_2,
            mineral_3
        };
        fields first_fields = fields::x_dim;
        fields last_fields = fields::mineral_3;

        struct display_map_elements {
            std::string text;
            int16_t key;
        };

        typedef std::vector <display_map_elements> display_maps;
        typedef std::vector <display_map_elements> name_lists;
        typedef std::list< display_map_elements> sort_lists;

        struct ui_lists {
            uint16_t current_display_value;  //  Not the value itself, but a reference to its index.
            int16_t current_value;           //  The integer representation of the value (if an enum).
            uint16_t current_index;          //  What's selected
            uint16_t focus;                  //  The value under the (possibly inactive) cursor
            display_maps list;               //  The strings to be displayed together with keys
                                             //  to allow location of the actual elements (e.g. a raws.inorganics mat_index
                                             //  or underlying enum value).
        };

        typedef std::vector <ui_lists*> uis;

        const DFHack::Screen::Pen active_pen(' ', COLOR_YELLOW);
        const DFHack::Screen::Pen passive_pen(' ', COLOR_DARKGREY);
        const DFHack::Screen::Pen normal_pen(' ', COLOR_GREY);
        const DFHack::Screen::Pen white_pen(' ', COLOR_WHITE);
        const DFHack::Screen::Pen lr_pen(' ', COLOR_LIGHTRED);

        //==========================================================================================================

        struct states {
            embark_assist::defs::find_callbacks find_callback;
            uis ui;
            display_maps finder_list;  //  Don't need the element key, but it's easier to use the same type.
            uint16_t finder_list_focus;
            bool finder_list_active;
            uint16_t max_inorganic;
        };

        static states *state = 0;

        //==========================================================================================================

        bool compare(const display_map_elements& first, const display_map_elements& second) {
            uint16_t i = 0;
            while (i < first.text.length() && i < second.text.length()) {
                if (first.text[i] < second.text[i]) {
                    return true;
                }
                else if (first.text[i] > second.text[i]) {
                    return false;
                }
                ++i;
            }
            return first.text.length() < second.text.length();
        }

        //==========================================================================================================

        void append(sort_lists *sort_list, display_map_elements element) {
            sort_lists::iterator iterator;
            for (iterator = sort_list->begin(); iterator != sort_list->end(); ++iterator) {
                if (iterator->key == element.key) {
                    return;
                }
            }
            sort_list->push_back(element);
        }

        //==========================================================================================================

        void ui_setup(embark_assist::defs::find_callbacks find_callback, uint16_t max_inorganic) {
//            color_ostream_proxy out(Core::getInstance().getConsole());
            if (!embark_assist::finder_ui::state) {
                state = new(states);
                state->finder_list_focus = 0;
                state->finder_list_active = true;
                state->find_callback = find_callback;
                state->max_inorganic = max_inorganic;
            }

            fields i = first_fields;
            ui_lists *element;

            while (true) {
                element = new ui_lists;
                element->current_display_value = 0;
                element->current_index = 0;
                element->focus = 0;

                switch (i) {
                case fields::x_dim:
                    for (int16_t k = 1; k < 16; k++) {
                        element->list.push_back({ std::to_string(k), k });
                    }

                    break;

                case fields::y_dim:
                    for (int16_t k = 1; k < 16; k++) {
                        element->list.push_back({ std::to_string(k), k });
                    }

                    break;

                case fields::savagery_calm:
                case fields::savagery_medium:
                case fields::savagery_savage:
                case fields::good:
                case fields::neutral:
                case fields::evil:
                {
                    embark_assist::defs::evil_savagery_values k = embark_assist::defs::evil_savagery_values::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::evil_savagery_values::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::evil_savagery_values::All:
                            element->list.push_back({ "All", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::evil_savagery_values::Present:
                            element->list.push_back({ "Present", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::evil_savagery_values::Absent:
                            element->list.push_back({ "Absent", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::evil_savagery_values::Absent) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::evil_savagery_values>(static_cast<int8_t>(k) + 1);
                    }
                }

                break;

                case fields::aquifer:
                {
                    embark_assist::defs::aquifer_ranges k = embark_assist::defs::aquifer_ranges::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::aquifer_ranges::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::All:
                            element->list.push_back({ "All", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::Present:
                            element->list.push_back({ "Present", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::Partial:
                            element->list.push_back({ "Partial", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::Not_All:
                            element->list.push_back({ "Not All", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::Absent:
                            element->list.push_back({ "Absent", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::aquifer_ranges::Absent) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::aquifer_ranges>(static_cast<int8_t>(k) + 1);
                    }
                }

                break;

                case fields::min_river:
                case fields::max_river:
                {
                    embark_assist::defs::river_ranges k = embark_assist::defs::river_ranges::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::river_ranges::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::river_ranges::None:
                            element->list.push_back({ "None", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::river_ranges::Brook:
                            element->list.push_back({ "Brook", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::river_ranges::Stream:
                            element->list.push_back({ "Stream", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::river_ranges::Minor:
                            element->list.push_back({ "Minor", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::river_ranges::Medium:
                            element->list.push_back({ "Medium", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::river_ranges::Major:
                            element->list.push_back({ "Major", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::river_ranges::Major) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::river_ranges>(static_cast<int8_t>(k) + 1);
                    }
                }

                break;

                case fields::waterfall:
                case fields::flat:
                case fields::evil_weather:
                case fields::reanimation:
                case fields::thralling:
                {
                    embark_assist::defs::yes_no_ranges k = embark_assist::defs::yes_no_ranges::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::yes_no_ranges::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::yes_no_ranges::Yes:
                            element->list.push_back({ "Yes", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::yes_no_ranges::No:
                            element->list.push_back({ "No", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::yes_no_ranges::No) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::yes_no_ranges>(static_cast<int8_t>(k) + 1);
                    }
                }

                break;

                case fields::soil_min_everywhere:
                {
                    embark_assist::defs::all_present_ranges k = embark_assist::defs::all_present_ranges::All;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::all_present_ranges::All:
                            element->list.push_back({ "All", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::all_present_ranges::Present:
                            element->list.push_back({ "Present", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::all_present_ranges::Present) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::all_present_ranges>(static_cast<int8_t>(k) + 1);
                    }
                }

                break;

                case fields::clay:
                case fields::sand:
                case fields::flux:
                {
                    embark_assist::defs::present_absent_ranges k = embark_assist::defs::present_absent_ranges::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::present_absent_ranges::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::present_absent_ranges::Present:
                            element->list.push_back({ "Present", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::present_absent_ranges::Absent:
                            element->list.push_back({ "Absent", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::present_absent_ranges::Absent) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::present_absent_ranges>(static_cast<int8_t>(k) + 1);
                    }
                }

                break;

                case fields::soil_min:
                case fields::soil_max:
                {
                    embark_assist::defs::soil_ranges k = embark_assist::defs::soil_ranges::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::soil_ranges::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::soil_ranges::None:
                            element->list.push_back({ "None", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::soil_ranges::Very_Shallow:
                            element->list.push_back({ "Very Shallow", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::soil_ranges::Shallow:
                            element->list.push_back({ "Shallow", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::soil_ranges::Deep:
                            element->list.push_back({ "Deep", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::soil_ranges::Very_Deep:
                            element->list.push_back({ "Very Deep", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::soil_ranges::Very_Deep) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::soil_ranges>(static_cast<int8_t>(k) + 1);
                    }
                }

                break;

                case fields::biome_count_min:
                case fields::biome_count_max:
                    for (int16_t k = 0; k < 10; k++) {
                        if (k == 0) {
                            element->list.push_back({ "N/A", -1 });
                        }
                        else {
                            element->list.push_back({ std::to_string(k), k });
                        }
                    }

                    break;

                case fields::region_type_1:
                case fields::region_type_2:
                case fields::region_type_3:
                {
                    std::list<display_map_elements> name;
                    std::list<display_map_elements>::iterator iterator;

                    FOR_ENUM_ITEMS(world_region_type, iter) {
                        name.push_back({ ENUM_KEY_STR(world_region_type, iter),  static_cast<int16_t>(iter) });
                    }
                    name.sort(compare);

                    element->list.push_back({ "N/A", -1 });

                    for (iterator = name.begin(); iterator != name.end(); ++iterator) {
                        element->list.push_back({ iterator->text, iterator->key });
                    }

                    name.clear();
                }
                break;

                case fields::biome_1:
                case fields::biome_2:
                case fields::biome_3:
                {
                    sort_lists name;
                    sort_lists::iterator iterator;

                    FOR_ENUM_ITEMS(biome_type, iter) {
                        std::string s = ENUM_KEY_STR(biome_type, iter);

                        if (s.substr(0, 4) != "POOL" &&
                            s.substr(0, 5) != "RIVER" &&
                            s.substr(0, 3) != "SUB") {
                            name.push_back({ s, static_cast<int16_t>(iter) });
                        }
                    }
                    name.sort(compare);

                    element->list.push_back({ "N/A", -1 });

                    for (iterator = name.begin(); iterator != name.end(); ++iterator) {
                        element->list.push_back({ iterator->text,  iterator->key });
                    }

                    name.clear();
                }
                break;

                case fields::metal_1:
                case fields::metal_2:
                case fields::metal_3:
                {
                    sort_lists name;
                    sort_lists::iterator iterator;

                    for (uint16_t k = 0; k < embark_assist::finder_ui::state->max_inorganic; k++) {
                        for (uint16_t l = 0; l < world->raws.inorganics[k]->metal_ore.mat_index.size(); l++) {
                            append(&name, { world->raws.inorganics[world->raws.inorganics[k]->metal_ore.mat_index[l]]->id,
                                world->raws.inorganics[k]->metal_ore.mat_index[l] });
                        }
                    }

                    name.sort(compare);

                    element->list.push_back({ "N/A", -1 });

                    for (iterator = name.begin(); iterator != name.end(); ++iterator) {
                        element->list.push_back({ iterator->text,  iterator->key });
                    }

                    name.clear();
                }
                break;

                case fields::economic_1:
                case fields::economic_2:
                case fields::economic_3:
                {
                    sort_lists name;
                    sort_lists::iterator iterator;

                    for (int16_t k = 0; k < embark_assist::finder_ui::state->max_inorganic; k++) {
                        if (world->raws.inorganics[k]->economic_uses.size() != 0 &&
                            !world->raws.inorganics[k]->material.flags.is_set(df::material_flags::IS_METAL)) {
                            append(&name, { world->raws.inorganics[k]->id, k });
                        }
                    }

                    name.sort(compare);

                    element->list.push_back({ "N/A", -1 });

                    for (iterator = name.begin(); iterator != name.end(); ++iterator) {
                        element->list.push_back({ iterator->text,  iterator->key });
                    }

                    name.clear();
                }
                break;

                case fields::mineral_1:
                case fields::mineral_2:
                case fields::mineral_3:
                {
                    sort_lists name;
                    sort_lists::iterator iterator;

                    for (int16_t k = 0; k < embark_assist::finder_ui::state->max_inorganic; k++) {
                        if (world->raws.inorganics[k]->environment.location.size() != 0 ||
                            world->raws.inorganics[k]->environment_spec.mat_index.size() != 0 ||
                            world->raws.inorganics[k]->flags.is_set(df::inorganic_flags::SEDIMENTARY) ||
                            world->raws.inorganics[k]->flags.is_set(df::inorganic_flags::IGNEOUS_EXTRUSIVE) ||
                            world->raws.inorganics[k]->flags.is_set(df::inorganic_flags::IGNEOUS_INTRUSIVE) ||
                            world->raws.inorganics[k]->flags.is_set(df::inorganic_flags::METAMORPHIC) ||
                            world->raws.inorganics[k]->flags.is_set(df::inorganic_flags::SOIL)) {
                            append(&name, { world->raws.inorganics[k]->id, k });
                        }
                    }

                    name.sort(compare);

                    element->list.push_back({ "N/A", -1 });

                    for (iterator = name.begin(); iterator != name.end(); ++iterator) {
                        element->list.push_back({ iterator->text,  iterator->key });
                    }

                    name.clear();
                }
                break;
                }

                element->current_value = element->list[0].key;

                state->ui.push_back(element);

                switch (i) {
                case fields::x_dim:
                    state->finder_list.push_back({ "X Dimension", static_cast<int8_t>(i) });
                    break;

                case fields::y_dim:
                    state->finder_list.push_back({ "Y Dimension", static_cast<int8_t>(i) });
                    break;

                case fields::savagery_calm:
                    state->finder_list.push_back({ "Low Savagery", static_cast<int8_t>(i) });
                    break;

                case fields::savagery_medium:
                    state->finder_list.push_back({ "Medium Savagery", static_cast<int8_t>(i) });
                    break;

                case fields::savagery_savage:
                    state->finder_list.push_back({ "High Savagery", static_cast<int8_t>(i) });
                    break;

                case fields::good:
                    state->finder_list.push_back({ "Good", static_cast<int8_t>(i) });
                    break;

                case fields::neutral:
                    state->finder_list.push_back({ "Neutral", static_cast<int8_t>(i) });
                    break;

                case fields::evil:
                    state->finder_list.push_back({ "Evil", static_cast<int8_t>(i) });
                    break;

                case fields::aquifer:
                    state->finder_list.push_back({ "Aquifer", static_cast<int8_t>(i) });
                    break;

                case fields::min_river:
                    state->finder_list.push_back({ "Min River", static_cast<int8_t>(i) });
                    break;

                case fields::max_river:
                    state->finder_list.push_back({ "Max River", static_cast<int8_t>(i) });
                    break;

                case fields::waterfall:
                    state->finder_list.push_back({ "Waterfall", static_cast<int8_t>(i) });
                    break;

                case fields::flat:
                    state->finder_list.push_back({ "Flat", static_cast<int8_t>(i) });
                    break;

                case fields::soil_min_everywhere:
                    state->finder_list.push_back({ "Min Soil Everywhere", static_cast<int8_t>(i) });
                    break;

                case fields::evil_weather:
                    state->finder_list.push_back({ "Evil Weather", static_cast<int8_t>(i) });
                    break;

                case fields::reanimation:
                    state->finder_list.push_back({ "Reanimation", static_cast<int8_t>(i) });
                    break;

                case fields::thralling:
                    state->finder_list.push_back({ "Thralling", static_cast<int8_t>(i) });
                    break;

                case fields::clay:
                    state->finder_list.push_back({ "Clay", static_cast<int8_t>(i) });
                    break;

                case fields::sand:
                    state->finder_list.push_back({ "Sand", static_cast<int8_t>(i) });
                    break;

                case fields::flux:
                    state->finder_list.push_back({ "Flux", static_cast<int8_t>(i) });
                    break;

                case fields::soil_min:
                    state->finder_list.push_back({ "Min Soil", static_cast<int8_t>(i) });
                    break;

                case fields::soil_max:
                    state->finder_list.push_back({ "Max Soil", static_cast<int8_t>(i) });
                    break;

                case fields::biome_count_min:
                    state->finder_list.push_back({ "Min Biome Count", static_cast<int8_t>(i) });
                    break;

                case fields::biome_count_max:
                    state->finder_list.push_back({ "Max Biome Count", static_cast<int8_t>(i) });
                    break;

                case fields::region_type_1:
                    state->finder_list.push_back({ "Region Type 1", static_cast<int8_t>(i) });
                    break;

                case fields::region_type_2:
                    state->finder_list.push_back({ "Region Type 2", static_cast<int8_t>(i) });
                    break;

                case fields::region_type_3:
                    state->finder_list.push_back({ "Region Type 3", static_cast<int8_t>(i) });
                    break;

                case fields::biome_1:
                    state->finder_list.push_back({ "Biome 1", static_cast<int8_t>(i) });
                    break;

                case fields::biome_2:
                    state->finder_list.push_back({ "Biome 2", static_cast<int8_t>(i) });
                    break;

                case fields::biome_3:
                    state->finder_list.push_back({ "Biome 3", static_cast<int8_t>(i) });
                    break;

                case fields::metal_1:
                    state->finder_list.push_back({ "Metal 1", static_cast<int8_t>(i) });
                    break;

                case fields::metal_2:
                    state->finder_list.push_back({ "Metal 2", static_cast<int8_t>(i) });
                    break;

                case fields::metal_3:
                    state->finder_list.push_back({ "Metal 3", static_cast<int8_t>(i) });
                    break;

                case fields::economic_1:
                    state->finder_list.push_back({ "Economic 1", static_cast<int8_t>(i) });
                    break;

                case fields::economic_2:
                    state->finder_list.push_back({ "Economic 2", static_cast<int8_t>(i) });
                    break;

                case fields::economic_3:
                    state->finder_list.push_back({ "Economic 3", static_cast<int8_t>(i) });
                    break;

                case fields::mineral_1:
                    state->finder_list.push_back({ "Mineral 1", static_cast<int8_t>(i) });
                    break;

                case fields::mineral_2:
                    state->finder_list.push_back({ "Mineral 2", static_cast<int8_t>(i) });
                    break;

                case fields::mineral_3:
                    state->finder_list.push_back({ "Mineral 3", static_cast<int8_t>(i) });
                    break;
                }

                if (i == last_fields) {
                    break;  // done
                }

                i = static_cast <fields>(static_cast<int8_t>(i) + 1);
            }

            //  Default embark area size to that of the current selection. The "size" calculation is actually one
            //  off to compensate for the list starting with 1 at index 0.
            //
            auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
            int16_t x = screen->location.region_pos.x;
            int16_t y = screen->location.region_pos.y;
            state->ui[static_cast<int8_t>(fields::x_dim)]->current_display_value =
                Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0)->location.embark_pos_max.x -
                Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0)->location.embark_pos_min.x;
            state->ui[static_cast<int8_t>(fields::x_dim)]->current_index =
                state->ui[static_cast<int8_t>(fields::x_dim)]->current_display_value;
            state->ui[static_cast<int8_t>(fields::x_dim)]->current_value =
                state->ui[static_cast<int8_t>(fields::x_dim)]->current_display_value + 1;

            state->ui[static_cast<int8_t>(fields::y_dim)]->current_display_value =
                Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0)->location.embark_pos_max.y -
                Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0)->location.embark_pos_min.y;
            state->ui[static_cast<int8_t>(fields::y_dim)]->current_index =
                state->ui[static_cast<int8_t>(fields::y_dim)]->current_display_value;
            state->ui[static_cast<int8_t>(fields::y_dim)]->current_value =
                state->ui[static_cast<int8_t>(fields::y_dim)]->current_display_value + 1;
        }


        //==========================================================================================================

        void find() {
//            color_ostream_proxy out(Core::getInstance().getConsole());
            embark_assist::defs::finders finder;
            fields i = first_fields;

            while (true) {
                switch (i) {
                case fields::x_dim:
                    finder.x_dim = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::y_dim:
                    finder.y_dim = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::savagery_calm:
                    finder.savagery[0] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::savagery_medium:
                    finder.savagery[1] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;
                case fields::savagery_savage:
                    finder.savagery[2] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::good:
                    finder.evilness[0] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::neutral:
                    finder.evilness[1] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::evil:
                    finder.evilness[2] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::aquifer:
                    finder.aquifer =
                        static_cast<embark_assist::defs::aquifer_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::min_river:
                    finder.min_river =
                        static_cast<embark_assist::defs::river_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::max_river:
                    finder.max_river =
                        static_cast<embark_assist::defs::river_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::waterfall:
                    finder.waterfall =
                        static_cast<embark_assist::defs::yes_no_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::flat:
                    finder.flat =
                        static_cast<embark_assist::defs::yes_no_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::soil_min_everywhere:
                    finder.soil_min_everywhere =
                        static_cast<embark_assist::defs::all_present_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::evil_weather:
                    finder.evil_weather =
                        static_cast<embark_assist::defs::yes_no_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::reanimation:
                    finder.reanimation =
                        static_cast<embark_assist::defs::yes_no_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::thralling:
                    finder.thralling =
                        static_cast<embark_assist::defs::yes_no_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::clay:
                    finder.clay =
                        static_cast<embark_assist::defs::present_absent_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::sand:
                    finder.sand =
                        static_cast<embark_assist::defs::present_absent_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::flux:
                    finder.flux =
                        static_cast<embark_assist::defs::present_absent_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::soil_min:
                    finder.soil_min =
                        static_cast<embark_assist::defs::soil_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::soil_max:
                    finder.soil_max =
                        static_cast<embark_assist::defs::soil_ranges>(state->ui[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::biome_count_min:
                    finder.biome_count_min = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::biome_count_max:
                    finder.biome_count_max = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::region_type_1:
                    finder.region_type_1 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::region_type_2:
                    finder.region_type_2 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::region_type_3:
                    finder.region_type_3 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::biome_1:
                    finder.biome_1 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::biome_2:
                    finder.biome_2 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::biome_3:
                    finder.biome_3 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::metal_1:
                    finder.metal_1 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::metal_2:
                    finder.metal_2 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::metal_3:
                    finder.metal_3 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::economic_1:
                    finder.economic_1 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::economic_2:
                    finder.economic_2 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::economic_3:
                    finder.economic_3 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::mineral_1:
                    finder.mineral_1 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::mineral_2:
                    finder.mineral_2 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::mineral_3:
                    finder.mineral_3 = state->ui[static_cast<uint8_t>(i)]->current_value;
                    break;
                }

                if (i == last_fields) {
                    break;  // done
                }

                i = static_cast <fields>(static_cast<int8_t>(i) + 1);
            }

            state->find_callback(finder);
        }

        //==========================================================================================================

        class ViewscreenFindUi : public dfhack_viewscreen
        {
        public:
            ViewscreenFindUi();

            void feed(std::set<df::interface_key> *input);

            void render();

            std::string getFocusString() { return "Finder UI"; }

        private:
        };

        //===============================================================================

        void ViewscreenFindUi::feed(std::set<df::interface_key> *input) {
            if (input->count(df::interface_key::LEAVESCREEN))
            {
                input->clear();
                Screen::dismiss(this);
                return;

            } else if (input->count(df::interface_key::CURSOR_LEFT) ||
                input->count(df::interface_key::CURSOR_RIGHT)) {
                    state->finder_list_active = !state->finder_list_active;

            } else if (input->count(df::interface_key::CURSOR_UP)) {
                if (state->finder_list_active) {
                    if (state->finder_list_focus > 0) {
                        state->finder_list_focus--;
                    }
                    else {
                        state->finder_list_focus = static_cast<uint16_t>(last_fields);
                    }
                }
                else {
                    if (state->ui[state->finder_list_focus]->current_index > 0) {
                        state->ui[state->finder_list_focus]->current_index--;
                    } else {
                        state->ui[state->finder_list_focus]->current_index = static_cast<uint16_t>(state->ui[state->finder_list_focus]->list.size()) - 1;
                    }
                }

            } else if (input->count(df::interface_key::CURSOR_DOWN)) {
                if (state->finder_list_active) {
                    if (state->finder_list_focus < static_cast<uint16_t>(last_fields)) {
                        state->finder_list_focus++;
                    } else {
                        state->finder_list_focus = 0;
                    }
                }
                else {
                    if (state->ui[state->finder_list_focus]->current_index < state->ui[state->finder_list_focus]->list.size() - 1) {
                        state->ui[state->finder_list_focus]->current_index++;
                    } else {
                        state->ui[state->finder_list_focus]->current_index = 0;
                    }
                }
            } else if (input->count(df::interface_key::SELECT)) {
                if (!state->finder_list_active) {
                    state->ui[state->finder_list_focus]->current_display_value = state->ui[state->finder_list_focus]->current_index;
                    state->ui[state->finder_list_focus]->current_value = state->ui[state->finder_list_focus]->list[state->ui[state->finder_list_focus]->current_index].key;
                    state->finder_list_active = true;
                }
            } else if (input->count(df::interface_key::CUSTOM_F)) {
                input->clear();
                Screen::dismiss(this);
                find();
                return;
            }
        }

        //===============================================================================

        void ViewscreenFindUi::render() {
//            color_ostream_proxy out(Core::getInstance().getConsole());
            auto screen_size = DFHack::Screen::getWindowSize();
            const int list_column = 53;
            uint16_t offset = 0;

            Screen::clear();
            Screen::drawBorder("Embark Assistant Site Finder");

            embark_assist::screen::paintString(lr_pen, 1, 1, "4/6");
            embark_assist::screen::paintString(white_pen, 4, 1, ":Shift list");
            embark_assist::screen::paintString(lr_pen, 16, 1, "8/2");
            embark_assist::screen::paintString(white_pen, 19, 1, ":Up/down");
            embark_assist::screen::paintString(lr_pen, 28, 1, "ENTER");
            embark_assist::screen::paintString(white_pen, 33, 1, ":Select item");
            embark_assist::screen::paintString(lr_pen, 46, 1, "f");
            embark_assist::screen::paintString(white_pen, 47, 1, ":Find");
            embark_assist::screen::paintString(lr_pen, 53, 1, "ESC");
            embark_assist::screen::paintString(white_pen, 56, 1, ":Abort");

            for (uint16_t i = 0; i < state->finder_list.size(); i++) {
                if (i == state->finder_list_focus) {
                    if (state->finder_list_active) {
                        embark_assist::screen::paintString(active_pen, 1, 2 + i, state->finder_list[i].text);
                    }
                    else {
                        embark_assist::screen::paintString(passive_pen, 1, 2 + i, state->finder_list[i].text);
                    }

                    embark_assist::screen::paintString(active_pen,
                        21,
                        2 + i,
                        state->ui[i]->list[state->ui[i]->current_display_value].text);
                }
                else {
                    embark_assist::screen::paintString(normal_pen, 1, 2 + i, state->finder_list[i].text);

                    embark_assist::screen::paintString(white_pen,
                        21,
                        2 + i,
                        state->ui[i]->list[state->ui[i]->current_display_value].text);
                }

            }

            //  Implement scrolling lists if they don't fit on the screen.
            if (state->ui[state->finder_list_focus]->list.size() > screen_size.y - 3) {
                offset = (screen_size.y - 3) / 2;
                if (state->ui[state->finder_list_focus]->current_index < offset) {
                    offset = 0;
                }
                else {
                    offset = state->ui[state->finder_list_focus]->current_index - offset;
                }

                if (state->ui[state->finder_list_focus]->list.size() - offset < screen_size.y - 3) {
                    offset = static_cast<uint16_t>(state->ui[state->finder_list_focus]->list.size()) - (screen_size.y - 3);
                }
            }

            for (uint16_t i = 0; i < state->ui[state->finder_list_focus]->list.size(); i++) {
                if (i == state->ui[state->finder_list_focus]->current_index) {
                    if (!state->finder_list_active) {  // Negated expression to get the display lines in the same order as above.
                        embark_assist::screen::paintString(active_pen, list_column, 2 + i - offset, state->ui[state->finder_list_focus]->list[i].text);
                    }
                    else {
                        embark_assist::screen::paintString(passive_pen, list_column, 2 + i - offset, state->ui[state->finder_list_focus]->list[i].text);
                    }
                }
                else {
                    embark_assist::screen::paintString(normal_pen, list_column, 2 + i - offset, state->ui[state->finder_list_focus]->list[i].text);
                }
            }

            dfhack_viewscreen::render();
        }

        //===============================================================================

        ViewscreenFindUi::ViewscreenFindUi() {
        }
    }
}

//===============================================================================
//  Exported operations
//===============================================================================

void embark_assist::finder_ui::init(DFHack::Plugin *plugin_self, embark_assist::defs::find_callbacks find_callback, uint16_t max_inorganic) {
    if (!embark_assist::finder_ui::state) {  //  First call. Have to do the setup
        embark_assist::finder_ui::ui_setup(find_callback, max_inorganic);
    }
    Screen::show(new ViewscreenFindUi(), plugin_self);
}

//===============================================================================

void embark_assist::finder_ui::activate() {
}

//===============================================================================

void embark_assist::finder_ui::shutdown() {
    if (embark_assist::finder_ui::state) {
        for (uint16_t i = 0; i < embark_assist::finder_ui::state->ui.size(); i++) {
            delete embark_assist::finder_ui::state->ui[i];
        }

        delete embark_assist::finder_ui::state;
        embark_assist::finder_ui::state = nullptr;
    }
}
