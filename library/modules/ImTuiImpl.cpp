#include "modules/ImTuiImpl.h"
#include "modules/Screen.h"
#include "ColorText.h"
#include "df/enabler.h"

using namespace DFHack;

#include <cmath>
#include <vector>
#include <algorithm>
#include <cctype>

int ImTuiInterop::name_to_colour_index(const std::string& name)
{
    std::map<std::string, int> names =
    {
        {"RESET",       -1},
        {"BLACK",        0},
        {"BLUE",         1},
        {"GREEN",        2},
        {"CYAN",         3},
        {"RED",          4},
        {"MAGENTA",      5},
        {"BROWN",        6},
        {"GREY",         7},
        {"DARKGREY",     8},
        {"LIGHTBLUE",    9},
        {"LIGHTGREEN",   10},
        {"LIGHTCYAN",    11},
        {"LIGHTRED",     12},
        {"LIGHTMAGENTA", 13},
        {"YELLOW",       14},
        {"WHITE",        15},
        {"MAX",          16},
    };

    return names.at(name);
}

//This isn't the only way that colour interop could be done
ImVec4 ImTuiInterop::colour_interop(std::vector<int> col3)
{
    col3.resize(3);

    //ImTui stuffs the character value in col.a
    return { static_cast<float>(col3[0]), static_cast<float>(col3[1]), static_cast<float>(col3[2]), 1.f };
}

ImVec4 ImTuiInterop::named_colours(const std::string& fg, const std::string& bg, bool bold)
{
    std::vector<int> vals;
    vals.resize(3);

    vals[0] = name_to_colour_index(fg);
    vals[1] = name_to_colour_index(bg);
    vals[2] = bold;

    return colour_interop(vals);
}

#define ABS(x) ((x >= 0) ? x : -x)

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

    std::vector<int> g_xrange;
    
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

                    ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(col);

                    //todo: colours
                    const Screen::Pen pen(0, col4.x, col4.y);

                    Screen::paintString(pen, x, y + ymin, " ");
                }
                ++x;
            }
        }
    }
}


void ImTuiInterop::impl::init_current_context()
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

    /*ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.15, 0.15, 0.15, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImVec4(0.35, 0.35, 0.35, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15, 0.15, 0.15, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.75, 0.75, 0.75, 0.5f);
    ImGui::GetStyle().Colors[ImGuiCol_NavHighlight] = ImVec4(0.00, 0.00, 0.00, 0.0f);*/

    ImGuiStyle& style = ImGui::GetStyle();

    for (int i = 0; i < ImGuiCol_COUNT; i++)
    {
        style.Colors[i] = named_colours("BLACK", "BLACK", false);
    }

    //I really need a transparency colour
    style.Colors[ImGuiCol_Text] = named_colours("WHITE", "BLACK", false);
    style.Colors[ImGuiCol_TextDisabled] = named_colours("GREY", "BLACK", false);
    style.Colors[ImGuiCol_TitleBg] = named_colours("BLACK", "BLUE", false);
    style.Colors[ImGuiCol_TitleBgActive] = named_colours("BLACK", "LIGHTBLUE", false);
    style.Colors[ImGuiCol_TitleBgCollapsed] = named_colours("BLACK", "BLUE", false);
    style.Colors[ImGuiCol_MenuBarBg] = named_colours("BLACK", "BLUE", false);

    //unsure about much of this
    style.Colors[ImGuiCol_CheckMark] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_SliderGrab] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_SliderGrabActive] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_Button] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_ButtonHovered] = named_colours("BLACK", "RED", false); //?
    style.Colors[ImGuiCol_ButtonActive] = named_colours("BLACK", "GREEN", false); //?
    style.Colors[ImGuiCol_Header] = named_colours("BLACK", "BLUE", false); //?
    style.Colors[ImGuiCol_HeaderHovered] = named_colours("BLACK", "BLUE", false); //?
    style.Colors[ImGuiCol_HeaderActive] = named_colours("BLACK", "BLUE", false); //?
    style.Colors[ImGuiCol_Separator] = named_colours("WHITE", "WHITE", false); //?
    style.Colors[ImGuiCol_SeparatorHovered] = named_colours("WHITE", "WHITE", false); //?
    style.Colors[ImGuiCol_SeparatorActive] = named_colours("WHITE", "WHITE", false); //?
    style.Colors[ImGuiCol_ResizeGrip] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_ResizeGripHovered] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_ResizeGripActive] = named_colours("WHITE", "BLACK", false); //?

    ImFontConfig fontConfig;
    fontConfig.GlyphMinAdvanceX = 1.0f;
    fontConfig.SizePixels = 1.00;
    ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);

    // Build atlas
    unsigned char* tex_pixels = NULL;
    int tex_w, tex_h;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);

    ImGui::GetIO().KeyMap[ImGuiKey_Tab] = 9;
    ImGui::GetIO().KeyMap[ImGuiKey_Backspace] = 0; //why
    ImGui::GetIO().KeyMap[ImGuiKey_Escape] = 27;
    ImGui::GetIO().KeyMap[ImGuiKey_Enter] = 10;
    ImGui::GetIO().KeyMap[ImGuiKey_Space] = 32;

    //super arbitrary, need to undo all the ascii mapping
    ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow] = df::enums::interface_key::CURSOR_LEFT + 256;
    ImGui::GetIO().KeyMap[ImGuiKey_RightArrow] = df::enums::interface_key::CURSOR_RIGHT + 256;
    ImGui::GetIO().KeyMap[ImGuiKey_UpArrow] = df::enums::interface_key::CURSOR_UP + 256;
    ImGui::GetIO().KeyMap[ImGuiKey_DownArrow] = df::enums::interface_key::CURSOR_DOWN + 256;

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

void ImTuiInterop::impl::new_frame(std::set<df::interface_key> keys)
{
    ImGuiIO& io = ImGui::GetIO();

    auto& keysDown = io.KeysDown;
    std::fill(keysDown, keysDown + 512, false);

    for (const df::interface_key& key : keys)
    {
        int charval = Screen::keyToChar(key);

        //escape
        if (key == df::enums::interface_key::LEAVESCREEN)
            charval = 27;

        //enter
        if (key == df::enums::interface_key::SELECT)
            charval = 10;

        //need to undo all the hackyness
        if (key == df::enums::interface_key::CURSOR_LEFT)
            charval = df::enums::interface_key::CURSOR_LEFT + 256;

        if (key == df::enums::interface_key::CURSOR_RIGHT)
            charval = df::enums::interface_key::CURSOR_RIGHT + 256;

        if (key == df::enums::interface_key::CURSOR_UP)
            charval = df::enums::interface_key::CURSOR_UP + 256;

        if (key == df::enums::interface_key::CURSOR_DOWN)
            charval = df::enums::interface_key::CURSOR_DOWN + 256;

        if (charval < 0)
            continue;

        keysDown[charval] = true;

        if(charval < 256 && std::isprint(charval))
          io.AddInputCharacter(charval);
    }

    df::coord2d dim = Screen::getWindowSize();
    ImGui::GetIO().DisplaySize = ImVec2(dim.x, dim.y);

    //todo: fill keysdown, special keys

    df::coord2d mouse_pos = Screen::getMousePos();

    io.MousePos.x = mouse_pos.x;
    io.MousePos.y = mouse_pos.y;

    //todo: frametime
    io.DeltaTime = 33.f / 1000.f;

    io.MouseDown[0] = 0;
    io.MouseDown[1] = 0;

    if (df::global::enabler) {
        if (df::global::enabler->mouse_lbut) {
            io.MouseDown[0] = 1;
            df::global::enabler->mouse_lbut_down = 0;
        }
        if (df::global::enabler->mouse_rbut) {
            io.MouseDown[1] = 1;
            df::global::enabler->mouse_rbut_down = 0;
        }
    }

    ImGui::NewFrame();
}

void ImTuiInterop::impl::draw_frame()
{
    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();

    int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);

    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }


    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            {
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    float lastCharX = -10000.0f;
                    float lastCharY = -10000.0f;

                    for (unsigned int i = 0; i < pcmd->ElemCount; i += 3) {
                        int vidx0 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 0];
                        int vidx1 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 1];
                        int vidx2 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 2];

                        auto pos0 = cmd_list->VtxBuffer[vidx0].pos;
                        auto pos1 = cmd_list->VtxBuffer[vidx1].pos;
                        auto pos2 = cmd_list->VtxBuffer[vidx2].pos;

                        pos0.x = std::max(std::min(float(clip_rect.z - 1), pos0.x), clip_rect.x);
                        pos1.x = std::max(std::min(float(clip_rect.z - 1), pos1.x), clip_rect.x);
                        pos2.x = std::max(std::min(float(clip_rect.z - 1), pos2.x), clip_rect.x);
                        pos0.y = std::max(std::min(float(clip_rect.w - 1), pos0.y), clip_rect.y);
                        pos1.y = std::max(std::min(float(clip_rect.w - 1), pos1.y), clip_rect.y);
                        pos2.y = std::max(std::min(float(clip_rect.w - 1), pos2.y), clip_rect.y);

                        auto uv0 = cmd_list->VtxBuffer[vidx0].uv;
                        auto uv1 = cmd_list->VtxBuffer[vidx1].uv;
                        auto uv2 = cmd_list->VtxBuffer[vidx2].uv;

                        auto col0 = cmd_list->VtxBuffer[vidx0].col;
                        //auto col1 = cmd_list->VtxBuffer[vidx1].col;
                        //auto col2 = cmd_list->VtxBuffer[vidx2].col;

                        if (uv0.x != uv1.x || uv0.x != uv2.x || uv1.x != uv2.x ||
                            uv0.y != uv1.y || uv0.y != uv2.y || uv1.y != uv2.y) {
                            int vvidx0 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 3];
                            int vvidx1 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 4];
                            int vvidx2 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 5];

                            auto ppos0 = cmd_list->VtxBuffer[vvidx0].pos;
                            auto ppos1 = cmd_list->VtxBuffer[vvidx1].pos;
                            auto ppos2 = cmd_list->VtxBuffer[vvidx2].pos;

                            float x = ((pos0.x + pos1.x + pos2.x + ppos0.x + ppos1.x + ppos2.x) / 6.0f);
                            float y = ((pos0.y + pos1.y + pos2.y + ppos0.y + ppos1.y + ppos2.y) / 6.0f) + 0.5f;

                            if (std::fabs(y - lastCharY) < 0.5f && std::fabs(x - lastCharX) < 0.5f) {
                                x = lastCharX + 1.0f;
                                y = lastCharY;
                            }

                            lastCharX = x;
                            lastCharY = y;

                            int xx = (x)+1;
                            int yy = (y)+0;
                            if (xx < clip_rect.x || xx >= clip_rect.z || yy < clip_rect.y || yy >= clip_rect.w) {
                            }
                            else {
                                /*auto& cell = screen->data[yy * screen->nx + xx];
                                cell &= 0xFF000000;
                                cell |= (col0 & 0xff000000) >> 24;
                                cell |= ((ImTui::TCell)(rgbToAnsi256(col0, false)) << 16);*/
                                
                                //It looks like imtui stuffs the actual character in the a component
                                char c = (col0 & 0xff000000) >> 24;
                                
                                ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(col0);

                                const Screen::Pen current_bg = Screen::readTile(xx, yy);

                                //I am text, and have no background
                                const Screen::Pen pen(0, col4.x, current_bg.bg);

                                Screen::paintString(pen, xx, yy, std::string(1, c));
                            }
                            i += 3;
                        }
                        else {
                            drawTriangle(pos0, pos1, pos2, col0);
                        }
                    }
                }
            }
        }
    }
}

void ImTuiInterop::impl::shutdown()
{

}

ImTuiInterop::ui_state::ui_state()
{
    last_context = nullptr;
    ctx = nullptr;
}

void ImTuiInterop::ui_state::feed(std::set<df::interface_key>* keys)
{
    if (keys == nullptr)
        return;

    unprocessed_keys.insert(keys->begin(), keys->end());
}

void ImTuiInterop::ui_state::activate()
{
    last_context = ImGui::GetCurrentContext();

    ImGui::SetCurrentContext(ctx);
}

void ImTuiInterop::ui_state::new_frame()
{
    ImTuiInterop::impl::new_frame(std::move(unprocessed_keys));
    unprocessed_keys.clear();
}

void ImTuiInterop::ui_state::draw_frame()
{
    ImTuiInterop::impl::draw_frame();
}

void ImTuiInterop::ui_state::deactivate()
{
    ImGui::SetCurrentContext(last_context);
}

ImTuiInterop::ui_state ImTuiInterop::make_ui_system()
{
    std::cout << "Made ImTui System\n";

    ImGuiContext* ctx = ImGui::CreateContext();
    
    ui_state st;
    st.ctx = ctx;

    st.activate();

    impl::init_current_context();

    st.deactivate();

    return st;
}