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

#ifndef EXPORTED_MAP_H
#define EXPORTED_MAP_H

#include <stdint.h>
#include <string>
#include <tuple>
#include <vector>

#include "./util/lodepng.h"

#include "MapTypes.h"

namespace exportmaps_plugin
{

  // Convenient alias for a RGB color
  typedef std::tuple<unsigned char, unsigned char, unsigned char> RGB_color;

  /*****************************************************************************
   Base class that represents a map.
   A map is a collection of bytes.
   Graphical maps is a collection of RGB values, so for each world pixel,
   3 bytes are needed.
   Raw maps are pure data values. As a unsigned char has a very limited range
   value, 2 bytes are used for each world position
  *****************************************************************************/
  class ExportedMapBase
  {
  protected:
    std::vector<unsigned char>  _image;    // The array of bytes
    std::string                 _filename; // The name of the file where the map will be saved
    int                         _width;    // World width in world coordinates
    int                         _height;   // World height in world coordinates
    MapType                     _type;     // Graphical map type
    MapTypeRaw                  _type_raw; // Raw map type
    MapTypeHeightMap            _type_hm;  // Heightmap type

  public:
    ExportedMapBase();

    ExportedMapBase(const std::string filename, // The name of the file where the map will be saved
                    int world_width,            // World width in world coordinates
                    int world_height,           // World height in world coordinates
                    MapType type,               // Graphical map type or NONE
                    MapTypeRaw type_raw,        // Raw map type or NONE_RAW
                    MapTypeHeightMap type_hm    // Height map type or NONE_HM
                    );

    //----------------------------------------------------------------------------//
    // Write a pixel in the map using world coordinates and offsets
    //----------------------------------------------------------------------------//
    virtual void write_world_pixel (int pos_x,     // x coordinate in world coordinates
                                    int pos_y,     // y coordinate in world coordinates
                                    int px,        // offset 0..15 respect to pos_x = embark coordinate x
                                    int py,        // offset 0..15 respect to pos_y = embark coordinate y
                                    RGB_color& rgb // pixel color
                                    ) = 0;
    //----------------------------------------------------------------------------//
    // Write a pixel in the map using embark coordinates
    //----------------------------------------------------------------------------//
    virtual void write_embark_pixel(int px,        // x coordinate in embark coordinates
                                    int py,        // y coordinate in embark coordinates
                                    RGB_color& rgb // pixel color
                                    ) = 0;

    //----------------------------------------------------------------------------//
    // Write a thick line (2 border pixels and 1 center) using embark coordinates
    //----------------------------------------------------------------------------//
    virtual void write_thick_line_point(int px,                  // x coordinate in embark coordinates
                                        int py,                  // y coordinate in embark coordinates
                                        RGB_color& color_center, // center pixel color
                                        RGB_color& color_border  // border pixels color
                                        ) = 0;
    //----------------------------------------------------------------------------//
    // Write data to a RAW map
    //----------------------------------------------------------------------------//
    virtual void write_data(int pos_x,         // x coordinate in world coordinates
                            int pos_y,         // y coordinate in world coordinates
                            int px,            // offset 0..15 respect to pos_x = embark coordinate x
                            int py,            // offset 0..15 respect to pos_y = embark coordinate y
                            int value          // value to be written
                            ) = 0;

    //----------------------------------------------------------------------------//
    // Write a map to disk
    //----------------------------------------------------------------------------//
    virtual int write_to_disk() = 0;

    //----------------------------------------------------------------------------//
    // Return the type of a graphical map
    //----------------------------------------------------------------------------//
    MapType get_type();

    //----------------------------------------------------------------------------//
    // Return the type of a raw map
    //----------------------------------------------------------------------------//
    MapTypeRaw get_type_raw();

    //----------------------------------------------------------------------------//
    // Return true if the map type is graphical
    //----------------------------------------------------------------------------//
    bool is_graphical_map();

    //----------------------------------------------------------------------------//
    // Return true if the map type is raw
    //----------------------------------------------------------------------------//
    bool is_raw_map();
  };

  /*****************************************************************************
   Subclass for graphical maps
  *****************************************************************************/

  class ExportedMapDF : public ExportedMapBase
  {
  public:
    ExportedMapDF();
    ExportedMapDF(const std::string filename, // The name of the file where the map will be saved
                  int world_width,            // World width in world coordinates
                  int world_height,           // World height in world coordinates
                  MapType type                // Graphical map type
                  );
    //----------------------------------------------------------------------------//
    // Write a pixel in the map using world coordinates and offsets
    //----------------------------------------------------------------------------//
    void write_world_pixel(int pos_x,     // x coordinate in world coordinates
                           int pos_y,     // y coordinate in world coordinates
                           int px,        // offset 0..15 respect to pos_x = embark coordinate x
                           int py,        // offset 0..15 respect to pos_y = embark coordinate y
                           RGB_color& rgb // pixel color
                           );
    //----------------------------------------------------------------------------//
    // Write a pixel in the map using embark coordinates
    //----------------------------------------------------------------------------//
    void write_embark_pixel(int px,        // x coordinate in embark coordinates
                            int py,        // y coordinate in embark coordinates
                            RGB_color& rgb // pixel color
                            );

    //----------------------------------------------------------------------------//
    // Write a thick line (2 border pixels and 1 center) using embark coordinates
    //----------------------------------------------------------------------------//
    void write_thick_line_point(int px,                  // x coordinate in embark coordinates
                                int py,                  // y coordinate in embark coordinates
                                RGB_color& color_center, // center pixel color
                                RGB_color& color_border  // border pixels color
                                );
    //----------------------------------------------------------------------------//
    // Write data to a RAW map.
    // Do nothing as this is a graphical map
    //----------------------------------------------------------------------------//
    void write_data(int pos_x,
                    int pos_y,
                    int px,
                    int py,
                    int value
                    );
    //----------------------------------------------------------------------------//
    // Write a map to disk
    //----------------------------------------------------------------------------//
    int write_to_disk();
  };

  /*****************************************************************************
   Subclass for raw maps
  *****************************************************************************/

  class ExportedMapRaw : public ExportedMapBase
  {
  public:
    ExportedMapRaw();
    ExportedMapRaw(const std::string filename, // The name of the file where the map will be saved
                   int world_width,            // World width in world coordinates
                   int world_height,           // World height in world coordinates
                   MapTypeRaw type_raw         // Raw map type
                   );

    //----------------------------------------------------------------------------//
    // Write a pixel in the map using world coordinates and offsets.
    // Do nothing as this is a raw map.
    //----------------------------------------------------------------------------//
    void write_world_pixel(int pos_x,
                           int pos_y,
                           int px,
                           int py,
                           RGB_color& rgb
                           );
    //----------------------------------------------------------------------------//
    // Write a pixel in the map using embark coordinates.
    // Do nothing as this is a raw map
    //----------------------------------------------------------------------------//
    void write_embark_pixel(int px,
                            int py,
                            RGB_color& rgb
                            );

    //----------------------------------------------------------------------------//
    // Write a thick line (2 border pixels and 1 center) using embark coordinates
    // Do nothing as this is a raw map
    //----------------------------------------------------------------------------//
    void write_thick_line_point(int px,                  // x coordinate in embark coordinates
                                int py,                  // y coordinate in embark coordinates
                                RGB_color& color_center, // center pixel color
                                RGB_color& color_border  // border pixels color
                                );

    //----------------------------------------------------------------------------//
    // Write data to a RAW map.
    //----------------------------------------------------------------------------//
    void write_data(int pos_x,         // x coordinate in world coordinates
                    int pos_y,         // y coordinate in world coordinates
                    int px,            // offset 0..15 respect to pos_x = embark coordinate x
                    int py,            // offset 0..15 respect to pos_y = embark coordinate y
                    int value          // value to be written
                    );
    //----------------------------------------------------------------------------//
    // Write a map to disk
    //----------------------------------------------------------------------------//
    int write_to_disk();
  };

  /*****************************************************************************
   Subclass for Heightmaps
  *****************************************************************************/

  class ExportedMapHM : public ExportedMapBase
  {
  public:
    ExportedMapHM();
    ExportedMapHM(const std::string filename, // The name of the file where the map will be saved
                  int world_width,            // World width in world coordinates
                  int world_height,           // World height in world coordinates
                  MapTypeHeightMap type       // Heightmap type
                  );
    //----------------------------------------------------------------------------//
    // Write a pixel in the map using world coordinates and offsets
    //----------------------------------------------------------------------------//
    void write_world_pixel(int pos_x,     // x coordinate in world coordinates
                           int pos_y,     // y coordinate in world coordinates
                           int px,        // offset 0..15 respect to pos_x = embark coordinate x
                           int py,        // offset 0..15 respect to pos_y = embark coordinate y
                           RGB_color& rgb // pixel color
                           );
    //----------------------------------------------------------------------------//
    // Write a pixel in the map using embark coordinates
    //----------------------------------------------------------------------------//
    void write_embark_pixel(int px,        // x coordinate in embark coordinates
                            int py,        // y coordinate in embark coordinates
                            RGB_color& rgb // pixel color
                            );

    //----------------------------------------------------------------------------//
    // Write a thick line (2 border pixels and 1 center) using embark coordinates
    //----------------------------------------------------------------------------//
    void write_thick_line_point(int px,                  // x coordinate in embark coordinates
                                int py,                  // y coordinate in embark coordinates
                                RGB_color& color_center, // center pixel color
                                RGB_color& color_border  // border pixels color
                                );
    //----------------------------------------------------------------------------//
    // Write data to a RAW map.
    // Do nothing as this is a graphical map
    //----------------------------------------------------------------------------//
    void write_data(int pos_x,
                    int pos_y,
                    int px,
                    int py,
                    int value
                    );
    //----------------------------------------------------------------------------//
    // Write a map to disk
    //----------------------------------------------------------------------------//
    int write_to_disk();
  };


}

#endif  // EXPORTED_MAP_H
