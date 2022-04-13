/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

// Fake - only structs. Shamelessly pilfered from the SDL library.
// Needed for processing its event types without polluting our namespaces with C garbage

#pragma once
#include "SDL_keysym.h"
#include <stdint.h>

namespace SDL
{
    /** Keysym structure
     *
     *  - The scancode is hardware dependent, and should not be used by general
     *    applications.  If no hardware scancode is available, it will be 0.
     *
     *  - The 'unicode' translated character is only available when character
     *    translation is enabled by the SDL_EnableUNICODE() API.  If non-zero,
     *    this is a UNICODE character corresponding to the keypress.  If the
     *    high 9 bits of the character are 0, then this maps to the equivalent
     *    ASCII character:
     *      @code
     *  char ch;
     *  if ( (keysym.unicode & 0xFF80) == 0 ) {
     *      ch = keysym.unicode & 0x7F;
     *  } else {
     *      An international character..
     *  }
     *      @endcode
     */
    typedef struct keysym
    {
        uint8_t scancode;       /**< hardware specific scancode */
        Key sym;             /**< SDL virtual keysym */
        Mod mod;             /**< current key modifiers */
        uint16_t unicode;         /**< translated character */
    } keysym;

    /** This is the mask which refers to all hotkey bindings */
    #define ALL_HOTKEYS     0xFFFFFFFF
}
