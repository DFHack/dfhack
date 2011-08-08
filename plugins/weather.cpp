#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>
#include <dfhack/modules/World.h>

using std::vector;
using std::string;
using namespace DFHack;

bool locked = false;
unsigned char locked_data[25];

DFhackCExport command_result weather (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "weather";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("weather",
               "Print the weather map or change weather.\
\n              Options: 'snow' = make it snow, 'rain' = make it rain.\
\n                       'clear' = clear the sky",weather));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result weather (Core * c, vector <string> & parameters)
{
    Console & con = c->con;
    bool lock = false;
    bool unlock = false;
    bool snow = false;
    bool rain = false;
    bool clear = false;
    bool help = false;
    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "rain")
            rain = true;
        else if(parameters[i] == "snow")
            snow = true;
        else if(parameters[i] == "clear")
            clear = true;
        else if(parameters[i] == "lock")
            lock = true;
        else if(parameters[i] == "unlock")
            unlock = true;
        else if(parameters[i] == "help" || parameters[i] == "?")
            help = true;
    }
    if(help)
    {
        
    }
    if(lock && unlock)
    {
        con << "Lock or unlock? DECIDE!" << std::endl;
        return CR_FAILURE;
    }
    int cnt = 0;
    cnt += rain;
    cnt += snow;
    cnt += clear;
    if(cnt > 1)
    {
        con << "Rain, snow or clear sky? DECIDE!" << std::endl;
        return CR_FAILURE;
    }
    bool something = lock || unlock || rain || snow || clear;
    c->Suspend();
    DFHack::World * w = c->getWorld();
    if(!w->wmap)
    {
        con << "Weather support seems broken :(" << std::endl;
        c->Resume();
        return CR_FAILURE;
    }
    if(!something)
    {
        // paint weather map
        con << "Weather map (C = clear, R = rain, S = snow):" << std::endl;
        for(int y = 0; y<5;y++)
        {
            for(int x = 0; x<5;x++)
            {
                switch((*w->wmap)[x][y])
                {
                    case DFHack::CLEAR:
                        con << "C ";
                        break;
                    case DFHack::RAINING:
                        con << "R ";
                        break;
                    case DFHack::SNOWING:
                        con << "S ";
                        break;
                    default:
                        con << (int) (*w->wmap)[x][y] << " ";
                        break;
                }
            }
            con << std::endl;
        }
    }
    else
    {
        // weather changing action!
        if(rain)
        {
            con << "Here comes the rain." << std::endl;
            w->SetCurrentWeather(RAINING);
        }
        if(snow)
        {
            con << "Snow everywhere!" << std::endl;
            w->SetCurrentWeather(SNOWING);
        }
        if(clear)
        {
            con << "Suddenly, sunny weather!" << std::endl;
            w->SetCurrentWeather(CLEAR);
        }
        // FIXME: weather lock needs map ID to work reliably... needs to be implemented.
    }
    c->Resume();
    return CR_OK;
}
