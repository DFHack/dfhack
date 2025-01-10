include(Plugins.cmake)

option(BUILD_STONESENSE "Build stonesense (needs a checkout first)." OFF)
if(BUILD_STONESENSE)
    add_subdirectory(stonesense)
endif()

option(BUILD_DEV_PLUGINS "Build developer plugins." OFF)
if(BUILD_DEV_PLUGINS)
    add_subdirectory(devel)
endif()

if(INSTALL_DATA_FILES)
    install(DIRECTORY lua/
        DESTINATION ${DFHACK_LUA_DESTINATION}/plugins
        FILES_MATCHING PATTERN "*.lua")
    install(DIRECTORY raw/
        DESTINATION ${DFHACK_DATA_DESTINATION}/raw
        FILES_MATCHING PATTERN "*.txt")
    install(DIRECTORY raw/
        DESTINATION ${DFHACK_DATA_DESTINATION}/raw
        FILES_MATCHING PATTERN "*.diff")
endif()

set_source_files_properties( Brushes.h PROPERTIES HEADER_FILE_ONLY TRUE )

# Plugins
# If you are adding a plugin that you do not intend to commit to the DFHack repo,
# see instructions for adding "external" plugins at the end of this file.

# Example plugin that uses protobufs
# proto file must be in the proto/ folder
# dfhack_plugin(rename rename.cpp LINK_LIBRARIES lua PROTOBUFS rename)

option(BUILD_SUPPORTED "Build the supported plugins (reveal, probe, etc.)." ON)
if(BUILD_SUPPORTED)
    dfhack_plugin(3dveins 3dveins.cpp)
    dfhack_plugin(add-spatter add-spatter.cpp)
    dfhack_plugin(aquifer aquifer.cpp LINK_LIBRARIES lua)
    dfhack_plugin(autobutcher autobutcher.cpp LINK_LIBRARIES lua)
    dfhack_plugin(autochop autochop.cpp LINK_LIBRARIES lua)
    dfhack_plugin(autoclothing autoclothing.cpp LINK_LIBRARIES lua)
    dfhack_plugin(design design.cpp LINK_LIBRARIES lua)
    dfhack_plugin(autodump autodump.cpp)
    dfhack_plugin(autofarm autofarm.cpp)
    #dfhack_plugin(autogems autogems.cpp LINK_LIBRARIES jsoncpp_static)
    add_subdirectory(autolabor)
    dfhack_plugin(autonestbox autonestbox.cpp LINK_LIBRARIES lua)
    dfhack_plugin(autoslab autoslab.cpp)
    dfhack_plugin(blueprint blueprint.cpp LINK_LIBRARIES lua)
    dfhack_plugin(burrow burrow.cpp LINK_LIBRARIES lua)
    #dfhack_plugin(building-hacks building-hacks.cpp LINK_LIBRARIES lua)
    add_subdirectory(buildingplan)
    dfhack_plugin(changeitem changeitem.cpp)
    dfhack_plugin(changelayer changelayer.cpp)
    dfhack_plugin(changevein changevein.cpp)
    #add_subdirectory(channel-safely)
    dfhack_plugin(cleanconst cleanconst.cpp)
    dfhack_plugin(cleaners cleaners.cpp LINK_LIBRARIES lua)
    dfhack_plugin(cleanowned cleanowned.cpp)
    dfhack_plugin(createitem createitem.cpp)
    dfhack_plugin(cursecheck cursecheck.cpp)
    dfhack_plugin(cxxrandom cxxrandom.cpp LINK_LIBRARIES lua)
    dfhack_plugin(deramp deramp.cpp)
    dfhack_plugin(debug debug.cpp LINK_LIBRARIES jsoncpp_static)
    dfhack_plugin(dig dig.cpp LINK_LIBRARIES lua)
    dfhack_plugin(dig-now dig-now.cpp LINK_LIBRARIES lua)
    #dfhack_plugin(digFlood digFlood.cpp)
    #add_subdirectory(diggingInvaders)
    dfhack_plugin(dwarfvet dwarfvet.cpp LINK_LIBRARIES lua)
    #dfhack_plugin(dwarfmonitor dwarfmonitor.cpp LINK_LIBRARIES lua)
    #add_subdirectory(embark-assistant)
    dfhack_plugin(eventful eventful.cpp LINK_LIBRARIES lua)
    dfhack_plugin(fastdwarf fastdwarf.cpp)
    dfhack_plugin(filltraffic filltraffic.cpp)
    dfhack_plugin(fix-occupancy fix-occupancy.cpp LINK_LIBRARIES lua)
    #dfhack_plugin(fixveins fixveins.cpp)
    dfhack_plugin(flows flows.cpp)
    #dfhack_plugin(follow follow.cpp)
    dfhack_plugin(forceequip forceequip.cpp)
    #dfhack_plugin(generated-creature-renamer generated-creature-renamer.cpp)
    dfhack_plugin(getplants getplants.cpp)
    dfhack_plugin(hotkeys hotkeys.cpp LINK_LIBRARIES lua)
    dfhack_plugin(infinite-sky infinite-sky.cpp LINK_LIBRARIES lua)
    #dfhack_plugin(jobutils jobutils.cpp)
    dfhack_plugin(lair lair.cpp)
    dfhack_plugin(liquids liquids.cpp Brushes.h LINK_LIBRARIES lua)
    dfhack_plugin(luasocket luasocket.cpp LINK_LIBRARIES clsocket lua)
    dfhack_plugin(logistics logistics.cpp LINK_LIBRARIES lua)
    #dfhack_plugin(manipulator manipulator.cpp)
    #dfhack_plugin(map-render map-render.cpp LINK_LIBRARIES lua)
    dfhack_plugin(misery misery.cpp LINK_LIBRARIES lua)
    #dfhack_plugin(mode mode.cpp)
    dfhack_plugin(nestboxes nestboxes.cpp)
    dfhack_plugin(orders orders.cpp LINK_LIBRARIES jsoncpp_static lua)
    dfhack_plugin(overlay overlay.cpp LINK_LIBRARIES lua)
    dfhack_plugin(pathable pathable.cpp LINK_LIBRARIES lua)
    dfhack_plugin(pet-uncapper pet-uncapper.cpp)
    dfhack_plugin(plant plant.cpp LINK_LIBRARIES lua)
    dfhack_plugin(preserve-rooms preserve-rooms.cpp LINK_LIBRARIES lua)
    dfhack_plugin(preserve-tombs preserve-tombs.cpp)
    dfhack_plugin(probe probe.cpp LINK_LIBRARIES lua)
    dfhack_plugin(prospector prospector.cpp LINK_LIBRARIES lua)
    #dfhack_plugin(power-meter power-meter.cpp LINK_LIBRARIES lua)
    dfhack_plugin(regrass regrass.cpp LINK_LIBRARIES lua)
    add_subdirectory(remotefortressreader)
    #add_subdirectory(rendermax)
    dfhack_plugin(reveal reveal.cpp LINK_LIBRARIES lua)
    dfhack_plugin(seedwatch seedwatch.cpp LINK_LIBRARIES lua)
    dfhack_plugin(showmood showmood.cpp)
    #dfhack_plugin(siege-engine siege-engine.cpp LINK_LIBRARIES lua)
    dfhack_plugin(sort sort.cpp LINK_LIBRARIES lua)
    #dfhack_plugin(steam-engine steam-engine.cpp)
    add_subdirectory(spectate)
    #dfhack_plugin(stockflow stockflow.cpp LINK_LIBRARIES lua)
    add_subdirectory(stockpiles)
    dfhack_plugin(stocks stocks.cpp LINK_LIBRARIES lua)
    dfhack_plugin(strangemood strangemood.cpp)
    dfhack_plugin(suspendmanager suspendmanager.cpp COMPILE_FLAGS_GCC -fno-gnu-unique LINK_LIBRARIES lua)
    dfhack_plugin(tailor tailor.cpp LINK_LIBRARIES lua)
    dfhack_plugin(tiletypes tiletypes.cpp Brushes.h LINK_LIBRARIES lua)
    dfhack_plugin(timestream timestream.cpp LINK_LIBRARIES lua)
    #dfhack_plugin(title-folder title-folder.cpp)
    dfhack_plugin(tubefill tubefill.cpp)
    add_subdirectory(tweak)
    #dfhack_plugin(workflow workflow.cpp LINK_LIBRARIES lua)
    dfhack_plugin(work-now work-now.cpp)
    dfhack_plugin(xlsxreader xlsxreader.cpp LINK_LIBRARIES lua xlsxio_read_STATIC zip expat)
    dfhack_plugin(zone zone.cpp LINK_LIBRARIES lua)
endif(BUILD_SUPPORTED)

# If you are adding a plugin that you do not intend to commit to the DFHack repo,
# see instructions for adding "external" plugins at the end of this file.

# this is the skeleton plugin. If you want to make your own, make a copy and then change it
option(BUILD_SKELETON "Build the skeleton plugin." OFF)
if(BUILD_SKELETON)
    dfhack_plugin(skeleton examples/skeleton.cpp)
endif()

macro(subdirlist result subdir)
    file(GLOB children ABSOLUTE ${subdir}/ ${subdir}/*/)
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${child})
            file(RELATIVE_PATH child ${CMAKE_CURRENT_SOURCE_DIR}/${subdir} ${child})
            list(APPEND dirlist ${child})
        endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()

# To add "external" plugins without committing them to the DFHack repo:
#
# 1. run CMake as you normally do (this is only necessary once if
#    `external/CMakeLists.txt` does not exist yet)
# 2. add the plugin to the `external` folder (relative to this file).
#    - for a multi-file plugin, either clone the repository inside of the
#      `external` folder, or add the folder there manually.
#    - for a single-file plugin, simply add the file there.
# 3. add an entry to `external/CMakeLists.txt`:
#    - add_subdirectory() for multi-file plugins in a subdirectory
#    - dfhack_plugin() for single-file plugins
# 4. build DFHack as normal. The plugins you added will be built as well.

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/CMakeLists.txt")
    set(content_str
"# Add external plugins here - this file is ignored by git

# Recommended: use add_subdirectory() for folders that you have created within
# this folder, or dfhack_plugin() for single files that you have added here.

# See the end of /plugins/CMakeLists.txt for more details.
")
    subdirlist(SUBDIRS external)
    foreach(subdir ${SUBDIRS})
        set(content_str "${content_str}add_subdirectory(${subdir})\n")
    endforeach()
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/external/CMakeLists.txt" ${content_str})
endif()

add_subdirectory(external)

# for backwards compatibility
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.custom.txt")
    include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.custom.txt")
endif()
