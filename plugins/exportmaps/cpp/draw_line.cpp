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

#include "../include/dfhack.h"
#include "../include/MapsExporter.h"
#include "../include/RegionDetails.h"
#include "../include/ExportedMap.h"

using namespace std;
using namespace DFHack;
using namespace exportmaps_plugin;

//----------------------------------------------------------------------------//
// Bresenham algorithm to draw a line between 2 points
//
// Returns a vector with all the point coordinates of the line
//----------------------------------------------------------------------------//
vector<df::coord2d> line_algorithm(int site1_center_x, // Point 1 x
                                   int site1_center_y, // Point 1 y
                                   int site2_center_x, // Point 2 x
                                   int site2_center_y  // Point 2 y
                                   )
{
  vector<df::coord2d> result;
  int x, y, dx, dy, p, incE, incNE, stepx, stepy;

  dx = (site2_center_x - site1_center_x);
  dy = (site2_center_y - site1_center_y);

 /* determinar que punto usar para empezar, cual para terminar */
  stepy = 1;
  if (dy < 0)
  {
    dy = -dy;
    stepy = -1;
  }

  stepx = 1;
  if (dx < 0)
  {
    dx = -dx;
    stepx = -1;
  }

  x = site1_center_x;
  y = site1_center_y;
  result.push_back(df::coord2d(x,y));

 /* se cicla hasta llegar al extremo de la línea */
  if (dx > dy)
  {
    p = 2*dy - dx;
    incE = 2*dy;
    incNE = 2*(dy-dx);
    while (x != site2_center_x)
    {
      x = x + stepx;
      if (p < 0)
      {
        p = p + incE;
      }
      else
      {
        y = y + stepy;
        p = p + incNE;
      }
      result.push_back(df::coord2d(x,y));
    }
  }
  else
  {
    p = 2*dx - dy;
    incE = 2*dx;
    incNE = 2*(dx-dy);
    while (y != site2_center_y)
    {
      y = y + stepy;
      if (p < 0)
      {
        p = p + incE;
      }
      else
      {
        x = x + stepx;
        p = p + incNE;
      }
      result.push_back(df::coord2d(x,y));
    }
  }
  return result;
}


//----------------------------------------------------------------------------//
// Draws a "thick" line in the map between point1 and point2.
// A thick line is 3 pixels wide, with a center pixel and 2 border pixels
//----------------------------------------------------------------------------//
void draw_thick_color_line(ExportedMapBase* map,    // The map where we are drawing
                           int x1,                  // Point 1 x coordinate
                           int y1,                  // Point 1 y coordinate
                           int x2,                  // Point 2 x coordinate
                           int y2,                  // Point 2 y coordinate
                           RGB_color& color_center, // Color of the center line
                           RGB_color& color_border  // Color of the border line
                           )
{
  // Get a "thin" line
  vector<df::coord2d> buffer = line_algorithm(x1, y1, x2, y2);

  // Project the buffer over the map, applying color to render it
  for (unsigned int i = 0; i < buffer.size(); ++i)
  {
    df::coord2d pos = buffer[i];
    map->write_thick_line_point(pos.x,
                                pos.y,
                                color_center,
                                color_border);
  }
}



//----------------------------------------------------------------------------//
// Draws a "thick" line in the map between point1 and point2 using 2 colors.
// A thick line is 3 pixels wide, with a center pixel and 2 border pixels
// Also, half of the line is painted with some colors and the other half
// is painted with different colors
//----------------------------------------------------------------------------//
void draw_thick_color_line_2_colors(ExportedMapBase* map,     // The map where we are drawing
                                    int x1,                   // Point 1 x coordinate
                                    int y1,                   // Point 1 x coordinate
                                    int x2,                   // Point 1 x coordinate
                                    int y2,                   // Point 1 x coordinate
                                    RGB_color& color_center1, // Half line1 center color
                                    RGB_color& color_border1, // Half line1 border color
                                    RGB_color& color_center2, // Half line2 center color
                                    RGB_color& color_border2  // Half line2 border color
                                    )
{
  // Get a thin line
  vector<df::coord2d> buffer = line_algorithm(x1, y1, x2, y2);

  // Project the buffer over the map, applying color to render it
  for (unsigned int i = 0; i < buffer.size(); ++i)
  {
    df::coord2d pos = buffer[i];

    if (i <= (buffer.size() >> 1))
      map->write_thick_line_point(pos.x,
                                  pos.y,
                                  color_center2,
                                  color_border2);

    else // Second half is in different colors
      map->write_thick_line_point(pos.x,
                                  pos.y,
                                  color_center1,
                                  color_border1);
  }
}



//----------------------------------------------------------------------------//
// Draws a site rectangle in the map
// A site rectangle is a solid one that spans over the site limits
// If the site has population (not empty) or not is reflecte
//----------------------------------------------------------------------------//
void draw_site_rectangle(ExportedMapBase* map,             // The map where we draw
                         df::world_site*  world_site,      // The site to be draw
                         int              site_population, // Site population
                         unsigned char    pixel_R,         // Pixel color
                         unsigned char    pixel_G,
                         unsigned char    pixel_B
                         )
{
  // Get the site boundary in world tiles
  int site_global_min_x = world_site->global_min_x;
  int site_global_max_x = world_site->global_max_x;
  int site_global_min_y = world_site->global_min_y;
  int site_global_max_y = world_site->global_max_y;

  // Define some colors
  RGB_color rgb_pixel_color_site_no_empty(pixel_R,
                                          pixel_G,
                                          pixel_B);


  RGB_color rgb_pixel_color_site_empty(pixel_R >> 2,
                                       pixel_G >> 2,
                                       pixel_B >> 2);


  RGB_color rgb_pixel_color_border(pixel_R >> 1,
                                   pixel_G >> 1,
                                   pixel_B >> 1);


  //Iterate over the site coordinates

  for (int y_it = site_global_max_y;    y_it >= site_global_min_y;  --y_it)
    for (int x_it = site_global_min_x;  x_it <= site_global_max_x;  ++x_it)
      if ((x_it == site_global_min_x) ||
          (x_it == site_global_max_x) ||
          (y_it == site_global_min_y) ||
          (y_it == site_global_max_y))
      {
        // We're in the border of a site
        // draw it with color / 2

        map->write_embark_pixel(x_it,                     // Embark coordinate x (0..15)
                                y_it,                     // Embark coordinate y (0..15)
                                rgb_pixel_color_border);  // Pixel color
      }
      else if ((site_population > 0)     ||
          (x_it < site_global_min_x + 2) ||
          (x_it > site_global_max_x - 2) ||
          (y_it < site_global_min_y + 2) ||
          (y_it > site_global_max_y - 2))
      {
        // If the site has population or
        // if the site doesn't have population but
        // we're just one embark tile inside the site,
        // draw it with the color;

        map->write_embark_pixel(x_it,                            // Embark coordinate x (0..15)
                                y_it,                            // Embark coordinate y (0..15)
                                rgb_pixel_color_site_no_empty);  // Pixel color
      }
      else
      {
        // We're inside a site with no population
        // Draw it with a color / 4

        map->write_embark_pixel(x_it,                         // Embark coordinate x (0..15)
                                y_it,                         // Embark coordinate y (0..15)
                                rgb_pixel_color_site_empty);  // Color to be used

      }
}


//----------------------------------------------------------------------------//
// Draws a rectangle with a interior offset respect the world site boundaries
//----------------------------------------------------------------------------//
void draw_site_rectangle_offseted(ExportedMapBase* map,        // The map where we draw
                                  df::world_site*  world_site, // The site to be draw
                                  unsigned char    pixel_R,    // Color to be used
                                  unsigned char    pixel_G,
                                  unsigned char    pixel_B,
                                  int              offset      // in pixels respect the site border
                                  )
{
  // Get the site boundary in world tiles
  int site_global_min_x = world_site->global_min_x;
  int site_global_max_x = world_site->global_max_x;
  int site_global_min_y = world_site->global_min_y;
  int site_global_max_y = world_site->global_max_y;

  // Offset them

  int site_global_min_xo = std::min(site_global_min_x + offset, site_global_max_x - offset);
  int site_global_max_xo = std::max(site_global_min_x + offset, site_global_max_x - offset);
  int site_global_min_yo = std::min(site_global_min_y + offset, site_global_max_y - offset);
  int site_global_max_yo = std::max(site_global_min_y + offset, site_global_max_y - offset);

  // Check limits
  if (site_global_min_xo > site_global_max_xo)
    site_global_min_xo = site_global_max_xo;
  if (site_global_max_xo < site_global_min_xo)
    site_global_max_xo = site_global_min_xo;
  if (site_global_min_yo > site_global_max_yo)
    site_global_min_yo = site_global_max_yo;
  if (site_global_max_yo < site_global_min_yo)
    site_global_max_yo = site_global_min_yo;

  //Iterate over the site coordinates

  tuple<unsigned char,unsigned char,unsigned char> rgb_pixel_color(pixel_R,
                                                                   pixel_G,
                                                                   pixel_B);

  for (int y_it = site_global_max_yo;    y_it >= site_global_min_yo;  --y_it)
    for (int x_it = site_global_min_xo;  x_it <= site_global_max_xo;  ++x_it)
      if ((x_it == site_global_min_xo) ||
          (x_it == site_global_max_xo) ||
          (y_it == site_global_min_yo) ||
          (y_it == site_global_max_yo))
      {
        // We're in the border
        map->write_embark_pixel(x_it,              // Embark coordinate x (0..15)
                                y_it,              // Embark coordinate y (0..15)
                                rgb_pixel_color);  // Pixel color
      }
}


//----------------------------------------------------------------------------//
//
//----------------------------------------------------------------------------//
void draw_site_rectangle_filled_offseted(ExportedMapBase* map,        // The map where we draw
                                         df::world_site*  world_site, // The site to be draw
                                         unsigned char    pixel_R,    // Color to be used
                                         unsigned char    pixel_G,
                                         unsigned char    pixel_B,
                                         int              offset      // in pixels respect to the site border
                                         )
{
  // Get the site boundary in world tiles
  int site_global_min_x = world_site->global_min_x;
  int site_global_max_x = world_site->global_max_x;
  int site_global_min_y = world_site->global_min_y;
  int site_global_max_y = world_site->global_max_y;

  // Offset them

  int site_global_min_xo = std::min(site_global_min_x + offset, site_global_max_x - offset);
  int site_global_max_xo = std::max(site_global_min_x + offset, site_global_max_x - offset);
  int site_global_min_yo = std::min(site_global_min_y + offset, site_global_max_y - offset);
  int site_global_max_yo = std::max(site_global_min_y + offset, site_global_max_y - offset);

  // Check limits
  if (site_global_min_xo > site_global_max_xo)
    site_global_min_xo = site_global_max_xo;
  if (site_global_max_xo < site_global_min_xo)
    site_global_max_xo = site_global_min_xo;
  if (site_global_min_yo > site_global_max_yo)
    site_global_min_yo = site_global_max_yo;
  if (site_global_max_yo < site_global_min_yo)
    site_global_max_yo = site_global_min_yo;

  //Iterate over the site coordinates

  tuple<unsigned char,unsigned char,unsigned char> rgb_pixel_color(pixel_R,
                                                                   pixel_G,
                                                                   pixel_B);

  for (int y_it = site_global_max_yo;    y_it >= site_global_min_yo;  --y_it)
    for (int x_it = site_global_min_xo;  x_it <= site_global_max_xo;  ++x_it)
        map->write_embark_pixel(x_it,              // Embark coordinate x (0..15)
                                y_it,              // Embark coordinate y (0..15)
                                rgb_pixel_color);  // Pixel color
}
