#include <stdio.h>
#include "Core.h"
#include <Console.h>

#include <modules/Gui.h>

#include "Types.h"

#include "MemAccess.h"
#include "df/biome_type.h"
#include "df/entity_raw.h"
#include "df/entity_raw_flags.h"
#include "df/inorganic_raw.h"
#include "df/material_flags.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_raws.h"
#include "df/world_region_type.h"
#include "df/world_raws.h"

#include "embark-assistant.h"
#include "finder_ui.h"
#include "screen.h"

using df::global::world;

#define profile_file_name "./data/init/embark_assistant_profile.txt"

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
            min_waterfall,
            flat,
            clay,
            sand,
            flux,
            coal,
            soil_min,
            soil_min_everywhere,
            soil_max,
            freezing,
            blood_rain,
            syndrome_rain,
            reanimation,
            spire_count_min,
            spire_count_max,
            magma_min,
            magma_max,
            biome_count_min,
            biome_count_max,
            region_type_1,
            region_type_2,
            region_type_3,
            biome_1,
            biome_2,
            biome_3,
            min_trees,
            max_trees,
            metal_1,
            metal_2,
            metal_3,
            economic_1,
            economic_2,
            economic_3,
            mineral_1,
            mineral_2,
            mineral_3,
            min_necro_neighbors,
            max_necro_neighbors,
            min_civ_neighbors,
            max_civ_neighbors,
            neighbors
        };
        fields first_fields = fields::x_dim;
        fields last_fields = fields::neighbors;

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

        struct civ_entities {
            int16_t id;
            std::string description;
        };

        //==========================================================================================================

        struct states {
            embark_assist::defs::find_callbacks find_callback;
            uis plotinfo;
            display_maps finder_list;  //  Don't need the element key, but it's easier to use the same type.
            uint16_t finder_list_focus;
            bool finder_list_active;
            uint16_t max_inorganic;
            std::vector<civ_entities> civs;
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

        void save_profile() {
            color_ostream_proxy out(Core::getInstance().getConsole());

            FILE* outfile = fopen(profile_file_name, "w");
            fields i = first_fields;
            size_t civ = 0;

            while (true) {
                for (size_t k = 0; k < state->plotinfo[static_cast<int8_t>(i)]->list.size(); k++) {
                    if (state->plotinfo[static_cast<int8_t>(i) + civ]->current_value == state->plotinfo[static_cast<int8_t>(i) + civ]->list[k].key) {
                        fprintf(outfile, "[%s:%s]\n", state->finder_list[static_cast<int8_t>(i) + civ].text.c_str(), state->plotinfo[static_cast<int8_t>(i) + civ]->list[k].text.c_str());
                        break;
                    }
                }
//                fprintf(outfile, "[%s:%i]\n", state->finder_list[static_cast<int8_t>(i)].text.c_str(), state->plotinfo[static_cast<int8_t>(i)]->current_value);
                if (i == last_fields) {
                    civ++;

                    if (civ == state->civs.size()) {
                        break;  // done
                    }
                }
                else {
                    i = static_cast <fields>(static_cast<int8_t>(i) + 1);
                }
            }

            fclose(outfile);
        }

        //==========================================================================================================

        void load_profile() {
            color_ostream_proxy out(Core::getInstance().getConsole());
            FILE* infile = fopen(profile_file_name, "r");
            size_t civ = 0;

            if (!infile) {
                out.printerr("No profile file found at %s\n", profile_file_name);
                return;
            }

            fields i = first_fields;
            char line[80];
            int count = 80;
            bool found;

            while (true) {
                if (!fgets(line, count, infile) || line[0] != '[') {
                    out.printerr("Failed to find token start '[' at line %i\n", static_cast<int8_t>(i));
                    fclose(infile);
                    return;
                }

                for (int k = 1; k < count; k++) {
                    if (line[k] == ':') {
                        for (int l = 1; l < k; l++) {
                            if (state->finder_list[static_cast<int8_t>(i) + civ].text.c_str()[l - 1] != line[l]) {
                                out.printerr("Token mismatch of %s vs %s\n", line, state->finder_list[static_cast<int8_t>(i) + civ].text.c_str());
                                fclose(infile);
                                return;
                            }
                        }

                        found = false;

                        for (size_t l = 0; l < state->plotinfo[static_cast<int8_t>(i) + civ]->list.size(); l++) {
                            for (int m = k + 1; m < count; m++) {
                                if (state->plotinfo[static_cast<int8_t>(i) + civ]->list[l].text.c_str()[m - (k + 1)] != line[m]) {
                                    if (state->plotinfo[static_cast<int8_t>(i) + civ]->list[l].text.c_str()[m - (k + 1)] == '\0' &&
                                        line[m] == ']') {
                                        found = true;
                                    }
                                    break;
                                }
                            }
                            if (found) {
                                break;
                            }
                        }

                        if (!found) {
                            out.printerr("Value extraction failure from %s\n", line);
                            fclose(infile);
                            return;
                        }

                        break;
                    }
                }

                if (!found) {
                    out.printerr("Value delimiter not found in %s\n", line);
                    fclose(infile);
                    return;
                }

                if (i == last_fields) {
                    civ++;

                    if (civ == state->civs.size()) {
                        break;  // done
                    }
                }
                else {
                    i = static_cast <fields>(static_cast<int8_t>(i) + 1);
                }
            }

            fclose(infile);

            //  Checking done. Now do the work.

            infile = fopen(profile_file_name, "r");
            i = first_fields;
            civ = 0;

            while (true) {
                if (!fgets(line, count, infile))
                {
                    break;
                }

                for (int k = 1; k < count; k++) {
                    if (line[k] == ':') {

                        found = false;

                        for (size_t l = 0; l < state->plotinfo[static_cast<int8_t>(i) + civ]->list.size(); l++) {
                            for (int m = k + 1; m < count; m++) {
                                if (state->plotinfo[static_cast<int8_t>(i) + civ]->list[l].text.c_str()[m - (k + 1)] != line[m]) {
                                    if (state->plotinfo[static_cast<int8_t>(i) + civ]->list[l].text.c_str()[m - (k + 1)] == '\0' &&
                                        line[m] == ']') {
                                        state->plotinfo[static_cast<int8_t>(i) + civ]->current_value = state->plotinfo[static_cast<int8_t>(i) + civ]->list[l].key;
                                        state->plotinfo[static_cast<int8_t>(i) + civ]->current_display_value = l;
                                        found = true;
                                    }

                                    break;
                                }
                            }
                            if (found) {
                                break;
                            }
                        }

                        break;
                    }
                }

                if (i == last_fields) {
                    civ++;

                    if (civ == state->civs.size()) {
                        break;  // done
                    }
                }
                else {
                    i = static_cast <fields>(static_cast<int8_t>(i) + 1);
                }
            }

            fclose(infile);
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
            int16_t controllable_civs = 0;
            int16_t max_civs;

            for (int16_t i = 0; i < (int16_t)world->raws.entities.size(); i++) {
                if (world->raws.entities[i]->flags.is_set(df::entity_raw_flags::CIV_CONTROLLABLE)) controllable_civs++;
            }

            for (int16_t i = 0; i < (int16_t)world->raws.entities.size(); i++) {
                if (!world->raws.entities[i]->flags.is_set(df::entity_raw_flags::LAYER_LINKED) &&  // Animal people
                    !world->raws.entities[i]->flags.is_set(df::entity_raw_flags::GENERATED) &&     // Vault guardians
                    (controllable_civs > 1 ||                                                              //  Suppress the playable civ when only 1
                        !world->raws.entities[i]->flags.is_set(df::entity_raw_flags::CIV_CONTROLLABLE))) { //  Too much work to change dynamically for modded worlds.
                    if (world->raws.entities[i]->translation == "") {
                        state->civs.push_back({ i, world->raws.entities[i]->code });                       //  Kobolds don't have a translation...
                    }
                    else {
                        state->civs.push_back({ i, world->raws.entities[i]->translation });
                    }
                }
            }

            max_civs = state->civs.size();

            if (controllable_civs > 1) max_civs = max_civs - 1;

            while (true) {
                element = new ui_lists;
                element->current_display_value = 0;
                element->current_index = 0;
                element->focus = 0;

                switch (i) {
                case fields::x_dim:
                    for (int16_t k = 1; k <= 16; k++) {
                        element->list.push_back({ std::to_string(k), k });
                    }

                    break;

                case fields::y_dim:
                    for (int16_t k = 1; k <= 16; k++) {
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

                        case embark_assist::defs::aquifer_ranges::None:
                            element->list.push_back({ "None", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::At_Most_Light:
                            element->list.push_back({ "<= Light", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::None_Plus_Light:
                            element->list.push_back({ "None + Light", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::None_Plus_At_Least_Light:
                            element->list.push_back({ "None + >= Light", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::Light:
                            element->list.push_back({ "Light", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::At_Least_Light:
                            element->list.push_back({ ">= Light", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::None_Plus_Heavy:
                            element->list.push_back({ "None + Heavy", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::At_Most_Light_Plus_Heavy:
                            element->list.push_back({ "<= Light + Heavy", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::Light_Plus_Heavy:
                            element->list.push_back({ "Light + Heavy", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::None_Light_Heavy:
                            element->list.push_back({ "None + Light + Heavy", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::aquifer_ranges::Heavy:
                            element->list.push_back({ "Heavy", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::aquifer_ranges::Heavy) {
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

                case fields::min_waterfall:

                    element->list.push_back({ "N/A", -1 });
                    element->list.push_back({ "Absent", 0 });

                    for (int16_t k = 1; k <= 9; k++) {
                        element->list.push_back({ std::to_string(k), k });
                    }

                    for (int16_t k = 10; k <= 50; k+=5) {
                        element->list.push_back({ std::to_string(k), k });
                    }

                break;

                case fields::blood_rain:
                case fields::flat:
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
                case fields::coal:
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

                case fields::freezing:
                {
                    embark_assist::defs::freezing_ranges k = embark_assist::defs::freezing_ranges::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::freezing_ranges::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::freezing_ranges::Permanent:
                            element->list.push_back({ "Permanent", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::freezing_ranges::At_Least_Partial:
                            element->list.push_back({ "At Least Partially Frozen", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::freezing_ranges::Partial:
                            element->list.push_back({ "Partially Frozen", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::freezing_ranges::At_Most_Partial:
                            element->list.push_back({ "At Most Partially Frozen", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::freezing_ranges::Never:
                            element->list.push_back({ "Never Frozen", static_cast<int8_t>(k) });
                            break;

                        }

                        if (k == embark_assist::defs::freezing_ranges::Never ||
                               (world->world_data->world_height != 17 &&  //  Can't handle temperature in non standard height worlds.
                                world->world_data->world_height != 33 &&
                                world->world_data->world_height != 65 &&
                                world->world_data->world_height != 129 &&
                                world->world_data->world_height != 257)) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::freezing_ranges>(static_cast<int8_t>(k) + 1);
                    }
                }

                break;

                case fields::syndrome_rain:
                {
                    embark_assist::defs::syndrome_rain_ranges k = embark_assist::defs::syndrome_rain_ranges::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::syndrome_rain_ranges::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::syndrome_rain_ranges::Any:
                            element->list.push_back({ "Any Syndrome", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::syndrome_rain_ranges::Permanent:
                            element->list.push_back({ "Permanent Syndrome", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::syndrome_rain_ranges::Temporary:
                            element->list.push_back({ "Temporary Syndrome", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::syndrome_rain_ranges::Not_Permanent:
                            element->list.push_back({ "Not Permanent Syndrome", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::syndrome_rain_ranges::None:
                            element->list.push_back({ "No Syndrome", static_cast<int8_t>(k) });
                            break;

                        }

                        if (k == embark_assist::defs::syndrome_rain_ranges::None) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::syndrome_rain_ranges>(static_cast<int8_t>(k) + 1);
                    }
                }

                break;

                case fields::reanimation:
                {
                    embark_assist::defs::reanimation_ranges k = embark_assist::defs::reanimation_ranges::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::reanimation_ranges::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::reanimation_ranges::Both:
                            element->list.push_back({ "Reanimation & Thralling", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::reanimation_ranges::Any:
                            element->list.push_back({ "Reanimation or Thralling", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::reanimation_ranges::Thralling:
                            element->list.push_back({ "Thralling", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::reanimation_ranges::Reanimation:
                            element->list.push_back({ "Reanimation", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::reanimation_ranges::Not_Thralling:
                            element->list.push_back({ "Not Thralling", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::reanimation_ranges::None:
                            element->list.push_back({ "None", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::reanimation_ranges::None) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::reanimation_ranges>(static_cast<int8_t>(k) + 1);
                    }
                }

                break;

                case fields::spire_count_min:
                case fields::spire_count_max:
                    for (int16_t k = -1; k <= 9; k++) {
                        if (k == -1) {
                            element->list.push_back({ "N/A", k });
                        }
                        else {
                            element->list.push_back({ std::to_string(k), k });
                        }
                    }

                    break;

                case fields::magma_min:
                case fields::magma_max:
                {
                    embark_assist::defs::magma_ranges k = embark_assist::defs::magma_ranges::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::magma_ranges::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::magma_ranges::Cavern_3:
                            element->list.push_back({ "Third Cavern", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::magma_ranges::Cavern_2:
                            element->list.push_back({ "Second Cavern", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::magma_ranges::Cavern_1:
                            element->list.push_back({ "First Cavern", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::magma_ranges::Volcano:
                            element->list.push_back({ "Volcano", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::magma_ranges::Volcano) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::magma_ranges>(static_cast<int8_t>(k) + 1);
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

                case fields::min_trees:
                case fields::max_trees:
                {
                    embark_assist::defs::tree_ranges k = embark_assist::defs::tree_ranges::NA;
                    while (true) {
                        switch (k) {
                        case embark_assist::defs::tree_ranges::NA:
                            element->list.push_back({ "N/A", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::tree_ranges::None:
                            element->list.push_back({ "None", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::tree_ranges::Very_Scarce:
                            element->list.push_back({ "Very Scarce", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::tree_ranges::Scarce:
                            element->list.push_back({ "Scarce", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::tree_ranges::Woodland:
                            element->list.push_back({ "Woodland", static_cast<int8_t>(k) });
                            break;

                        case embark_assist::defs::tree_ranges::Heavily_Forested:
                            element->list.push_back({ "Heavily Forested", static_cast<int8_t>(k) });
                            break;
                        }

                        if (k == embark_assist::defs::tree_ranges::Heavily_Forested) {
                            break;
                        }

                        k = static_cast <embark_assist::defs::tree_ranges>(static_cast<int8_t>(k) + 1);
                    }
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

                case fields::min_necro_neighbors:
                case fields::max_necro_neighbors:
                    for (int16_t k = -1; k <= 16; k++) {
                        if (k == -1) {
                            element->list.push_back({ "N/A", k });
                        }
                        else {
                            element->list.push_back({ std::to_string(k), k });
                        }
                    }

                    break;

                case fields::min_civ_neighbors:
                case fields::max_civ_neighbors:
                    for (int16_t k = -1; k <= max_civs; k++) {
                        if (k == -1) {
                            element->list.push_back({ "N/A", k });
                        }
                        else {
                            element->list.push_back({ std::to_string(k), k });
                        }
                    }

                    break;

                case fields::neighbors:
                    for (size_t l = 0; l < state->civs.size(); l++) {
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

                        if (l < state->civs.size() - 1) {
                            element->current_value = element->list[0].key;
                            state->plotinfo.push_back(element);
                            element = new ui_lists;
                            element->current_display_value = 0;
                            element->current_index = 0;
                            element->focus = 0;
                        }
                    }

                    break;

                }

                element->current_value = element->list[0].key;
                state->plotinfo.push_back(element);

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

                case fields::min_waterfall:
                    state->finder_list.push_back({ "Min Waterfall Drop", static_cast<int8_t>(i) });
                    break;

                case fields::flat:
                    state->finder_list.push_back({ "Flat", static_cast<int8_t>(i) });
                    break;

                case fields::soil_min_everywhere:
                    state->finder_list.push_back({ "Min Soil Everywhere", static_cast<int8_t>(i) });
                    break;

                case fields::freezing:
                    state->finder_list.push_back({ "Freezing", static_cast<int8_t>(i) });
                    break;

                case fields::blood_rain:
                    state->finder_list.push_back({ "Blood Rain", static_cast<int8_t>(i) });
                    break;

                case fields::syndrome_rain:
                    state->finder_list.push_back({ "Syndrome Rain", static_cast<int8_t>(i) });
                    break;

                case fields::reanimation:
                    state->finder_list.push_back({ "Reanimation", static_cast<int8_t>(i) });
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

                case fields::coal:
                    state->finder_list.push_back({ "Coal", static_cast<int8_t>(i) });
                    break;

                case fields::soil_min:
                    state->finder_list.push_back({ "Min Soil", static_cast<int8_t>(i) });
                    break;

                case fields::soil_max:
                    state->finder_list.push_back({ "Max Soil", static_cast<int8_t>(i) });
                    break;

                case fields::spire_count_min:
                    state->finder_list.push_back({ "Min Adamantine", static_cast<int8_t>(i) });
                    break;

                case fields::spire_count_max:
                    state->finder_list.push_back({ "Max Adamantine", static_cast<int8_t>(i) });
                    break;

                case fields::magma_min:
                    state->finder_list.push_back({ "Min Magma", static_cast<int8_t>(i) });
                    break;

                case fields::magma_max:
                    state->finder_list.push_back({ "Max Magma", static_cast<int8_t>(i) });
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

                case fields::min_trees:
                    state->finder_list.push_back({ "Min Trees", static_cast<int8_t>(i) });
                    break;

                case fields::max_trees:
                    state->finder_list.push_back({ "Max Trees", static_cast<int8_t>(i) });
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

                case fields::min_necro_neighbors:
                    state->finder_list.push_back({ "Min Necro Tower", static_cast<int8_t>(i) });
                    break;

                case fields::max_necro_neighbors:
                    state->finder_list.push_back({ "Max Necro Tower", static_cast<int8_t>(i) });
                    break;

                case fields::min_civ_neighbors:
                    state->finder_list.push_back({ "Min Near Civs", static_cast<int8_t>(i) });
                    break;

                case fields::max_civ_neighbors:
                    state->finder_list.push_back({ "Max Near Civs", static_cast<int8_t>(i) });
                    break;

                case fields::neighbors:
                    for (uint8_t k = 0; k < state->civs.size(); k++) {
                        state->finder_list.push_back({ state->civs[k].description, (int16_t)(static_cast<int8_t>(i) + k) });
                    }
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
            state->plotinfo[static_cast<int8_t>(fields::x_dim)]->current_display_value =
                screen->location.embark_pos_max.x -
                screen->location.embark_pos_min.x;
            state->plotinfo[static_cast<int8_t>(fields::x_dim)]->current_index =
                state->plotinfo[static_cast<int8_t>(fields::x_dim)]->current_display_value;
            state->plotinfo[static_cast<int8_t>(fields::x_dim)]->current_value =
                state->plotinfo[static_cast<int8_t>(fields::x_dim)]->current_display_value + 1;

            state->plotinfo[static_cast<int8_t>(fields::y_dim)]->current_display_value =
                screen->location.embark_pos_max.y -
                screen->location.embark_pos_min.y;
            state->plotinfo[static_cast<int8_t>(fields::y_dim)]->current_index =
                state->plotinfo[static_cast<int8_t>(fields::y_dim)]->current_display_value;
            state->plotinfo[static_cast<int8_t>(fields::y_dim)]->current_value =
                state->plotinfo[static_cast<int8_t>(fields::y_dim)]->current_display_value + 1;
        }

        //==========================================================================================================

        void find() {
//            color_ostream_proxy out(Core::getInstance().getConsole());
            embark_assist::defs::finders finder = {};
            fields i = first_fields;

            while (true) {
                switch (i) {
                case fields::x_dim:
                    finder.x_dim = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::y_dim:
                    finder.y_dim = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::savagery_calm:
                    finder.savagery[0] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::savagery_medium:
                    finder.savagery[1] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;
                case fields::savagery_savage:
                    finder.savagery[2] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::good:
                    finder.evilness[0] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::neutral:
                    finder.evilness[1] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::evil:
                    finder.evilness[2] =
                        static_cast<embark_assist::defs::evil_savagery_values>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::aquifer:
                    finder.aquifer =
                        static_cast<embark_assist::defs::aquifer_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::min_river:
                    finder.min_river =
                        static_cast<embark_assist::defs::river_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::max_river:
                    finder.max_river =
                        static_cast<embark_assist::defs::river_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::min_waterfall:
                    finder.min_waterfall = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::flat:
                    finder.flat =
                        static_cast<embark_assist::defs::yes_no_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::soil_min_everywhere:
                    finder.soil_min_everywhere =
                        static_cast<embark_assist::defs::all_present_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::freezing:
                    finder.freezing =
                        static_cast<embark_assist::defs::freezing_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::blood_rain:
                    finder.blood_rain =
                        static_cast<embark_assist::defs::yes_no_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::syndrome_rain:
                    finder.syndrome_rain =
                        static_cast<embark_assist::defs::syndrome_rain_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::reanimation:
                    finder.reanimation =
                        static_cast<embark_assist::defs::reanimation_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::clay:
                    finder.clay =
                        static_cast<embark_assist::defs::present_absent_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::sand:
                    finder.sand =
                        static_cast<embark_assist::defs::present_absent_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::flux:
                    finder.flux =
                        static_cast<embark_assist::defs::present_absent_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::coal:
                    finder.coal =
                        static_cast<embark_assist::defs::present_absent_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::soil_min:
                    finder.soil_min =
                        static_cast<embark_assist::defs::soil_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::soil_max:
                    finder.soil_max =
                        static_cast<embark_assist::defs::soil_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::spire_count_min:
                    finder.spire_count_min = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::spire_count_max:
                    finder.spire_count_max = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::magma_min:
                    finder.magma_min =
                        static_cast<embark_assist::defs::magma_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::magma_max:
                    finder.magma_max =
                        static_cast<embark_assist::defs::magma_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::biome_count_min:
                    finder.biome_count_min = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::biome_count_max:
                    finder.biome_count_max = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::region_type_1:
                    finder.region_type_1 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::region_type_2:
                    finder.region_type_2 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::region_type_3:
                    finder.region_type_3 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::biome_1:
                    finder.biome_1 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::biome_2:
                    finder.biome_2 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::biome_3:
                    finder.biome_3 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::min_trees:
                    finder.min_trees = static_cast<embark_assist::defs::tree_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::max_trees:
                    finder.max_trees = static_cast<embark_assist::defs::tree_ranges>(state->plotinfo[static_cast<uint8_t>(i)]->current_value);
                    break;

                case fields::metal_1:
                    finder.metal_1 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::metal_2:
                    finder.metal_2 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::metal_3:
                    finder.metal_3 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::economic_1:
                    finder.economic_1 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::economic_2:
                    finder.economic_2 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::economic_3:
                    finder.economic_3 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::mineral_1:
                    finder.mineral_1 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::mineral_2:
                    finder.mineral_2 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::mineral_3:
                    finder.mineral_3 = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::min_necro_neighbors:
                    finder.min_necro_neighbors = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::max_necro_neighbors:
                    finder.max_necro_neighbors = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::min_civ_neighbors:
                    finder.min_civ_neighbors = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::max_civ_neighbors:
                    finder.max_civ_neighbors = state->plotinfo[static_cast<uint8_t>(i)]->current_value;
                    break;

                case fields::neighbors:
                    for (size_t k = 0; k < state->civs.size(); k++) {
                        finder.neighbors.push_back({ state->civs[k].id, static_cast<embark_assist::defs::present_absent_ranges>(state->plotinfo[static_cast<uint8_t>(i) + k]->current_value) });
                    }
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

            } else if (input->count(df::interface_key::STANDARDSCROLL_LEFT) ||
                input->count(df::interface_key::STANDARDSCROLL_RIGHT)) {
                    state->finder_list_active = !state->finder_list_active;

            } else if (input->count(df::interface_key::STANDARDSCROLL_UP)) {
                if (state->finder_list_active) {
                    if (state->finder_list_focus > 0) {
                        state->finder_list_focus--;
                    }
                    else {
                        state->finder_list_focus = static_cast<uint16_t>(last_fields) + state->civs.size() - 1;
                    }
                }
                else {
                    if (state->plotinfo[state->finder_list_focus]->current_index > 0) {
                        state->plotinfo[state->finder_list_focus]->current_index--;
                    } else {
                        state->plotinfo[state->finder_list_focus]->current_index = static_cast<uint16_t>(state->plotinfo[state->finder_list_focus]->list.size()) - 1;
                    }
                }

            } else if (input->count(df::interface_key::STANDARDSCROLL_DOWN)) {
                if (state->finder_list_active) {
                    if (state->finder_list_focus < static_cast<uint16_t>(last_fields) + state->civs.size() - 1) {
                        state->finder_list_focus++;
                    } else {
                        state->finder_list_focus = 0;
                    }
                }
                else {
                    if (state->plotinfo[state->finder_list_focus]->current_index < state->plotinfo[state->finder_list_focus]->list.size() - 1) {
                        state->plotinfo[state->finder_list_focus]->current_index++;
                    } else {
                        state->plotinfo[state->finder_list_focus]->current_index = 0;
                    }
                }

            } else if (input->count(df::interface_key::SELECT)) {
                if (!state->finder_list_active) {
                    state->plotinfo[state->finder_list_focus]->current_display_value = state->plotinfo[state->finder_list_focus]->current_index;
                    state->plotinfo[state->finder_list_focus]->current_value = state->plotinfo[state->finder_list_focus]->list[state->plotinfo[state->finder_list_focus]->current_index].key;
                    state->finder_list_active = true;
                }

            } else if (input->count(df::interface_key::CUSTOM_F)) {
                input->clear();
                Screen::dismiss(this);
                find();
                return;

            } else if (input->count(df::interface_key::CUSTOM_S)) {  //  Save
                save_profile();

            } else if (input->count(df::interface_key::CUSTOM_L)) {  //  Load
                load_profile();
            }
        }

        //===============================================================================

        void ViewscreenFindUi::render() {
//            color_ostream_proxy out(Core::getInstance().getConsole());
            auto screen_size = DFHack::Screen::getWindowSize();
            uint16_t top_row = 2;
            uint16_t list_column = 53;
            uint16_t offset = 0;

            Screen::clear();
            Screen::drawBorder("  Embark Assistant Site Finder  ");

            embark_assist::screen::paintString(lr_pen, 1, 1, DFHack::Screen::getKeyDisplay(df::interface_key::STANDARDSCROLL_LEFT).c_str());
            embark_assist::screen::paintString(white_pen, 2, 1, "/");
            embark_assist::screen::paintString(lr_pen, 3, 1, DFHack::Screen::getKeyDisplay(df::interface_key::STANDARDSCROLL_RIGHT).c_str());
            embark_assist::screen::paintString(white_pen, 4, 1, ": \x1b/\x1a");
            embark_assist::screen::paintString(lr_pen, 10, 1, DFHack::Screen::getKeyDisplay(df::interface_key::STANDARDSCROLL_UP).c_str());
            embark_assist::screen::paintString(white_pen, 11, 1, "/");
            embark_assist::screen::paintString(lr_pen, 12, 1, DFHack::Screen::getKeyDisplay(df::interface_key::STANDARDSCROLL_DOWN).c_str());
            embark_assist::screen::paintString(white_pen, 13, 1, ": \x18/\x19");
            embark_assist::screen::paintString(lr_pen, 19, 1, DFHack::Screen::getKeyDisplay(df::interface_key::SELECT).c_str());
            embark_assist::screen::paintString(white_pen, 24, 1, ": Select");
            embark_assist::screen::paintString(lr_pen, 33, 1, DFHack::Screen::getKeyDisplay(df::interface_key::CUSTOM_F).c_str());
            embark_assist::screen::paintString(white_pen, 34, 1, ": Find");
            embark_assist::screen::paintString(lr_pen, 41, 1, DFHack::Screen::getKeyDisplay(df::interface_key::LEAVESCREEN).c_str());
            embark_assist::screen::paintString(white_pen, 44, 1, ": Abort");
            embark_assist::screen::paintString(lr_pen, 52, 1, DFHack::Screen::getKeyDisplay(df::interface_key::CUSTOM_S).c_str());
            embark_assist::screen::paintString(white_pen, 53, 1, ": Save");
            embark_assist::screen::paintString(lr_pen, 60, 1, DFHack::Screen::getKeyDisplay(df::interface_key::CUSTOM_L).c_str());
            embark_assist::screen::paintString(white_pen, 61, 1, ": Load");

            //  Implement scrolling lists if they don't fit on the screen.
            if (int32_t(state->finder_list.size()) > screen_size.y - top_row - 1) {
                offset = (screen_size.y - top_row - 1) / 2;
                if (state->finder_list_focus < offset) {
                    offset = 0;
                }
                else {
                    offset = state->finder_list_focus - offset;
                }

                if (int32_t(state->finder_list.size() - offset) < screen_size.y - top_row - 1) {
                    offset = static_cast<uint16_t>(state->finder_list.size()) - (screen_size.y - top_row - 1);
                }
            }

            for (uint16_t i = offset; i < state->finder_list.size(); i++) {
                if (i == state->finder_list_focus) {
                    if (state->finder_list_active) {
                        embark_assist::screen::paintString(active_pen, 1, top_row + i - offset, state->finder_list[i].text);
                    }
                    else {
                        embark_assist::screen::paintString(passive_pen, 1, top_row + i - offset, state->finder_list[i].text);
                    }

                    embark_assist::screen::paintString(active_pen,
                        21,
                        top_row + i - offset,
                        state->plotinfo[i]->list[state->plotinfo[i]->current_display_value].text);
                }
                else {
                    embark_assist::screen::paintString(normal_pen, 1, top_row + i - offset, state->finder_list[i].text);

                    embark_assist::screen::paintString(white_pen,
                        21,
                        top_row + i - offset,
                        state->plotinfo[i]->list[state->plotinfo[i]->current_display_value].text);
                }
            }

            //  Implement scrolling lists if they don't fit on the screen.
            offset = 0;

            if (int32_t(state->plotinfo[state->finder_list_focus]->list.size()) > screen_size.y - top_row - 1) {
                offset = (screen_size.y - top_row - 1) / 2;
                if (state->plotinfo[state->finder_list_focus]->current_index < offset) {
                    offset = 0;
                }
                else {
                    offset = state->plotinfo[state->finder_list_focus]->current_index - offset;
                }

                if (int32_t(state->plotinfo[state->finder_list_focus]->list.size() - offset) < screen_size.y - top_row - 1) {
                    offset = static_cast<uint16_t>(state->plotinfo[state->finder_list_focus]->list.size()) - (screen_size.y - top_row - 1);
                }
            }

            for (uint16_t i = offset; i < state->plotinfo[state->finder_list_focus]->list.size(); i++) {
                if (i == state->plotinfo[state->finder_list_focus]->current_index) {
                    if (!state->finder_list_active) {  // Negated expression to get the display lines in the same order as above.
                        embark_assist::screen::paintString(active_pen, list_column, top_row + i - offset, state->plotinfo[state->finder_list_focus]->list[i].text);
                    }
                    else {
                        embark_assist::screen::paintString(passive_pen, list_column, top_row + i - offset, state->plotinfo[state->finder_list_focus]->list[i].text);
                    }
                }
                else {
                    embark_assist::screen::paintString(normal_pen, list_column, top_row + i - offset, state->plotinfo[state->finder_list_focus]->list[i].text);
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

void embark_assist::finder_ui::init(DFHack::Plugin *plugin_self, embark_assist::defs::find_callbacks find_callback, uint16_t max_inorganic, bool fileresult) {
    if (!embark_assist::finder_ui::state) {  //  First call. Have to do the setup
        embark_assist::finder_ui::ui_setup(find_callback, max_inorganic);
    }
    if (!fileresult) {
        Screen::show(std::make_unique<ViewscreenFindUi>(), plugin_self);
    }
    else
    {
        load_profile();
        find();
    }
}

//===============================================================================

void embark_assist::finder_ui::activate() {
}

//===============================================================================

void embark_assist::finder_ui::shutdown() {
    if (embark_assist::finder_ui::state) {
        for (uint16_t i = 0; i < embark_assist::finder_ui::state->plotinfo.size(); i++) {
            delete embark_assist::finder_ui::state->plotinfo[i];
        }

        delete embark_assist::finder_ui::state;
        embark_assist::finder_ui::state = nullptr;
    }
}
