#ifndef CL_MOD_POSITION
#define CL_MOD_POSITION
/*
* View position and size and cursor position
*/
#include "Export.h"
namespace DFHack
{
    class APIPrivate;
    class DFHACK_EXPORT Position
    {
        public:
        
        Position(APIPrivate * d);
        ~Position();
        /*
        * Cursor and window coords
        */
        bool getViewCoords (int32_t &x, int32_t &y, int32_t &z);
        bool setViewCoords (const int32_t x, const int32_t y, const int32_t z);
        
        bool getCursorCoords (int32_t &x, int32_t &y, int32_t &z);
        bool setCursorCoords (const int32_t x, const int32_t y, const int32_t z);
        
        /*
        * Window size in tiles
        */
        bool getWindowSize(int32_t & width, int32_t & height);
        
        private:
        struct Private;
        Private *d;
    };
}
#endif
