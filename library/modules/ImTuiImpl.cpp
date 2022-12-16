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
    ImGui::GetStyle().Alpha = 1.0f;
    ImGui::GetStyle().WindowPadding = ImVec2(0.5f, 0.0f);
    ImGui::GetStyle().WindowRounding = 0.0f;
    ImGui::GetStyle().WindowBorderSize = 0.0f;
    ImGui::GetStyle().WindowMinSize = ImVec2(4.0f, 2.0f);
    ImGui::GetStyle().WindowTitleAlign = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_Left;
    ImGui::GetStyle().ChildRounding = 0.0f;
    ImGui::GetStyle().ChildBorderSize = 0.0f;
    ImGui::GetStyle().PopupRounding = 0.0f;
    ImGui::GetStyle().PopupBorderSize = 0.0f;
    ImGui::GetStyle().FramePadding = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().FrameRounding = 0.0f;
    ImGui::GetStyle().FrameBorderSize = 0.0f;
    ImGui::GetStyle().ItemSpacing = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().ItemInnerSpacing = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().TouchExtraPadding = ImVec2(0.5f, 0.0f);
    ImGui::GetStyle().IndentSpacing = 1.0f;
    ImGui::GetStyle().ColumnsMinSpacing = 1.0f;
    ImGui::GetStyle().ScrollbarSize = 0.5f;
    ImGui::GetStyle().ScrollbarRounding = 0.0f;
    ImGui::GetStyle().GrabMinSize = 0.1f;
    ImGui::GetStyle().GrabRounding = 0.0f;
    ImGui::GetStyle().TabRounding = 0.0f;
    ImGui::GetStyle().TabBorderSize = 0.0f;
    ImGui::GetStyle().ColorButtonPosition = ImGuiDir_Right;
    ImGui::GetStyle().ButtonTextAlign = ImVec2(0.5f, 0.0f);
    ImGui::GetStyle().SelectableTextAlign = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().DisplayWindowPadding = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().CellPadding = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().MouseCursorScale = 1.0f;
    ImGui::GetStyle().AntiAliasedLines = false;
    ImGui::GetStyle().AntiAliasedFill = false;
    ImGui::GetStyle().CurveTessellationTol = 1.25f;

    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.15, 0.15, 0.15, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImVec4(0.35, 0.35, 0.35, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15, 0.15, 0.15, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.75, 0.75, 0.75, 0.5f);
    ImGui::GetStyle().Colors[ImGuiCol_NavHighlight] = ImVec4(0.00, 0.00, 0.00, 0.0f);

    ImFontConfig fontConfig;
    fontConfig.GlyphMinAdvanceX = 1.0f;
    fontConfig.SizePixels = 1.00;
    ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);

    // Build atlas
    unsigned char* tex_pixels = NULL;
    int tex_w, tex_h;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);

    /*
    ImGui::GetIO().KeyMap[ImGuiKey_Tab]         = 9;
    ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow]   = 260;
    ImGui::GetIO().KeyMap[ImGuiKey_RightArrow]  = 261;
    ImGui::GetIO().KeyMap[ImGuiKey_UpArrow]     = 259;
    ImGui::GetIO().KeyMap[ImGuiKey_DownArrow]   = 258;
    ImGui::GetIO().KeyMap[ImGuiKey_PageUp]      = 339;
    ImGui::GetIO().KeyMap[ImGuiKey_PageDown]    = 338;
    ImGui::GetIO().KeyMap[ImGuiKey_Home]        = 262;
    ImGui::GetIO().KeyMap[ImGuiKey_End]         = 360;
    ImGui::GetIO().KeyMap[ImGuiKey_Insert]      = 331;
    ImGui::GetIO().KeyMap[ImGuiKey_Delete]      = 330;
    ImGui::GetIO().KeyMap[ImGuiKey_Backspace]   = 263;
    ImGui::GetIO().KeyMap[ImGuiKey_Space]       = 32;
    ImGui::GetIO().KeyMap[ImGuiKey_Enter]       = 10;
    ImGui::GetIO().KeyMap[ImGuiKey_Escape]      = 27;
    ImGui::GetIO().KeyMap[ImGuiKey_KeyPadEnter] = 343;
    ImGui::GetIO().KeyMap[ImGuiKey_A]           = 1;
    ImGui::GetIO().KeyMap[ImGuiKey_C]           = 3;
    ImGui::GetIO().KeyMap[ImGuiKey_V]           = 22;
    ImGui::GetIO().KeyMap[ImGuiKey_X]           = 24;
    ImGui::GetIO().KeyMap[ImGuiKey_Y]           = 25;
    ImGui::GetIO().KeyMap[ImGuiKey_Z]           = 26;

    ImGui::GetIO().KeyRepeatDelay = 0.050;
    ImGui::GetIO().KeyRepeatRate = 0.050;

	int screenSizeX = 0;
	int screenSizeY = 0;

	getmaxyx(stdscr, screenSizeY, screenSizeX);
	ImGui::GetIO().DisplaySize = ImVec2(screenSizeX, screenSizeY);
*/

    df::coord2d dim = Screen::getWindowSize();
    ImGui::GetIO().DisplaySize = ImVec2(dim.x, dim.y);
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