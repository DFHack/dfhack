#include "modules/ImTuiImpl.h"
#include "modules/Screen.h"
#include "ColorText.h"

using namespace DFHack;

#include <cmath>
#include <vector>
#include <algorithm>

#define ABS(x) ((x >= 0) ? x : -x)

static std::vector<int> g_xrange;

void ScanLine(int x1, int y1, int x2, int y2, int ymax, std::vector<int>& xrange) {
    int sx, sy, dx1, dy1, dx2, dy2, x, y, m, n, k, cnt;

    sx = x2 - x1;
    sy = y2 - y1;

    if (sx > 0) dx1 = 1;
    else if (sx < 0) dx1 = -1;
    else dx1 = 0;

    if (sy > 0) dy1 = 1;
    else if (sy < 0) dy1 = -1;
    else dy1 = 0;

    m = ABS(sx);
    n = ABS(sy);
    dx2 = dx1;
    dy2 = 0;

    if (m < n)
    {
        m = ABS(sy);
        n = ABS(sx);
        dx2 = 0;
        dy2 = dy1;
    }

    x = x1; y = y1;
    cnt = m + 1;
    k = n / 2;

    while (cnt--) {
        if ((y >= 0) && (y < ymax)) {
            if (x < xrange[2 * y + 0]) xrange[2 * y + 0] = x;
            if (x > xrange[2 * y + 1]) xrange[2 * y + 1] = x;
        }

        k += n;
        if (k < m) {
            x += dx2;
            y += dy2;
        }
        else {
            k -= m;
            x += dx1;
            y += dy1;
        }
    }
}

void drawTriangle(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImU32 col) {
    df::coord2d dim = Screen::getWindowSize();
    
    int screen_size = dim.x * dim.y;

    int ymin = std::min(std::min(std::min((float)screen_size, p0.y), p1.y), p2.y);
    int ymax = std::max(std::max(std::max(0.0f, p0.y), p1.y), p2.y);

    int ydelta = ymax - ymin + 1;

    if ((int) g_xrange.size() < 2*ydelta) {
        g_xrange.resize(2*ydelta);
    }

    for (int y = 0; y < ydelta; y++) {
        g_xrange[2*y+0] = 999999;
        g_xrange[2*y+1] = -999999;
    }

    ScanLine(p0.x, p0.y - ymin, p1.x, p1.y - ymin, ydelta, g_xrange);
    ScanLine(p1.x, p1.y - ymin, p2.x, p2.y - ymin, ydelta, g_xrange);
    ScanLine(p2.x, p2.y - ymin, p0.x, p0.y - ymin, ydelta, g_xrange);

    for (int y = 0; y < ydelta; y++) {
        if (g_xrange[2*y+1] >= g_xrange[2*y+0]) {
            int x = g_xrange[2*y+0];
            int len = 1 + g_xrange[2*y+1] - g_xrange[2*y+0];

            while (len--) {
                if (x >= 0 && x < dim.x && y + ymin >= 0 && y + ymin < dim.y) {
                    /*auto& cell = screen->data[(y + ymin) * screen->nx + x];
                    cell &= 0x00FF0000;
                    cell |= ' ';
                    cell |= ((ImTui::TCell)(col) << 24);*/

                    //todo: colours
                    const Screen::Pen pen(0, COLOR_BLUE, 0);

                    Screen::paintString(pen, x, y + ymin, " ");
                }
                ++x;
            }
        }
    }
}


void ImTuiInterop::start()
{

}

void ImTuiInterop::new_frame()
{

}

void ImTuiInterop::draw_frame()
{

}

void ImTuiInterop::shutdown()
{

}