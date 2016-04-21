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

#include "../../include/dfhack.h"
#include "DFHackVersion.h"
#include "modules/Filesystem.h"

using namespace std;

static unsigned int address_Windows = 0x0BFC780 ; // Default for DF42.06
static unsigned int address_Linux   = 0x90B71F0 ; // Default for DF42.06
static unsigned int address_Mac     = 0x107D770 ; // Default for DF42.06




/*****************************************************************************
Local functions forward declaration
*****************************************************************************/

int init_linux();

void fill_region_details_Linux_OSX(unsigned int address_DF_sub,
                                   void* addres_world_region_details,
                                   int world_pos_x,
                                   int world_pos_y
                                   );

void fill_region_details_Windows(unsigned int address_DF_sub,
                                 void* addres_world_region_details,
                                 int world_pos_x,
                                 int world_pos_y
                                 );

/**************************************************************************
 Main function
 This function calls a DF subroutine with a [x,y] world position as
 arguments.
 The DF subroutine append a new entry in df::global::world.world_data.region_details array
 initialized with the world data associated to this world position.

 The routine calls one version for Linux and another for Windows
**************************************************************************/

int fill_world_region_details(int world_pos_x,
                              int world_pos_y
                              )
{

  // Below is the address of DF42.04 fill_region_details for different OSs
#ifdef _LINUX // Linux
  unsigned int address_DF_sub = address_Linux;

#endif

#ifdef _DARWIN // MacOS X
  unsigned int address_DF_sub = address_Mac;
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
  unsigned int address_DF_sub = address_Windows; // For 42.04 Windows
#endif

  // Locate fill world.world_data.region_details vector address
  df::world_data* ptr = df::global::world->world_data;

  // Get the address of world_data->region_details vector
  int result = df::global::world->world_data->region_details.size();
  std::vector<df::world_region_details*>* ptr_rd = &(ptr->region_details);
  void* pv = (void*)ptr_rd;

#if defined(_LINUX) || defined(_DARWIN) // Linux & Mac
  fill_region_details_Linux_OSX(address_DF_sub,
                                pv,
                                world_pos_x,
                                world_pos_y
                                );
#endif // Linux & Mac

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
  fill_region_details_Windows(address_DF_sub,
                              pv,
                              world_pos_x,
                              world_pos_y
                              );
#endif // WINDOWS

  return result;
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void fill_region_details_Linux_OSX(unsigned int address_DF_sub,
                                   void*        address_world_region_details,
                                   int          world_pos_x,
                                   int          world_pos_y
                                   )
{
  #if defined(_LINUX) || defined(_DARWIN)


  if ((world_pos_x != 256) &&
      (world_pos_y != 256)
      )
  {
    // For all world types except for the large one (size = 257 world coordinates)

    // Setup a thiscall arranged as cdecl call for passing the following parameters to the
    // routine (world.world_data.region_details, coord_x, coord_y, 0, 0)
    // Then call DF routine

    asm volatile ("movl %0    ,%%eax;    "  /* address_DF_sub to eax                                                 */
                  "movl %1    ,%%ecx;    "  /* address of world.world_data.region_details to ecx                     */
                  "mov  %2    ,%%dh;     "  /* position_x to dh                                                      */
                  "mov  %3    ,%%dl;     "  /* position_y to dl                                                      */
                  "sub  $0x18 ,%%esp;    "  /* make space in the heap for the parameters                             */
                  "mov  %%ecx ,(%%esp);  "  /* store param 1                                                         */
                  "movl $0    ,4(%%esp); "  /* the heap could have any 32bit value and whe use 16bit, so init zero   */
                  "mov  %%dh  ,4(%%esp); "  /* set param 2                                                           */
                  "movl $0    ,8(%%esp); "  /* the heap could have any 32bit value and whe use 16bit, so init zero   */
                  "mov  %%dl  ,8(%%esp); "  /* set param 3                                                           */
                  "mov  $0    ,%%ecx;    "  /* the rest of parameters are 0                                          */
                  "mov  %%ecx ,12(%%esp);"  /* set param 4                                                           */
                  "mov  %%ecx ,16(%%esp);"  /* set param 5                                                           */
                  "call *%%eax;          "  /* call the DF subroutine                                                */
                  "add  $0x18,%%esp;     "  /* release the space used in the heap for the parameters                 */
                  :
                  : "m" (address_DF_sub),
                    "m" (address_world_region_details),
                    "m" (world_pos_x),
                    "m" (world_pos_y)       /* input parameters                                                      */
                  : "eax", "ecx", "edx"     /* used registers                                                        */
                  );
    return;
  }

  if ((world_pos_x == 256) &&
      (world_pos_y != 256)
      )
  {
    // Large world. Coordinate X = 256. Coordinate Y in 0..255

    asm volatile ("movl %0     ,%%eax;    "  /* address_DF_sub to eax                                                 */
                  "movl %1     ,%%ecx;    "  /* address of world.world_data.region_details to ecx                     */
                  "mov  %2     ,%%dh;     "  /* position_x to dh                                                      */
                  "mov  %3     ,%%dl;     "  /* position_y to dl                                                      */
                  "sub  $0x18  ,%%esp;    "  /* make space in the heap for the parameters                             */
                  "mov  %%ecx  ,(%%esp);  "  /* store param 1                                                         */
                  "movl $0     ,4(%%esp); "  /* the heap could have any 32bit value and whe use 16bit, so init zero   */
                  "mov  %%dh   ,4(%%esp); "  /* set param 2 = 256                                                     */
                  "orl  $0x100 ,4(%%esp); "  /* set param 2 = 256                                                     */
                  "movl $0     ,8(%%esp); "  /* the heap could have any 32bit value and whe use 16bit, so init zero   */
                  "mov  %%dl   ,8(%%esp); "  /* set param 3                                                           */
                  "mov  $0     ,%%ecx;    "  /* the rest of parameters are 0                                          */
                  "mov  %%ecx  ,12(%%esp);"  /* set param 4                                                           */
                  "mov  %%ecx  ,16(%%esp);"  /* set param 5                                                           */
                  "call *%%eax;           "  /* call the DF subroutine                                                */
                  "add  $0x18  ,%%esp;    "  /* release the space used in the heap for the parameters                 */
                  :
                  : "m" (address_DF_sub),
                    "m" (address_world_region_details),
                    "m" (world_pos_x),
                    "m" (world_pos_y)       /* input parameters                                                       */
                  : "eax", "ecx", "edx"     /* used registers                                                         */
                  );
    return;
  }

  if ((world_pos_x != 256) &&
      (world_pos_y == 256)
      )
  {
    // Large world. Coordinate Y = 256. Coordinate X in 0..255

    asm volatile ("movl %0     ,%%eax;    "  /* address_DF_sub to eax                                                 */
                  "movl %1     ,%%ecx;    "  /* address of world.world_data.region_details to ecx                     */
                  "mov  %2     ,%%dh;     "  /* position_x to dh                                                      */
                  "mov  %3     ,%%dl;     "  /* position_y to dl                                                      */
                  "sub  $0x18  ,%%esp;    "  /* make space in the heap for the parameters                             */
                  "mov  %%ecx  ,(%%esp);  "  /* store param 1                                                         */
                  "movl $0     ,4(%%esp); "  /* the heap could have any 32bit value and whe use 16bit, so init zero   */
                  "mov  %%dh   ,4(%%esp); "  /* set param 2                                                           */
                  "movl $0     ,8(%%esp); "  /* the heap could have any 32bit value and whe use 16bit, so init zero   */
                  "mov  %%dl   ,8(%%esp); "  /* set param 3 = 256                                                     */
                  "orl  $0x100 ,8(%%esp); "  /* set param 3 = 256                                                     */
                  "mov  $0     ,%%ecx;    "  /* the rest of parameters are 0                                          */
                  "mov  %%ecx  ,12(%%esp);"  /* set param 4                                                           */
                  "mov  %%ecx  ,16(%%esp);"  /* set param 5                                                           */
                  "call *%%eax;           "  /* call the DF subroutine                                                */
                  "add  $0x18  ,%%esp;    "  /* release the space used in the heap for the parameters                 */
                  :
                  : "m" (address_DF_sub),
                    "m" (address_world_region_details),
                    "m" (world_pos_x),
                    "m" (world_pos_y)       /* input parameters                                                       */
                  : "eax", "ecx", "edx"     /* used registers                                                         */
                  );
    return;
  }

  if ((world_pos_x == 256) &&
      (world_pos_y == 256)
      )
  {
    // Large world. Coordinate X = 256. Coordinate Y = 256

    asm volatile ("movl %0     ,%%eax;    "  /* address_DF_sub to eax                                                 */
                  "movl %1     ,%%ecx;    "  /* address of world.world_data.region_details to ecx                     */
                  "mov  %2     ,%%dh;     "  /* position_x to dh                                                      */
                  "mov  %3     ,%%dl;     "  /* position_y to dl                                                      */
                  "sub  $0x18  ,%%esp;    "  /* make space in the heap for the parameters                             */
                  "mov  %%ecx  ,(%%esp);  "  /* store param 1                                                         */
                  "movl $0     ,4(%%esp); "  /* the heap could have any 32bit value and whe use 16bit, so init zero   */
                  "mov  %%dh   ,4(%%esp); "  /* set param 2 = 256                                                     */
                  "orl  $0x100 ,4(%%esp); "  /* set param 2 = 256                                                     */
                  "movl $0     ,8(%%esp); "  /* the heap could have any 32bit value and whe use 16bit, so init zero   */
                  "mov  %%dl   ,8(%%esp); "  /* set param 3 = 256                                                     */
                  "orl  $0x100 ,8(%%esp); "  /* set param 3 = 256                                                     */
                  "mov  $0     ,%%ecx;    "  /* the rest of parameters are 0                                          */
                  "mov  %%ecx  ,12(%%esp);"  /* set param 4                                                           */
                  "mov  %%ecx  ,16(%%esp);"  /* set param 5                                                           */
                  "call *%%eax;           "  /* call the DF subroutine                                                */
                  "add  $0x18  ,%%esp;    "  /* release the space used in the heap for the parameters                 */
                  :
                  : "m" (address_DF_sub),
                    "m" (address_world_region_details),
                    "m" (world_pos_x),
                    "m" (world_pos_y)       /* input parameters                                                       */
                  : "eax", "ecx", "edx"     /* used registers                                                         */
                  );
    return;
  }

  #endif
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void fill_region_details_Windows(unsigned int address_DF_sub,
                                 void*        address_world_region_details,
                                 int          world_pos_x,
                                 int          world_pos_y
                                 )
{
  // Windows DF has Address Space Randomization active, so the virtual address
  // doesn't have to be the same every time DF is running.
  // DFHack solves this for us
  unsigned int delta = DFHack::Core::getInstance().vinfo->getRebaseDelta();
  address_DF_sub += delta; // Corrected subroutine address

  #if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    // setup a __fastcall for passing the following parameters to the
    // routine(world.world_data.region_details, coord_x, coord_y, 0, 0)
    // Then call DF routine
    __asm xor  eax, eax                          /* eax = 0                       */
    __asm push eax                               /* 5th parameter to the stack    */
    __asm push eax                               /* 4th parameter to the stack    */
    __asm mov  eax, world_pos_y                  /* 3th parameter to the stack    */
    __asm push eax
    __asm mov  edx, world_pos_x                  /* 2nd parameter in register edx */
    __asm mov  ecx, address_world_region_details /* 1st parameter in register ecx */
    __asm mov  eax, address_DF_sub               /* eax = address DF subroutine   */
    __asm call address_DF_sub                    /* call the DF subroutine        */

#endif // WINDOWS
}
