set(PLUGIN_SRCS
    dispatch.cpp
    main.cpp
    types.cpp
    validate.cpp
)

dfhack_plugin(check-structures-sanity ${PLUGIN_SRCS} LINK_LIBRARIES lua)
