/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// You can always find the latest version of this plugin in Github
// https://github.com/ragundo/exportmaps

#include <algorithm>
#include "../include/Mac_compat.h"
#include "../include/ExportMaps.h"
#include "../include/Logger.h"
#include "modules/Filesystem.h"

using namespace std;
using namespace DFHack;

using df::global::world;

DFHACK_PLUGIN("exportmaps");
DFHACK_PLUGIN_IS_ENABLED(s_enabled);

using namespace exportmaps_plugin;

/*****************************************************************************
Local function definitions
*****************************************************************************/
std::tuple<unsigned int,
           unsigned int,
           unsigned int,
           std::vector<int>
          >process_command_line(std::vector <std::string>& options);

/*****************************************************************************
Plugin global variables
*****************************************************************************/
MapsExporter maps_exporter;


/*****************************************************************************
Here go all the command declarations...
Mostly to allow having the mandatory stuff on top of the file and commands on
the bottom
*****************************************************************************/


//----------------------------------------------------------------------------//
// [Optional] Called when the user enables or disables out plugin
//----------------------------------------------------------------------------//
DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    s_enabled = enable;

    return CR_OK;
}

//----------------------------------------------------------------------------//
// This is called right before the plugin library is removed from memory.
//----------------------------------------------------------------------------//
DFhackCExport command_result plugin_shutdown (color_ostream& con)
{
    return CR_OK;
}

//----------------------------------------------------------------------------//
DFhackCExport command_result plugin_onupdate ( color_ostream& out )
{
    return CR_OK;
}


/*****************************************************************************
Plugin main function
*****************************************************************************/
DFhackCExport command_result exportmaps (color_ostream& con,                   // DFHack console
                                         std::vector <std::string>& parameters // Parameters received by the console
                                        )
{
  // Init the loger object
  Logger logger(con);

  // Process command line arguments
  std::tuple<unsigned int,
             unsigned int,
             unsigned int,
             std::vector<int>
            > command_line;

  // tuple.first is a uint where each bit ON means a graphical map type to be generated
  // tuple.second is a uint ehere each bit ON means a raw map type to be generated
  // tuple.third is a uint ehere each bit ON means a heightmap type to be generated
  // tuple.fourth is a vector with indexes to wrong arguments in the command line
  command_line = process_command_line(parameters);

  // Alias to the vector of index to wrong options
  std::vector<int>& unknown_options = std::get<3>(command_line);

  // Warn the user about unknown options
  for (unsigned int i = 0; i < unknown_options.size(); ++i)
    if (unknown_options[i] != -1)
      con << "ERROR: unknown command line option: " << parameters[unknown_options[i]] << std::endl;

  // No data can be generated if a world is not loaded, so check it
  if (df::global::world->world_data == nullptr)
  {
    con << "ERROR: no world loaded" << std::endl;
    return CR_OK;
  }

  // Choose what maps to export
  maps_exporter.setup_maps(std::get<0>(command_line), // Graphical maps
                           std::get<1>(command_line), // Raw maps
                           std::get<2>(command_line)  // Height maps
                           );

  // Begin generating data for the threads(consumers) and use the logger
  // object for updating the progress to the DFHack console
  maps_exporter.generate_maps(logger);

  // Done
  return CR_OK;
}

/*****************************************************************************
[Mandatory] init function. If you have some global state, create it here.
*****************************************************************************/
DFhackCExport command_result plugin_init (color_ostream& con,                   // DFHack console
                                          std::vector <PluginCommand>& commands // Parameters received by the console
                                          )
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand("exportmaps",                                                                        // Plugin name
                                     "Export world maps in different formats to disk in Fortress/Adventure/Legends Mode", // Plugin brief description
                                     exportmaps                                                                           // Subroutine to be executed when the plugin is called
                                     /*,
                                     true or false - true means that the command can't be used from non-interactive user interface'*/
                                     ));
    return CR_OK;
}

/*****************************************************************************************
Process the command line arguments
returns a tuple, where
 tuple.first is a uint bit each bit meaning a graphical map type to be generated
 tuple.second is a uint bit each bit meaning a raw map type to be generated
 tuple.third is a vector with index to wrong arguments

*****************************************************************************************/
std::tuple<unsigned int, unsigned int, unsigned int, std::vector<int> >
process_command_line(std::vector <std::string>& options)
{
  unsigned int     maps_to_generate     = 0; // Graphical maps to generate
  unsigned int     maps_to_generate_raw = 0; // Raw maps to generate
  unsigned int     maps_to_generate_hm  = 0; // Heightmaps to generate
  std::vector<int> errors(options.size());   // Vector with index to wrong command line options

  // Iterate over all the command line options received
  for ( unsigned int argv_iterator = 0; argv_iterator < options.size(); ++argv_iterator)
  {
    // Get a command line option
    std::string option = options[argv_iterator];

    // Convert to lowercase
    std::transform(option.begin(), option.end(), option.begin(), ::tolower);

    // No error in argument until now
    errors[argv_iterator] = -1;

    // Check command line
    if (option == "-all-df")                                         // All DF maps
      return std::tuple<unsigned int,
                        unsigned int,
                        unsigned int,
                        std::vector<int>
                       >(    -1, // All graphical maps
                              0, // All raw maps
                              0, // All height maps
                         errors
                         );

    if (option == "-all-raw")                                       // All raws maps
      return std::tuple<unsigned int,
                        unsigned int,
                        unsigned int,
                        std::vector<int>
                       >(     0, // All graphical maps
                             -1, // All raw maps
                              0, // All height maps
                         errors
                         );

    if (option == "-all-hm")                                       // All raws maps
      return std::tuple<unsigned int,
                        unsigned int,
                        unsigned int,
                        std::vector<int>
                       >(     0, // All graphical maps
                              0, // All raw maps
                             -1, // All height maps
                         errors
                         );

    if (option == "-temperature")                                  // map DF style
      maps_to_generate |= MapType::TEMPERATURE;

    else if (option == "-rainfall")                                // map DF style
      maps_to_generate |= MapType::RAINFALL;

    else if (option == "-region")                                  // map DF style
      maps_to_generate |= MapType::REGION;

    else if (option == "-drainage")                                // map DF style
      maps_to_generate |= MapType::DRAINAGE;

    else if (option == "-savagery")                                // map DF style
      maps_to_generate |= MapType::SAVAGERY;

    else if (option == "-volcanism")                               // map DF style
      maps_to_generate |= MapType::VOLCANISM;

    else if (option == "-vegetation")                              // map DF style
      maps_to_generate |= MapType::VEGETATION;

    else if (option == "-evilness")                                // map DF style
      maps_to_generate |= MapType::EVILNESS;

    else if (option == "-salinity")                                // map DF style
      maps_to_generate |= MapType::SALINITY;

    else if (option == "-hydrosphere")                             // map DF style
      maps_to_generate |= MapType::HYDROSPHERE;

    else if (option == "-elevation")                               // map DF style
      maps_to_generate |= MapType::ELEVATION;

    else if (option == "-elevation-water")                         // map DF style
      maps_to_generate |= MapType::ELEVATION_WATER;

    else if (option == "-biome")                                   // map DF style
      maps_to_generate |= MapType::BIOME;

    else if (option == "-trading")                                 // map DF style
      maps_to_generate |= MapType::TRADING;

    else if (option == "-nobility")                                // map DF style
      maps_to_generate |= MapType::NOBILITY;

    else if (option == "-diplomacy")                               // map DF style
      maps_to_generate |= MapType::DIPLOMACY;

    else if (option == "-sites")                                   // map DF style
      maps_to_generate |= MapType::SITES;

    // Raw maps

    else if (option == "-temperature-raw")                         // map raw data file
      maps_to_generate_raw |= MapTypeRaw::TEMPERATURE_RAW;

    else if (option == "-rainfall-raw")                            // map raw data file
      maps_to_generate_raw |= MapTypeRaw::RAINFALL_RAW;

    else if (option == "-drainage-raw")                            // map raw data file
      maps_to_generate_raw |= MapTypeRaw::DRAINAGE_RAW;

    else if (option == "-savagery-raw")                            // map raw data file
      maps_to_generate_raw |= MapTypeRaw::SAVAGERY_RAW;

    else if (option == "-volcanism-raw")                           // map raw data file
      maps_to_generate_raw |= MapTypeRaw::VOLCANISM_RAW;

    else if (option == "-vegetation-raw")                          // map raw data file
      maps_to_generate_raw |= MapTypeRaw::VEGETATION_RAW;

    else if (option == "-evilness-raw")                            // map raw data file
      maps_to_generate_raw |= MapTypeRaw::EVILNESS_RAW;

    else if (option == "-salinity-raw")                            // map raw data file
      maps_to_generate_raw |= MapTypeRaw::SALINITY_RAW;

    else if (option == "-hydrosphere-raw")                         // map raw data file
      maps_to_generate_raw |= MapTypeRaw::HYDROSPHERE_RAW;

    else if (option == "-elevation-raw")                           // map raw data file
      maps_to_generate_raw |= MapTypeRaw::ELEVATION_RAW;

    else if (option == "-elevation-water-raw")                     // map raw data file
      maps_to_generate_raw |= MapTypeRaw::ELEVATION_WATER_RAW;

    else if (option == "-biome-raw")                               // map raw data file
    {
      maps_to_generate_raw |= MapTypeRaw::BIOME_TYPE_RAW;
      maps_to_generate_raw |= MapTypeRaw::BIOME_REGION_RAW;
    }
    else if (option == "-trading-raw")                             // map raw data file
      maps_to_generate_raw |= MapTypeRaw::TRADING_RAW;

    else if (option == "-nobility-raw")                            // map raw data file
      maps_to_generate_raw |= MapTypeRaw::NOBILITY_RAW;

    else if (option == "-diplomacy-raw")                           // map raw data file
      maps_to_generate_raw |= MapTypeRaw::DIPLOMACY_RAW;

    else if (option == "-sites-raw")                               // map raw data file
      maps_to_generate_raw |= MapTypeRaw::SITES_RAW;

    // Heightmaps

    else if (option == "-elevation-hm")                            // Heighmap style
      maps_to_generate_hm |= MapTypeHeightMap::ELEVATION_HM;

    else if (option == "-elevation-water-hm")                      // Heightmap style
      maps_to_generate_hm |= MapTypeHeightMap::ELEVATION_WATER_HM;

    else                                                           // ERROR - unknown argument
      errors[argv_iterator] = argv_iterator;
  }
  return std::tuple<unsigned int,
                    unsigned int,
                    unsigned int,
                    std::vector<int>
                   >(maps_to_generate,
                     maps_to_generate_raw,
                     maps_to_generate_hm,
                     errors
                     );
}
