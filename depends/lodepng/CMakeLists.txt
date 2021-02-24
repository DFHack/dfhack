project(lodepng)
add_library(lodepng STATIC EXCLUDE_FROM_ALL lodepng.cpp lodepng.h)
ide_folder(lodepng "Depends")
