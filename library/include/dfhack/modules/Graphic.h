/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

/*******************************************************************************
                                    GRAPHIC
                            Changing tile cache
*******************************************************************************/
#pragma once
#ifndef CL_MOD_GRAPHIC
#define CL_MOD_GRAPHIC

#include <stdint.h>
#include "dfhack/Export.h"
#include "dfhack/Module.h"

namespace DFHack
{
	// SDL stuff
	typedef signed short SINT16;
	typedef struct
	{
		int16_t x, y;
		uint16_t w, h;
	} DFSDL_Rect;
	typedef struct
	{
		uint32_t flags;
		void* format; // PixelFormat*
		int w, h;
		int pitch;
		void* pixels;
		void* userdata; // as far as i could see DF doesnt use this
		int locked;
		void* lock_data;
		DFSDL_Rect clip_rect;
		void* map;
		int refcount;
	} DFSDL_Surface;

	// =========
	struct DFTileSurface
	{
		bool paintOver; // draw over original tile?
		DFSDL_Surface* surface; // from where it should be drawn
		DFSDL_Rect* rect; // from which coords (NULL to draw whole surface)
		DFSDL_Rect* dstResize; // if not NULL dst rect will be resized (x/y/w/h will be added to original dst)
	};


	class DFHACK_EXPORT Graphic : public Module
	{
		public:
			Graphic();
			~Graphic();
			bool Finish()
			{
				return true;
			}
			bool Register(DFTileSurface* (*func)(int,int));
			bool Unregister(DFTileSurface* (*func)(int,int));
			DFTileSurface* Call(int x, int y);

		private:
			struct Private;
			Private *d;
	};

}

#endif