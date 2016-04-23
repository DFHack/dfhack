PROJECT (ExportMaps)
# A list of source files
SET(PROJECT_SRCS
  # IDA functions
  ./cpp/util/ofsub.cpp

  # DF utility functions
  ./cpp/df_utils/adjust_coordinates_to_region.cpp
  ./cpp/df_utils/biome_type.cpp
  ./cpp/df_utils/df_binary_searches.cpp

  # Calls to native DF functions
  ./cpp/df_utils/fill_world_region_details.cpp
  ./cpp/df_utils/delete_world_region_details.cpp
  ./cpp/df_utils/delete_world_region_details_vector.cpp
  ./cpp/df_utils/init_world_site_realization.cpp
  ./cpp/df_utils/delete_world_site_realization.cpp

  # Consumers for each different DF map
  ./cpp/consumers/DF/biome_consumer.cpp
  ./cpp/consumers/DF/drainage_consumer.cpp
  ./cpp/consumers/DF/elevation_consumer.cpp
  ./cpp/consumers/DF/elevation_water_consumer.cpp
  ./cpp/consumers/DF/evilness_consumer.cpp
  ./cpp/consumers/DF/geology_consumer.cpp
  ./cpp/consumers/DF/hydro_consumer.cpp
  ./cpp/consumers/DF/rainfall_consumer.cpp
  ./cpp/consumers/DF/region_consumer.cpp
  ./cpp/consumers/DF/salinity_consumer.cpp
  ./cpp/consumers/DF/savagery_consumer.cpp
  ./cpp/consumers/DF/vegetation_consumer.cpp
  ./cpp/consumers/DF/volcanism_consumer.cpp
  ./cpp/consumers/DF/temperature_consumer.cpp
  ./cpp/consumers/DF/trade_consumer.cpp
  ./cpp/consumers/DF/nobility_consumer.cpp
  ./cpp/consumers/DF/diplomacy_consumer.cpp
  ./cpp/consumers/DF/sites_consumer.cpp

  # Consumers for each different raw map
  ./cpp/consumers/RAW/biome_type_raw_consumer.cpp
  ./cpp/consumers/RAW/biome_region_raw_consumer.cpp
  ./cpp/consumers/RAW/drainage_raw_consumer.cpp
  ./cpp/consumers/RAW/elevation_raw_consumer.cpp
  ./cpp/consumers/RAW/elevation_water_raw_consumer.cpp
  ./cpp/consumers/RAW/evilness_raw_consumer.cpp
  ./cpp/consumers/RAW/hydro_raw_consumer.cpp
  ./cpp/consumers/RAW/rainfall_raw_consumer.cpp
  ./cpp/consumers/RAW/salinity_raw_consumer.cpp
  ./cpp/consumers/RAW/savagery_raw_consumer.cpp
  ./cpp/consumers/RAW/temperature_raw_consumer.cpp
  ./cpp/consumers/RAW/vegetation_raw_consumer.cpp
  ./cpp/consumers/RAW/volcanism_raw_consumer.cpp

  # Consumers for heightmaps
  ./cpp/consumers/heightmaps/elevation_heightmap_consumer.cpp
  ./cpp/consumers/heightmaps/elevation_water_heightmap_consumer.cpp


  # Plugin code
  ./cpp/draw_line.cpp
  ./cpp/Logger.cpp
  ./cpp/Producer.cpp
  ./cpp/ExportedMap.cpp
  ./cpp/MapsExporter.cpp
  ./cpp/MapsExporter_queues_state.cpp
  ./cpp/MapsExporter_push_pop.cpp
  ./cpp/MapsExporter_setup_maps.cpp
  ./cpp/MapsExporter_threads.cpp

  # JSON support
  #./cpp/util/jsonxx.cpp

  # PNG support
  ./cpp/util/lodepng.cpp

  # Plugin interface to DFHack
  ./cpp/exportmaps.cpp
)
# A list of headers
SET(PROJECT_HDRS
    ./include/dfhack.h
)
SET_SOURCE_FILES_PROPERTIES( ${PROJECT_HDRS} PROPERTIES HEADER_FILE_ONLY TRUE)

# mash them together (headers are marked as headers and nothing will try to compile them)
LIST(APPEND PROJECT_SRCS ${PROJECT_HDRS})

#linux
IF(UNIX)
    add_definitions(-DLINUX_BUILD)
    SET(PROJECT_LIBS
        # add any extra linux libs here
        dfhack-tinythread
        ${PROJECT_LIBS}
    )
# windows
ELSE(UNIX)
    SET(PROJECT_LIBS
        # add any extra windows libs here
        dfhack-tinythread
        ${PROJECT_LIBS}
        $(NOINHERIT)
    )
ENDIF(UNIX)
# this makes sure all the stuff is put in proper places and linked to dfhack
DFHACK_PLUGIN(exportmaps ${PROJECT_SRCS} LINK_LIBRARIES ${PROJECT_LIBS} COMPILE_FLAGS_GCC "-O0")
