project(dfhack-md5)
ADD_LIBRARY(dfhack-md5 STATIC EXCLUDE_FROM_ALL md5.cpp md5wrapper.cpp)
IDE_FOLDER(dfhack-md5 "Depends")