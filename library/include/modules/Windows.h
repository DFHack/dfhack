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
#include <cstddef>

namespace DFHack
{
namespace Windows
{
    /*
     * DF window stuffs
     */
    enum df_color
    {
        black,
        blue,
        green,
        cyan,
        red,
        magenta,
        brown,
        lgray,
        dgray,
        lblue,
        lgreen,
        lcyan,
        lred,
        lmagenta,
        yellow,
        white
        // maybe add transparency?
    };

    // The tile format DF uses internally
    struct df_screentile
    {
        uint8_t symbol;
        uint8_t foreground; ///< df_color
        uint8_t background; ///< df_color
        uint8_t bright;
    };


    // our silly painter things and window things follow.
    class df_window;
    struct df_tilebuf
    {
        df_screentile * data;
        unsigned int width;
        unsigned int height;
    };

    DFHACK_EXPORT df_screentile *getScreenBuffer();

    class DFHACK_EXPORT painter
    {
        friend class df_window;
    public:
        df_screentile* get(unsigned int x, unsigned int y)
        {
            if(x >= width || y >= height)
                return 0;
            return &buffer[x*height + y];
        };
        bool set(unsigned int x, unsigned int y, df_screentile tile )
        {
            if(x >= width || y >= height)
                return false;
            buffer[x*height + y] = tile;
            return true;
        }
        df_color foreground (df_color change = (df_color) -1)
        {
            if(change != -1)
                current_foreground = change;
            return current_foreground;
        }
        df_color background (df_color change = (df_color) -1)
        {
            if(change != -1)
                current_background = change;
            return current_background;
        }
        void bright (bool change)
        {
            current_bright = change;
        }
        bool bright ()
        {
            return current_bright;
        }
        void printStr(std::string & str, bool wrap = false)
        {
            for ( auto iter = str.begin(); iter != str.end(); iter++)
            {
                auto elem = *iter;
                if(cursor_y >= (int)height)
                    break;
                if(wrap)
                {
                    if(cursor_x >= (int)width)
                        cursor_x = wrap_column;
                }
                df_screentile & tile = buffer[cursor_x * height + cursor_y];
                tile.symbol = elem;
                tile.foreground = current_foreground;
                tile.background = current_background;
                tile.bright = current_bright;
                cursor_x++;
            }
        }
        void set_wrap (int new_column)
        {
            wrap_column = new_column;
        }
        void gotoxy(unsigned int x, unsigned int y)
        {
            cursor_x = x;
            cursor_y = y;
        }
        void reset()
        {
            cursor_x = 0;
            cursor_y = 0;
            current_background = black;
            current_foreground = white;
            current_bright = false;
            wrap_column = 0;
        }
    private:
        painter (df_window * orig, df_screentile * buf, unsigned int width, unsigned int height)
        {
            origin = orig;
            this->width = width;
            this->height = height;
            this->buffer = buf;
            reset();
        }
        df_window* origin;
        unsigned int width;
        unsigned int height;
        df_screentile* buffer;
        // current paint cursor position
        int cursor_x;
        int cursor_y;
        int wrap_column;
        // current foreground color
        df_color current_foreground;
        // current background color
        df_color current_background;
        // make bright?
        bool current_bright;
    };

    class DFHACK_EXPORT df_window
    {
        friend class painter;
    public:
        df_window(int x, int y, unsigned int width, unsigned int height);
        virtual ~df_window();
        virtual bool move (int left_, int top_, unsigned int width_, unsigned int height_) = 0;
        virtual void paint () = 0;
        virtual painter * lock();
        bool unlock (painter * painter);
        virtual bool addChild(df_window *);
        virtual df_tilebuf getBuffer() = 0;
    public:
        df_screentile* buffer;
        unsigned int width;
        unsigned int height;
    protected:
        df_window * parent;
        std::vector <df_window *> children;
        int left;
        int top;
        // FIXME: FAKE
        bool locked;
        painter * current_painter;
    };

    class DFHACK_EXPORT top_level_window : public df_window
    {
    public:
        top_level_window();
        virtual bool move (int left_, int top_, unsigned int width_, unsigned int height_);
        virtual void paint ();
        virtual painter * lock();
        virtual df_tilebuf getBuffer();
    };
    class DFHACK_EXPORT buffered_window : public df_window
    {
    public:
        buffered_window(int x, int y, unsigned int width, unsigned int height):df_window(x,y,width, height)
        {
            buffer = new df_screentile[width*height];
        };
        virtual ~buffered_window()
        {
            delete buffer;
        }
        virtual void blit_to_parent ()
        {
            df_tilebuf par = parent->getBuffer();
            for(unsigned xi = 0; xi < width; xi++)
            {
                for(unsigned yi = 0; yi < height; yi++)
                {
                    unsigned parx = left + xi;
                    unsigned pary = top + yi;
                    if(pary >= par.height) continue;
                    if(parx >= par.width) continue;
                    par.data[parx * par.height + pary] = buffer[xi * height + yi];
                }
            }
        }
        virtual df_tilebuf getBuffer()
        {
            df_tilebuf buf;
            buf.data = buffer;
            buf.width = width;
            buf.height = height;
            return buf;
        };
    };
    class DFHACK_EXPORT dfhack_dummy : public buffered_window
    {
    public:
        dfhack_dummy(int x, int y):buffered_window(x,y,6,1){};
        virtual bool move (int left_, int top_, unsigned int width_, unsigned int height_)
        {
            top = top_;
            left = left_;
            return true;
        }
        virtual void paint ()
        {
            painter * p = lock();
            p->bright(true);
            p->background(black);
            p->foreground(white);
            std::string dfhack = "DFHack";
            p->printStr(dfhack);
            blit_to_parent();
        }
    };
}
}