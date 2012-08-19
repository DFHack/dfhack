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

#pragma once
#include "Export.h"
#include "Module.h"
#include "BitArray.h"
#include "ColorText.h"
#include <string>

#include "DataDefs.h"
#include "df/graphic.h"
#include "df/viewscreen.h"

/**
 * \defgroup grp_screen utilities for painting to the screen
 * @ingroup grp_screen
 */

namespace DFHack
{
    class Core;

    /**
     * The Screen module
     * \ingroup grp_modules
     * \ingroup grp_screen
     */
    namespace Screen
    {
        /// Data structure describing all properties a screen tile can have
        struct Pen {
            // Ordinary text symbol
            char ch;
            int8_t fg, bg;
            bool bold;

            // Graphics tile
            int tile;
            enum TileMode {
                AsIs,      // Tile colors used without modification
                CharColor, // The fg/bg pair is used
                TileColor  // The fields below are used
            } tile_mode;
            int8_t tile_fg, tile_bg;

            Pen(char ch = 0, int8_t fg = 7, int8_t bg = 0, int tile = 0, bool color_tile = false)
              : ch(ch), fg(fg&7), bg(bg), bold(!!(fg&8)),
                tile(tile), tile_mode(color_tile ? CharColor : AsIs), tile_fg(0), tile_bg(0)
            {}
            Pen(char ch, int8_t fg, int8_t bg, bool bold, int tile = 0, bool color_tile = false)
              : ch(ch), fg(fg), bg(bg), bold(bold),
                tile(tile), tile_mode(color_tile ? CharColor : AsIs), tile_fg(0), tile_bg(0)
            {}
            Pen(char ch, int8_t fg, int8_t bg, int tile, int8_t tile_fg, int8_t tile_bg)
              : ch(ch), fg(fg&7), bg(bg), bold(!!(fg&8)),
                tile(tile), tile_mode(TileColor), tile_fg(tile_fg), tile_bg(tile_bg)
            {}
            Pen(char ch, int8_t fg, int8_t bg, bool bold, int tile, int8_t tile_fg, int8_t tile_bg)
              : ch(ch), fg(fg), bg(bg), bold(bold),
                tile(tile), tile_mode(TileColor), tile_fg(tile_fg), tile_bg(tile_bg)
            {}
        };

        DFHACK_EXPORT df::coord2d getMousePos();
        DFHACK_EXPORT df::coord2d getWindowSize();

        /// Paint one screen tile with the given pen
        DFHACK_EXPORT bool paintTile(const Pen &pen, int x, int y);

        /// Paint a string onto the screen. Ignores ch and tile of pen.
        DFHACK_EXPORT bool paintString(const Pen &pen, int x, int y, const std::string &text);

        /// Fills a rectangle with one pen. Possibly more efficient than a loop over paintTile.
        DFHACK_EXPORT bool fillRect(const Pen &pen, int x1, int y1, int x2, int y2);

        /// Find a loaded graphics tile from graphics raws.
        DFHACK_EXPORT bool findGraphicsTile(const std::string &page, int x, int y, int *ptile, int *pgs = NULL);

        // Push and remove viewscreens
        DFHACK_EXPORT bool show(df::viewscreen *screen, df::viewscreen *before = NULL);
        DFHACK_EXPORT void dismiss(df::viewscreen *screen);
        DFHACK_EXPORT bool isDismissed(df::viewscreen *screen);
    }

    class DFHACK_EXPORT dfhack_viewscreen : public df::viewscreen {
    public:
        dfhack_viewscreen();
        virtual ~dfhack_viewscreen();

        static bool is_instance(df::viewscreen *screen);

        virtual std::string getFocusString() = 0;
    };
}
