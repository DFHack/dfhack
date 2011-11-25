PROJECT (server)
SET(PROJECT_SRCS
    main.cpp
)

include_directories( ${CMAKE_SOURCE_DIR} )
IF(UNIX)
    OPTION(SERVER_INTERNAL_SO "Link with prebuilt internal zeromq lib and headers." ON)
    IF(SERVER_INTERNAL_SO)
        SET(PROJECT_LIBS
            ${server_SOURCE_DIR}/zeromq/libzmq.so.1
            ${PROJECT_LIBS}
        )
        include_directories (
            ${include_directories}
            ${server_SOURCE_DIR}/zeromq
        )
        install(PROGRAMS ${server_SOURCE_DIR}/zeromq/libzmq.so.1 DESTINATION "hack/deplibs")
    ELSE()
        SET(PROJECT_LIBS
            zmq
            ${PROJECT_LIBS}
        )
    ENDIF()
ELSE()
        SET(PROJECT_LIBS
            ${server_SOURCE_DIR}/zeromq/libzmq.lib
            ${PROJECT_LIBS}
        )
        include_directories (
            ${include_directories}
            ${server_SOURCE_DIR}/zeromq
        )
        install(PROGRAMS ${server_SOURCE_DIR}/zeromq/libzmq.dll DESTINATION ".")

ENDIF()

DFHACK_PLUGIN(server ${PROJECT_SRCS} LINK_LIBRARIES ${PROJECT_LIBS})

add_executable ( helloclient hello.cpp )
target_link_libraries ( helloclient ${PROJECT_LIBS})
install(TARGETS helloclient RUNTIME DESTINATION . )