/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// You can always find the latest version of this plugin in Github
// https://github.com/ragundo/exportmaps

#include <math.h>
#include <iostream>
#include <fstream>

#include "../include/ExportedMap.h"

using namespace exportmaps_plugin;

/*****************************************************************************
ExportedMapBase methods
*****************************************************************************/

ExportedMapBase::ExportedMapBase()
{
}

//----------------------------------------------------------------------------//
// Constructor
//----------------------------------------------------------------------------//
ExportedMapBase::ExportedMapBase(const std::string filename, // name of the file that will store the map
                                 int world_width,            // world width in world coordinates
                                 int world_height,           // world height in world coordinates
                                 MapType type,               // Graphical map type or NONE
                                 MapTypeRaw type_raw,        // Raw map type or NONE_RAW
                                 MapTypeHeightMap type_hm    // Heightmap type or NONE_HM
                                 )
    : _filename(filename),
      _width(world_width*16),
      _height(world_height*16),
      _type(type),
      _type_raw(type_raw),
      _type_hm(type_hm)
{
}

//----------------------------------------------------------------------------//
// Returns the map type (biome, elevation, etc)
//----------------------------------------------------------------------------//
MapType ExportedMapBase::get_type()
{
  return _type;
}

//----------------------------------------------------------------------------//
// Returns the type of a raw map
//----------------------------------------------------------------------------//
MapTypeRaw ExportedMapBase::get_type_raw()
{
  return _type_raw;
}

//----------------------------------------------------------------------------//
// Return true if the map type is graphical
//----------------------------------------------------------------------------//
bool ExportedMapBase::is_graphical_map()
{
  return _type != MapType::NONE;
}

//----------------------------------------------------------------------------//
// Return true if the map type is raw
//----------------------------------------------------------------------------//
bool ExportedMapBase::is_raw_map()
{
  return _type_raw != MapTypeRaw::NONE_RAW;
}

/*****************************************************************************
ExportedMapDF methods
*****************************************************************************/

ExportedMapDF::ExportedMapDF()
{
}

//----------------------------------------------------------------------------//
// Constructor
//----------------------------------------------------------------------------//
ExportedMapDF::ExportedMapDF(const std::string filename, // name of the file that will store the map
                             int world_width,            // world width in world coordinates
                             int world_height,           // world height in world coordinates
                             MapType type                // The map type to be draw (biome, elevation, etc)
                             )
  : ExportedMapBase(filename,
                    world_width,
                    world_height,
                    type,
                    MapTypeRaw::NONE_RAW,
                    MapTypeHeightMap::NONE_HM
                    )
{
  // Each world tile has 16 * 16 embark pixels. Each pixel needs 4 bytes
  // so resize the image to its correct size
  _image.resize(world_width * world_height * 16 * 16 * 4); // 4 = RGBA for PNG
}

//----------------------------------------------------------------------------//
// Write a pixel to the image using world coordinates.
// As the maps are draw using embark coordinates, for drawing a pixel we need
// the world coordinate and the offset in the 16x16 pixel array that a world
// tile has of embark tiles
//----------------------------------------------------------------------------//
void ExportedMapDF::write_world_pixel(int pos_x,     // pixel world coordinate x
                                      int pos_y,     // pixel world coordinate y
                                      int px,        // Delta x (0..15)
                                      int py,        // Delta y (0..15)
                                      RGB_color& rgb // Pixel color
                                      )
{
  int index_png = pos_y * 16 * _height +
                  pos_x * 16          +
                  py         * _height +
                  px;

  _image[4*index_png + 0] = std::get<0>(rgb);
  _image[4*index_png + 1] = std::get<1>(rgb);
  _image[4*index_png + 2] = std::get<2>(rgb);
  _image[4*index_png + 3] = 255;              // Solid color
}

//----------------------------------------------------------------------------//
// Write a pixel to the image using embark coordinates.
//----------------------------------------------------------------------------//
void ExportedMapDF::write_embark_pixel(int px,        // Pixel embark coordinate x
                                       int py,        // Pixel embark coordinate y
                                       RGB_color& rgb // Pixel color
                                       )
{
  // Compute world coordinates
  int dpy = py >> 4;
  int dpx = px >> 4;
  int mpy = py % 16;
  int mpx = px % 16;

  int index_png = dpy * 16 * _height +
                  dpx * 16          +
                  mpy      * _height +
                  mpx;

  _image[4*index_png + 0] = std::get<0>(rgb);
  _image[4*index_png + 1] = std::get<1>(rgb);
  _image[4*index_png + 2] = std::get<2>(rgb);
  _image[4*index_png + 3] = 255;              // Solid color
}

//----------------------------------------------------------------------------//
// Write a "thick" point of a line.
// A thick line has a center pixel and two border pixels
//----------------------------------------------------------------------------//
void ExportedMapDF::write_thick_line_point(int px,                  // Pixel embark coordinate x
                                           int py,                  // Pixel embark coordinate y
                                           RGB_color& color_center, // Center color
                                           RGB_color& color_border  // Border color
                                           )
{
  // Compute world coordinates
  int dpy = py >> 4;
  int dpx = px >> 4;
  int mpy = py % 16;
  int mpx = px % 16;

  int index_png = dpy * 16 * _height +
                  dpx * 16          +
                  mpy      * _height +
                  mpx;

  unsigned char r_center = std::get<0>(color_center);
  unsigned char g_center = std::get<1>(color_center);
  unsigned char b_center = std::get<2>(color_center);


  // Draw the center pixel
  _image[4*index_png + 0] = r_center;
  _image[4*index_png + 1] = g_center;
  _image[4*index_png + 2] = b_center;
  _image[4*index_png + 3] = 255;    // Solid color

  unsigned char r_border = std::get<0>(color_border);
  unsigned char g_border = std::get<1>(color_border);
  unsigned char b_border = std::get<2>(color_border);

  // Now draw a rectangle around the center pixel using the
  // border color, but check that we don't overwrite previous
  // center pixels of the line
  if (!((_image[4*index_png + 4 + 0] == r_center) &&
        (_image[4*index_png + 4 + 1] == g_center) &&
        (_image[4*index_png + 4 + 2] == b_center)
       )
     )
  {
    _image[4*index_png + 4 + 0] = r_border;
    _image[4*index_png + 4 + 1] = g_border;
    _image[4*index_png + 4 + 2] = b_border;
    _image[4*index_png + 4 + 3] = 255;    // Solid color
  }

  if (!((_image[4*index_png - 4 + 0] == r_center) &&
        (_image[4*index_png - 4 + 1] == g_center) &&
        (_image[4*index_png - 4 + 2] == b_center)
       )
     )
  {
    _image[4*index_png - 4 + 0] = r_border;
    _image[4*index_png - 4 + 1] = g_border;
    _image[4*index_png - 4 + 2] = b_border;
    _image[4*index_png - 4 + 3] = 255;    // Solid color
  }

  // Go up one line
  index_png -= this->_width;

  if (!((_image[4*index_png + 4 + 0] == r_center) &&
        (_image[4*index_png + 4 + 1] == g_center) &&
        (_image[4*index_png + 4 + 2] == b_center)
       )
     )
  {
    _image[4*index_png + 4 + 0] = r_border;
    _image[4*index_png + 4 + 1] = g_border;
    _image[4*index_png + 4 + 2] = b_border;
    _image[4*index_png + 4 + 3] = 255;    // Solid color
  }

  if (!((_image[4*index_png + 0 + 0] == r_center) &&
        (_image[4*index_png + 0 + 1] == g_center) &&
        (_image[4*index_png + 0 + 2] == b_center)
       )
     )
  {
    _image[4*index_png + 0 + 0] = r_border;
    _image[4*index_png + 0 + 1] = g_border;
    _image[4*index_png + 0 + 2] = b_border;
    _image[4*index_png + 0 + 3] = 255;    // Solid color
  }

  if (!((_image[4*index_png - 4 + 0] == r_center) &&
        (_image[4*index_png - 4 + 1] == g_center) &&
        (_image[4*index_png - 4 + 2] == b_center)
       )
     )
  {
    _image[4*index_png - 4 + 0] = r_border;
    _image[4*index_png - 4 + 1] = g_border;
    _image[4*index_png - 4 + 2] = b_border;
    _image[4*index_png - 4 + 3] = 255;    // Solid color
  }

  // Go down two lines
  index_png += this->_width * 2;

  if (!((_image[4*index_png + 4 + 0] == r_center) &&
        (_image[4*index_png + 4 + 1] == g_center) &&
        (_image[4*index_png + 4 + 2] == b_center)
       )
     )
  {
    _image[4*index_png + 4 + 0] = r_border;
    _image[4*index_png + 4 + 1] = g_border;
    _image[4*index_png + 4 + 2] = b_border;
    _image[4*index_png + 4 + 3] = 255;    // Solid color
  }

  if (!((_image[4*index_png + 0 + 0] == r_center) &&
        (_image[4*index_png + 0 + 1] == g_center) &&
        (_image[4*index_png + 0 + 2] == b_center)
       )
     )
  {
    _image[4*index_png + 0 + 0] = r_border;
    _image[4*index_png + 0 + 1] = g_border;
    _image[4*index_png + 0 + 2] = b_border;
    _image[4*index_png + 0 + 3] = 255;    // Solid color
  }

  if (!((_image[4*index_png - 4 + 0] == r_center) &&
        (_image[4*index_png - 4 + 1] == g_center) &&
        (_image[4*index_png - 4 + 2] == b_center)
       )
     )
  {
    _image[4*index_png - 4 + 0] = r_border;
    _image[4*index_png - 4 + 1] = g_border;
    _image[4*index_png - 4 + 2] = b_border;
    _image[4*index_png - 4 + 3] = 255;    // Solid color
  }
}


//----------------------------------------------------------------------------//
// Virtual method
// The base class does nothing
//----------------------------------------------------------------------------//
void ExportedMapDF::write_data(int pos_x,
                               int pos_y,
                               int px,
                               int py,
                               int value
                               )
{
}


//----------------------------------------------------------------------------//
// Write the image as PNG file to disk
//----------------------------------------------------------------------------//
int ExportedMapDF::write_to_disk()
{
  //Encode from raw pixels to disk with a single function call
  //The image argument has width * height RGBA pixels or width * height * 4 bytes
  return lodepng::encode(_filename,
                         _image,
                         _width,
                         _height
                         );
}



/*****************************************************************************
ExportedMapRaw methods
*****************************************************************************/

ExportedMapRaw::ExportedMapRaw()
{
}

ExportedMapRaw::ExportedMapRaw(const std::string filename, // name of the file that will store the map
                               int world_width,            // world width in world coordinates
                               int world_height,           // world height in world coordinates
                               MapTypeRaw type_raw         // The raw map type to be written (biome, elevation, etc)
                               )
  : ExportedMapBase(filename,
                    world_width,
                    world_height,
                    MapType::NONE,
                    type_raw,
                    MapTypeHeightMap::NONE_HM
                    )
{
    // Each world tile has 16 * 16 entries. Each entry needs 2 bytes
    _image.resize(world_width * world_height * 16 * 16 * 2);
}

//----------------------------------------------------------------------------//
// Write a pixel in the map using world coordinates and offsets.
// Do nothing as this is a raw map.
//----------------------------------------------------------------------------//
void ExportedMapRaw::write_world_pixel(int pos_x,
                                       int pos_y,
                                       int px,
                                       int py,
                                       RGB_color& rgb
                                       )
{
}

//----------------------------------------------------------------------------//
// Write a pixel in the map using embark coordinates.
// Do nothing as this is a raw map
//----------------------------------------------------------------------------//
void ExportedMapRaw::write_embark_pixel(int px,
                                        int py,
                                        RGB_color& rgb
                                        )
{
}

//----------------------------------------------------------------------------//
// Write a "thick" point of a line.
// A thick line has a center pixel and two border pixels
// Do nothing as this is a raw map
//----------------------------------------------------------------------------//
void ExportedMapRaw::write_thick_line_point(int px,                  // Pixel embark coordinate x
                                            int py,                  // Pixel embark coordinate y
                                            RGB_color& color_center, // Center color
                                            RGB_color& color_border  // Border color
                                            )
{
}

//----------------------------------------------------------------------------//
// Write data to a RAW map.
//----------------------------------------------------------------------------//
void ExportedMapRaw::write_data(int pos_x,         // pixel world coordinate x
                                int pos_y,         // pixel world coordinate y
                                int px,            // Delta x (0..15)
                                int py,            // Delta y (0..15)
                                int value          // value to be written to the file
                                )
{
  int index_buffer = pos_y * 16 * _height +
                     pos_x * 16           +
                     py         * _height +
                     px;

  if (value < 0)
  {
   value = value + 65536;
  }
  float f = (float)value/256;

  unsigned char lsb = (value <= 255 ? value : value % 256);
  unsigned char msb = (value <= 255 ? 0     : (unsigned int)floor(f));

  // Use little endian format (LSB first, MSB last)
  _image[2*index_buffer + 0] = lsb;
  _image[2*index_buffer + 1] = msb;
}

//----------------------------------------------------------------------------//
// Write a unsigned int to a stream using Little Endian (LSB,MSB)
//----------------------------------------------------------------------------//
void write_little_endian(std::ostream& outs, // The stream where to write
                         unsigned int value  // Value to be written
                         )
{
  for (unsigned size = sizeof(unsigned int); size; --size, value >>= 8)
    outs.put( static_cast <char> (value & 0xFF) );
}

int ExportedMapRaw::write_to_disk()
{
  // Write the buffer to the fstream
  std::ofstream outfile(_filename, std::ios::out | std::ios::binary);

  write_little_endian(outfile,
                      _width
                      );

  write_little_endian(outfile,
                      _height
                      );

  outfile.write((const char*)&_image[0], _image.size());

  // Close the stream
  outfile.close();

  // Done
  return 0;
}


/*****************************************************************************
ExportedMapHM methods
*****************************************************************************/

ExportedMapHM::ExportedMapHM()
{
}

//----------------------------------------------------------------------------//
// Constructor
//----------------------------------------------------------------------------//
ExportedMapHM::ExportedMapHM(const std::string filename, // name of the file that will store the map
                             int world_width,            // world width in world coordinates
                             int world_height,           // world height in world coordinates
                             MapTypeHeightMap type       // The heightmap type
                             )
  : ExportedMapBase(filename,
                    world_width,
                    world_height,
                    MapType::NONE,
                    MapTypeRaw::NONE_RAW,
                    type
                    )
{
  // Each world tile has 16 * 16 embark pixels. Each pixel needs 4 bytes
  // so resize the image to its correct size
  _image.resize(world_width * world_height * 16 * 16 * 4); // 4 = RGBA for PNG
}

//----------------------------------------------------------------------------//
// Write a pixel to the image using world coordinates.
// As the maps are draw using embark coordinates, for drawing a pixel we need
// the world coordinate and the offset in the 16x16 pixel array that a world
// tile has of embark tiles
//----------------------------------------------------------------------------//
void ExportedMapHM::write_world_pixel(int pos_x,     // pixel world coordinate x
                                      int pos_y,     // pixel world coordinate y
                                      int px,        // Delta x (0..15)
                                      int py,        // Delta y (0..15)
                                      RGB_color& rgb // Pixel color
                                      )
{
  int index_png = pos_y * 16 * _height +
                  pos_x * 16          +
                  py         * _height +
                  px;

  _image[4*index_png + 0] = std::get<0>(rgb);
  _image[4*index_png + 1] = std::get<1>(rgb);
  _image[4*index_png + 2] = std::get<2>(rgb);
  _image[4*index_png + 3] = 255;              // Solid color
}

//----------------------------------------------------------------------------//
// Write a pixel to the image using embark coordinates.
//----------------------------------------------------------------------------//
void ExportedMapHM::write_embark_pixel(int px,        // Pixel embark coordinate x
                                       int py,        // Pixel embark coordinate y
                                       RGB_color& rgb // Pixel color
                                       )
{

}

//----------------------------------------------------------------------------//
// Write a "thick" point of a line.
// A thick line has a center pixel and two border pixels
//----------------------------------------------------------------------------//
void ExportedMapHM::write_thick_line_point(int px,                  // Pixel embark coordinate x
                                           int py,                  // Pixel embark coordinate y
                                           RGB_color& color_center, // Center color
                                           RGB_color& color_border  // Border color
                                           )
{

}


//----------------------------------------------------------------------------//
// Virtual method
// The base class does nothing
//----------------------------------------------------------------------------//
void ExportedMapHM::write_data(int pos_x,
                               int pos_y,
                               int px,
                               int py,
                               int value
                               )
{
}


//----------------------------------------------------------------------------//
// Write the image as PNG file to disk
//----------------------------------------------------------------------------//
int ExportedMapHM::write_to_disk()
{
  //Encode from raw pixels to disk with a single function call
  //The image argument has width * height RGBA pixels or width * height * 4 bytes
  return lodepng::encode(_filename,
                         _image,
                         _width,
                         _height
                         );
}



