project(sizecheck)
add_library(sizecheck SHARED sizecheck.cpp)
ide_folder(sizecheck "Depends")
install(TARGETS sizecheck
    LIBRARY DESTINATION ${DFHACK_LIBRARY_DESTINATION}
    RUNTIME DESTINATION ${DFHACK_LIBRARY_DESTINATION})
