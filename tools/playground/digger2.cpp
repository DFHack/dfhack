/**
 * @file digger2.cpp
 * @author rOut
 * 
 * Improved digger tool.
 *
 * Takes a text file as first an only argument.
 * The text file is read as a grid, and every character represents a designation for a tile.
 * Allowed characters are 'd' for dig, 'u' for up stairs, 'j' for down stairs, 'i' for up and down stairs, 'h' for channel, 'r' for upward ramp and 'x' to remove designation.
 * Other characters don't do anything and can be used for padding.
 * The designation pattern is the wrote in game memory, centered on the current cursor position. Thus, the game needs to be in designation mode or, perhaps, any other mode that have a cursor.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <cstdlib>
#include <algorithm>
#include <assert.h>
using namespace std;

#include <DFHack.h>
#include <DFTileTypes.h>
#define BLOCK_SIZE 16


void dig(DFHack::Maps* layers, DFHack::Gui* Gui, ::std::vector< ::std::string >& dig_map, bool verbose = false)
{
  int32_t x_cent;
  int32_t y_cent;
  int32_t z_cent;
  Gui->getCursorCoords(x_cent, y_cent, z_cent);

// ::std::cout << "x_cent: " << x_cent << " y_cent: " << y_cent << " z_cent: " << z_cent << ::std::endl;

  int32_t z_from = z_cent;
  int32_t z = 0;

  uint32_t x_max;
  uint32_t y_max;
  uint32_t z_max;
  layers->getSize(x_max, y_max, z_max);

// ::std::cout << "x_max: " << x_max << " y_max: " << y_max << " z_max: " << z_max << ::std::endl;

  int32_t dig_height = dig_map.size();
  int32_t y_from = y_cent - (dig_height / 2);
// ::std::cout << "dig_height: " << dig_height << ::std::endl;
// ::std::cout << "y_from: " << y_from << ::std::endl;

  int32_t y = 0;
  DFHack::designations40d designations;
  DFHack::tiletypes40d tiles;
  ::std::vector< ::std::string >::iterator str_it;
  for (str_it = dig_map.begin(); str_it != dig_map.end(); ++str_it) {
    int32_t dig_width = str_it->size();
    int32_t x_from = x_cent - (dig_width / 2);

// ::std::cout << "x_cent: " << x_cent << " y_cent: " << y_cent << " z_cent: " << z_cent << ::std::endl;
// ::std::cout << "dig_width: " << dig_width << ::std::endl;
// ::std::cout << "x_from: " << x_from << ::std::endl;

    int32_t x = 0;
    ::std::string::iterator chr_it;
    for (chr_it = str_it->begin(); chr_it != str_it ->end(); ++chr_it)
    {
      int32_t x_grid = (x_from + x) / BLOCK_SIZE;
      int32_t y_grid = (y_from + y) / BLOCK_SIZE;
      int32_t z_grid = z_from + z;
      int32_t x_locl = (x_from + x) - x_grid * BLOCK_SIZE;
      int32_t y_locl = (y_from + y) - y_grid * BLOCK_SIZE;
      int32_t z_locl = 0;

      if (x_grid >= 0 && y_grid >= 0 && x_grid < x_max && y_grid < y_max)
      {
        // TODO this could probably be made much better, theres a big chance the trees are on the same grid
        layers->ReadDesignations(x_grid, y_grid, z_grid, &designations);
        layers->ReadTileTypes(x_grid, y_grid, z_grid, &tiles);

// ::std::cout << ::std::hex << "designations: " << designations[x_locl][y_locl].bits.dig << ::std::dec << ::std::endl;
        DFHack::naked_designation & des = designations[x_locl][y_locl].bits;
        if ( DFHack::tileShape(tiles[x_locl][y_locl]) == DFHack::WALL)
        {
          switch ((char) *chr_it)
          {
            case 'd':
              des.dig = DFHack::designation_default;
              break;
            case 'u':
              des.dig = DFHack::designation_u_stair;
              break;
            case 'j':
              des.dig = DFHack::designation_d_stair;
              break;
            case 'i':
              des.dig = DFHack::designation_ud_stair;
              break;
            case 'h':
              des.dig = DFHack::designation_channel;
              break;
            case 'r':
              des.dig = DFHack::designation_ramp;
              break;
            case 'x':
              des.dig = DFHack::designation_no;
              break;
          }

          if (verbose)
          {
            ::std::cout << "designating " << (char) *chr_it << " at " << x_from + x << " " << y_from + y << " " << z_from + z << ::std::endl;
          }

          layers->WriteDesignations(x_grid, y_grid, z_grid, &designations);

          // Mark as dirty so the jobs are properly picked up by the dwarves
          layers->WriteDirtyBit(x_grid, y_grid, z_grid, true);
        }
      }

      ++x;
    }
    ++y;
  }
}

int main(int argc, char** argv) {
  if(argc < 2) {
    ::std::cout << "gimme a file!" << ::std::endl;
    return 1;
  }

  ::std::ifstream map_in(argv[1]);

  ::std::vector< ::std::string > dig_map;
  while (map_in.good() && !map_in.eof() && !map_in.bad()) {
    ::std::string line;
    map_in >> line;

    dig_map.push_back(line);
  }
  dig_map.resize(dig_map.size() - 1);

  DFHack::ContextManager DFMgr("Memory.xml");
  DFHack::Context * DF = DFMgr.getSingleContext();

  try {
    DF->Attach();
  } catch (::std::exception& e) {
    ::std::cerr << e.what() << ::std::endl;
#ifndef LINUX_BUILD
    ::std::cin.ignore();
#endif
    return 1;
  }

  DFHack::Maps *layers = DF->getMaps();
  if (layers && layers->Start()) {

    dig(layers, DF->getGui(), dig_map, true);

    ::std::cout << "Finished digging" << ::std::endl;
    layers->Finish();

    if (!DF->Detach()) {
      ::std::cerr << "Unable to detach DF process" << ::std::endl;
    }

  } else {
    ::std::cerr << "Unable to init map" << ::std::endl;
  }

#ifndef LINUX_BUILD
  ::std::cout << "Done. Press any key to continue" << ::std::endl;
  ::std::cin.ignore();
#endif
  return 0;
}
