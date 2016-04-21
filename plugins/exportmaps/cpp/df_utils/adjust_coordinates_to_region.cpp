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

#include <tuple>

std::pair<int,int> adjust_coordinates_to_region(int x,
                                                int y,
                                                int delta,
                                                int pos_x,
                                                int pos_y,
                                                int world_width,
                                                int world_height
                                                )
{
  if ((x < 0) || (y < 0) || (x > 16) || (y > 16))
    return std::pair<int,int>(-1,-1); // invalid coordinates

  int adjusted_x = pos_x;
  int adjusted_y = pos_y;

  switch (delta)
  {
      case 1: // SW
              --adjusted_x; ++adjusted_y; break;
      case 2: // S
                            ++adjusted_y; break;
      case 3: // SE
              ++adjusted_x; ++adjusted_y; break;
      case 4: // W
              --adjusted_x;               break;
      case 5: // Center
                                          break;
      case 6: // E
              ++adjusted_x;               break;
      case 7: // NW
              --adjusted_x; --adjusted_y; break;
      case 8: // N
                            --adjusted_y; break;
      case 9: // NE
              ++adjusted_x; --adjusted_y; break;
  }

  // Check if it's out ot bounds
  if (adjusted_x < 0)
    adjusted_x = 0;

  if (adjusted_x >= world_width)
    adjusted_x = world_width - 1;

  if (adjusted_y < 0)
    adjusted_y = 0;

  if (adjusted_y >= world_height)
    adjusted_y = world_height - 1;

  return std::pair<int,int>(adjusted_x,adjusted_y);
}
