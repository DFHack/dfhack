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

#include "../../include/Mac_compat.h"
#include "../../include/dfhack.h"


using namespace std;

/*****************************************************************************
Module local variables

*****************************************************************************/
static unsigned int address_Windows = 0x6FE600; // Default for DF42.06
static unsigned int address_Linux   = 0x0;      // Not used in Linux
static unsigned int address_Mac     = 0x0;      // Not used in OSX

/*****************************************************************************
External functions
*****************************************************************************/
extern void delete_world_region_details(df::world_region_details*);

/*****************************************************************************
Local functions forward declaration
*****************************************************************************/
void delete_world_region_details_vector_Windows();
void delete_world_region_details_vector_Linux_OSX();


/**************************************************************************
 Main function

**************************************************************************/
void delete_world_region_details_vector()
{
  #if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
  delete_world_region_details_vector_Windows();
  #endif // Windows

  #if defined(LINUX_BUILD) || defined(_DARWIN)
  delete_world_region_details_vector_Linux_OSX();
  #endif // Linux and Mac
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void delete_world_region_details_vector_Windows()
{
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)

  // Adjust the real address
  unsigned int delta = DFHack::Core::getInstance().vinfo->getRebaseDelta();
  unsigned int address_DF_sub = address_Windows + delta;

  // Call DF function
  unsigned int vector_address = (unsigned int)&(df::global::world->world_data->region_details);
  __asm mov  ebx, vector_address
  __asm call address_DF_sub                    /* call the DF subroutine */

#endif
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void delete_world_region_details_vector_Linux_OSX()
{
#if defined(LINUX_BUILD) || defined(_DARWIN)

  int num_elements_vector = df::global::world->world_data->region_details.size();
  for (int l = num_elements_vector - 1; l >= 0 ; l--)
  {
    df::world_region_details* rd = df::global::world->world_data->region_details[l];
    if (rd != nullptr)
      delete_world_region_details(rd);
  }

  // Delete all entries in world_region_details vector
  df::global::world->world_data->region_details.erase(df::global::world->world_data->region_details.begin(),
                                                      df::global::world->world_data->region_details.end());
#endif
}



