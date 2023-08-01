#include "Core.h"
#include <Console.h>

#include <string>
#include <set>
#include <modules/Gui.h>

#include "Types.h"

#include "help_ui.h"
#include "screen.h"

using std::vector;

namespace embark_assist{
    namespace help_ui {
        enum class pages {
            Intro,
            General,
            Finder,
            Caveats_1,
            Caveats_2
        };

        class ViewscreenHelpUi : public dfhack_viewscreen
        {
        public:
            ViewscreenHelpUi();

            void feed(std::set<df::interface_key> *input);

            void render();

            std::string getFocusString() { return "Help UI"; }

        private:
            pages current_page = pages::Intro;
        };

        //===============================================================================

        void ViewscreenHelpUi::feed(std::set<df::interface_key> *input) {
            if (input->count(df::interface_key::LEAVESCREEN))
            {
                input->clear();
                Screen::dismiss(this);
                return;
            }
            else if (input->count(df::interface_key::CHANGETAB)) {
                switch (current_page) {
                case pages::Intro:
                    current_page = pages::General;
                    break;

                case pages::General:
                    current_page = pages::Finder;
                    break;

                case pages::Finder:
                    current_page = pages::Caveats_1;
                    break;

                case pages::Caveats_1:
                    current_page = pages::Caveats_2;
                    break;

                case pages::Caveats_2:
                    current_page = pages::Intro;
                    break;
                }
            }
            else if (input->count(df::interface_key::SEC_CHANGETAB)) {
                switch (current_page) {
                case pages::Intro:
                    current_page = pages::Caveats_2;
                    break;

                case pages::General:
                    current_page = pages::Intro;
                    break;

                case pages::Finder:
                    current_page = pages::General;
                    break;

                case pages::Caveats_1:
                    current_page = pages::Finder;
                    break;

                case pages::Caveats_2:
                    current_page = pages::Caveats_1;
                    break;
                }
            }
        }

        //===============================================================================

        void ViewscreenHelpUi::render() {
            color_ostream_proxy out(Core::getInstance().getConsole());
            Screen::Pen pen(' ', COLOR_WHITE);
            Screen::Pen site_pen = Screen::Pen(' ', COLOR_YELLOW, COLOR_BLACK, false);
            Screen::Pen pen_lr(' ', COLOR_LIGHTRED);

            std::vector<std::string> help_text;

            Screen::clear();

            switch (current_page) {
            case pages::Intro:
                Screen::drawBorder("  Embark Assistant Help/Info Introduction Page  ");

                help_text.push_back("Embark Assistant is used on the embark selection screen to provide information");
                help_text.push_back("to help selecting a suitable embark site. It provides three services:");
                help_text.push_back("- Display of normally invisible sites overlayed on the region map.");
                help_text.push_back("- Embark rectangle resources. More detailed and correct than vanilla DF.");
                help_text.push_back("- Site find search. Richer set of selection criteria than the vanilla");
                help_text.push_back("  DF Find that Embark Assistant suppresses (by using the same key).");
                help_text.push_back("");
                help_text.push_back("The functionality requires a screen height larger than the default 80*25,");
                help_text.push_back("and while the Finder screen provides for scrolling, the embark resources");
                help_text.push_back("list will spill over the bottom if many resources are present and the");
                help_text.push_back("screen isn't deep enough. This help info is adapted to fit onto 80*46.");
                help_text.push_back("This help/info is split over several screens, and you can move between them");
                help_text.push_back("using the TAB/Shift-TAB keys, and leave the help from any screen using ESC.");
                help_text.push_back("");
                help_text.push_back("When the Embark Assistant is started it provides site information (if any)");
                help_text.push_back("as an overlay over the region map. Beneath that you will find a summary of");
                help_text.push_back("the resources in the current embark rectangle (explained in more detail on");
                help_text.push_back("the the next screen).");
                help_text.push_back("On the right side the command keys the Embark Assistant uses are listed");
                help_text.push_back("(this partially overwrites the DFHack Embark Tools information until the");
                help_text.push_back("screen is resized). It can also be mentioned that the DF 'f'ind key help");
                help_text.push_back("at the bottom of the screen is masked as the functionality is overridden by");
                help_text.push_back("the Embark Assistant.");
                help_text.push_back("Main screen control keys used by the Embark Assistant:");
                help_text.push_back("i: Info/Help. Brings up this display.");
                help_text.push_back("f: Brings up the Find Embark screen. See the Find page for more information.");
                help_text.push_back("c: Clears the results of a Find operation, or cancels an operation if one is");
                help_text.push_back("   under way (at which time a second 'c' clears it).");
                help_text.push_back("q: Quits the Embark Assistant and brings you back to the vanilla DF interface.");
                help_text.push_back("   It can be noted that the Embark Assistant automatically cancels itself");
                help_text.push_back("   when DF leaves the embark screen either through <ESC>Abort Game or by");
                help_text.push_back("   embarking.");
                help_text.push_back("Below this a Matching World Tiles count is displayed. It shows the number");
                help_text.push_back("of World Tiles that have at least one embark matching the Find criteria.");
                help_text.push_back("Note that World Tiles are the ones shown on the 'Region' map: the 'World'");
                help_text.push_back("typically merges several World Tiles into each of its tiles.");

                break;

            case pages::General:
                Screen::drawBorder("  Embark Assistant Help/Info General Page  ");

                help_text.push_back("The Embark Assistant overlays the region map with characters indicating sites");
                help_text.push_back("normally not displayed by DF. The following key is used:");
                help_text.push_back("C: Camp");
                help_text.push_back("c: Cave. Only displayed if the DF worldgen parameter does not display caves.");
                help_text.push_back("i: Important Location. The author doesn't actually know what those are.");
                help_text.push_back("m: Lair (Simple Mound)");
                help_text.push_back("b: Lair (Simple Burrow)");
                help_text.push_back("l: Lair (Wilderness Location)");
                help_text.push_back("L: Labyrinth");
                help_text.push_back("M: Monument. The author is unsure how/if this is broken down further.");
                help_text.push_back("S: Shrine");
                help_text.push_back("V: Vault");
                help_text.push_back("The Embark info below the region map differs from the vanilla DF display in a");
                help_text.push_back("few respects. Firstly, it shows resources in the embark rectangle, rather than");
                help_text.push_back("DF's display of resources in the region DF currently displays. Secondly, the");
                help_text.push_back("DF display doesn't take elevation based soil erosion or the magma sea depth");
                help_text.push_back("into consideration, so it can display resources that actually are cut away.");
                help_text.push_back("Thirdly, it takes 'incursions', i.e. small sections of neighboring tiles'");
                help_text.push_back("biomes into consideration for many fields.");
                help_text.push_back("(It can be noted that the DFHack Sand indicator does take the first two");
                help_text.push_back("elements into account).");
                help_text.push_back("The info the Embark Assistant displays is:");
                help_text.push_back("Incompl. Survey if all incursions couldn't be examined because that requires");
                help_text.push_back("info from neighboring world tiles that haven't been surveyed.");
                help_text.push_back("Sand, if present, including through incursions.");
                help_text.push_back("Clay, if present, including thorugh incursions.");
                help_text.push_back("Min and Max soil depth in the embark rectangle, including incursions.");
                help_text.push_back("Flat indicator if all the tiles and incursions have the same elevation.");
                help_text.push_back("Aquifer indicator: 'No', 'Lt', 'Hv' for presence/type including incursions.");
                help_text.push_back("Waterfall and largest Z level drop if the river has elevation differences");
                help_text.push_back("Evil weather, when present: BR = Blood Rain, TS = Temporary Syndrome");
                help_text.push_back("PS = Permanent Syndrome, Re = Reanimating, and Th = Thralling. Incursions.");
                help_text.push_back("Flux, if present. NOT allowing for small incursion bits.");
                help_text.push_back("A list of all metals present in the embark. Not incursions.");
                help_text.push_back("A list of all economic minerals present in the embark. Both clays and flux");
                help_text.push_back("stones are economic, so they show up here as well. Not incursions.");
                help_text.push_back("Neighbors, including Kobolds (SKULKING) (hidden in vanilla) and Towers.");
                help_text.push_back("In addition to the above, the Find functionality can also produce blinking");
                help_text.push_back("overlays over the Local, Region, and World maps to indicate where");
                help_text.push_back("matching embarks are found. The Local display marks the top left corner of");
                help_text.push_back("a matching embark rectangle as a matching tile.");

                break;

            case pages::Finder:
                Screen::drawBorder("  Embark Assistant Help/Info Find Page  ");

                help_text.push_back("The Embark Assist Finder page is brought up with the f command key.");
                help_text.push_back("The top of the Finder page lists the command keys available on the page:");
                help_text.push_back("4/6 or horizontal arrow keys to move between the element and element values.");
                help_text.push_back("8/2 or vertical arrow keys to move up/down in the current list.");
                help_text.push_back("ENTER to select a value in the value list, entering it among the selections.");
                help_text.push_back("f to activate the Find functionality using the values in the middle column.");
                help_text.push_back("ESC to leave the screen without activating a Find operation.");
                help_text.push_back("s/l is used to save/load search profile to/from embark_assistant_profile.txt");
                help_text.push_back("stored in ./data/init. There's some minor error detection that will refuse");
                help_text.push_back("to load a file that doesn't check out.");
                help_text.push_back("The X and Y dimensions are those of the embark to search for. Unlike DF");
                help_text.push_back("itself these parameters are initiated to match the actual embark rectangle");
                help_text.push_back("when a new search is initiated (prior results are cleared.");
                help_text.push_back("The 6 Savagery and Evilness parameters takes some getting used to. They");
                help_text.push_back("allow for searching for embarks with e.g. has both Good and Evil tiles.");
                help_text.push_back("All as a parameter means every embark tile has to have this value.");
                help_text.push_back("Present means at least one embark tile has to have this value.");
                help_text.push_back("Absent means the feature mustn't exist in any of the embark tiles.");
                help_text.push_back("N/A means no restrictions are applied.");
                help_text.push_back("The Aquifer criterion allows you to search for a number of combinations.");
                help_text.push_back("Min/Max rivers should be self explanatory. The Yes and No values of");
                help_text.push_back("Clay, etc. means one has to be Present and Absent respectivey.");
                help_text.push_back("Min Waterfall Drop finds embarks with drops of at least that number");
                help_text.push_back("of Z levels, but Absent = no waterfall at all.");
                help_text.push_back("Min/Max soil uses the same terminology as DF for 1-4. The Min Soil");
                help_text.push_back("Everywhere toggles the Min Soil parameter between acting as All and");
                help_text.push_back("Present.");
                help_text.push_back("Freezing allows you to select embarks to select/avoid various freezing");
                help_text.push_back("conditions. Note that the minimum temperature is held for only 10 ticks");
                help_text.push_back("in many embarks.");
                help_text.push_back("Syndrome Rain allows you to search for Permanent and Temporary syndromes,");
                help_text.push_back("where Permanent allows for Temporary ones as well, but not the reverse, and");
                help_text.push_back("Not Permanent matches everything except Permanent syndromes.");
                help_text.push_back("Reanimation packages thralling and reanimation into a single search");
                help_text.push_back("criterion. Not Tralling means nothing and just reanimation is matched.");
                help_text.push_back("The parameters for biomes, regions, etc. all require that the required");
                help_text.push_back("feature is Present in the embark, and entering the same value multiple");
                help_text.push_back("times does nothing (the first match ticks all requirements off). It can be");
                help_text.push_back("noted that all the Economic materials are found among the longer Minerals.");
                help_text.push_back("Min/Max Necro Towers and Min/Max Near Civs work on neighbor counts, while");
                help_text.push_back("the civ entity selections let you choose your specific neighbors.");
                help_text.push_back("Note that Find is a fairly time consuming task (as it is in vanilla).");
                break;

            case pages::Caveats_1:
                Screen::drawBorder("  Embark Assistant Help/Info Caveats 1 Page  ");

                help_text.push_back("The plugin surveys world tiles through two actions: using the 'f'ind");
                help_text.push_back("function and through manual movement of the embark rectangle between world");
                help_text.push_back("tiles. In both cases the whole world tile is surveyed, regardless of which");
                help_text.push_back("tiles the embark rectangle covers.");
                help_text.push_back("'Find' searching first does a sanity check (e.g. max < min) and then a rough");
                help_text.push_back("world tile match to find tiles that may have matching embarks. This results");
                help_text.push_back("in overlays of inverted yellow X on top of the Region and World maps. Then");
                help_text.push_back("those tiles are scanned in detail, one feature shell (16*16 world tile");
                help_text.push_back("block) at a time, and the results are displayed as green inverted X on");
                help_text.push_back("the same map (replacing or erasing the yellow ones). Local map overlay");
                help_text.push_back("data is generated as well.");
                help_text.push_back("Since 'incursion' processing requires that the neighboring tiles that may");
                help_text.push_back("provide them are surveyed before the current tile and tiles have to be");
                help_text.push_back("surveyed in some order, the find function can not perform a complete");
                help_text.push_back("survey of prospective embarks that border world tiles yet to be surveyed");
                help_text.push_back("so the very first 'find' will fail to mark such embarks that actually do");
                help_text.push_back("match, while the second and following 'find' operations will locate them");
                help_text.push_back("because critical information from the first scan is kept for subsequent");
                help_text.push_back("ones.");
                help_text.push_back("");
                help_text.push_back("Caveats & technical stuff:");
                help_text.push_back("- embark-tools' neutralizing of DF random rectangle placement on world");
                help_text.push_back("  tile shifts changes placement info after it's read. This is compensated");
                help_text.push_back("  for, but at the cost of incorrect info without it. In tile move back");
                help_text.push_back("  and forth causes the correct info to be displayed.");
                help_text.push_back("- The plugin does in fact allow for a single, optional case sensitive");
                help_text.push_back("  parameter when invoked: 'fileresult'. When this parameter is provided");
                help_text.push_back("  The plugin will read the search profile stored to file and immediately");
                help_text.push_back("  initiate a search for matches. This search is performed twice to ensure");
                help_text.push_back("  incursions are handled correctly, and then the number of matching world");
                help_text.push_back("  is written to the file <DF>/data/init/embark_assistant_fileresult.txt.");
                help_text.push_back("  It can be noted that this file is deleted before the first search is");
                help_text.push_back("  started. The purpose of this mode is to allow external harnesses to");
                help_text.push_back("  generate worlds, search them for matches, and use the file results to");
                help_text.push_back("  to determine which worlds to keep. It can be noted that after the search");
                help_text.push_back("  the plugin continues to work essentially as usual, including external");
                help_text.push_back("  to terminate, and that the author plugin can provide no help when it comes");
                help_text.push_back("  to setting up any kind of harness using the plugin functionality.");
                help_text.push_back("- The Find searching uses simulated cursor movement input to DF to get it");
                help_text.push_back("  to load feature shells and detailed region data, and this costs the");
                help_text.push_back("  least when done one feature shell at a time.");
                help_text.push_back("- The search strategy causes detailed region data to update surveyed");
                help_text.push_back("  world info, and this can cause a subsequent search to generate a smaller");
                help_text.push_back("  set of preliminary matches (yellow tiles) than a previous search.");
                help_text.push_back("  Note that the first search can miss a fair number of matches for");
                help_text.push_back("  technical reasons discussed above and below.");

                break;

            case pages::Caveats_2:
                Screen::drawBorder("  Embark Assistant Help/Info Caveats 2 Page  ");

                help_text.push_back("- Aquifer indications are based on the author's belief that they occur");
                help_text.push_back("  whenever an aquifer supporting layer is present at a depth of 3 or");
                help_text.push_back("  more. In addition, Toady's description on how Heavy aquifers are");
                help_text.push_back("  selected has been used.");
                help_text.push_back("- Thralling is determined by whether material interactions causes");
                help_text.push_back("  blinking, which the author believes is one of 4 thralling changes.");
                help_text.push_back("- The geo information is gathered by code which is essentially a");
                help_text.push_back("  copy of parts of Prospector's code adapted for this plugin.");
                help_text.push_back("- Clay determination is made by finding the reaction MAKE_CLAY_BRICKS.");
                help_text.push_back("- Flux determination is made by finding the reaction PIG_IRON_MAKING.");
                help_text.push_back("- Coal is detected by finding COAL producing reactions on minerals.");
                help_text.push_back("- There's currently a DF bug (#0010267) that causes adamantine spires");
                help_text.push_back("  reaching caverns that have been removed at world gen to fail to be");
                help_text.push_back("  generated at all. It's likely this bug also affects magma pools.");
                help_text.push_back("  This plugin does not address this but scripts can correct it.");
                help_text.push_back("- The plugin detects 'incursions' of neighboring tiles into embarks, but");
                help_text.push_back("  this functionality is incomplete when the incursion comes from a");
                help_text.push_back("  neighboring tile that hasn't been surveyed yet. The embark info displays");
                help_text.push_back("  what it can, while indicating if it is incomplete, while the first 'f'ind");
                help_text.push_back("  will automatically fail to match any embarks that can not be analyzed");
                help_text.push_back("  fully. Such failures only appear on embarks that touch an edge of the");
                help_text.push_back("  world tile, and a second (and all subsequent) searches will be complete.");
                help_text.push_back("  Since searches can take considerable time, it's left to the user to decide");
                help_text.push_back("  whether to make a second, completing, search.");
                help_text.push_back("- Incursions are taken into consideration when looking for Aquifers,");
                help_text.push_back("  Clay, Sand, Min Soil when Everywhere, Biomes, Regions, Evil Weather,");
                help_text.push_back("  Savagery, Evilness, Freezing and Flatness, and trees, but ignored for");
                help_text.push_back("  metals/economics/minerals (including Flux and Coal) as any volumes are");
                help_text.push_back("  typically too small to be of interest. Rivers, Waterfalls, Spires, and");
                help_text.push_back("  Magma Pools are not incursion related features.");
                help_text.push_back("- Neighbor determination makes use of a flag in entities.");
                help_text.push_back("- There are special rules for handing of incursions from Lakes and Oceans,");
                help_text.push_back("  as well as Mountains into everything that isn't a Lake or Ocean, and the");
                help_text.push_back("  rules state that these incursions should be reversed (i.e. 'normal' biomes");
                help_text.push_back("  should push into Lakes, Oceans, and Mountains, even when the indicators");
                help_text.push_back("  say otherwise). This rule is clear for edges, but not for corners, as it");
                help_text.push_back("  does not specify which of the potentially multiple 'superior' biomes");
                help_text.push_back("  should be used. The plugin uses the arbitrarily selected rule that the");
                help_text.push_back("  touching corner to the NW should be selected if eligible, then the one to");
                help_text.push_back("  the N, followed by the one to the W, and lastly the one acting as the");
                help_text.push_back("  reference. This means there's a risk embarks with such 'trouble' corners");
                help_text.push_back("  may get affected corner(s) evaluated incorrectly.");
                help_text.push_back("Version 0.14 2021-06-13");

                break;
            }

            //  Add control keys to first line.
            embark_assist::screen::paintString(pen_lr, 1, 1, DFHack::Screen::getKeyDisplay(df::interface_key::CHANGETAB).c_str());
            embark_assist::screen::paintString(pen, 4, 1, "/");
            embark_assist::screen::paintString(pen_lr, 5, 1, DFHack::Screen::getKeyDisplay(df::interface_key::SEC_CHANGETAB).c_str());
            embark_assist::screen::paintString(pen, 14, 1, ": Next/Previous Page");
            embark_assist::screen::paintString(pen_lr, 35, 1, DFHack::Screen::getKeyDisplay(df::interface_key::LEAVESCREEN).c_str());
            embark_assist::screen::paintString(pen, 38, 1, ": Leave Info/Help");

            for (uint16_t i = 0; i < help_text.size(); i++) {
                embark_assist::screen::paintString(pen, 1, 2 + i, help_text[i]);
            }

            switch (current_page) {
            case pages::Intro:
                embark_assist::screen::paintString(pen_lr, 1, 26, DFHack::Screen::getKeyDisplay(df::interface_key::CUSTOM_I).c_str());
                embark_assist::screen::paintString(pen_lr, 1, 27, DFHack::Screen::getKeyDisplay(df::interface_key::CUSTOM_F).c_str());
                embark_assist::screen::paintString(pen_lr, 1, 28, DFHack::Screen::getKeyDisplay(df::interface_key::CUSTOM_C).c_str());
                embark_assist::screen::paintString(pen_lr, 1, 30, DFHack::Screen::getKeyDisplay(df::interface_key::CUSTOM_Q).c_str());
                break;

            case pages::General:
                embark_assist::screen::paintString(site_pen, 1, 4, "C");
                embark_assist::screen::paintString(site_pen, 1, 5, "c");
                embark_assist::screen::paintString(site_pen, 1, 6, "i");
                embark_assist::screen::paintString(site_pen, 1, 7, "m");
                embark_assist::screen::paintString(site_pen, 1, 8, "b");
                embark_assist::screen::paintString(site_pen, 1, 9, "l");
                embark_assist::screen::paintString(site_pen, 1, 10, "L");
                embark_assist::screen::paintString(site_pen, 1, 11, "M");
                embark_assist::screen::paintString(site_pen, 1, 12, "S");
                embark_assist::screen::paintString(site_pen, 1, 13, "V");
                break;

            case pages::Finder:
                embark_assist::screen::paintString(pen_lr, 1, 4, DFHack::Screen::getKeyDisplay(df::interface_key::STANDARDSCROLL_LEFT).c_str());
                embark_assist::screen::paintString(pen_lr, 3, 4, DFHack::Screen::getKeyDisplay(df::interface_key::STANDARDSCROLL_RIGHT).c_str());
                embark_assist::screen::paintString(pen_lr, 1, 5, DFHack::Screen::getKeyDisplay(df::interface_key::STANDARDSCROLL_UP).c_str());
                embark_assist::screen::paintString(pen_lr, 3, 5, DFHack::Screen::getKeyDisplay(df::interface_key::STANDARDSCROLL_DOWN).c_str());
                embark_assist::screen::paintString(pen_lr, 1, 6, DFHack::Screen::getKeyDisplay(df::interface_key::SELECT).c_str());
                embark_assist::screen::paintString(pen_lr, 1, 7, DFHack::Screen::getKeyDisplay(df::interface_key::CUSTOM_F).c_str());
                embark_assist::screen::paintString(pen_lr, 1, 8, DFHack::Screen::getKeyDisplay(df::interface_key::LEAVESCREEN).c_str());
                embark_assist::screen::paintString(pen_lr, 1, 9, DFHack::Screen::getKeyDisplay(df::interface_key::CUSTOM_S).c_str());
                embark_assist::screen::paintString(pen_lr, 3, 9, DFHack::Screen::getKeyDisplay(df::interface_key::CUSTOM_L).c_str());
                break;

            case pages::Caveats_1:
                break;

            case pages::Caveats_2:
                break;
            }
            dfhack_viewscreen::render();
        }

        //===============================================================================

        ViewscreenHelpUi::ViewscreenHelpUi() {
        }
    }
}

//===============================================================================
//  Exported operations
//===============================================================================

void embark_assist::help_ui::init(DFHack::Plugin *plugin_self) {
    Screen::show(std::make_unique<embark_assist::help_ui::ViewscreenHelpUi>(), plugin_self);
}
