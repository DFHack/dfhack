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
            Caveats
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
                    current_page = pages::Caveats;
                    break;

                case pages::Caveats:
                    current_page = pages::Intro;
                    break;
                }
            }
            else if (input->count(df::interface_key::SEC_CHANGETAB)) {
                switch (current_page) {
                case pages::Intro:
                    current_page = pages::Caveats;
                    break;

                case pages::General:
                    current_page = pages::Intro;
                    break;

                case pages::Finder:
                    current_page = pages::General;
                    break;

                case pages::Caveats:
                    current_page = pages::Intro;
                    break;
                }
            }
        }

        //===============================================================================

        void ViewscreenHelpUi::render() {
            color_ostream_proxy out(Core::getInstance().getConsole());
            auto screen_size = DFHack::Screen::getWindowSize();
            Screen::Pen pen(' ', COLOR_WHITE);
            Screen::Pen site_pen = Screen::Pen(' ', COLOR_YELLOW, COLOR_BLACK, false);
            Screen::Pen pen_lr(' ', COLOR_LIGHTRED);

            std::vector<std::string> help_text;

            Screen::clear();

            switch (current_page) {
            case pages::Intro:
                Screen::drawBorder("Embark Assistant Help/Info Introduction Page");

                help_text.push_back("Embark Assistant is used on the embark selection screen to provide information");
                help_text.push_back("to help selecting a suitable embark site. It provides three services:");
                help_text.push_back("- Display of normally invisible sites overlayed on the region map.");
                help_text.push_back("- Embark rectangle resources. More detailed and correct than vanilla DF.");
                help_text.push_back("- Site find search. Richer set of selection criteria than the vanilla");
                help_text.push_back("  DF Find that Embark Assistant suppresses (by using the same key).");
                help_text.push_back("");
                help_text.push_back("The functionality requires a screen height of at least 42 lines to display");
                help_text.push_back("correctly (that's the height of the Finder screen), as fitting everything");
                help_text.push_back("onto a standard 80*25 screen would be too challenging. The help is adjusted");
                help_text.push_back("to fit into onto an 80*42 screen.");
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
                help_text.push_back("c: Clears the results of a Find operation, and also cancels an operation if");
                help_text.push_back("   one is under way.");
                help_text.push_back("q: Quits the Embark Assistant and brings you back to the vanilla DF interface.");
                help_text.push_back("   It can be noted that the Embark Assistant automatically cancels itself");
                help_text.push_back("   when DF leaves the embark screen either through <ESC>Abort Game or by");
                help_text.push_back("   embarking.");
                help_text.push_back("Below this a Matching World Tiles count is displayed. It shows the number");
                help_text.push_back("of World Tiles that have at least one embark matching the Find criteria.");

                break;

            case pages::General:
                Screen::drawBorder("Embark Assistant Help/Info General Page");

                help_text.push_back("The Embark Assistant overlays the region map with characters indicating sites");
                help_text.push_back("normally not displayed by DF. The following key is used:");
                help_text.push_back("C: Camp");
                help_text.push_back("c: Cave. Only displayed if the DF worldgen parameter does not display caves.");
                help_text.push_back("i: Important Location. The author doesn't actually know what those are.");
                help_text.push_back("l: Lair");
                help_text.push_back("L: Labyrinth");
                help_text.push_back("M: Monument. The author is unsure how/if this is broken down further.");
                help_text.push_back("S: Shrine");
                help_text.push_back("V: Vault");
                help_text.push_back("The Embark info below the region map differs from the vanilla DF display in a");
                help_text.push_back("few respects. Firstly, it shows resources in the embark rectangle, rather than");
                help_text.push_back("DF's display of resources in the region DF currently displays. Secondly, the");
                help_text.push_back("DF display doesn't take elevation based soil erosion or the magma sea depth");
                help_text.push_back("into consideration, so it can display resources that actually are cut away.");
                help_text.push_back("(It can be noted that the DFHack Sand indicator does take these elements into");
                help_text.push_back("account).");
                help_text.push_back("The info the Embark Assistant displays is:");
                help_text.push_back("Sand, if present");
                help_text.push_back("Clay, if present");
                help_text.push_back("Min and Max soil depth in the embark rectangle.");
                help_text.push_back("Flat indicator if all the tiles in the embark have the same elevation.");
                help_text.push_back("Aquifer indicator, color coded as blue if all tiles have an aquifer and light");
                help_text.push_back("blue if some, but not all, tiles have one.");
                help_text.push_back("Waterfall, if the embark has river elevation differences.");
                help_text.push_back("Flux, if present");
                help_text.push_back("A list of all metals present in the embark.");
                help_text.push_back("A list of all economic minerals present in the embark. Both clays and flux");
                help_text.push_back("stones are economic, so they show up here as well.");
                help_text.push_back("In addition to the above, the Find functionality can also produce blinking");
                help_text.push_back("overlays over the region map and the middle world map to indicate where");
                help_text.push_back("matching embarks are found. The region display marks the top left corner of");
                help_text.push_back("a matching embark rectangle as a matching tile.");

                break;

            case pages::Finder:
                Screen::drawBorder("Embark Assistant Help/Info Find Page");

                help_text.push_back("The Embark Assist Finder page is brought up with the f command key.");
                help_text.push_back("The top of the Finder page lists the command keys available on the page:");
                help_text.push_back("4/6 or horizontal arrow keys to move between the element and element values.");
                help_text.push_back("8/2 or vertical arrow keys to move up/down in the current list.");
                help_text.push_back("ENTER to select a value in the value list, entering it among the selections.");
                help_text.push_back("f to activate the Find functionality using the values in the middle column.");
                help_text.push_back("ESC to leave the screen without activating a Find operation.");
                help_text.push_back("The X and Y dimensions are those of the embark to search for. Unlike DF");
                help_text.push_back("itself these parameters are initiated to match the actual embark rectangle");
                help_text.push_back("when a new search is initiated (prior results are cleared.");
                help_text.push_back("The 6 Savagery and Evilness parameters takes some getting used to. They");
                help_text.push_back("allow for searching for embarks with e.g. has both Good and Evil tiles.");
                help_text.push_back("All as a parameter means every embark tile has to have this value.");
                help_text.push_back("Present means at least one embark tile has to have this value.");
                help_text.push_back("Absent means the feature mustn't exist in any of the embark tiles.");
                help_text.push_back("N/A means no restrictions are applied.");
                help_text.push_back("The Aquifer criterion introduces some new parameters:");
                help_text.push_back("Partial means at least one tile has to have an aquifer, but it also has");
                help_text.push_back("to be absent from at least one tile. Not All means an aquifer is tolerated");
                help_text.push_back("as long as at least one tile doesn't have one, but it doesn't have to have");
                help_text.push_back("any at all.");
                help_text.push_back("Min/Max rivers should be self explanatory. The Yes and No values of");
                help_text.push_back("Waterfall, Flat, etc. means one has to be Present and Absent respectivey.");
                help_text.push_back("Min/Max soil uses the same terminology as DF for 1-4. The Min Soil");
                help_text.push_back("Everywhere toggles the Min Soil parameter between acting as All and");
                help_text.push_back("and Present.");
                help_text.push_back("The parameters for biomes, regions, etc. all require that the required");
                help_text.push_back("feature is Present in the embark, and entering the same value multiple");
                help_text.push_back("times does nothing (the first match ticks all requirements off). It can be");
                help_text.push_back("noted that all the Economic materials are found in the much longer Mineral");
                help_text.push_back("list. Note that Find is a fairly time consuming task (is it is in vanilla).");
                break;

            case pages::Caveats:
                Screen::drawBorder("Embark Assistant Help/Info Caveats Page");

                help_text.push_back("Find searching first does a sanity check (e.g. max < min) and then a rough");
                help_text.push_back("world tile match to find tiles that may have matching embarks. This results");
                help_text.push_back("in an overlay of inverted yellow X on top of the middle world map. Then");
                help_text.push_back("those tiles are scanned in detail, one feature shell (16*16 world tile");
                help_text.push_back("block) at a time, and the results are displayed as green inverted X on");
                help_text.push_back("the same map (replacing or erasing the yellow ones). region map overlay");
                help_text.push_back("data is generated as well.");
                help_text.push_back("");
                help_text.push_back("Caveats & technical stuff:");
                help_text.push_back("- The Find searching uses simulated cursor movement input to DF to get it");
                help_text.push_back("  to load feature shells and detailed region data, and this costs the");
                help_text.push_back("  least when done one feature shell at a time.");
                help_text.push_back("- The search strategy causes detailed region data to update surveyed");
                help_text.push_back("  world info, and this can cause a subsequent search to generate a smaller");
                help_text.push_back("  set of preliminary matches (yellow tiles) than a previous search.");
                help_text.push_back("  However, this is a bug only if it causes the search to fail to find");
                help_text.push_back("  actual existing matches.");
                help_text.push_back("- The site info is deduced by the author, so there may be errors and");
                help_text.push_back("  there are probably site types that end up not being identified.");
                help_text.push_back("- Aquifer indications are based on the author's belief that they occur");
                help_text.push_back("  whenever an aquifer supporting layer is present at a depth of 3 or");
                help_text.push_back("  more.");
                help_text.push_back("- The biome determination logic comes from code provided by Ragundo,");
                help_text.push_back("  with only marginal changes by the author. References can be found in");
                help_text.push_back("  the source file.");
                help_text.push_back("- Thralling is determined by weather material interactions causing");
                help_text.push_back("  blinking, which the author believes is one of 4 thralling changes.");
                help_text.push_back("- The geo information is gathered by code which is essentially a");
                help_text.push_back("  copy of parts of prospector's code adapted for this plugin.");
                help_text.push_back("- Clay determination is made by finding the reaction MAKE_CLAY_BRICKS.");
                help_text.push_back("  Flux determination is made by finding the reaction PIG_IRON_MAKING.");
                help_text.push_back("- Right world map overlay not implemented as author has failed to");
                help_text.push_back("  emulate the sizing logic exactly.");
                help_text.push_back("Version 0.1 2017-08-30");

                break;
            }

            //  Add control keys to first line.
            embark_assist::screen::paintString(pen_lr, 1, 1, "TAB/Shift-TAB");
            embark_assist::screen::paintString(pen, 14, 1, ":Next/Previous Page");
            embark_assist::screen::paintString(pen_lr, 34, 1, "ESC");
            embark_assist::screen::paintString(pen, 37, 1, ":Leave Info/Help");

            for (uint16_t i = 0; i < help_text.size(); i++) {
                embark_assist::screen::paintString(pen, 1, 2 + i, help_text[i]);
            }

            switch (current_page) {
            case pages::Intro:
                embark_assist::screen::paintString(pen_lr, 1, 26, "i");
                embark_assist::screen::paintString(pen_lr, 1, 27, "f");
                embark_assist::screen::paintString(pen_lr, 1, 28, "c");
                embark_assist::screen::paintString(pen_lr, 1, 30, "q");
                break;

            case pages::General:
                embark_assist::screen::paintString(site_pen, 1, 4, "C");
                embark_assist::screen::paintString(site_pen, 1, 5, "c");
                embark_assist::screen::paintString(site_pen, 1, 6, "i");
                embark_assist::screen::paintString(site_pen, 1, 7, "l");
                embark_assist::screen::paintString(site_pen, 1, 8, "L");
                embark_assist::screen::paintString(site_pen, 1, 9, "M");
                embark_assist::screen::paintString(site_pen, 1, 10, "S");
                embark_assist::screen::paintString(site_pen, 1, 11, "V");
                break;

            case pages::Finder:
                embark_assist::screen::paintString(pen_lr, 1, 4, "4/6");
                embark_assist::screen::paintString(pen_lr, 1, 5, "8/2");
                embark_assist::screen::paintString(pen_lr, 1, 6, "ENTER");
                embark_assist::screen::paintString(pen_lr, 1, 7, "f");
                embark_assist::screen::paintString(pen_lr, 1, 8, "ESC");
                break;

            case pages::Caveats:
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
    Screen::show(new embark_assist::help_ui::ViewscreenHelpUi(), plugin_self);
}
