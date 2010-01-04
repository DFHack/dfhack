/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

/**
 * This is the source for the DF <-> dfhack shm bridge
 */
#include "shms.h"
extern char *shm;
// SDL functions used in 40d16
/*
SDL_AddTimer
SDL_CondSignal
SDL_CondWait
SDL_ConvertSurface
SDL_CreateCond
SDL_CreateMutex
SDL_CreateRGBSurface
SDL_CreateRGBSurfaceFrom
SDL_DestroyCond
SDL_DestroyMutex
SDL_EnableKeyRepeat
SDL_EnableUNICODE
SDL_FreeSurface
SDL_GL_GetAttribute
SDL_GL_SetAttribute
SDL_GL_SwapBuffers
SDL_GetError
SDL_GetKeyState
SDL_GetTicks
SDL_GetVideoInfo
SDL_Init
SDL_LockSurface
SDL_MapRGB
SDL_PollEvent
SDL_Quit
SDL_RWFromFile
SDL_RemoveTimer
SDL_SaveBMP_RW
SDL_SetAlpha
SDL_SetColorKey
SDL_SetModuleHandle
SDL_SetVideoMode
SDL_ShowCursor
SDL_UnlockSurface
SDL_UpperBlit
SDL_WM_SetCaption
SDL_WM_SetIcon
SDL_mutexP
SDL_strlcpy
*/

// TO BE DONE