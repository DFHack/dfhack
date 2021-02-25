project(dfhack-lodepng)
add_library(dfhack-lodepng STATIC EXCLUDE_FROM_ALL lodepng.cpp lodepng.h)
ide_folder(dfhack-lodepng "Depends")
