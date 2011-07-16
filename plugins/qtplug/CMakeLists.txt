find_package(Qt4 QUIET)
find_package(OpenGL QUIET)

if(QT4_FOUND AND OPENGL_FOUND AND OPENGL_GLU_FOUND)
    IF(QT_VERSION_MAJOR MATCHES 4 AND QT_VERSION_MINOR GREATER 6)
        set( QT_USE_QTGUI TRUE )
        set( QT_USE_QTOPENGL TRUE )
        INCLUDE( ${QT_USE_FILE} )

        include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

        set ( qtplug_SRCS
            qtplug.cpp
            blankslade.cpp
            glwidget.cpp
        )

        SET ( qtplug_UI
            gui/main.ui
        )

        SET( qtplug_RCS
            gui/resources.qrc
        )

        # this command will generate rules that will run rcc on all files from blankslade_RCS
        # in result blankslade_RC_SRCS variable will contain paths to files produced by rcc
        QT4_ADD_RESOURCES( qtplug_RC_SRCS ${qtplug_RCS} )

        QT4_WRAP_UI(qtplug_UI_h ${qtplug_UI})
        qt4_automoc(${qtplug_SRCS})
        DFHACK_PLUGIN(qtplug ${qtplug_SRCS} ${qtplug_RC_SRCS} ${qtplug_UI_h})
        target_link_libraries(qtplug ${OPENGL_LIBRARIES} ${QT_LIBRARIES} )
    ELSE(QT_VERSION_MAJOR MATCHES 4 AND QT_VERSION_MINOR GREATER 6)
        MESSAGE(STATUS "Can't build the Qt plugin. Your Qt is too old.")
    ENDIF(QT_VERSION_MAJOR MATCHES 4 AND QT_VERSION_MINOR GREATER 6)
else(QT4_FOUND AND OPENGL_FOUND AND OPENGL_GLU_FOUND)
    MESSAGE(STATUS "Required libraries (Qt, GL, GLU) not found - Qt plugin can't be built.")
endif(QT4_FOUND AND OPENGL_FOUND AND OPENGL_GLU_FOUND)