set(PLUGIN_SRCS
    dispatch.cpp
    main.cpp
    types.cpp
    validate.cpp
)

dfhack_plugin(check-structures-sanity ${PLUGIN_SRCS} LINK_LIBRARIES lua COMPILE_FLAGS_GCC "-O0 -ggdb3" COMPILE_FLAGS_MSVC "/Od")
