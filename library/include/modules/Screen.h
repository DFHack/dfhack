/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include "Types.h"

#include <string>
#include <set>

#include "DataDefs.h"
#include "df/graphic.h"
#include "df/viewscreen.h"

#include "modules/GuiHooks.h"

namespace df
{
    struct job;
    struct item;
    struct unit;
    struct building;
}

/**
 * \defgroup grp_screen utilities for painting to the screen
 * @ingroup grp_screen
 */

namespace DFHack
{
    class Core;

    typedef std::set<df::interface_key> interface_key_set;

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

            bool valid() const { return tile >= 0; }
            bool empty() const { return ch == 0 && tile == 0; }

            // NOTE: LuaApi.cpp assumes this struct is plain data and has empty destructor

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

            void adjust(int8_t nfg) { fg = nfg&7; bold = !!(nfg&8); }
            void adjust(int8_t nfg, bool nbold) { fg = nfg; bold = nbold; }
            void adjust(int8_t nfg, int8_t nbg) { adjust(nfg); bg = nbg; }
            void adjust(int8_t nfg, bool nbold, int8_t nbg) { adjust(nfg, nbold); bg = nbg; }

            Pen color(int8_t nfg) const { Pen cp(*this); cp.adjust(nfg); return cp; }
            Pen color(int8_t nfg, bool nbold) const { Pen cp(*this); cp.adjust(nfg, nbold); return cp; }
            Pen color(int8_t nfg, int8_t nbg) const { Pen cp(*this); cp.adjust(nfg, nbg); return cp; }
            Pen color(int8_t nfg, bool nbold, int8_t nbg) const { Pen cp(*this); cp.adjust(nfg, nbold, nbg); return cp; }

            Pen chtile(char ch) { Pen cp(*this); cp.ch = ch; return cp; }
            Pen chtile(char ch, int tile) { Pen cp(*this); cp.ch = ch; cp.tile = tile; return cp; }
        };

        class DFHACK_EXPORT PenArray {
            Pen *buffer;
            unsigned int dimx;
            unsigned int dimy;
            bool static_alloc;
        public:
            PenArray(unsigned int bufwidth, unsigned int bufheight);
            PenArray(unsigned int bufwidth, unsigned int bufheight, void *buf);
            ~PenArray();
            void clear();
            unsigned int get_dimx() { return dimx; }
            unsigned int get_dimy() { return dimy; }
            Pen get_tile(unsigned int x, unsigned int y);
            void set_tile(unsigned int x, unsigned int y, Screen::Pen pen);
            void draw(unsigned int x, unsigned int y, unsigned int width, unsigned int height,
                unsigned int bufx = 0, unsigned int bufy = 0);
        };

        struct DFHACK_EXPORT ViewRect {
            rect2d view, clip;

            ViewRect(rect2d area) : view(area), clip(area) {}
            ViewRect(rect2d area, rect2d clip) : view(area), clip(clip) {}

            bool isDefunct() const {
                return clip.first.x > clip.second.x || clip.first.y > clip.second.y;
            }
            int width() const { return view.second.x-view.first.x+1; }
            int height() const { return view.second.y-view.first.y+1; }
            df::coord2d local(df::coord2d pos) const {
                return df::coord2d(pos.x - view.first.x, pos.y - view.first.y);
            }
            df::coord2d global(df::coord2d pos) const {
                return df::coord2d(pos.x + view.first.x, pos.y + view.first.y);
            }
            df::coord2d global(int x, int y) const {
                return df::coord2d(x + view.first.x, y + view.first.y);
            }
            bool inClipGlobal(int x, int y) const {
                return x >= clip.first.x && x <= clip.second.x &&
                       y >= clip.first.y && y <= clip.second.y;
            }
            bool inClipGlobal(df::coord2d pos) const {
                return inClipGlobal(pos.x, pos.y);
            }
            bool inClipLocal(int x, int y) const {
                return inClipGlobal(x + view.first.x, y + view.first.y);
            }
            bool inClipLocal(df::coord2d pos) const {
                return inClipLocal(pos.x, pos.y);
            }
            ViewRect viewport(rect2d area) const {
                rect2d nview(global(area.first), global(area.second));
                return ViewRect(nview, intersect(nview, clip));
            }
        };

        DFHACK_EXPORT df::coord2d getMousePos();
        DFHACK_EXPORT df::coord2d getWindowSize();

        inline rect2d getScreenRect() {
            return rect2d(df::coord2d(0,0), getWindowSize()-df::coord2d(1,1));
        }

        /// Returns the state of [GRAPHICS:YES/NO]
        DFHACK_EXPORT bool inGraphicsMode();

        /// Paint one screen tile with the given pen
        DFHACK_EXPORT bool paintTile(const Pen &pen, int x, int y, bool map = false);

        /// Retrieves one screen tile from the buffer
        DFHACK_EXPORT Pen readTile(int x, int y);

        /// Paint a string onto the screen. Ignores ch and tile of pen.
        DFHACK_EXPORT bool paintString(const Pen &pen, int x, int y, const std::string &text, bool map = false);

        /// Fills a rectangle with one pen. Possibly more efficient than a loop over paintTile.
        DFHACK_EXPORT bool fillRect(const Pen &pen, int x1, int y1, int x2, int y2, bool map = false);

        /// Draws a standard dark gray window border with a title string
        DFHACK_EXPORT bool drawBorder(const std::string &title);

        /// Wipes the screen to full black
        DFHACK_EXPORT bool clear();

        /// Requests repaint
        DFHACK_EXPORT bool invalidate();

        /// Find a loaded graphics tile from graphics raws.
        DFHACK_EXPORT bool findGraphicsTile(const std::string &page, int x, int y, int *ptile, int *pgs = NULL);

        // Push and remove viewscreens
        DFHACK_EXPORT bool show(df::viewscreen *screen, df::viewscreen *before = NULL);
        DFHACK_EXPORT void dismiss(df::viewscreen *screen, bool to_first = false);
        DFHACK_EXPORT bool isDismissed(df::viewscreen *screen);

        /// Retrieve the string representation of the bound key.
        DFHACK_EXPORT std::string getKeyDisplay(df::interface_key key);

        /// Return the character represented by this key, or -1
        DFHACK_EXPORT int keyToChar(df::interface_key key);
        /// Return the key code matching this character, or NONE
        DFHACK_EXPORT df::interface_key charToKey(char code);

        /// A painter class that implements a clipping area and cursor/pen state
        struct DFHACK_EXPORT Painter : ViewRect {
            df::coord2d gcursor;
            Pen cur_pen, cur_key_pen;

            static const Pen default_pen;
            static const Pen default_key_pen;

            Painter(const ViewRect &area, const Pen &pen = default_pen, const Pen &kpen = default_key_pen)
                : ViewRect(area), gcursor(area.view.first), cur_pen(pen), cur_key_pen(kpen)
            {}

            df::coord2d cursor() const { return local(gcursor); }
            int cursorX() const { return gcursor.x - view.first.x; }
            int cursorY() const { return gcursor.y - view.first.y; }

            bool isValidPos() const { return inClipGlobal(gcursor); }

            Painter viewport(rect2d area) const {
                return Painter(ViewRect::viewport(area), cur_pen, cur_key_pen);
            }

            Painter &seek(df::coord2d pos) { gcursor = global(pos); return *this; }
            Painter &seek(int x, int y) { gcursor = global(x,y); return *this; }
            Painter &advance(int dx) { gcursor.x += dx; return *this; }
            Painter &advance(int dx, int dy) { gcursor.x += dx; gcursor.y += dy; return *this; }
            Painter &newline(int dx = 0) { gcursor.y++; gcursor.x = view.first.x + dx; return *this; }

            const Pen &pen() const { return cur_pen; }
            Painter &pen(const Pen &np) { cur_pen = np; return *this; }
            Painter &pen(int8_t fg) { cur_pen.adjust(fg); return *this; }

            const Pen &key_pen() const { return cur_key_pen; }
            Painter &key_pen(const Pen &np) { cur_key_pen = np; return *this; }
            Painter &key_pen(int8_t fg) { cur_key_pen.adjust(fg); return *this; }

            Painter &clear() {
                fillRect(Pen(' ',0,0,false), clip.first.x, clip.first.y, clip.second.x, clip.second.y);
                return *this;
            }

            Painter &fill(const rect2d &area, const Pen &pen, bool map = false) {
                rect2d irect = intersect(area, clip);
                fillRect(pen, irect.first.x, irect.first.y, irect.second.x, irect.second.y, map);
                return *this;
            }
            Painter &fill(const rect2d &area, bool map = false) { return fill(area, cur_pen, map); }

            Painter &tile(const Pen &pen, bool map = false) {
                if (isValidPos()) paintTile(pen, gcursor.x, gcursor.y, map);
                return advance(1);
            }
            Painter &tile(bool map = false) { return tile(cur_pen, map); }
            Painter &tile(char ch, bool map = false) { return tile(cur_pen.chtile(ch), map); }
            Painter &tile(char ch, int tileid, bool map = false) { return tile(cur_pen.chtile(ch, tileid), map); }

            Painter &string(const std::string &str, const Pen &pen, bool map = false) {
                do_paint_string(str, pen, map); return advance(str.size());
            }
            Painter &string(const std::string &str, bool map = false) { return string(str, cur_pen, map); }
            Painter &string(const std::string &str, int8_t fg, bool map = false) { return string(str, cur_pen.color(fg), map); }

            Painter &key(df::interface_key kc, const Pen &pen, bool map = false) {
                return string(getKeyDisplay(kc), pen, map);
            }
            Painter &key(df::interface_key kc, bool map = false) { return key(kc, cur_key_pen, map); }

        private:
            void do_paint_string(const std::string &str, const Pen &pen, bool map = false);
        };

        namespace Hooks {
            GUI_HOOK_DECLARE(set_tile, void, (const Pen &pen, int x, int y, bool map));
        }

    }

    class DFHACK_EXPORT dfhack_viewscreen : public df::viewscreen {
        df::coord2d last_size;
        void check_resize();

    protected:
        bool text_input_mode;

    public:
        dfhack_viewscreen();
        virtual ~dfhack_viewscreen();

        static bool is_instance(df::viewscreen *screen);
        static dfhack_viewscreen *try_cast(df::viewscreen *screen);

        virtual void logic();
        virtual void render();

        virtual int8_t movies_okay() { return 1; }
        virtual bool key_conflict(df::interface_key key);

        virtual bool is_lua_screen() { return false; }

        virtual std::string getFocusString() = 0;
        virtual void onShow() {};
        virtual void onDismiss() {};
        virtual df::unit *getSelectedUnit() { return NULL; }
        virtual df::item *getSelectedItem() { return NULL; }
        virtual df::job *getSelectedJob() { return NULL; }
        virtual df::building *getSelectedBuilding() { return NULL; }
    };

    class DFHACK_EXPORT dfhack_lua_viewscreen : public dfhack_viewscreen {
        std::string focus;

        void update_focus(lua_State *L, int idx);

        bool safe_call_lua(int (*pf)(lua_State *), int args, int rvs);
        static dfhack_lua_viewscreen *get_self(lua_State *L);

        static int do_destroy(lua_State *L);
        static int do_render(lua_State *L);
        static int do_notify(lua_State *L);
        static int do_input(lua_State *L);

        bool allow_options;
    public:
        dfhack_lua_viewscreen(lua_State *L, int table_idx);
        virtual ~dfhack_lua_viewscreen();

        static df::viewscreen *get_pointer(lua_State *L, int idx, bool make);

        virtual bool is_lua_screen() { return true; }
        virtual std::string getFocusString() { return focus; }

        virtual void render();
        virtual void logic();
        virtual void help();
        virtual void resize(int w, int h);
        virtual void feed(std::set<df::interface_key> *keys);
        virtual bool key_conflict(df::interface_key key);

        virtual void onShow();
        virtual void onDismiss();

        virtual df::unit *getSelectedUnit();
        virtual df::item *getSelectedItem();
        virtual df::job *getSelectedJob();
        virtual df::building *getSelectedBuilding();
    };
}
