// Make raised drawbridge tiles indicate the bridge's direction

#include "df/building_drawbuffer.h"
#include "df/building_bridgest.h"

struct drawbridge_tiles_hook : df::building_bridgest {
    typedef df::building_bridgest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, drawBuilding, (uint32_t curtick, df::building_drawbuffer *buf, int16_t z_offset))
    {
        static const unsigned char tiles[4][3] =
        {
            { 0xB7, 0xB6, 0xBD },
            { 0xD6, 0xC7, 0xD3 },
            { 0xD4, 0xCF, 0xBE },
            { 0xD5, 0xD1, 0xB8 },
        };
        INTERPOSE_NEXT(drawBuilding)(curtick, buf, z_offset);

        // Only redraw "raised" drawbridges
        if (!gate_flags.bits.raised || direction == -1)
            return;

        // Figure out the extents and the axis
        int p1, p2;
        bool iy = false;
        switch (direction)
        {
        case 0: // Left
        case 1: // Right
            p1 = buf->y1; p2 = buf->y2; iy = true;
            break;
        case 2: // Up
        case 3: // Down
            p1 = buf->x1; p2 = buf->x2;
            break;
        default:
            // Already ignoring retracting above
            return;
        }

        int x = 0, y = 0;
        if (p1 == p2)
            buf->tile[0][0] = tiles[direction][1];
        else for (int p = p1; p <= p2; p++)
        {
            if (p == p1)
                buf->tile[x][y] = tiles[direction][0];
            else if (p == p2)
                buf->tile[x][y] = tiles[direction][2];
            else
                buf->tile[x][y] = tiles[direction][1];
            if (iy)
                y++;
            else
                x++;
        }
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(drawbridge_tiles_hook, drawBuilding);
