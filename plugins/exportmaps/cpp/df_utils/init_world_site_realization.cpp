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

/*****************************************************************************
Module local variables

*****************************************************************************/
static unsigned int address_Windows = 0x0CAE3E0 ; // Default for DF42.06
static unsigned int address_Linux   = 0x9087470 ; // Default for DF42.06
static unsigned int address_Mac     = 0x1058A70 ; // Default for DF42.06


/*****************************************************************************
Local functions forward declaration
*****************************************************************************/

void init_world_site_realization_Linux_OSX(unsigned int address_DF_sub,
                                           df::world_site* world_site
                                           );


void init_world_site_realization_Windows(unsigned int address_DF_sub,
                                         df::world_site* world_site
                                         );

/**************************************************************************
 Main function
 This function calls a DF subroutine with a df::world_site parameter.

 The DF sub dows the hard work of creating and initializing all the
 structures that make a world site realization

 The routine calls one version for Linux and another for Windows
**************************************************************************/

void init_world_site_realization(df::world_site* world_site)
{

  // Below is the address of the subroutine for different OSs
#ifdef _LINUX // Linux
  unsigned int address_DF_sub = address_Linux;

#endif

#ifdef _DARWIN // MacOS X
  unsigned int address_DF_sub = address_Mac;
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
  unsigned int address_DF_sub = address_Windows;
#endif


#if defined(_DARWIN) // Mac
  init_world_site_realization_Linux_OSX(address_DF_sub,
                                        world_site
                                        );
#endif // Mac

#if defined(_LINUX) // Linux
  init_world_site_realization_Linux_OSX(address_DF_sub,
                                        world_site
                                        );
#endif // Linux

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
  init_world_site_realization_Windows(address_DF_sub,
                                      world_site
                                      );
#endif // WINDOWS

}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void init_world_site_realization_Linux_OSX(unsigned int    address_DF_sub,
                                           df::world_site* world_site
                                           )
{
    #if defined(_LINUX) || defined(_DARWIN)

  // Setup the stack and then call DF routine

  // About the strange 0x18 stack size when only 2 parameters are used
  // As DF for linux use MMX instructions, the parameters
  // must be aligned to a 16 byte frame.  If not, the call
  // to DF crashes insive a MOVAPS assembly instruction

  asm volatile ("movl %0    ,%%eax;    "  /* address_DF_sub to eax                                  */
                "movl %1    ,%%ecx;    "  /* pointer to world_site to ecx                           */
                "sub  $0x18 ,%%esp;    "  /* make space in the stack for the parameters             */
                "mov  %%ecx ,0(%%esp); "  /* store param 1                                          */
                "movl $0    ,4(%%esp); "  /* store param 2                                          */
                "call *%%eax;          "  /* call the DF subroutine                                 */
                "add  $0x18 ,%%esp;    "  /* release the space used in the stack for the parameters */
                :
                : "m" (address_DF_sub),   /* input parameter                                         */
                  "m" (world_site)        /* input parameter                                         */
                : "eax","ecx"             /* used registers                                          */
                );

    #endif
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void init_world_site_realization_Windows(unsigned int address_DF_sub,
                                         df::world_site* world_site
                                         )
{
  // Windows DF has Address Space Randomization active, so the virtual address
  // doesn't have to be the same every time DF is running.
  // DFHack solves this for us
  unsigned int delta = DFHack::Core::getInstance().vinfo->getRebaseDelta();
  unsigned int address_DF_sub_Win = address_DF_sub + delta; // Corrected subroutine address

  #if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)

  __asm xor  eax, eax                          /* eax = 0                                       */
  __asm push eax                               /* 2nd parameter to the stack = 0                */
  __asm push world_site                        /* 1st parameter to the stack = df_world_site*   */
  __asm mov  eax, address_DF_sub_Win               /* eax = address DF subroutine                   */
  __asm call address_DF_sub_Win                    /* call DF subroutine                            */

#endif // WINDOWS
}
