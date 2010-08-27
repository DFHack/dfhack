
project(dfoffsetedit)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 QUIET)
if(QT4_FOUND)
    add_subdirectory (src)
else(QT4_FOUND)
    MESSAGE(STATUS "Qt4 libraries not found - offset editor can't be built.")
endif(QT4_FOUND)
