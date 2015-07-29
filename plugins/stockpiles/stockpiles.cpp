#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataFuncs.h"
#include "LuaTools.h"
#include "modules/Filesystem.h"

#include "../uicommon.h"

#include "StockpileUtils.h"
#include "StockpileSerializer.h"


#include "modules/Filesystem.h"
#include "modules/Gui.h"
#include "modules/Filesystem.h"

#include "df/world.h"
#include "df/world_data.h"

#include "DataDefs.h"
#include "df/ui.h"
#include "df/building_stockpilest.h"
#include "df/stockpile_settings.h"
#include "df/global_objects.h"
#include "df/viewscreen_dwarfmodest.h"

//  stl
#include <functional>
#include <vector>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;
using namespace google::protobuf;
using namespace dfstockpiles;

DFHACK_PLUGIN ( "stockpiles" );
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(selection_rect);

using df::building_stockpilest;
using std::placeholders::_1;

static command_result copystock ( color_ostream &out, vector <string> & parameters );
static bool copystock_guard ( df::viewscreen *top );

static command_result savestock ( color_ostream &out, vector <string> & parameters );
static bool savestock_guard ( df::viewscreen *top );

static command_result loadstock ( color_ostream &out, vector <string> & parameters );
static bool loadstock_guard ( df::viewscreen *top );

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands )
{
    if ( world && ui )
    {
        commands.push_back (
            PluginCommand (
                "copystock", "Copy stockpile under cursor.",
                copystock, copystock_guard,
                "  - In 'q' or 't' mode: select a stockpile and invoke in order\n"
                "    to switch to the 'p' stockpile creation mode, and initialize\n"
                "    the custom settings from the selected stockpile.\n"
                "  - In 'p': invoke in order to switch back to 'q'.\n"
            )
        );
        commands.push_back (
            PluginCommand (
                "savestock", "Save the active stockpile's settings to a file.",
                savestock, savestock_guard,
                "Must be in 'q' mode and have a stockpile selected.\n"
                "example: 'savestock food.dfstock' will save the settings to 'food.dfstock'\n"
                "in your stockpile folder.\n"
                "Omitting the filename will result in text output directly to the console\n\n"
                " -d, --debug: enable debug output\n"
                " <filename>     : filename to save stockpile settings to (will be overwritten!)\n"
            )
        );
        commands.push_back (
            PluginCommand (
                "loadstock", "Load settings from a file and apply them to the active stockpile.",
                loadstock, loadstock_guard,
                "Must be in 'q' mode and have a stockpile selected.\n"
                "example: 'loadstock food.dfstock' will load the settings from 'food.dfstock'\n"
                "in your stockpile folder and apply them to the selected stockpile.\n"
                " -d, --debug: enable debug output\n"
                " <filename>     : filename to load stockpile settings from\n"
            )
        );
    }

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange ( color_ostream &out, state_change_event event )
{
    switch ( event )
    {
    case SC_MAP_LOADED:
        break;
    default:
        break;
    }

    return CR_OK;
}

static bool copystock_guard ( df::viewscreen *top )
{
    using namespace ui_sidebar_mode;

    if ( !Gui::dwarfmode_hotkey ( top ) )
        return false;

    switch ( ui->main.mode )
    {
    case Stockpiles:
        return true;
    case BuildingItems:
    case QueryBuilding:
        return !!virtual_cast<building_stockpilest> ( world->selected_building );
    default:
        return false;
    }
}

static command_result copystock ( color_ostream &out, vector <string> & parameters )
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED

    // For convenience: when used in the stockpiles mode, switch to 'q'
    if ( ui->main.mode == ui_sidebar_mode::Stockpiles )
    {
        world->selected_building = NULL; // just in case it contains some kind of garbage
        ui->main.mode = ui_sidebar_mode::QueryBuilding;
        selection_rect->start_x = -30000;

        out << "Switched back to query building." << endl;
        return CR_OK;
    }

    building_stockpilest *sp = virtual_cast<building_stockpilest> ( world->selected_building );
    if ( !sp )
    {
        out.printerr ( "Selected building isn't a stockpile.\n" );
        return CR_WRONG_USAGE;
    }

    ui->stockpile.custom_settings = sp->settings;
    ui->main.mode = ui_sidebar_mode::Stockpiles;
    world->selected_stockpile_type = stockpile_category::Custom;

    out << "Stockpile options copied." << endl;
    return CR_OK;
}


static bool savestock_guard ( df::viewscreen *top )
{
    using namespace ui_sidebar_mode;

    if ( !Gui::dwarfmode_hotkey ( top ) )
        return false;

    switch ( ui->main.mode )
    {
    case Stockpiles:
        return true;
    case BuildingItems:
    case QueryBuilding:
        return !!virtual_cast<building_stockpilest> ( world->selected_building );
    default:
        return false;
    }
}

static bool loadstock_guard ( df::viewscreen *top )
{
    using namespace ui_sidebar_mode;

    if ( !Gui::dwarfmode_hotkey ( top ) )
        return false;

    switch ( ui->main.mode )
    {
    case Stockpiles:
        return true;
    case BuildingItems:
    case QueryBuilding:
        return !!virtual_cast<building_stockpilest> ( world->selected_building );
    default:
        return false;
    }
}

//  exporting
static command_result savestock ( color_ostream &out, vector <string> & parameters )
{
    building_stockpilest *sp = virtual_cast<building_stockpilest> ( world->selected_building );
    if ( !sp )
    {
        out.printerr ( "Selected building isn't a stockpile.\n" );
        return CR_WRONG_USAGE;
    }

    if ( parameters.size() > 2 )
    {
        out.printerr ( "Invalid parameters\n" );
        return CR_WRONG_USAGE;
    }

    bool debug = false;
    std::string file;
    for ( size_t i = 0; i < parameters.size(); ++i )
    {
        const std::string o = parameters.at ( i );
        if ( o == "--debug"  ||  o ==  "-d" )
            debug =  true;
        else  if ( !o.empty() && o[0] !=  '-' )
        {
            file = o;
        }
    }
    if ( file.empty() )
    {
        out.printerr ( "You must supply a valid filename.\n" );
        return CR_WRONG_USAGE;
    }

    StockpileSerializer cereal ( sp );
    if ( debug )
        cereal.enable_debug ( out );

    if ( !is_dfstockfile ( file ) ) file += ".dfstock";
    if ( !cereal.serialize_to_file ( file ) )
    {
        out.printerr ( "could not save to %s\n", file.c_str() );
        return CR_FAILURE;
    }
    return CR_OK;
}


// importing
static command_result loadstock ( color_ostream &out, vector <string> & parameters )
{
    building_stockpilest *sp = virtual_cast<building_stockpilest> ( world->selected_building );
    if ( !sp )
    {
        out.printerr ( "Selected building isn't a stockpile.\n" );
        return CR_WRONG_USAGE;
    }

    if ( parameters.size() < 1 ||  parameters.size() > 2 )
    {
        out.printerr ( "Invalid parameters\n" );
        return CR_WRONG_USAGE;
    }

    bool debug = false;
    std::string file;
    for ( size_t i = 0; i < parameters.size(); ++i )
    {
        const std::string o = parameters.at ( i );
        if ( o == "--debug"  ||  o ==  "-d" )
            debug =  true;
        else  if ( !o.empty() && o[0] !=  '-' )
        {
            file = o;
        }
    }
    if ( file.empty() ) {
        out.printerr ( "ERROR: missing .dfstock file parameter\n");
        return DFHack::CR_WRONG_USAGE;
    }
    if ( !is_dfstockfile ( file ) )
        file += ".dfstock";
    if ( !Filesystem::exists ( file ) )
    {
        out.printerr ( "ERROR: the .dfstock file doesn't exist: %s\n",  file.c_str());
        return CR_WRONG_USAGE;
    }

    StockpileSerializer cereal ( sp );
    if ( debug )
        cereal.enable_debug ( out );
    if ( !cereal.unserialize_from_file ( file ) )
    {
        out.printerr ( "unserialization failed\n" );
        return CR_FAILURE;
    }
    return CR_OK;
}

/**
 * calls the lua function manage_settings() to kickoff the GUI
 */
bool manage_settings ( building_stockpilest *sp )
{
    auto L = Lua::Core::State;
    color_ostream_proxy out ( Core::getInstance().getConsole() );

    CoreSuspendClaimer suspend;
    Lua::StackUnwinder top ( L );

    if ( !lua_checkstack ( L, 2 ) )
        return false;

    if ( !Lua::PushModulePublic ( out, L, "plugins.stockpiles", "manage_settings" ) )
        return false;

    Lua::Push ( L, sp );

    if ( !Lua::SafeCall ( out, L, 1, 2 ) )
        return false;

    return true;
}

bool show_message_box ( const std::string & title,  const std::string & msg,  bool is_error = false )
{
    auto L = Lua::Core::State;
    color_ostream_proxy out ( Core::getInstance().getConsole() );

    CoreSuspendClaimer suspend;
    Lua::StackUnwinder top ( L );

    if ( !lua_checkstack ( L, 4 ) )
        return false;

    if ( !Lua::PushModulePublic ( out, L, "plugins.stockpiles", "show_message_box" ) )
        return false;

    Lua::Push ( L, title );
    Lua::Push ( L, msg );
    Lua::Push ( L, is_error );

    if ( !Lua::SafeCall ( out, L, 3, 0 ) )
        return false;

    return true;
}

struct stockpiles_import_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool handleInput ( set<df::interface_key> *input )
    {
        df::building_stockpilest *sp = get_selected_stockpile();
        if ( !sp )
            return false;

        if ( input->count ( interface_key::CUSTOM_L ) )
        {
            manage_settings ( sp );
            return true;
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE ( void, feed, ( set<df::interface_key> *input ) )
    {
        if ( !handleInput ( input ) )
            INTERPOSE_NEXT ( feed ) ( input );
    }

    DEFINE_VMETHOD_INTERPOSE ( void, render, () )
    {
        INTERPOSE_NEXT ( render ) ();

        df::building_stockpilest *sp = get_selected_stockpile();
        if ( !sp )
            return;

        auto dims = Gui::getDwarfmodeViewDims();
        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        int y = dims.y2 - 7;

        int links = 0;
        links += sp->links.give_to_pile.size();
        links += sp->links.take_from_pile.size();
        links += sp->links.give_to_workshop.size();
        links += sp->links.take_from_workshop.size();
        if ( links + 12 >= y )
        {
            y += 1;
        }

        OutputHotkeyString ( x, y, "Load/Save Settings", "l", true, left_margin, COLOR_WHITE, COLOR_LIGHTRED );
    }
};

IMPLEMENT_VMETHOD_INTERPOSE ( stockpiles_import_hook, feed );
IMPLEMENT_VMETHOD_INTERPOSE ( stockpiles_import_hook, render );

DFHACK_PLUGIN_IS_ENABLED ( is_enabled );

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable )
{
    if ( !gps )
        return CR_FAILURE;

    if ( enable != is_enabled )
    {
        if (
            !INTERPOSE_HOOK ( stockpiles_import_hook, feed ).apply ( enable ) ||
            !INTERPOSE_HOOK ( stockpiles_import_hook, render ).apply ( enable )
        )
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

static std::vector<std::string> list_dir ( const std::string &path, bool recursive = false )
{
//     color_ostream_proxy out ( Core::getInstance().getConsole() );
    std::vector<std::string> files;
    std::stack<std::string> dirs;
    dirs.push(path);
//     out <<  "list_dir start" <<  endl;
    while (!dirs.empty() ) {
        const std::string current = dirs.top();
//         out <<  "\t walking " <<  current << endl;
        dirs.pop();
        std::vector<std::string> entries;
        const int res = DFHack::getdir(current,  entries);
        if ( res !=  0 )
            continue;
        for ( std::vector<std::string>::iterator it = entries.begin() ; it != entries.end(); ++it )
        {
            if ( (*it).empty() ||  (*it)[0] ==  '.' ) continue;
            //  shitty cross platform c++ we've got to construct the actual path manually
            std::ostringstream child_path_s;
            child_path_s <<  current <<  "/" <<  *it;
            const std::string child = child_path_s.str();
            if ( recursive && Filesystem::isdir ( child ) )
            {
//                 out <<  "\t\tgot child dir: " <<  child <<  endl;
                dirs.push ( child );
            }
            else if ( Filesystem::isfile ( child ) )
            {
                const std::string  rel_path ( child.substr ( std::string ( "./"+path).length()-1 ) );
//                 out <<  "\t\t adding file: " <<  child << "  as   " <<  rel_path  <<  endl;
                files.push_back ( rel_path );
            }
        }
    }
//     out <<  "listdir_stop" <<  endl;
    return files;
}

static std::vector<std::string> clean_dfstock_list ( const std::string &path )
{
    if ( !Filesystem::exists ( path ) )
    {
        return std::vector<std::string>();
    }
    std::vector<std::string> files ( list_dir ( path,  true) );
    files.erase ( std::remove_if ( files.begin(), files.end(), [] ( const std::string &f )
    {
        return !is_dfstockfile ( f );
    } ),  files.end() );
    std::transform ( files.begin(), files.end(), files.begin(), [] ( const std::string &f )
    {
        return f.substr ( 0, f.find_last_of ( "." ) );
    } );
    std::sort ( files.begin(),files.end(), CompareNoCase );
    return files;
}

static bool isEnabled( lua_State *L )
{
    Lua::Push(L,  is_enabled);
    return 1;
}

static int stockpiles_list_settings ( lua_State *L )
{
    auto path = luaL_checkstring ( L, 1 );
    color_ostream &out = *Lua::GetOutput ( L );
    if ( Filesystem::exists ( path ) && !Filesystem::isdir ( path ) )
    {
        lua_pushfstring ( L,  "stocksettings path invalid: %s",  path );
        lua_error ( L );
        return 0;
    }
    std::vector<std::string> files = clean_dfstock_list ( path );
    Lua::PushVector ( L, files, true );
    return 1;
}

static void stockpiles_load ( color_ostream &out, std::string filename )
{
    std::vector<std::string> params;
    params.push_back ( filename );
    command_result r = loadstock ( out, params );
    if ( r !=  CR_OK )
        show_message_box ( "Stockpile Settings Error", "Couldn't load. Does the folder exist?",  true );
}


static void stockpiles_save ( color_ostream &out, std::string filename )
{
    std::vector<std::string> params;
    params.push_back ( filename );
    command_result r = savestock ( out, params );
    if ( r !=  CR_OK )
        show_message_box ( "Stockpile Settings Error", "Couldn't save. Does the folder exist?",  true );
}

DFHACK_PLUGIN_LUA_FUNCTIONS
{
    DFHACK_LUA_FUNCTION ( stockpiles_load ),
    DFHACK_LUA_FUNCTION ( stockpiles_save ),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS
{
    DFHACK_LUA_COMMAND ( stockpiles_list_settings ),
    DFHACK_LUA_END
};




