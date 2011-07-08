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

#pragma once
#include "keyboard.h"

namespace FakeSDL
{
    enum ButtonState
    {
        BTN_RELEASED = 0,
        BTN_PRESSED = 1
    };

    /** Event enumerations */
    enum EventType
    {
           ET_NOEVENT = 0,         /**< Unused (do not remove) */
           ET_ACTIVEEVENT,         /**< Application loses/gains visibility */
           ET_KEYDOWN,             /**< Keys pressed */
           ET_KEYUP,               /**< Keys released */
           ET_MOUSEMOTION,         /**< Mouse moved */
           ET_MOUSEBUTTONDOWN,     /**< Mouse button pressed */
           ET_MOUSEBUTTONUP,       /**< Mouse button released */
           ET_JOYAXISMOTION,       /**< Joystick axis motion */
           ET_JOYBALLMOTION,       /**< Joystick trackball motion */
           ET_JOYHATMOTION,        /**< Joystick hat position change */
           ET_JOYBUTTONDOWN,       /**< Joystick button pressed */
           ET_JOYBUTTONUP,         /**< Joystick button released */
           ET_QUIT,                /**< User-requested quit */
           ET_SYSWMEVENT,          /**< System specific event */
           ET_EVENT_RESERVEDA,     /**< Reserved for future use.. */
           ET_EVENT_RESERVEDB,     /**< Reserved for future use.. */
           ET_VIDEORESIZE,         /**< User resized video mode */
           ET_VIDEOEXPOSE,         /**< Screen needs to be redrawn */
           ET_EVENT_RESERVED2,     /**< Reserved for future use.. */
           ET_EVENT_RESERVED3,     /**< Reserved for future use.. */
           ET_EVENT_RESERVED4,     /**< Reserved for future use.. */
           ET_EVENT_RESERVED5,     /**< Reserved for future use.. */
           ET_EVENT_RESERVED6,     /**< Reserved for future use.. */
           ET_EVENT_RESERVED7,     /**< Reserved for future use.. */
           /** Events ET_USEREVENT through ET_MAXEVENTS-1 are for your use */
           ET_USEREVENT = 24,
           /** This last event is only for bounding internal arrays
        *  It is the number of bits in the event mask datatype -- Uint32
            */
           ET_NUMEVENTS = 32
    };

    /** Application visibility event structure */
     struct ActiveEvent
    {
        uint8_t type;     /**< ET_ACTIVEEVENT */
        uint8_t gain;     /**< Whether given states were gained or lost (1/0) */
        uint8_t state;    /**< A mask of the focus states */
    };

    /** Keyboard event structure */
    struct KeyboardEvent
    {
        uint8_t type;     /**< ET_KEYDOWN or ET_KEYUP */
        uint8_t which;    /**< The keyboard device index */
        uint8_t state;    /**< BTN_PRESSED or BTN_RELEASED */
        keysym ksym;
    };

    /** Mouse motion event structure */
    struct MouseMotionEvent
    {
        uint8_t type;   /**< ET_MOUSEMOTION */
        uint8_t which;  /**< The mouse device index */
        uint8_t state;  /**< The current button state */
        uint16_t x, y;  /**< The X/Y coordinates of the mouse */
        int16_t xrel;   /**< The relative motion in the X direction */
        int16_t yrel;   /**< The relative motion in the Y direction */
    };

    /** Mouse button event structure */
    struct MouseButtonEvent
    {
        uint8_t type;   /**< ET_MOUSEBUTTONDOWN or ET_MOUSEBUTTONUP */
        uint8_t which;  /**< The mouse device index */
        uint8_t button; /**< The mouse button index */
        uint8_t state;  /**< BTN_PRESSED or BTN_RELEASED */
        uint16_t x, y;  /**< The X/Y coordinates of the mouse at press time */
    };

    /** Joystick axis motion event structure */
    struct JoyAxisEvent
    {
        uint8_t type;   /**< ET_JOYAXISMOTION */
        uint8_t which;  /**< The joystick device index */
        uint8_t axis;   /**< The joystick axis index */
        int16_t value;  /**< The axis value (range: -32768 to 32767) */
    };

    /** Joystick trackball motion event structure */
    struct JoyBallEvent
    {
        uint8_t type;   /**< ET_JOYBALLMOTION */
        uint8_t which;  /**< The joystick device index */
        uint8_t ball;   /**< The joystick trackball index */
        int16_t xrel;   /**< The relative motion in the X direction */
        int16_t yrel;   /**< The relative motion in the Y direction */
    };

    /** Joystick hat position change event structure */
    struct JoyHatEvent
    {
        uint8_t type;   /**< ET_JOYHATMOTION */
        uint8_t which;  /**< The joystick device index */
        uint8_t hat;    /**< The joystick hat index */
        uint8_t value;  /**< The hat position value:
                         *   SDL_HAT_LEFTUP   SDL_HAT_UP       SDL_HAT_RIGHTUP
                         *   SDL_HAT_LEFT     SDL_HAT_CENTERED SDL_HAT_RIGHT
                         *   SDL_HAT_LEFTDOWN SDL_HAT_DOWN     SDL_HAT_RIGHTDOWN
                         *  Note that zero means the POV is centered.
                         */
    };

    /** Joystick button event structure */
    struct JoyButtonEvent
    {
        uint8_t type;   /**< ET_JOYBUTTONDOWN or ET_JOYBUTTONUP */
        uint8_t which;  /**< The joystick device index */
        uint8_t button; /**< The joystick button index */
        uint8_t state;  /**< BTN_PRESSED or BTN_RELEASED */
    };

    /** The "window resized" event
     *  When you get this event, you are responsible for setting a new video
     *  mode with the new width and height.
     */
    struct ResizeEvent
    {
        uint8_t type;   /**< ET_VIDEORESIZE */
        int w;          /**< New width */
        int h;          /**< New height */
    };

    /** The "screen redraw" event */
    struct ExposeEvent
    {
        uint8_t type;   /**< ET_VIDEOEXPOSE */
    };

    /** The "quit requested" event */
    struct QuitEvent
    {
        uint8_t type;   /**< ET_QUIT */
    };

    /** A user-defined event type */
    struct UserEvent
    {
        uint8_t type;   /**< ETL_USEREVENT through ET_NUMEVENTS-1 */
        int code;       /**< User defined event code */
        void *data1;    /**< User defined data pointer */
        void *data2;    /**< User defined data pointer */
    };

    /** If you want to use this event, you should include SDL_syswm.h */
    struct SysWMmsg;
    struct SysWMEvent
    {
        uint8_t type;
        SysWMmsg *msg;
    };

    /** General event structure */
    union Event
    {
        uint8_t type;
        ActiveEvent active;
        KeyboardEvent key;
        MouseMotionEvent motion;
        MouseButtonEvent button;
        JoyAxisEvent jaxis;
        JoyBallEvent jball;
        JoyHatEvent jhat;
        JoyButtonEvent jbutton;
        ResizeEvent resize;
        ExposeEvent expose;
        QuitEvent quit;
        UserEvent user;
        SysWMEvent syswm;
    };
}