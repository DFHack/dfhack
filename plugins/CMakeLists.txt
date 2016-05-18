INCLUDE(Plugins.cmake)

OPTION(BUILD_STONESENSE "Build stonesense (needs a checkout first)." OFF)
if(BUILD_STONESENSE)
    add_subdirectory (stonesense)
endif()

OPTION(BUILD_ISOWORLD "Build isoworld (needs a checkout first)." OFF)
if(BUILD_ISOWORLD)
    add_subdirectory (isoworld)
    IF(UNIX)
        if (APPLE)
            #TODO: add an OSX runner script
        else()
        # On linux, copy our version of the df launch script which sets LD_PRELOAD
        install(PROGRAMS ${dfhack_SOURCE_DIR}/package/linux/runisoworld
            DESTINATION .)
        endif()
    ENDIF()
endif()

OPTION(BUILD_DEV_PLUGINS "Build developer plugins." OFF)
if(BUILD_DEV_PLUGINS)
    add_subdirectory (devel)
endif()

OPTION(BUILD_RUBY "Build ruby binding." ON)
if (BUILD_RUBY)
    add_subdirectory (ruby)
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
FILE(GLOB PROJECT_PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

STRING(REPLACE ".proto" ".pb.cc" PROJECT_PROTO_SRCS "${PROJECT_PROTOS}")
STRING(REPLACE ".proto" ".pb.h" PROJECT_PROTO_HDRS "${PROJECT_PROTOS}")

ADD_CUSTOM_COMMAND(
    OUTPUT ${PROJECT_PROTO_SRCS} ${PROJECT_PROTO_HDRS}
    COMMAND protoc-bin -I=${dfhack_SOURCE_DIR}/library/proto/
                       -I=${CMAKE_CURRENT_SOURCE_DIR}/proto/
            --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto/
            ${PROJECT_PROTOS}
    DEPENDS protoc-bin ${PROJECT_PROTOS}
)
add_custom_target(generate_proto DEPENDS ${PROJECT_PROTO_SRCS} ${PROJECT_PROTO_HDRS})

SET_SOURCE_FILES_PROPERTIES( Brushes.h PROPERTIES HEADER_FILE_ONLY TRUE )

ADD_LIBRARY(buildingplan-lib STATIC buildingplan-lib.cpp)
TARGET_LINK_LIBRARIES(buildingplan-lib dfhack)

# Plugins
OPTION(BUILD_SUPPORTED "Build the supported plugins (reveal, probe, etc.)." ON)
if (BUILD_SUPPORTED)
    DFHACK_PLUGIN(3dveins 3dveins.cpp)
    DFHACK_PLUGIN(add-spatter add-spatter.cpp)
#    DFHACK_PLUGIN(advtools advtools.cpp)
    DFHACK_PLUGIN(autochop autochop.cpp)
    DFHACK_PLUGIN(autodump autodump.cpp)
    DFHACK_PLUGIN(autogems autogems.cpp LINK_LIBRARIES jsoncpp)
    DFHACK_PLUGIN(autohauler autohauler.cpp)
    DFHACK_PLUGIN(autolabor autolabor.cpp)
    DFHACK_PLUGIN(automaterial automaterial.cpp)
    DFHACK_PLUGIN(automelt automelt.cpp)
    DFHACK_PLUGIN(autotrade autotrade.cpp)
    DFHACK_PLUGIN(blueprint blueprint.cpp)
    DFHACK_PLUGIN(burrows burrows.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(building-hacks building-hacks.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(buildingplan buildingplan.cpp LINK_LIBRARIES buildingplan-lib)
    DFHACK_PLUGIN(changeitem changeitem.cpp)
    DFHACK_PLUGIN(changelayer changelayer.cpp)
    DFHACK_PLUGIN(changevein changevein.cpp)
    DFHACK_PLUGIN(cleanconst cleanconst.cpp)
    DFHACK_PLUGIN(cleaners cleaners.cpp)
    DFHACK_PLUGIN(cleanowned cleanowned.cpp)
    DFHACK_PLUGIN(command-prompt command-prompt.cpp)
    DFHACK_PLUGIN(confirm confirm.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(createitem createitem.cpp)
    DFHACK_PLUGIN(cursecheck cursecheck.cpp)
    DFHACK_PLUGIN(deramp deramp.cpp)
    DFHACK_PLUGIN(dig dig.cpp)
    DFHACK_PLUGIN(digFlood digFlood.cpp)
    add_subdirectory(diggingInvaders)
    DFHACK_PLUGIN(dwarfmonitor dwarfmonitor.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(embark-tools embark-tools.cpp)
    DFHACK_PLUGIN(eventful eventful.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(fastdwarf fastdwarf.cpp)
    DFHACK_PLUGIN(filltraffic filltraffic.cpp)
    DFHACK_PLUGIN(fix-armory fix-armory.cpp)
    DFHACK_PLUGIN(fix-unit-occupancy fix-unit-occupancy.cpp)
    DFHACK_PLUGIN(fixveins fixveins.cpp)
    DFHACK_PLUGIN(flows flows.cpp)
    DFHACK_PLUGIN(follow follow.cpp)
    DFHACK_PLUGIN(forceequip forceequip.cpp)
    DFHACK_PLUGIN(fortplan fortplan.cpp LINK_LIBRARIES buildingplan-lib)
    DFHACK_PLUGIN(getplants getplants.cpp)
    DFHACK_PLUGIN(hotkeys hotkeys.cpp)
    DFHACK_PLUGIN(infiniteSky infiniteSky.cpp)
    DFHACK_PLUGIN(isoworldremote isoworldremote.cpp PROTOBUFS isoworldremote)
    DFHACK_PLUGIN(jobutils jobutils.cpp)
    DFHACK_PLUGIN(lair lair.cpp)
    DFHACK_PLUGIN(liquids liquids.cpp Brushes.h LINK_LIBRARIES lua)
    DFHACK_PLUGIN(luasocket luasocket.cpp LINK_LIBRARIES clsocket lua dfhack-tinythread)
    DFHACK_PLUGIN(manipulator manipulator.cpp)
    DFHACK_PLUGIN(mode mode.cpp)
    #DFHACK_PLUGIN(misery misery.cpp)
    DFHACK_PLUGIN(mousequery mousequery.cpp)
    DFHACK_PLUGIN(petcapRemover petcapRemover.cpp)
    DFHACK_PLUGIN(plants plants.cpp)
    DFHACK_PLUGIN(probe probe.cpp)
    DFHACK_PLUGIN(prospector prospector.cpp)
    DFHACK_PLUGIN(power-meter power-meter.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(regrass regrass.cpp)
    DFHACK_PLUGIN(RemoteFortressReader remotefortressreader.cpp PROTOBUFS RemoteFortressReader)
    DFHACK_PLUGIN(rename rename.cpp LINK_LIBRARIES lua PROTOBUFS rename)
    add_subdirectory(rendermax)
    DFHACK_PLUGIN(resume resume.cpp)
    DFHACK_PLUGIN(reveal reveal.cpp)
    DFHACK_PLUGIN(search search.cpp)
    DFHACK_PLUGIN(seedwatch seedwatch.cpp)
    DFHACK_PLUGIN(showmood showmood.cpp)
    DFHACK_PLUGIN(siege-engine siege-engine.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(sort sort.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(steam-engine steam-engine.cpp)
    DFHACK_PLUGIN(stockflow stockflow.cpp LINK_LIBRARIES lua)
    add_subdirectory(stockpiles)
    DFHACK_PLUGIN(stocks stocks.cpp)
    DFHACK_PLUGIN(strangemood strangemood.cpp)
    DFHACK_PLUGIN(tiletypes tiletypes.cpp Brushes.h)
    DFHACK_PLUGIN(title-version title-version.cpp)
    DFHACK_PLUGIN(trackstop trackstop.cpp)
#    DFHACK_PLUGIN(treefarm treefarm.cpp)
    DFHACK_PLUGIN(tubefill tubefill.cpp)
    add_subdirectory(tweak)
    DFHACK_PLUGIN(workflow workflow.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(workNow workNow.cpp)
    DFHACK_PLUGIN(zone zone.cpp LINK_LIBRARIES lua)
endif()

# this is the skeleton plugin. If you want to make your own, make a copy and then change it
OPTION(BUILD_SKELETON "Build the skeleton plugin." OFF)
if(BUILD_SKELETON)
    add_subdirectory(skeleton)
endif()

INCLUDE(CMakeLists.custom.txt)
