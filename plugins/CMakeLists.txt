include(Plugins.cmake)

option(BUILD_STONESENSE "Build stonesense (needs a checkout first)." OFF)
if(BUILD_STONESENSE)
    add_subdirectory(stonesense)
endif()

option(BUILD_ISOWORLD "Build isoworld (needs a checkout first)." OFF)
if(BUILD_ISOWORLD)
    #add_subdirectory(isoworld)
    if(UNIX)
        if(APPLE)
            # TODO: add an OSX runner script
        else()
        # On linux, copy our version of the df launch script which sets LD_PRELOAD
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/linux/runisoworld
            DESTINATION .)
        endif()
    endif()
endif()

option(BUILD_DEV_PLUGINS "Build developer plugins." OFF)
if(BUILD_DEV_PLUGINS)
    #add_subdirectory(devel)
endif()

install(DIRECTORY lua/
    DESTINATION ${DFHACK_LUA_DESTINATION}/plugins
    FILES_MATCHING PATTERN "*.lua")
install(DIRECTORY raw/
    DESTINATION ${DFHACK_DATA_DESTINATION}/raw
    FILES_MATCHING PATTERN "*.txt")
install(DIRECTORY raw/
    DESTINATION ${DFHACK_DATA_DESTINATION}/raw
    FILES_MATCHING PATTERN "*.diff")

# Protobuf
file(GLOB PROJECT_PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

string(REPLACE ".proto" ".pb.cc" PROJECT_PROTO_SRCS "${PROJECT_PROTOS}")
string(REPLACE ".proto" ".pb.h" PROJECT_PROTO_HDRS "${PROJECT_PROTOS}")
string(REPLACE "/proto/" "/proto/tmp/" PROJECT_PROTO_TMP_FILES "${PROJECT_PROTO_SRCS};${PROJECT_PROTO_HDRS}")
set_source_files_properties(${PROJECT_PROTO_SRCS} ${PROJECT_PROTO_HDRS}
    PROPERTIES GENERATED TRUE)

# Force a re-gen if any *.pb.* files are missing
# (only runs when cmake is run, but better than nothing)
foreach(file IN LISTS PROJECT_PROTO_SRCS PROJECT_PROTO_HDRS)
    if(NOT EXISTS ${file})
        # message("Resetting generate_proto because '${file}' is missing")
        file(REMOVE ${PROJECT_PROTO_TMP_FILES})
        break()
    endif()
endforeach()

add_custom_command(
    OUTPUT ${PROJECT_PROTO_TMP_FILES}
    COMMAND protoc-bin -I=${dfhack_SOURCE_DIR}/library/proto/
                       -I=${CMAKE_CURRENT_SOURCE_DIR}/proto/
            --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto/tmp/
            ${PROJECT_PROTOS}
    COMMAND ${PERL_EXECUTABLE} ${dfhack_SOURCE_DIR}/depends/copy-if-different.pl
            ${PROJECT_PROTO_TMP_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/proto/
    COMMENT "Generating plugin protobufs"
    DEPENDS protoc-bin ${PROJECT_PROTOS}
)
add_custom_target(generate_proto DEPENDS ${PROJECT_PROTO_TMP_FILES})

set_source_files_properties( Brushes.h PROPERTIES HEADER_FILE_ONLY TRUE )

# Plugins
# If you are adding a plugin that you do not intend to commit to the DFHack repo,
# see instructions for adding "external" plugins at the end of this file.

#dfhack_plugin(3dveins 3dveins.cpp)
#dfhack_plugin(add-spatter add-spatter.cpp)
dfhack_plugin(autobutcher autobutcher.cpp LINK_LIBRARIES lua)
dfhack_plugin(autochop autochop.cpp LINK_LIBRARIES lua)
dfhack_plugin(autoclothing autoclothing.cpp)
dfhack_plugin(autodump autodump.cpp)
dfhack_plugin(autofarm autofarm.cpp)
#dfhack_plugin(autogems autogems.cpp LINK_LIBRARIES jsoncpp_static)
#add_subdirectory(autolabor)
#dfhack_plugin(automaterial automaterial.cpp LINK_LIBRARIES lua)
dfhack_plugin(automelt automelt.cpp LINK_LIBRARIES lua)
#dfhack_plugin(autonestbox autonestbox.cpp LINK_LIBRARIES lua)
#dfhack_plugin(autotrade autotrade.cpp)
dfhack_plugin(blueprint blueprint.cpp LINK_LIBRARIES lua)
#dfhack_plugin(burrows burrows.cpp LINK_LIBRARIES lua)
#dfhack_plugin(building-hacks building-hacks.cpp LINK_LIBRARIES lua)
dfhack_plugin(buildingplan buildingplan.cpp LINK_LIBRARIES lua)
#dfhack_plugin(changeitem changeitem.cpp)
dfhack_plugin(changelayer changelayer.cpp)
dfhack_plugin(changevein changevein.cpp)
add_subdirectory(channel-safely)
dfhack_plugin(cleanconst cleanconst.cpp)
dfhack_plugin(cleaners cleaners.cpp)
dfhack_plugin(cleanowned cleanowned.cpp)
#dfhack_plugin(confirm confirm.cpp LINK_LIBRARIES lua)
#dfhack_plugin(createitem createitem.cpp)
dfhack_plugin(cursecheck cursecheck.cpp)
#dfhack_plugin(cxxrandom cxxrandom.cpp LINK_LIBRARIES lua)
#dfhack_plugin(deramp deramp.cpp)
dfhack_plugin(debug debug.cpp LINK_LIBRARIES jsoncpp_static)
#dfhack_plugin(dig dig.cpp)
dfhack_plugin(dig-now dig-now.cpp LINK_LIBRARIES lua)
#dfhack_plugin(digFlood digFlood.cpp)
#add_subdirectory(diggingInvaders)
#dfhack_plugin(dwarfvet dwarfvet.cpp)
#dfhack_plugin(dwarfmonitor dwarfmonitor.cpp LINK_LIBRARIES lua)
#add_subdirectory(embark-assistant)
#dfhack_plugin(embark-tools embark-tools.cpp)
dfhack_plugin(eventful eventful.cpp LINK_LIBRARIES lua)
dfhack_plugin(fastdwarf fastdwarf.cpp)
dfhack_plugin(filltraffic filltraffic.cpp)
#dfhack_plugin(fix-unit-occupancy fix-unit-occupancy.cpp)
#dfhack_plugin(fixveins fixveins.cpp)
#dfhack_plugin(flows flows.cpp)
#dfhack_plugin(follow follow.cpp)
#dfhack_plugin(forceequip forceequip.cpp)
#dfhack_plugin(generated-creature-renamer generated-creature-renamer.cpp)
#dfhack_plugin(getplants getplants.cpp)
dfhack_plugin(hotkeys hotkeys.cpp LINK_LIBRARIES lua)
#dfhack_plugin(infiniteSky infiniteSky.cpp)
#dfhack_plugin(isoworldremote isoworldremote.cpp PROTOBUFS isoworldremote)
#dfhack_plugin(jobutils jobutils.cpp)
#dfhack_plugin(lair lair.cpp)
dfhack_plugin(liquids liquids.cpp Brushes.h LINK_LIBRARIES lua)
#dfhack_plugin(luasocket luasocket.cpp LINK_LIBRARIES clsocket lua dfhack-tinythread)
#dfhack_plugin(manipulator manipulator.cpp)
#dfhack_plugin(map-render map-render.cpp LINK_LIBRARIES lua)
dfhack_plugin(misery misery.cpp)
#dfhack_plugin(mode mode.cpp)
#dfhack_plugin(mousequery mousequery.cpp)
dfhack_plugin(nestboxes nestboxes.cpp)
dfhack_plugin(autoslab autoslab.cpp)
dfhack_plugin(orders orders.cpp LINK_LIBRARIES jsoncpp_static lua)
dfhack_plugin(overlay overlay.cpp LINK_LIBRARIES lua)
dfhack_plugin(pathable pathable.cpp LINK_LIBRARIES lua)
#dfhack_plugin(petcapRemover petcapRemover.cpp)
#dfhack_plugin(plants plants.cpp)
dfhack_plugin(probe probe.cpp)
#dfhack_plugin(prospector prospector.cpp LINK_LIBRARIES lua)
#dfhack_plugin(power-meter power-meter.cpp LINK_LIBRARIES lua)
dfhack_plugin(regrass regrass.cpp)
add_subdirectory(remotefortressreader)
#dfhack_plugin(rename rename.cpp LINK_LIBRARIES lua PROTOBUFS rename)
#add_subdirectory(rendermax)
dfhack_plugin(reveal reveal.cpp LINK_LIBRARIES lua)
#dfhack_plugin(search search.cpp)
dfhack_plugin(seedwatch seedwatch.cpp)
dfhack_plugin(showmood showmood.cpp)
#dfhack_plugin(siege-engine siege-engine.cpp LINK_LIBRARIES lua)
#dfhack_plugin(sort sort.cpp LINK_LIBRARIES lua)
#dfhack_plugin(steam-engine steam-engine.cpp)
#add_subdirectory(spectate)
#dfhack_plugin(stockflow stockflow.cpp LINK_LIBRARIES lua)
#add_subdirectory(stockpiles)
#dfhack_plugin(stocks stocks.cpp)
#dfhack_plugin(strangemood strangemood.cpp)
dfhack_plugin(tailor tailor.cpp)
dfhack_plugin(tiletypes tiletypes.cpp Brushes.h LINK_LIBRARIES lua)
#dfhack_plugin(title-folder title-folder.cpp)
#dfhack_plugin(title-version title-version.cpp)
#dfhack_plugin(trackstop trackstop.cpp)
#dfhack_plugin(tubefill tubefill.cpp)
#add_subdirectory(tweak)
#dfhack_plugin(workflow workflow.cpp LINK_LIBRARIES lua)
#dfhack_plugin(workNow workNow.cpp)
dfhack_plugin(xlsxreader xlsxreader.cpp LINK_LIBRARIES lua xlsxio_read_STATIC zip expat)
#dfhack_plugin(zone zone.cpp)

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
