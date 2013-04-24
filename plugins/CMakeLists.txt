INCLUDE(Plugins.cmake)

# Dfusion plugin
OPTION(BUILD_DFUSION "Build DFusion." ON)
if(BUILD_DFUSION)
    add_subdirectory (Dfusion)
endif()

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

#It's dead :<
#OPTION(BUILD_DF2MC "Build DF2MC (needs a checkout first)." OFF)
#if(BUILD_DF2MC)
#    add_subdirectory (df2mc)
#endif()

OPTION(BUILD_MAPEXPORT "Build map exporter." ON)
if (BUILD_MAPEXPORT)
    add_subdirectory (mapexport)
endif()

OPTION(BUILD_DWARFEXPORT "Build dwarf exporter." ON)
if (BUILD_DWARFEXPORT)
    add_subdirectory (dwarfexport)
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

# Plugins
OPTION(BUILD_SUPPORTED "Build the supported plugins (reveal, probe, etc.)." ON)
if (BUILD_SUPPORTED)
    DFHACK_PLUGIN(reveal reveal.cpp)
    DFHACK_PLUGIN(probe probe.cpp)
    # this is a plugin which helps detect cursed creatures (vampires, necromancers, werebeasts, ...)
    DFHACK_PLUGIN(cursecheck cursecheck.cpp)
    # automatically assign labors to dwarves!
    DFHACK_PLUGIN(autolabor autolabor.cpp)
    DFHACK_PLUGIN(dig dig.cpp)
    DFHACK_PLUGIN(drybuckets drybuckets.cpp)
    DFHACK_PLUGIN(getplants getplants.cpp)
    DFHACK_PLUGIN(plants plants.cpp)
    DFHACK_PLUGIN(fastdwarf fastdwarf.cpp)
    DFHACK_PLUGIN(prospector prospector.cpp)
    DFHACK_PLUGIN(cleaners cleaners.cpp)
    DFHACK_PLUGIN(weather weather.cpp)
    DFHACK_PLUGIN(colonies colonies.cpp)
    DFHACK_PLUGIN(mode mode.cpp)
    DFHACK_PLUGIN(liquids liquids.cpp Brushes.h LINK_LIBRARIES lua)
    DFHACK_PLUGIN(tiletypes tiletypes.cpp Brushes.h)
    DFHACK_PLUGIN(tubefill tubefill.cpp)
    DFHACK_PLUGIN(autodump autodump.cpp)
    DFHACK_PLUGIN(cleanowned cleanowned.cpp)
    DFHACK_PLUGIN(deramp deramp.cpp)
    DFHACK_PLUGIN(flows flows.cpp)
    DFHACK_PLUGIN(filltraffic filltraffic.cpp)
    DFHACK_PLUGIN(seedwatch seedwatch.cpp)
    DFHACK_PLUGIN(initflags initflags.cpp)
    DFHACK_PLUGIN(stockpiles stockpiles.cpp)
    DFHACK_PLUGIN(rename rename.cpp LINK_LIBRARIES lua PROTOBUFS rename)
    DFHACK_PLUGIN(jobutils jobutils.cpp)
    DFHACK_PLUGIN(workflow workflow.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(showmood showmood.cpp)
    DFHACK_PLUGIN(fixveins fixveins.cpp)
    DFHACK_PLUGIN(fixpositions fixpositions.cpp)
    DFHACK_PLUGIN(follow follow.cpp)
    DFHACK_PLUGIN(changevein changevein.cpp)
    DFHACK_PLUGIN(changelayer changelayer.cpp)
    DFHACK_PLUGIN(changeitem changeitem.cpp)
    DFHACK_PLUGIN(advtools advtools.cpp)
    DFHACK_PLUGIN(tweak tweak.cpp)
    DFHACK_PLUGIN(feature feature.cpp)
    DFHACK_PLUGIN(lair lair.cpp)
    DFHACK_PLUGIN(zone zone.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(catsplosion catsplosion.cpp)
    DFHACK_PLUGIN(regrass regrass.cpp)
    DFHACK_PLUGIN(forceequip forceequip.cpp)
    DFHACK_PLUGIN(manipulator manipulator.cpp)
    DFHACK_PLUGIN(search search.cpp)
    DFHACK_PLUGIN(automaterial automaterial.cpp)
    # this one exports functions to lua
    DFHACK_PLUGIN(burrows burrows.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(sort sort.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(steam-engine steam-engine.cpp)
    DFHACK_PLUGIN(power-meter power-meter.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(siege-engine siege-engine.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(eventful eventful.cpp LINK_LIBRARIES lua)
    DFHACK_PLUGIN(add-spatter add-spatter.cpp)
    DFHACK_PLUGIN(fix-armory fix-armory.cpp)
    # not yet. busy with other crud again...
    #DFHACK_PLUGIN(versionosd versionosd.cpp)
    DFHACK_PLUGIN(misery misery.cpp)
    DFHACK_PLUGIN(workNow workNow.cpp)
    #DFHACK_PLUGIN(dfstream dfstream.cpp LINK_LIBRARIES clsocket dfhack-tinythread)
    DFHACK_PLUGIN(autoSyndrome autoSyndrome.cpp)
    DFHACK_PLUGIN(trueTransformation trueTransformation.cpp)
    DFHACK_PLUGIN(infiniteSky infiniteSky.cpp)
    DFHACK_PLUGIN(createitem createitem.cpp)
    DFHACK_PLUGIN(isoworldremote isoworldremote.cpp PROTOBUFS isoworldremote)
    DFHACK_PLUGIN(buildingplan buildingplan.cpp)
    DFHACK_PLUGIN(resume resume.cpp)
    DFHACK_PLUGIN(dwarfmonitor dwarfmonitor.cpp)
    DFHACK_PLUGIN(mousequery mousequery.cpp)
    DFHACK_PLUGIN(autotrade autotrade.cpp)
    DFHACK_PLUGIN(stocks stocks.cpp)
endif()


# this is the skeleton plugin. If you want to make your own, make a copy and then change it
OPTION(BUILD_SKELETON "Build the skeleton plugin." OFF)
if(BUILD_SKELETON)
    add_subdirectory(skeleton)
endif()
