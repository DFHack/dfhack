#include <iostream>
#include <libnotify/notify.h>

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include "modules/EventManager.h"

#include "DataDefs.h"
#include "df/job.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN( "notifier" );
REQUIRE_GLOBAL( world );

void sendNotification( std::string message )
{
    notify_init( "Dwarf Fortress" );
    NotifyNotification *notification = notify_notification_new( "Dwarf Fortress",
                                       message.c_str(),
                                       0 );
    notify_notification_set_timeout( notification, 5000 );
    notify_notification_show( notification, 0 );
}

void handleMood( color_ostream &out, void *job )
{
    if( ( static_cast<df::job *>( job )->job_type >= job_type::StrangeMoodCrafter ) &&
        ( static_cast<df::job *>( job )->job_type <= job_type::StrangeMoodMechanics ) ) {
        sendNotification( "Dwarf taken by mood." );
    }
}

void handleInvasion( color_ostream &out, void *invasion )
{
    sendNotification( "Invasion." );
}

command_result notify( color_ostream &out, std::vector <std::string> &parameters )
{
    if( parameters.empty() ) {
        return CR_WRONG_USAGE;
    }
    for( int i = 0; i < parameters.size(); i++ ) {
        if( parameters[i] == "mood" ) {
            EventManager::EventHandler jobHandler( handleMood, 1 );
            EventManager::registerListener( EventManager::EventType::JOB_INITIATED, jobHandler, plugin_self );
        } else if( parameters[i] == "invasion" ) {
            EventManager::EventHandler invasionHandler( handleInvasion, 1 );
            EventManager::registerListener( EventManager::EventType::INVASION, invasionHandler, plugin_self );
        }
        /*
        else if (parameters[i] == "birth") {}
        else if (parameters[i] == "migrants") {}
        else if (parameters[i] == "caravan") {}
        else if (parameters[i] == "weather") {}
        */
        else if( parameters[i] == "all" ) {
            EventManager::EventHandler jobHandler( handleMood, 1 );
            EventManager::registerListener( EventManager::EventType::JOB_INITIATED, jobHandler, plugin_self );
            EventManager::EventHandler invasionHandler( handleInvasion, 1 );
            EventManager::registerListener( EventManager::EventType::INVASION, invasionHandler, plugin_self );
        }

        else {
            return CR_WRONG_USAGE;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_init( color_ostream &out,
        std::vector <PluginCommand> &commands )
{
    commands.push_back( PluginCommand(
                            "notify", "Sets up desktop notifications",
                            notify, false,
                            "  This plugin sets up desktop notifications to be sent when certain\n"
                            "  events happen in your fort.\n"
                            "Arguments:\n"
                            "  mood     - trigger notification when a dwarf is taken by a mood\n"
                            "  invasion - trigger notification when an invasion arrives\n"
                            "  birth    - trigger notification when a child is born in the fort\n"
                            "  migrants - trigger notification when migrants arrive\n"
                            "  caravan  - trigger notification when a trade caravan arrives\n"
                            "  weather  - trigger notification when an evil weather event happens\n"
                            "  all      - trigger notification when any of the above events happen"
                        ) );
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown( color_ostream &out )
{
    EventManager::unregisterAll( plugin_self );

    return CR_OK;
}
