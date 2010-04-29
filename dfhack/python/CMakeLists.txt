PROJECT (pydfhack)
FIND_PACKAGE(PythonLibs)

SET(PYTHON_MODULE_PREFIX "")

SET(PROJECT_LIBS ${PYTHON_LIBRARIES} dfhack )

IF(UNIX)
  add_definitions(-DLINUX_BUILD)
  add_definitions(-DUSE_CONFIG_H)
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -pedantic")
  SET(PYTHON_MODULE_SUFFIX ".so")
ENDIF(UNIX)
IF(WIN32)
  #windows
  SET(PYTHON_MODULE_SUFFIX ".pyd")
ENDIF(WIN32)

IF(PYTHONLIBS_FOUND)
    INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
    PYTHON_ADD_MODULE(pydfhack
    DF_API.cpp
    DF_Buildings.cpp
    DF_Constructions.cpp
    DF_CreatureManager.cpp
    DF_GUI.cpp
    DF_Maps.cpp
    DF_Material.cpp
    DF_Position.cpp
    DF_Translate.cpp
    DF_Vegetation.cpp
    pydfhack.cpp
    )
    SET_TARGET_PROPERTIES(pydfhack PROPERTIES PREFIX "")
    # fix suffix on windows
    SET_TARGET_PROPERTIES(pydfhack PROPERTIES SUFFIX ${PYTHON_MODULE_SUFFIX})

    TARGET_LINK_LIBRARIES(pydfhack ${PROJECT_LIBS})
    
ELSE(PYTHONLIBS_FOUND)
    MESSAGE("UNABLE TO BUILD PYTHON BINDINGS!")
ENDIF(PYTHONLIBS_FOUND)
