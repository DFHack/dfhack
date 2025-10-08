#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_video.h>
#include <algorithm>
#include <assert.h>
#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>
#include <ranges>
#include <condition_variable>
#include <cmath>
#include <numeric>
#include <set>

#include "SDL_pixels.h"
#include "Core.h"
#include "modules/DFSDL.h"
#include "SDLConsole.h"
#include "SDLConsole_impl.h"

using namespace DFHack;

namespace sdl_console {
namespace {

// These macros to be removed.
#define CONSOLE_SYMBOL_ADDR(sym) nullptr
#define CONSOLE_DECLARE_SYMBOL(sym) decltype(sym)* sym = CONSOLE_SYMBOL_ADDR(sym)

CONSOLE_DECLARE_SYMBOL(SDL_CaptureMouse);
CONSOLE_DECLARE_SYMBOL(SDL_ConvertSurfaceFormat);
CONSOLE_DECLARE_SYMBOL(SDL_CreateRenderer);
CONSOLE_DECLARE_SYMBOL(SDL_CreateRGBSurface);
CONSOLE_DECLARE_SYMBOL(SDL_CreateRGBSurfaceWithFormat);
CONSOLE_DECLARE_SYMBOL(SDL_CreateTexture);
CONSOLE_DECLARE_SYMBOL(SDL_CreateTextureFromSurface);
CONSOLE_DECLARE_SYMBOL(SDL_CreateWindow);
CONSOLE_DECLARE_SYMBOL(SDL_DestroyRenderer);
CONSOLE_DECLARE_SYMBOL(SDL_DestroyTexture);
CONSOLE_DECLARE_SYMBOL(SDL_DestroyWindow);
CONSOLE_DECLARE_SYMBOL(SDL_free);
CONSOLE_DECLARE_SYMBOL(SDL_FreeSurface);
CONSOLE_DECLARE_SYMBOL(SDL_GetClipboardText);
CONSOLE_DECLARE_SYMBOL(SDL_GetError);
CONSOLE_DECLARE_SYMBOL(SDL_GetEventFilter);
CONSOLE_DECLARE_SYMBOL(SDL_GetModState);
CONSOLE_DECLARE_SYMBOL(SDL_GetRendererOutputSize);
CONSOLE_DECLARE_SYMBOL(SDL_GetWindowFlags);
CONSOLE_DECLARE_SYMBOL(SDL_GetWindowID);
CONSOLE_DECLARE_SYMBOL(SDL_GetTicks64);
CONSOLE_DECLARE_SYMBOL(SDL_HideWindow);
CONSOLE_DECLARE_SYMBOL(SDL_iconv_string);
CONSOLE_DECLARE_SYMBOL(SDL_InitSubSystem);
CONSOLE_DECLARE_SYMBOL(SDL_MapRGB);
CONSOLE_DECLARE_SYMBOL(SDL_memset);
CONSOLE_DECLARE_SYMBOL(SDL_RenderClear);
CONSOLE_DECLARE_SYMBOL(SDL_RenderCopy);
CONSOLE_DECLARE_SYMBOL(SDL_RenderDrawRect);
CONSOLE_DECLARE_SYMBOL(SDL_RenderFillRect);
CONSOLE_DECLARE_SYMBOL(SDL_RenderFillRects);
CONSOLE_DECLARE_SYMBOL(SDL_RenderPresent);
CONSOLE_DECLARE_SYMBOL(SDL_RenderSetIntegerScale);
CONSOLE_DECLARE_SYMBOL(SDL_RenderSetViewport);
//CONSOLE_DECLARE_SYMBOL(SDL_PointInRect); // defined in SDL's header
CONSOLE_DECLARE_SYMBOL(SDL_SetClipboardText);
CONSOLE_DECLARE_SYMBOL(SDL_SetColorKey);
CONSOLE_DECLARE_SYMBOL(SDL_SetEventFilter);
CONSOLE_DECLARE_SYMBOL(SDL_SetHint);
CONSOLE_DECLARE_SYMBOL(SDL_SetRenderDrawBlendMode);
CONSOLE_DECLARE_SYMBOL(SDL_SetRenderDrawColor);
CONSOLE_DECLARE_SYMBOL(SDL_SetTextureBlendMode);
CONSOLE_DECLARE_SYMBOL(SDL_SetTextureColorMod);
CONSOLE_DECLARE_SYMBOL(SDL_SetWindowMinimumSize);
CONSOLE_DECLARE_SYMBOL(SDL_SetWindowOpacity);
CONSOLE_DECLARE_SYMBOL(SDL_ShowCursor);
CONSOLE_DECLARE_SYMBOL(SDL_ShowWindow);
CONSOLE_DECLARE_SYMBOL(SDL_StartTextInput);
CONSOLE_DECLARE_SYMBOL(SDL_StopTextInput);
CONSOLE_DECLARE_SYMBOL(SDL_UpperBlit);
CONSOLE_DECLARE_SYMBOL(SDL_UpdateTexture);
CONSOLE_DECLARE_SYMBOL(SDL_QuitSubSystem);

void bind_sdl_symbols()
{
    static bool didit = false;
    if (didit) return;
    didit = true;

    struct Symbol {
        const char* name;
        void** addr;
    };

    #define CONSOLE_ADD_SYMBOL(sym)     \
    {                               \
        #sym, (void**)&sdl_console::sym \
    }

    /* This list must be in parity with CONSOLE_DEFINE_SYMBOL */
    std::vector<Symbol> symbols = {
        CONSOLE_ADD_SYMBOL(SDL_CaptureMouse),
        CONSOLE_ADD_SYMBOL(SDL_ConvertSurfaceFormat),
        CONSOLE_ADD_SYMBOL(SDL_CreateRenderer),
        CONSOLE_ADD_SYMBOL(SDL_CreateRGBSurface),
        CONSOLE_ADD_SYMBOL(SDL_CreateRGBSurfaceWithFormat),
        CONSOLE_ADD_SYMBOL(SDL_CreateTexture),
        CONSOLE_ADD_SYMBOL(SDL_CreateTextureFromSurface),
        CONSOLE_ADD_SYMBOL(SDL_CreateWindow),
        CONSOLE_ADD_SYMBOL(SDL_DestroyRenderer),
        CONSOLE_ADD_SYMBOL(SDL_DestroyTexture),
        CONSOLE_ADD_SYMBOL(SDL_DestroyWindow),
        CONSOLE_ADD_SYMBOL(SDL_free),
        CONSOLE_ADD_SYMBOL(SDL_FreeSurface),
        CONSOLE_ADD_SYMBOL(SDL_GetClipboardText),
        CONSOLE_ADD_SYMBOL(SDL_GetError),
        CONSOLE_ADD_SYMBOL(SDL_GetEventFilter),
        CONSOLE_ADD_SYMBOL(SDL_GetModState),
        CONSOLE_ADD_SYMBOL(SDL_GetRendererOutputSize),
        CONSOLE_ADD_SYMBOL(SDL_GetWindowFlags),
        CONSOLE_ADD_SYMBOL(SDL_GetWindowID),
        CONSOLE_ADD_SYMBOL(SDL_GetTicks64),
        CONSOLE_ADD_SYMBOL(SDL_HideWindow),
        CONSOLE_ADD_SYMBOL(SDL_iconv_string),
        CONSOLE_ADD_SYMBOL(SDL_InitSubSystem),
        CONSOLE_ADD_SYMBOL(SDL_MapRGB),
        CONSOLE_ADD_SYMBOL(SDL_memset),
        CONSOLE_ADD_SYMBOL(SDL_RenderClear),
        CONSOLE_ADD_SYMBOL(SDL_RenderCopy),
        CONSOLE_ADD_SYMBOL(SDL_RenderDrawRect),
        CONSOLE_ADD_SYMBOL(SDL_RenderFillRect),
        CONSOLE_ADD_SYMBOL(SDL_RenderFillRects),
        CONSOLE_ADD_SYMBOL(SDL_RenderPresent),
        CONSOLE_ADD_SYMBOL(SDL_RenderSetIntegerScale),
        CONSOLE_ADD_SYMBOL(SDL_RenderSetViewport),
//        CONSOLE_ADD_SYMBOL(SDL_PointInRect), // defined in header
        CONSOLE_ADD_SYMBOL(SDL_SetClipboardText),
        CONSOLE_ADD_SYMBOL(SDL_SetColorKey),
        CONSOLE_ADD_SYMBOL(SDL_SetEventFilter),
        CONSOLE_ADD_SYMBOL(SDL_SetHint),
        CONSOLE_ADD_SYMBOL(SDL_SetRenderDrawBlendMode),
        CONSOLE_ADD_SYMBOL(SDL_SetRenderDrawColor),
        CONSOLE_ADD_SYMBOL(SDL_SetTextureBlendMode),
        CONSOLE_ADD_SYMBOL(SDL_SetTextureColorMod),
        CONSOLE_ADD_SYMBOL(SDL_SetWindowMinimumSize),
        CONSOLE_ADD_SYMBOL(SDL_SetWindowOpacity),
        CONSOLE_ADD_SYMBOL(SDL_ShowCursor),
        CONSOLE_ADD_SYMBOL(SDL_ShowWindow),
        CONSOLE_ADD_SYMBOL(SDL_StartTextInput),
        CONSOLE_ADD_SYMBOL(SDL_StopTextInput),
        CONSOLE_ADD_SYMBOL(SDL_UpperBlit),
        CONSOLE_ADD_SYMBOL(SDL_UpdateTexture),
        CONSOLE_ADD_SYMBOL(SDL_QuitSubSystem)
    };
    #undef CONSOLE_ADD_SYMBOL

    for (auto& sym : symbols) {
        *sym.addr = DFSDL::lookup_DFSDL_Symbol(sym.name);
        if (*sym.addr == nullptr)
            throw std::runtime_error(std::string("SDLConsole: SDL symbol not found: ") + sym.name);
    }
}
}

namespace text {
    static std::string to_utf8(const std::u32string& u32_string)
    {
        char* conv = sdl_console::SDL_iconv_string("UTF-8", "UTF-32LE",
                                                   reinterpret_cast<const char*>(u32_string.c_str()),
                                                   (u32_string.size()+1) * sizeof(char32_t));
        if (!conv)
            return "?u8?";

        std::string result(conv);
        sdl_console::SDL_free(conv);
        return result;
    }

    static std::u32string from_utf8(const std::string& u8_string)
    {
        char* conv = sdl_console::SDL_iconv_string("UTF-32LE", "UTF-8",
                                      u8_string.c_str(),
                                      u8_string.size() + 1);
        if (!conv)
            return U"?u8?";

        std::u32string result(reinterpret_cast<char32_t*>(conv));
        sdl_console::SDL_free(conv);
        return result;
    }

    static bool is_newline(char32_t ch) {
        return ch == U'\n' || ch == U'\r';
    }

    static bool is_wspace(char32_t ch) {
        return ch == U' ' || ch == U'\t';
    }

    std::pair<size_t, size_t> find_run_with_pred(const std::u32string& text, size_t pos, const std::function<bool(char32_t)> predicate) {
        if (text.empty()) return { std::u32string::npos, std::u32string::npos };

        if (pos >= text.size()) return { std::u32string::npos, std::u32string::npos };

        auto left = text.begin() + pos;
        auto right = left;

        while (left != text.begin() && predicate(*(left - 1)))
            --left;

        auto t = right;
        while (right != text.end() && predicate(*right))
            ++right;

        if (t != right)
            --right;

        return {
            std::distance(text.begin(), left),
            std::distance(text.begin(), right)
        };
    }

    size_t skip_wspace(const std::u32string& text, size_t pos) {
        if (text.empty()) return 0;
        if (pos >= text.size()) return text.size() - 1;

        return std::distance(text.begin(),
                             std::ranges::find_if_not(std::ranges::subrange(text.begin() + pos, text.end() - 1),
                                                      is_wspace));
    }

    size_t skip_wspace_reverse(const std::u32string& text, size_t pos) {
        if (text.empty()) return 0;
        if (pos >= text.size()) pos = text.size() - 1;

        auto it = text.begin() + pos;
        while (it != text.begin() && is_wspace(*it)) {
            --it;
        }
        return std::distance(text.begin(), it);
    }

    size_t skip_graph(const std::u32string& text, size_t pos) {
        if (text.empty()) return 0;
        if (pos >= text.size()) return text.size() - 1;

        return std::distance(text.begin(),
                             std::ranges::find_if(std::ranges::subrange(text.begin() + pos, text.end() - 1),
                                                  is_wspace));
    }

    size_t skip_graph_reverse(const std::u32string& text, size_t pos) {
        if (text.empty()) return 0;
        if (pos >= text.size()) pos = text.size() - 1;

        auto it = text.begin() + pos;
        while (it != text.begin() && !is_wspace(*it)) {
            --it;
        }
        return std::distance(text.begin(), it);
    }

    /*
     * Finds the end of the previous word or non-space character in the text,
     * starting from `pos`. If `pos` points to a space, it skips consecutive
     * spaces to find the previous word. If `pos` is already at a word, it skips
     * the current word and trailing spaces to find the next one. Returns the
     * position of the end of the previous word or non-space character.
     */
    size_t find_prev_word(const std::u32string& text, size_t pos) {
        size_t start = pos;
        start = skip_wspace_reverse(text, start);
        if (start == pos) {
            pos = skip_graph_reverse(text, pos);
            pos = skip_wspace_reverse(text, pos);
        } else {
            pos = start;
        }
        return pos;
    }

    /*
     * Finds the start of the next word or non-space character in the text,
     * starting from `pos`. If `pos` points to a space, it skips consecutive
     * spaces to find the next word. If `pos` is already at a word, it skips
     * the current word and trailing spaces to find the next one. Returns the
     * position of the start of the next word or non-space character.
     */
    size_t find_next_word(const std::u32string& text, size_t pos) {
        size_t start = pos;
        start = skip_wspace(text, start);
        if (start == pos) {
            pos = skip_graph(text, pos);
            pos = skip_wspace(text, pos);
        } else {
            pos = start;
        }
        return pos;
    }

    std::pair<size_t, size_t> find_wspace_run(const std::u32string& text, size_t pos) {
        return find_run_with_pred(text, pos, [](char32_t ch) { return is_wspace(ch); });
    }

    std::pair<size_t, size_t> find_run(const std::u32string& text, size_t pos) {
        // Bounds check here since .at() is used below
        if (text.empty() || pos >= text.size()) {
            return { std::u32string::npos, std::u32string::npos };
        }

        if (is_wspace(text.at(pos))) {
            return find_wspace_run(text, pos);
        }
        return find_run_with_pred(text, pos, [](char32_t ch) { return !is_wspace(ch); });
    }
}

namespace geometry {
#if 0
void center_rect(SDL_Rect& r)
{
    r.x = r.x - r.w / 2;
    r.y = r.y - r.h / 2;
}
#endif

static bool in_rect(int x, int y, SDL_Rect& r)
{
    return ((x >= r.x) && (x < (r.x + r.w)) && (y >= r.y) && (y < (r.y + r.h)));
}

static bool in_rect(SDL_Point& p, SDL_Rect& r)
{
    return bool(SDL_PointInRect(&p, &r));
}

static bool is_y_within_bounds(int y, int y_top, int height)
{
    return (y >= y_top && y <= y_top + height);
}
} // geometry

/*
 * These utility functions provide basic grid-based boundary calculations.
 * They are likely better suited for a dedicated Grid class which can
 * simplify the logic in components like OutputPane.
 */
namespace grid {
#if 0
static int floor_boundary(int position, int cell_size)
{
    return std::floor(float(position) / cell_size) * cell_size;
}

static int ceil_boundary(int position, int cell_size)
{
    return std::ceil(float(position) / cell_size) * cell_size;
}
#endif
} // grid

// Should probably be kept for effecient mapping --
// although char16_t should probably be used instead of char32_t.
static const std::unordered_map<uint16_t, uint8_t> unicode_to_cp437 = {
    // Control characters and symbols
    /* NULL        */ { 0x263A, 0x01 }, { 0x263B, 0x02 }, { 0x2665, 0x03 },
    { 0x2666, 0x04 }, { 0x2663, 0x05 }, { 0x2660, 0x06 }, { 0x2022, 0x07 },
    { 0x25D8, 0x08 }, { 0x25CB, 0x09 }, { 0x25D9, 0x0A }, { 0x2642, 0x0B },
    { 0x2640, 0x0C }, { 0x266A, 0x0D }, { 0x266B, 0x0E }, { 0x263C, 0x0F },

    { 0x25BA, 0x10 }, { 0x25C4, 0x11 }, { 0x2195, 0x12 }, { 0x203C, 0x13 },
    { 0x00B6, 0x14 }, { 0x00A7, 0x15 }, { 0x25AC, 0x16 }, { 0x21A8, 0x17 },
    { 0x2191, 0x18 }, { 0x2193, 0x19 }, { 0x2192, 0x1A }, { 0x2190, 0x1B },
    { 0x221F, 0x1C }, { 0x2194, 0x1D }, { 0x25B2, 0x1E }, { 0x25BC, 0x1F },

    // ASCII, no mapping needed

    // Extended Latin characters and others
    { 0x2302, 0x7F },

    { 0x00C7, 0x80 }, { 0x00FC, 0x81 }, { 0x00E9, 0x82 }, { 0x00E2, 0x83 },
    { 0x00E4, 0x84 }, { 0x00E0, 0x85 }, { 0x00E5, 0x86 }, { 0x00E7, 0x87 },
    { 0x00EA, 0x88 }, { 0x00EB, 0x89 }, { 0x00E8, 0x8A }, { 0x00EF, 0x8B },
    { 0x00EE, 0x8C }, { 0x00EC, 0x8D }, { 0x00C4, 0x8E }, { 0x00C5, 0x8F },

    { 0x00C9, 0x90 }, { 0x00E6, 0x91 }, { 0x00C6, 0x92 }, { 0x00F4, 0x93 },
    { 0x00F6, 0x94 }, { 0x00F2, 0x95 }, { 0x00FB, 0x96 }, { 0x00F9, 0x97 },
    { 0x00FF, 0x98 }, { 0x00D6, 0x99 }, { 0x00DC, 0x9A }, { 0x00A2, 0x9B },
    { 0x00A3, 0x9C }, { 0x00A5, 0x9D }, { 0x20A7, 0x9E }, { 0x0192, 0x9F },

    { 0x00E1, 0xA0 }, { 0x00ED, 0xA1 }, { 0x00F3, 0xA2 }, { 0x00FA, 0xA3 },
    { 0x00F1, 0xA4 }, { 0x00D1, 0xA5 }, { 0x00AA, 0xA6 }, { 0x00BA, 0xA7 },
    { 0x00BF, 0xA8 }, { 0x2310, 0xA9 }, { 0x00AC, 0xAA }, { 0x00BD, 0xAB },
    { 0x00BC, 0xAC }, { 0x00A1, 0xAD }, { 0x00AB, 0xAE }, { 0x00BB, 0xAF },

    // Box drawing characters
    { 0x2591, 0xB0 }, { 0x2592, 0xB1 }, { 0x2593, 0xB2 }, { 0x2502, 0xB3 },
    { 0x2524, 0xB4 }, { 0x2561, 0xB5 }, { 0x2562, 0xB6 }, { 0x2556, 0xB7 },
    { 0x2555, 0xB8 }, { 0x2563, 0xB9 }, { 0x2551, 0xBA }, { 0x2557, 0xBB },
    { 0x255D, 0xBC }, { 0x255C, 0xBD }, { 0x255B, 0xBE }, { 0x2510, 0xBF },

    { 0x2514, 0xC0 }, { 0x2534, 0xC1 }, { 0x252C, 0xC2 }, { 0x251C, 0xC3 },
    { 0x2500, 0xC4 }, { 0x253C, 0xC5 }, { 0x255E, 0xC6 }, { 0x255F, 0xC7 },
    { 0x255A, 0xC8 }, { 0x2554, 0xC9 }, { 0x2569, 0xCA }, { 0x2566, 0xCB },
    { 0x2560, 0xCC }, { 0x2550, 0xCD }, { 0x256C, 0xCE }, { 0x2567, 0xCF },

    { 0x2568, 0xD0 }, { 0x2564, 0xD1 }, { 0x2565, 0xD2 }, { 0x2559, 0xD3 },
    { 0x2558, 0xD4 }, { 0x2552, 0xD5 }, { 0x2553, 0xD6 }, { 0x256B, 0xD7 },
    { 0x256A, 0xD8 }, { 0x2518, 0xD9 }, { 0x250C, 0xDA }, { 0x2588, 0xDB },
    { 0x2584, 0xDC }, { 0x258C, 0xDD }, { 0x2590, 0xDE }, { 0x2580, 0xDF },

    // Mathematical symbols and others
    { 0x03B1, 0xE0 }, { 0x00DF, 0xE1 }, { 0x0393, 0xE2 }, { 0x03C0, 0xE3 },
    { 0x03A3, 0xE4 }, { 0x03C3, 0xE5 }, { 0x00B5, 0xE6 }, { 0x03C4, 0xE7 },
    { 0x03A6, 0xE8 }, { 0x0398, 0xE9 }, { 0x03A9, 0xEA }, { 0x03B4, 0xEB },
    { 0x221E, 0xEC }, { 0x03C6, 0xED }, { 0x03B5, 0xEE }, { 0x2229, 0xEF },

    { 0x2261, 0xF0 }, { 0x00B1, 0xF1 }, { 0x2265, 0xF2 }, { 0x2264, 0xF3 },
    { 0x2320, 0xF4 }, { 0x2321, 0xF5 }, { 0x00F7, 0xF6 }, { 0x2248, 0xF7 },
    { 0x00B0, 0xF8 }, { 0x2219, 0xF9 }, { 0x00B7, 0xFA }, { 0x221A, 0xFB },
    { 0x207F, 0xFC }, { 0x00B2, 0xFD }, { 0x25A0, 0xFE }, { 0x00A0, 0xFF },
};


enum class ScrollAction : std::uint8_t {
    up,
    down,
    page_up,
    page_down
};

/*
 * SDL_EventType has storage uint32, but
 * only uses up to uint16 for use by its internal arrays. This leaves
 * plenty of room for custom types.
 */

/*
struct InternalEventType {
    enum Type : Uint32 {
        new_command_input = SDL_LASTEVENT + 1,
        new_input,
        clicked,
        font_size_changed,
        value_changed,
        text_selection_changed,
    };
};*/

enum class TextEntryType: std::uint8_t {
    input,
    output
};

namespace colors {
    // Default palette. Needs more. Needs configurable.
    const SDL_Color white = { 255, 255, 255, 255 };
//    const SDL_Color lightgray = { 211, 211, 211, 255 };
    const SDL_Color mediumgray = { 65, 65, 65, 255 };
//    const SDL_Color charcoal = { 54, 69, 79, 255 };
    const SDL_Color darkgray = { 27, 27, 27, 255 };

    const SDL_Color mauve = { 100,68,84, 255};
    const SDL_Color gold = { 247,193,41, 255};
    const SDL_Color teal = {  94, 173, 146, 255};
}

/*
static void render_texture(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const SDL_Rect& dst);
*/

static int set_draw_color(SDL_Renderer*, const SDL_Color&);

/*
 * Manages the lifetime of thread-specific, long-term SDL resources.
 * This class should not be used to store temporary resources.
 *
 * - Long-term resources: SDL objects (e.g., textures, renderers, windows)
 *   that persist beyond the scope of a single function or operation and are shared
 *   across multiple parts of a thread. This class ensures they are properly tracked
 *   and destroyed from the correct thread when no longer needed.
 *
 * - Temporary resources: SDL objects created and used entirely within the scope
 *   of a function or operation (e.g., a texture generated to render a single frame
 *   or a surface used for immediate computation). Such resources should be managed
 *   directly within the components that create them and do not need to be tracked
 *   by this class.
 */
class SDLThreadSpecificData {
public:
    using Texture = std::unique_ptr<SDL_Texture, decltype(sdl_console::SDL_DestroyTexture)>;
    using Renderer = std::unique_ptr<SDL_Renderer, decltype(sdl_console::SDL_DestroyRenderer)>;
    using Window = std::unique_ptr<SDL_Window, decltype(sdl_console::SDL_DestroyWindow)>;

    SDLThreadSpecificData() = default;

    SDL_Texture* CreateTextureFromSurface(SDL_Renderer* r, SDL_Surface* s)
    {
        auto* p = sdl_console::SDL_CreateTextureFromSurface(r, s);
        if (!p) { return nullptr; }
        textures_.emplace_back(make_unique_texture(p));
        return p;
    }

    SDL_Texture* CreateTexture(SDL_Renderer* r,
                                Uint32 format, int access,
                                int w, int h)
    {
        auto* p = sdl_console::SDL_CreateTexture(r, format, access, w, h);
        if (!p) { return nullptr; }
        textures_.emplace_back(make_unique_texture(p));
        return p;
    }

    void DestroyTexture(SDL_Texture *texture)
    {
        auto it = std::ranges::find_if(textures_,
                                        [texture](const Texture& ptr) {
                                            return ptr.get() == texture;
                                        });
        if (it != textures_.end()) {
            textures_.erase(it);
        }
    }

    SDL_Renderer* CreateRenderer(SDL_Window *handle, int index, Uint32 flags)
    {
        auto* p = sdl_console::SDL_CreateRenderer(handle, index, flags);
        if (!p) { return nullptr; }
        renderers_.emplace_back(make_unique_renderer(p));
        return p;
    }

    SDL_Window* CreateWindow(const char *title,
                              int x, int y, int w, int h,
                              Uint32 flags)
    {
        auto* p = sdl_console::SDL_CreateWindow(title, x, y, w, h, flags);
        if (!p) { return nullptr; }
        windows_.emplace_back(make_unique_window(p));
        return p;
    }

    void clear()
    {
        // Order matters!
        textures_.clear();
        renderers_.clear();
        windows_.clear();
    }

    ~SDLThreadSpecificData()
    {
        // TODO: Investigate whether it is safe to destroy resources
        // after SDL_Quit() is called.
        clear();
    }

private:
    static Texture make_unique_texture(SDL_Texture* texture)
    {
        return Texture(texture, sdl_console::SDL_DestroyTexture);
    }

    static Renderer make_unique_renderer(SDL_Renderer* renderer)
    {
        return Renderer(renderer, sdl_console::SDL_DestroyRenderer);
    }

    static Window make_unique_window(SDL_Window* window)
    {
        return Window(window, sdl_console::SDL_DestroyWindow);
    }

    std::vector<Window> windows_;
    std::vector<Renderer> renderers_;
    std::vector<Texture> textures_;
};

static thread_local SDLThreadSpecificData sdl_tsd;

/*
 * A lightweight implementation inspired by Qt's signals and slots, designed
 * for integrating with SDL_Event.
 *
 * TODO: Consider adding a custom event type for internal events to eliminate
 * the dependency on SDL_UserEvent.
 *
 * ISlot: An interface for slots, which are event handlers. A slot can:
 *   - Be invoked with an SDL_Event.
 *   - Be connected or disconnected from a signal.
 *   - Check its connection status.
 *
 * ISignal: An interface for signals, which are event emitters. A signal can:
 *   - Disconnect specific slots based on the event type.
 *   - Reconnect slots to specific event types.
 *   - Check if a slot is connected to an event type.
 *
 * Slot<EventType>: A templated implementation of ISlot for specific SDL event
 * types (e.g., SDL_KeyboardEvent, SDL_MouseButtonEvent). It wraps a callable
 * object (std::function) that is invoked when the event occurs.
 */

class ISlot {
public:
    virtual ~ISlot() = default;

    virtual void invoke(void* event_ptr) = 0;
    virtual Uint32 event_type() = 0;

    virtual void disconnect() = 0;
    virtual void connect() = 0;
    virtual bool is_connected() = 0;
};

class ISignal {
public:
    virtual ~ISignal() = default;
    virtual void disconnect(ISlot* slot) = 0;
    virtual void reconnect(ISlot* slot) = 0;
    virtual bool is_connected(ISlot* slot) = 0;
};

template <typename EventType>
class Slot : public ISlot {
public:
    using Func = std::function<void(EventType&)>;

    template <typename F>
    Slot(ISignal& emitter, uint32_t event_type, F&& func)
    : emitter_(emitter)
    , event_type_(event_type)
    , func_(std::forward<F>(func))
    {
    }

    void invoke(void* event_ptr) override
    {
        EventType& e = *static_cast<EventType*>(event_ptr);
        func_(e);
    }

    void disconnect() override { emitter_.disconnect(this); }
    void connect() override { emitter_.reconnect(this); }
    bool is_connected() override { return emitter_.is_connected(this); }
    uint32_t event_type() override { return event_type_; }

private:
    ISignal& emitter_;
    uint32_t event_type_;
    const Func func_;
};

class SignalEmitter : public ISignal {
public:
    template <typename EventType, typename F>
    ISlot* connect(uint32_t event_type, F&& func) {
        auto slot_ptr = make_slot<EventType>(event_type, std::forward<F>(func));

        if (emit_depth_ == 0) {
            slots_.active[event_type].emplace_back(slot_ptr);
        } else {
            pending_.active.insert(slot_ptr);
        }
        return slot_ptr;
    }

    template <typename EventType, typename F>
    ISlot* connect_later(uint32_t event_type, F&& func) {
        auto slot_ptr = make_slot<EventType>(event_type, std::forward<F>(func));

        if (emit_depth_ == 0) {
            slots_.inactive[event_type].emplace_back(slot_ptr);
        } else {
            pending_.inactive.insert(slot_ptr);
        }
        return slot_ptr;
    }

    void disconnect(ISlot* slot) override {
        if (emit_depth_ == 0) {
            move_slot(slots_.active, slots_.inactive, slot);
        } else {
            pending_.active.erase(slot);
            pending_.inactive.insert(slot);
        }
    }

    void reconnect(ISlot* slot) override {
        if (emit_depth_ == 0) {
            move_slot(slots_.inactive, slots_.active, slot);
        } else {
            pending_.inactive.erase(slot);
            pending_.active.insert(slot);
        }
    }

    bool is_connected(ISlot* slot) override {
        auto it = slots_.active.find(slot->event_type());
        if (it == slots_.active.end()) return false;

        return std::ranges::any_of(it->second, [slot](const auto* s) {
            return s == slot;
        });
    }

    void process_pending() {
        if (!pending_.inactive.empty()) {
            for (auto* slot : pending_.inactive) {
                move_slot(slots_.active, slots_.inactive, slot);
            }
            pending_.inactive.clear();
        }

        if (!pending_.active.empty()) {
            for (auto* slot : pending_.active) {
                move_slot(slots_.inactive, slots_.active, slot);
            }
            pending_.active.clear();
        }
    }

    template <typename EventType>
    void emit(EventType&& event) {
        auto it = slots_.active.find(event.type);
        if (it == slots_.active.end()) return;

        ++emit_depth_;
        for (auto* s : it->second) {
            s->invoke(&event);
        }
        --emit_depth_;

        if (emit_depth_ == 0) process_pending();
    }

    void clear() {
        slots_.active.clear();
        slots_.inactive.clear();
        pending_.active.clear();
        pending_.inactive.clear();
        owned_.clear();
        emit_depth_ = 0;
    }

private:
    std::vector<std::unique_ptr<ISlot>> owned_;

    using SlotMap = std::map<uint32_t, std::vector<ISlot*>>;
    struct Slots {
        SlotMap active;
        SlotMap inactive;
    };
    Slots slots_;

    struct PendingSlots {
        std::set<ISlot*> active;
        std::set<ISlot*> inactive;
    };
    PendingSlots pending_;

    int emit_depth_{0};

    template<typename EventType, typename F>
    ISlot* make_slot(uint32_t event_type, F&& func) {
        auto slot = std::make_unique<Slot<EventType>>(*this, event_type, std::forward<F>(func));
        ISlot* p = slot.get();
        owned_.emplace_back(std::move(slot));
        return p;
    }

    static void move_slot(SlotMap& from, SlotMap& to, ISlot* slot) {
        auto et = slot->event_type();
        auto it = from.find(et);
        if (it == from.end()) {
            // If it's not in from, it was pending
            to[et].emplace_back(slot);
            return;
        }

        auto& vec = it->second;
        auto sit = std::ranges::find_if(vec, [slot](const auto s) {
            return s == slot;
        });
        if (sit != vec.end()) {
            to[et].emplace_back(*sit);
            vec.erase(sit);
        }
    }
};

struct InternalEventType {
    enum Type : uint32_t {
        new_command_input = SDL_LASTEVENT + 1,
        new_input,
        clicked,
        font_size_changed,
        value_changed,
        text_selection_changed,
    };
};

struct NewCommandInputEvent {
    static constexpr uint32_t type = InternalEventType::new_command_input;
    std::u32string text;
};

struct NewInputEvent {
    static constexpr uint32_t type = InternalEventType::new_input;
    std::u32string text;
};

struct ClickedEvent {
    static constexpr uint32_t type = InternalEventType::clicked;
};

struct FontSizeChangedEvent {
    static constexpr uint32_t type = InternalEventType::font_size_changed;
};

struct ValueChangedEvent {
    static constexpr uint32_t type = InternalEventType::value_changed;
    int value{};
};

struct TextSelectionChangedEvent {
    static constexpr uint32_t type = InternalEventType::text_selection_changed;
    bool selected{};
};


/*
 * Stores configuration for components.
 */


class Property {
public:
    template <typename T>
    struct Key {
        std::string_view name;
    };

    using Value = std::variant<std::string, std::u32string, int64_t, int, size_t, SDL_Rect>;
    template <typename T>
    void set(Key<T> key, const T& value) {
        std::scoped_lock l(m_);
        props_[std::string(key.name)] = value;
    }

    template <typename T>
    std::optional<T> get(Key<T> key) {
        std::scoped_lock l(m_);
        auto it = props_.find(std::string(key.name));
        if (it == props_.end()) { return std::nullopt; }
        if (auto p = std::get_if<T>(&it->second)) { return *p; }
        return std::nullopt;
    }

private:
    std::unordered_map<std::string, Value> props_;
    std::recursive_mutex m_;
};


namespace property {
    // These must be set before SDLConsole::init()
    constexpr Property::Key<SDL_Rect> WINDOW_MAIN_RECT { "window.main.rect" };
    constexpr Property::Key<std::string> WINDOW_MAIN_TITLE { "window.main.title" };

    // Set any time.
    constexpr Property::Key<int> OUTPUT_SCROLLBACK { "output.scrollback" };

    constexpr Property::Key<std::u32string> PROMPT_TEXT { "prompt.text" };

    // Runtime information. Read only.
    constexpr Property::Key<size_t> RT_OUTPUT_ROWS { "rt.output.rows" };
    constexpr Property::Key<size_t> RT_OUTPUT_COLUMNS { "rt.output.columns" };
}

class Widget;
SDL_Texture* create_text_texture(Widget&, const std::u32string&, const SDL_Color&);

class TextEntry {
public:
    // A fragment is simply a chunk of text.
    struct Fragment {
        std::u32string_view text;
        size_t index;
        size_t start_offset; // 0-based start position of this fragment
        size_t end_offset; // 0-based send position of this fragment

        Fragment(std::u32string_view text, size_t index, size_t start_offset, size_t end_offset)
        : text(text)
        , index(index)
        , start_offset(start_offset)
        , end_offset(end_offset) {};

        Fragment(const Fragment&) = delete;
        Fragment& operator=(const Fragment&) = delete;
    };
    using Fragments = std::deque<Fragment>;

    TextEntryType type;
    size_t id;
    std::u32string text; // whole text
    std::optional<SDL_Color> color_opt;

    TextEntry() = default;

    ~TextEntry() = default;

    TextEntry(TextEntryType type, size_t id, std::u32string text, std::optional<SDL_Color>& color)
        : type(type)
        , id(id)
        , text(std::move(text))
        , color_opt(color)
        {};

    auto& add_fragment(std::u32string_view text, size_t start_offset, size_t end_offset)
    {
        fragments_.emplace_back(text, fragments_.size(), start_offset, end_offset);
        return fragments_.back();
    }

    void clear()
    {
        fragments_.clear();
    }

    auto size()
    {
        return fragments_.size();
    }

    Fragments& fragments()
    {
        return fragments_;
    }

    void wrap_text(int char_width, int viewport_width) {
        clear();

        int range_start = 0;
        int delim_idx = -1;
        int idx = 0;

        auto close = [this, &range_start](int end_idx) {
            if (end_idx >= range_start) {
                add_fragment(std::u32string_view(text).substr(range_start,
                                                              end_idx - range_start + 1),
                                                              range_start, end_idx);
            }
        };

        auto open = [&range_start](int new_start) {
            range_start = new_start + 1;
        };

        for (auto ch : text) {
            if (text::is_newline(ch)) {
                close(idx - 1); // Up to newline
                open(idx); // Skip new line
                delim_idx = -1;
            } else if (text::is_wspace(ch)) {
                delim_idx = idx; // Last whitespace
            }

            if ((idx - range_start + 1) * char_width >= viewport_width) {
                if (delim_idx != -1) {
                    close(delim_idx); // Wrap at the last character
                    open(delim_idx);
                    delim_idx = -1;
                } else {
                    close(idx); // Wrap at current character (no whitespace)
                    open(idx);
                }
            }

            ++idx;
        }

        // remaining text
        close(int(text.size()) - 1);
    }

    Fragment* fragment_from_offset(size_t index) {
        for (auto& frag : fragments_) {
            if (index >= frag.start_offset && index <= frag.end_offset) {
                return &frag;
            }
        }
        return nullptr;
    }

    TextEntry(const TextEntry&) = delete;
    TextEntry& operator=(const TextEntry&) = delete;

private:
    Fragments fragments_;
};

struct Glyph {
    SDL_Rect rect;
};

struct GlyphPosition {
    SDL_Rect src;   // from the font atlas
    SDL_Rect dst;
};

using GlyphPosVector = std::vector<GlyphPosition>;

// XXX, TODO: cleanup.
class Font : public SignalEmitter {
    class ScopedColor {
    public:
        explicit ScopedColor(Font* font) : font_(font) {}
        ScopedColor(Font* font, const SDL_Color& color)
        : font_(font)
        {
            set(color);
        }

        void set(const SDL_Color& color)
        {
            SDL_SetTextureColorMod(font_->texture_, color.r, color.g, color.b);
        }

        ~ScopedColor() {
            SDL_SetTextureColorMod(font_->texture_, 255, 255, 255);
        }

        ScopedColor(const ScopedColor&) = delete;
        ScopedColor& operator=(const ScopedColor&) = delete;

    private:
        Font* font_;
    };

// FIXME: make members private and add accessors
public:
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    std::vector<Glyph> glyphs;
    int char_width;
    int line_height;
    int orig_char_width;
    int orig_line_height;
    int vertical_spacing;
    float scale_factor { 1 };
    int size_change_delta_ { 2 };

    Font(SDL_Renderer* renderer, SDL_Texture* texture, std::vector<Glyph>& glyphs, int char_width, int line_height)
        : renderer_(renderer)
        , texture_(texture)
        , glyphs(glyphs)
        , char_width(char_width)
        , line_height(line_height)
        , orig_char_width(char_width)
        , orig_line_height(line_height)
    {
        // FIXME: magic numbers
        vertical_spacing = line_height * 0.5;
    }

    std::optional<ScopedColor> set_color(const std::optional<SDL_Color>& color)
    {
        if (!color.has_value()) { return std::nullopt; }
        return std::make_optional<ScopedColor>(this, color.value());
    }

    ScopedColor set_color(const SDL_Color& color)
    {
        return ScopedColor(this, color);
    }

    GlyphPosVector get_glyph_layout(const std::u32string_view& text, int x, int y)
    {
        GlyphPosVector g_pos;
        g_pos.reserve(text.size());
        for (const auto& ch : text) {
            char32_t index;
            if (ch <= 127) {
                index = ch;
            } else {
                index = unicode_glyph_index(ch);
            }
            Glyph& g = glyphs.at(index);
            SDL_Rect dst = { x, y + (vertical_spacing / 2), (int)(g.rect.w * scale_factor), (int)((g.rect.h * scale_factor)) };
            x += g.rect.w * scale_factor;
            g_pos.push_back({g.rect, dst});
        }
        return g_pos;
    }

    void render(const GlyphPosVector& vec) const
    {
        for (const auto& p : vec) {
            sdl_console::SDL_RenderCopy(renderer_, texture_, &p.src, &p.dst);
        }
    }

    void render(const std::u32string_view& text, int x, int y)
    {
        GlyphPosVector g_pos = get_glyph_layout(text, x, y);
        for (const auto& p : g_pos) {
            sdl_console::SDL_RenderCopy(renderer_, texture_, &p.src, &p.dst);
        }
    }

    // Get the surface size of a text.
    // Mono-spaced faces have the equal widths and heights.
    void size_text(const std::u32string& s, int& w, int& h) const
    {
        w = s.length() * char_width;
        h = line_height_with_spacing();
    }

    int line_height_with_spacing() const
    {
        return line_height + vertical_spacing;
    }

    void incr_size()
    {
        resize(size_change_delta_);
        emit(FontSizeChangedEvent{});
    }

    void decr_size()
    {
        resize(-size_change_delta_);
        emit(FontSizeChangedEvent{});
    }

    // Returns '?' if not found.
    static char32_t unicode_glyph_index(const char32_t ch)
    {
        auto it = unicode_to_cp437.find(ch);
        if (it != unicode_to_cp437.end()) {
            return it->second;
        }
        return '?';
    }

    Font clone()
    {
        return *this;
    }

    Font(Font&& other) noexcept
        : renderer_(other.renderer_)
        , texture_(other.texture_)
        , glyphs(std::move(other.glyphs))
        , char_width(other.char_width)
        , line_height(other.line_height)
        , orig_char_width(other.orig_char_width)
        , orig_line_height(other.orig_line_height)
        , vertical_spacing(other.vertical_spacing)
        , scale_factor(other.scale_factor)
    {
    }

    Font& operator=(Font&& other) noexcept
    {
        if (this != &other) {
            renderer_ = other.renderer_;
            texture_ = other.texture_;
            glyphs = other.glyphs;
            char_width = other.char_width;
            line_height = other.line_height;
            vertical_spacing = other.vertical_spacing;
            scale_factor = other.scale_factor;
            orig_char_width = other.char_width;
            orig_line_height = other.line_height;
        }
        return *this;
    }

    // No copy
    Font& operator=(const Font&) = delete;

private:
    void resize(int delta) {
        // Enforce minimum size 8, maximum size 32
        if ((char_width <= 8 && delta < 0) || (char_width >= 32 && delta > 0)) {
            return;
        }

        scale_factor = float(char_width + delta) / orig_char_width;

        char_width = orig_char_width * scale_factor;
        line_height = orig_line_height * scale_factor;
    }

    // For internal clone()
    Font(const Font& other)
    : renderer_(other.renderer_)
    , texture_(other.texture_)
    , glyphs(other.glyphs)
    , char_width(other.char_width)
    , line_height(other.line_height)
    , orig_char_width(other.orig_char_width)
    , orig_line_height(other.orig_line_height)
    , vertical_spacing(other.vertical_spacing)
    , scale_factor(other.scale_factor)
    {
    }
};

class BMPFontLoader {
    using FontMap = std::map<std::string, Font>;
private:
    FontMap fmap_;
    SDL_Renderer* renderer_;
    std::vector<SDL_Texture*> textures_;
public:
    BMPFontLoader() = default;
    ~BMPFontLoader() = default;

    void init(SDL_Renderer* renderer)
    {
        renderer_ = renderer;
    }

    Font* base_font()
    {
        return &fmap_.begin()->second;
    }

    Font* clone(std::string key, Font* font)
    {
        auto it = fmap_.emplace(key, font->clone());
        return &it.first->second;
    }

    Font* load(const std::string& path)
    {
        SDL_Surface* surface = DFSDL::DFIMG_Load(path.c_str());
        if (surface == nullptr) {
            return nullptr;
        }

        //  FIXME: hardcoded magenta
        // Make this keyed color transparent.
        uint32_t bg_color = sdl_console::SDL_MapRGB(surface->format, 255, 0, 255);
        sdl_console::SDL_SetColorKey(surface, SDL_TRUE, bg_color);

        // Create a surface in ARGB8888 format, and replace the keyed color
        // with fully transparant pixels. This step completely removes the color.
        // NOTE: Do not use surface->pitch
        SDL_Surface* conv_surface = sdl_console::SDL_CreateRGBSurfaceWithFormat(0, surface->w, surface->h, 32,
                                                                                SDL_PixelFormatEnum::SDL_PIXELFORMAT_ARGB8888);
        sdl_console::SDL_BlitSurface(surface, nullptr, conv_surface, nullptr);
        sdl_console::SDL_FreeSurface(surface);
        surface = conv_surface;

         // FIXME: magic numbers
        int cols = 16;
        int rows = 16;
        int glyph_w = surface->w / cols;
        int glyph_h = surface->h / rows;

        std::vector<Glyph> glyphs;
        glyphs = build_glyph_rects(surface->w, surface->h, cols, rows);

        auto* texture = sdl_tsd.CreateTextureFromSurface(renderer_, surface);
        sdl_console::SDL_FreeSurface(surface);
        if (!texture ) {
            std::cerr << "SDL_CreateTextureFromSurface Error: " << sdl_console::SDL_GetError() << '\n';
            return nullptr;
        }
        sdl_console::SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        textures_.push_back(texture);

        // FIXME: magic numbers
        auto it = fmap_.emplace(path, Font(renderer_, texture, glyphs, glyph_w, glyph_h));
        return &it.first->second;
    }

    BMPFontLoader(const BMPFontLoader&) = delete;
    BMPFontLoader& operator=(const BMPFontLoader&) = delete;


    BMPFontLoader(BMPFontLoader&& other) noexcept = default;
    BMPFontLoader& operator=(BMPFontLoader&& other) noexcept = default;

private:
    static std::vector<Glyph> build_glyph_rects(int sheet_w, int sheet_h, int columns, int rows)
    {
        int tile_w = sheet_w / columns;
        int tile_h = sheet_h / rows;
        int total_glyphs = rows * columns;

        std::vector<Glyph> glyphs;
        glyphs.reserve(total_glyphs);

        for (int i = 0; i < total_glyphs; ++i) {
            int r = i / columns;
            int c = i % columns;
            Glyph glyph;
            glyph.rect = {
                .x = tile_w * c,
                .y = tile_h * r,
                .w = tile_w,
                .h = tile_h
            }; // Rectangle in pixel dimensions
            glyphs.push_back(glyph);
        }
        return glyphs;
    }
};

class MainWindow;

/*
 * Shared context object for a window and its children.
 *
 * Non-movable. Non-copyable.
 */
struct ImplContext {
    Property& props;

    explicit ImplContext(Property& p) : props(p) {};

    struct UI {
        struct Window {
            SDL_Window* handle{nullptr};
            SDL_Renderer* renderer{nullptr};
            BMPFontLoader font_loader;
            decltype(sdl_console::SDL_GetWindowID(nullptr)) id{};
            SDL_Point mouse_pos{};

            Window() = default;

            void init(SDL_Window* h, SDL_Renderer* r)
            {
                handle = h;
                renderer = r;
                id = sdl_console::SDL_GetWindowID(h);
                font_loader.init(r);
            }

        } window;
    } ui;

    ImplContext(ImplContext&&) = delete;
    ImplContext& operator=(ImplContext&&) = delete;

    ImplContext(const ImplContext&) = delete;
    ImplContext& operator=(const ImplContext&) = delete;
};

namespace layout {
enum class Align : uint8_t {
    None   = 0,
    Left   = 1 << 0,
    Right  = 1 << 1,
    HCenter= 1 << 2,
    Top    = 1 << 3,
    Bottom = 1 << 4,
    VCenter= 1 << 5,
    Fill   = 1 << 6,
};

static Align operator|(Align a, Align b) {
    return static_cast<Align>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
    );
}

#if 0
static Align& operator|=(Align& a, Align b) {
    a = a | b;
    return a;
}

static bool operator&(Align a, Align b) {
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}
#endif

static bool has_align(Align a, Align flag) {
    return (uint8_t(a) & uint8_t(flag)) != 0;
}

struct Margins {
    int left   = 0;
    int top    = 0;
    int right  = 0;
    int bottom = 0;
};

struct Layout {
    Margins margin {.left = 0, .top = 0, .right = 0, .bottom = 0};
    int stretch = 0;           // relative flex factor (0 = fixed)
    int preferred_width = 0;
    int preferred_height = 0;
    Align align = Align::Fill;
};

} // namespace layout

// TODO: needs work
// Non-movable. Non-copyable.
class Widget : public SignalEmitter {
public:
    Widget* parent;
    Font* font;
    SDL_Rect frame {};
    ImplContext& context;
    Widget& window;

    layout::Layout layout;

    std::vector<std::unique_ptr<Widget>> children;

    explicit Widget(Widget* parent)
        : parent(parent)
        , font(parent->font)
        , frame(parent->frame)
        , context(parent->context)
        , window(parent->window)
    {
    }

    Widget(Widget* parent, const SDL_Rect& rect)
        : parent(parent)
        , font(parent->font)
        , frame(rect)
        , context(parent->context)
        , window(parent->window)
    {
    }

    // Constructor for Window
    explicit Widget(ImplContext& ctx)
        : parent(nullptr)
        , context(ctx)
        , window(*this)

    {
    }

    SDL_Renderer* renderer() const
    {
        return context.ui.window.renderer;
    }

    SDL_Point mouse_pos() const
    {
        return context.ui.window.mouse_pos;
    }

    SDL_Point map_to_local(const SDL_Point& point) const
    {
        return { point.x - frame.x, point.y - frame.y };
    }

    static SDL_Point map_to(const SDL_Point& point, const SDL_Rect& rect)
    {
        return { point.x - rect.x, point.y - rect.y };
    }

    Property& props() const
    {
        return context.props;
    }

    virtual void render() = 0;
    //virtual void resize(const SDL_Rect& rect) = 0;

    virtual void layout_children() {};

    virtual void resize(const SDL_Rect& new_frame) {
        frame = new_frame;

        layout_children();

        for (auto& child : children) {
            child->resize(child->frame);
        }
    }

    template <typename T, typename... Args>
    T& add_child(Args&&... args) {
        auto ptr = std::make_unique<T>(this, std::forward<Args>(args)...);
        T& ref = *ptr;
        children.emplace_back(std::move(ptr));
        return ref;
    }

    ~Widget() override = default;

    Widget(Widget&&) = delete;
    Widget& operator=(Widget&&) = delete;

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
};

class VBox : public Widget {
public:
    int spacing = 0;

    explicit VBox(Widget* parent)
    : Widget(parent)
    {}

    explicit VBox(ImplContext& ctx)
    : Widget(ctx)
    {}

    void render() override
    {
        for (auto& child : children) {
            child->render();
        }
    }

    void layout_children() override {
        if (children.empty())
            return;

        int total_stretch = 0;
        int total_fixed = 0;

        const int avail_w = frame.w - (layout.margin.left + layout.margin.right);
        const int avail_h = frame.h - (layout.margin.top + layout.margin.bottom);

        for (auto& child : children) {
            if (child->layout.stretch > 0) {
                total_stretch += child->layout.stretch;
            } else {
                total_fixed += (child->layout.preferred_height > 0 ? child->layout.preferred_height : child->frame.h);
            }
        }

        total_fixed += spacing * (std::ssize(children) - 1);
        int available = std::max(avail_h - total_fixed, 0);
        int offset = frame.y + layout.margin.top;

        for (size_t i = 0; i < children.size(); ++i) {
            auto& child = children[i];
            SDL_Rect r = child->frame;

            r.x = frame.x + layout.margin.left;
            r.w = avail_w;

            if (child->layout.stretch > 0) {
                r.h = available * child->layout.stretch / total_stretch;
            } else if (child->layout.preferred_height > 0) {
                r.h = child->layout.preferred_height;
            }

            r.y = offset;
            offset += r.h;
            if (i < children.size() - 1)
                offset += spacing;

            // Horizontal alignment
            if (child->layout.align != layout::Align::Fill) {
                if (has_align(child->layout.align, layout::Align::HCenter))
                    r.x = frame.x + layout.margin.left + (avail_w - r.w) / 2;
                else if (has_align(child->layout.align, layout::Align::Right))
                    r.x = frame.x + avail_w - r.w - layout.margin.right;
            }
            child->resize(r);
        }
    }
};

class HBox : public Widget {
public:
    int spacing = 0;

    explicit HBox(Widget* parent)
    : Widget(parent)
    {}

    explicit HBox(ImplContext& ctx)
    : Widget(ctx)
    {}

    void render() override
    {
        for (auto& child : children) {
            child->render();
        }
    }

    void layout_children() override
    {
        if (children.empty())
            return;

        int total_stretch = 0;
        int total_fixed = 0;

        const int avail_w = frame.w - (layout.margin.left + layout.margin.right);
        const int avail_h = frame.h - (layout.margin.top + layout.margin.bottom);

        for (auto& child : children) {
            if (child->layout.stretch > 0) {
                total_stretch += child->layout.stretch;
            } else {
                total_fixed += (child->layout.preferred_width  > 0 ? child->layout.preferred_width  : child->frame.w);
            }
        }

        total_fixed += spacing * (std::ssize(children) - 1);
        int available = std::max(avail_w - total_fixed, 0);
        int offset = frame.x + layout.margin.left;

        for (size_t i = 0; i < children.size(); ++i) {
            auto& child = children[i];
            SDL_Rect r = child->frame;

            r.y = frame.y + layout.margin.top;

            if (child->layout.preferred_height > 0) {
                r.h = child->layout.preferred_height;
            } else {
                r.h = avail_h;
            }

            if (child->layout.stretch > 0) {
                r.w = available * child->layout.stretch / total_stretch;
            } else if (child->layout.preferred_width >  0) {
                r.w = child->layout.preferred_width;
            }

            r.x = offset;
            offset += r.w;
            if (i < children.size() - 1)
                offset += spacing;

            // Vertical alignment
            if (child->layout.align != layout::Align::Fill) {
                if (layout::has_align(child->layout.align, layout::Align::VCenter))
                    r.y = frame.y + (avail_h - r.h) / 2;
                else if (layout::has_align(child->layout.align, layout::Align::Bottom))
                    r.y = frame.y + (avail_h - r.h);
            }
            child->resize(r);
        }
    }
};

class Cursor {
private:
    SDL_Renderer* const renderer_;
    size_t position_ { 0 };
public:
    SDL_Rect rect;
    bool visible{true}; // false when scrolling away from the line it sits on
    bool rebuild{true};

    explicit Cursor(SDL_Renderer* renderer) : renderer_(renderer)
    {
    }

    size_t position() const
    {
        return position_;
    }

    void reset_position()
    {
        position_ = 0;
    }

    void render() const
    {
        if (!visible) { return; }
        SDL_Color c = colors::white;
        c.a = 128;
        set_draw_color(renderer_, c); // 128 - about 50% transparant
        sdl_console::SDL_RenderFillRect(renderer_, &rect);
        c.a = 255;
        set_draw_color(renderer_, c);
    }

    Cursor& operator=(size_t position) {
        position_ = position;
        rebuild = true;
        return *this;
    }

    Cursor& operator++() {
        ++position_;
        rebuild = true;
        return *this;
    }

    Cursor& operator--() {
        if (position_ > 0) {
            --position_;
            rebuild = true;
        }
        return *this;
    }

    Cursor& operator+=(std::size_t n) {
        position_ += n;
        rebuild = true;
        return *this;
    }

    Cursor& operator-=(std::size_t n) {
        position_ = (n > position_) ? 0 : position_ - n;
        rebuild = true;
        return *this;
    }

    Cursor(const Cursor&) = delete;
    Cursor& operator=(const Cursor&) = delete;
    Cursor(Cursor&&) = delete;
    Cursor& operator=(Cursor&&) = delete;
};

struct VisibleRow {
    //TextEntry& entry;
    size_t entry_id;
    TextEntry::Fragment& frag;
    SDL_Point coord;
    GlyphPosVector gpv;
    std::optional<SDL_Color> color;
};

struct VisibleRowsCache {
    std::vector<VisibleRow> rows;
    bool rebuild { true };

    VisibleRow* find_row_at_y(Font* font, int y)
    {
        auto it = std::ranges::find_if(rows, [font, y](VisibleRow const& r) {
            return geometry::is_y_within_bounds(y, r.coord.y, font->line_height);
        });

        if (it != rows.end()) {
            return &(*it);
        }

        return nullptr;
    }
};

// Non-movable. Non-copyable.
class Prompt : public Widget {
public:
    // Holds wrapped lines from input
    TextEntry entry;
    // The text of the prompt itself.
    std::u32string prompt_text;
    // The input portion of the prompt.
    std::u32string* input;
    std::u32string saved_input;
    Cursor cursor;
    bool rebuild { true };
    /*
     * For input history.
     * use deque to hold a stable reference.
     */
    std::deque<std::u32string> history;
    int history_index;

    explicit Prompt(Widget* parent)
        : Widget(parent), cursor(renderer())
    {
        input = &history.emplace_back(U"");

        set_prompt_text(props().get(property::PROMPT_TEXT).value_or(U"> "));

        window.connect<SDL_KeyboardEvent>(SDL_KEYDOWN, [this](auto& e) {
            on_SDL_KEYDOWN(e);
        });

        window.connect<SDL_TextInputEvent>(SDL_TEXTINPUT, [this](auto& e) {
            put_input_at_cursor(text::from_utf8(e.text));
        });
    }

    ~Prompt() override = default;

    /* OutputPane does this */
    void render() override
    {
    }

    void put_input_from_clipboard()
    {
        auto* str = sdl_console::SDL_GetClipboardText();
        if (*str != '\0') {
            put_input_at_cursor(text::from_utf8(str));
        }
        // Always free, even when empty.
        sdl_console::SDL_free(str);
    }

    void on_SDL_KEYDOWN(const SDL_KeyboardEvent& e)
    {
        // TODO: check if keysym.sym mapping is universally locale friendly
        auto sym = e.keysym.sym;
        switch (sym) {
        case SDLK_BACKSPACE:
            erase_input();
            break;

        case SDLK_UP:
            set_input_from_history(ScrollAction::up);
            break;

        case SDLK_DOWN:
            set_input_from_history(ScrollAction::down);
            break;

        case SDLK_LEFT:
            move_cursor_left();
            break;

        case SDLK_RIGHT:
            move_cursor_right();
            break;

        case SDLK_RETURN:
            new_command_input();
        [[fallthrough]];
        case SDLK_HOME:
            cursor = 0;
            break;

        case SDLK_END:
            cursor = input->length();
            break;

        case SDLK_b:
            if (sdl_console::SDL_GetModState() & KMOD_CTRL) {
                cursor = text::find_prev_word(*input, cursor.position());
            }
            break;

        case SDLK_f:
            if (sdl_console::SDL_GetModState() & KMOD_CTRL) {
                cursor = text::find_next_word(*input, cursor.position());
            }
            break;
        case SDLK_c:
            if (sdl_console::SDL_GetModState() & KMOD_CTRL) {
                *input += U"^C";
                save();
            }
            break;
        default:;
        }
    }

    void set_command_history(std::deque<std::u32string> saved_history)
    {
        std::swap(history, saved_history);
        input = &history.emplace_back(U"");
        history_index = history.size() - 1;
        cursor = 0;
        rebuild = true;
    }

    void new_command_input()
    {
        emit(NewCommandInputEvent{.text = *input});

        // If empty, log an empty line? But don't add it to history.
        if (input->empty()) { return; }

        if (input == &history.back()) {
            input = &history.emplace_back(U"");
        } else { // Command came from history.
            input = &history.back();
        }
        history_index = history.size() - 1;
        cursor = 0;
        wrap_text();
    }

    // Save interrupted input
    void save()
    {
        saved_input = *input;
        emit(NewInputEvent{.text = *input});
        input->clear();
        cursor = 0;
        wrap_text();
    }

    void restore()
    {
        *input = saved_input;
        cursor = input->length();
        wrap_text();
    }

    void set_prompt_text(const std::u32string& value)
    {
        prompt_text = value;
        wrap_text();
    }

    /*
     * Set the current line. We can go UP (next) or DOWN (previous) through the
     * lines. This function essentially acts as a history viewer. This function
     * will skip lines with zero length. The cursor is always set to the length of
     * the line's input.
     */
    void set_input_from_history(const ScrollAction sa)
    {
        if (history.empty()) { return; }

        if (sa == ScrollAction::up && history_index > 0) {
            history_index--;
        } else if (sa == ScrollAction::down && history_index < std::ssize(history) - 1) {
            history_index++;
        } else {
            return;
        }

        input = &history.at(history_index);
        cursor = input->length();
        wrap_text();
    }

    void put_input_at_cursor(const std::u32string& str)
    {
        /* if cursor is at end of line, it's a simple concatenation */
        if (cursor.position() == input->length()) {
            *input += str;
        } else {
        /* else insert text into line at cursor's index */
            input->insert(cursor.position(), str);
        }
        cursor += str.length();

        wrap_text();
    }

    void set_input(const std::u32string& str)
    {
        *input = str;
        cursor = str.length();
        wrap_text();
    }

    void erase_input()
    {
        if (cursor.position() == 0 || input->empty()) {
            return;
        }

        if (input->length() == cursor.position()) {
            input->pop_back();
        } else {
            /* else shift the text from cursor left by one character */
            input->erase(cursor.position()-1, 1);
        }
        --cursor;
        wrap_text();
    }

    void move_cursor_left()
    {
        if (cursor.position() > 0) {
            --cursor;
        }
    }

    void move_cursor_right()
    {
        if (cursor.position() < input->length()) {
            ++cursor;
        }
    }

    void resize(const SDL_Rect& r) override
    {
        frame = r;
        wrap_text();
    }

    void wrap_text()
    {
        entry.text = prompt_text + *input;
        entry.wrap_text(font->char_width, frame.w);
        rebuild = true;
    }

    void update_cursor_geometry_for_render(std::span<const VisibleRow> vrows)
    {
        if (entry.fragments().empty()) { // Shouldn't happen'
             return;
        }

        // cursor's starting position
        auto cursor_pos = cursor.position() + prompt_text.length();
        TextEntry::Fragment *line;

        // cursor is at the end
        if (cursor_pos == entry.text.length()) {
            line = &entry.fragments().back();
        // else find the line containing the cursor
        } else {
            line = entry.fragment_from_offset(cursor_pos);
            if (!line) {
                return; // should not happen
            }
        }

        auto it = std::ranges::find_if(vrows, [line](VisibleRow const& r) {
            return &r.frag == line;
        });

        if (it == vrows.end()) {
            cursor.visible = false;
            return;
        }

        cursor.visible = true;
        VisibleRow const& vr = *it;

        auto lh = font->line_height_with_spacing();
        auto cw = font->char_width;
        auto cx = int(cursor_pos - line->start_offset) * cw;
        auto cy = vr.coord.y;

        cursor.rect = { .x = cx,
                        .y = cy,
                        .w = cw,
                        .h = lh };
    }

    void render_cursor() const
    {
        cursor.render();
    }

    bool rebuild_needed() const
    {
        return rebuild || cursor.rebuild;
    }

    Prompt(Prompt&&) = delete;
    Prompt& operator=(Prompt&&) = delete;

    Prompt(const Prompt&) = delete;
    Prompt& operator=(const Prompt&) = delete;
};

// Non-movable. Non-copyable.
class Scrollbar : public Widget {
private:
    struct Thumb {
        SDL_Rect rect{};
    };

    int page_size_; // Height of visible area.
    int content_size_ { 0 }; // Total height of content.
    int scroll_offset_ { 0 };
    bool depressed_ { false };
    ISlot* mouse_motion_slot_ { nullptr };
    Thumb thumb_;

public:
    Scrollbar(Widget* parent)
        : Widget(parent)
    {
        window.connect<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONDOWN, [this](auto& e) {
            on_SDL_MOUSEBUTTONDOWN(e);
        });

        window.connect<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONUP, [this](auto& e) {
            on_SDL_MOUSEBUTTONUP(e);
        });

        mouse_motion_slot_ = window.connect_later<SDL_MouseMotionEvent>(SDL_MOUSEMOTION, [this](auto& e) {
            on_SDL_MOUSEMOTION(e);
        });

        thumb_.rect = frame;
        set_thumb_height();
    }

    void resize(const SDL_Rect& r) override
    {
        frame = r;
        thumb_.rect = frame;
        resize_thumb();
    }

    void set_page_size(size_t size)
    {
        page_size_ = size;
        resize_thumb();
    }

    void set_content_size(size_t size)
    {
        content_size_ = size;
        resize_thumb();
    }

    void scroll_to(size_t position)
    {
        scroll_offset_ = position;
        move_thumb_to(track_position_from_scroll_offset());
    }

    void render() override
    {
        set_draw_color(renderer(), colors::gold);

        SDL_RenderDrawRect(renderer(), &frame);

        set_draw_color(renderer(), colors::mauve);

        // FIXME: hardcoded magic
        SDL_Rect tr{thumb_.rect.x + 4, thumb_.rect.y + 4, thumb_.rect.w - 8, thumb_.rect.h - 8};
        SDL_RenderFillRect(renderer(), &tr);

        set_draw_color(renderer(), colors::darkgray);
    }

    Scrollbar(Scrollbar&&) = delete;
    Scrollbar& operator=(Scrollbar&&) = delete;

    Scrollbar(const Scrollbar&) = delete;
    Scrollbar& operator=(const Scrollbar&) = delete;

private:
    void resize_thumb()
    {
        set_thumb_height();
        move_thumb_to(track_position_from_scroll_offset());
    }

    void on_SDL_MOUSEBUTTONDOWN(auto& e)
    {
        if (!geometry::in_rect(e.x, e.y, frame)) {
            return;
        }

        if (!mouse_motion_slot_->is_connected()) {
            mouse_motion_slot_->connect();
        }

        depressed_ = true;
        scroll_offset_ = scroll_offset_from_track_position(e.y);
        move_thumb_to(track_position_from_scroll_offset());
        emit(ValueChangedEvent{.value = scroll_offset_});
    }

    void on_SDL_MOUSEBUTTONUP(auto& /*e*/)
    {
        if (depressed_) {
            depressed_ = false;
            mouse_motion_slot_->disconnect();
        }
    }

    void on_SDL_MOUSEMOTION(auto& e)
    {
        if (!depressed_) { return; }

        scroll_offset_ = scroll_offset_from_track_position(e.y);
        move_thumb_to(track_position_from_scroll_offset());

        emit(ValueChangedEvent{.value = scroll_offset_});
    }

    int calculate_thumb_position(int target_y)
    {
        int track_top = frame.y;
        int track_bot = frame.y + frame.h;

        // Position with offset and constrain within track limits
        return std::clamp(target_y - thumb_.rect.h, track_top, track_bot - thumb_.rect.h);
    }

    void move_thumb_to(int y)
    {
        thumb_.rect.y = calculate_thumb_position(y);
    }

    void set_thumb_height()
    {
        if (content_size_ > 0) {
            float scroll_ratio = float(page_size_) / content_size_;
            int h = (int)std::round(scroll_ratio * frame.h);
            // 30 is minimum height.
            thumb_.rect.h = std::clamp(h, 30, frame.h);
        } else {
            thumb_.rect.h = frame.h;
        }
    }

    int scroll_offset_from_track_position(int y)
    {
        int track_h = frame.h;

        // Track position is aligns with the middle of the thumb.
        int thumb_mid_y = y - (thumb_.rect.h / 2);
        thumb_mid_y = std::clamp(thumb_mid_y, frame.y, frame.y + track_h - thumb_.rect.h);

        float pos_ratio = (float)(thumb_mid_y - frame.y) / (track_h - thumb_.rect.h);
        int offset = (int)((1.0F - pos_ratio) * (content_size_ - page_size_));

        // Ensure the scroll offset does not go beyond the valid range
        return std::clamp(offset, 0, content_size_ - page_size_);
    }

    int track_position_from_scroll_offset()
    {
        int track_h = frame.h;

        if (content_size_ <= page_size_ || content_size_ == 0) {
            return frame.y;
        }

        float offset_ratio = (float)scroll_offset_ / (content_size_);
        int pos = (int)((1.0F - offset_ratio) * track_h);

        return pos + frame.y;
    }

};

class Button : public Widget {
public:
    Button(Widget* parent, std::u32string& label)
        : Widget(parent)
        , label(label)
    {
        resize_button();
        window.connect<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONDOWN, [this](auto& e) {
            on_SDL_MOUSEBUTTONDOWN(e);
        });

        window.connect<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONUP, [this](auto& e) {
            on_SDL_MOUSEBUTTONUP(e);
        });

        font->connect<FontSizeChangedEvent>(InternalEventType::font_size_changed, [this](auto& /*e*/) {
            resize_button();
        });
    }

    void resize(const SDL_Rect& /*r*/) override
    {
        label_rect.x = frame.x + (frame.w / 2) - (label_rect.w / 2);
        label_rect.y = (frame.h / 2) - (label_rect.h / 2);
        resize_button();
    }

    void resize_button()
    {
        font->size_text(this->label, label_rect.w, label_rect.h);
        frame.w = label_rect.w + (font->char_width * 2);
    }

    void on_SDL_MOUSEBUTTONDOWN(SDL_MouseButtonEvent& e)
    {
        if (!geometry::in_rect(e.x, e.y, frame)) {
            return;
        }
        depressed = true;
    }

    void on_SDL_MOUSEBUTTONUP(SDL_MouseButtonEvent& e)
    {
        if (!geometry::in_rect(e.x, e.y, frame)) {
            if (depressed) {
                depressed = false;
            }
            return;
        }

        if (depressed) {
            emit(ClickedEvent{});
            depressed = false;
        }
    }

    void render() override
    {
        if (enabled) {
            SDL_Point coord = mouse_pos();
            if (depressed) {
                set_draw_color(renderer(), colors::teal);
                sdl_console::SDL_RenderFillRect(renderer(), &frame);
                // SDL_RenderDrawRect(ui.renderer, &w.rect);
                set_draw_color(renderer(), colors::darkgray);
            } else if (geometry::in_rect(coord, frame)) {
                set_draw_color(renderer(), colors::teal);
                sdl_console::SDL_RenderDrawRect(renderer(), &frame);
                set_draw_color(renderer(), colors::darkgray);
            }
            font->render(label, label_rect.x, label_rect.y);
        } else {
            auto scoped_color = font->set_color(colors::mediumgray);
            font->render(label, label_rect.x, label_rect.y);
        }
    }

    Button(Button&&) = delete;
    Button& operator=(Button&&) = delete;

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    std::u32string label;
    SDL_Rect label_rect {};
    bool depressed { false };
    bool enabled { true };
};

class Toolbar : public Widget {
public:
    explicit Toolbar(Widget* parent);
    ~Toolbar() override = default;
    void render() override;
    void resize(const SDL_Rect& rect) override;
    void layout_buttons();
    Button* add_button(std::u32string text);
    int compute_widgets_startx();
    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;
    // Should be changed to children and probably moved to base class
    std::deque<std::unique_ptr<Widget>> widgets;
};

class CommandPipe {
public:
    CommandPipe() = default;

    void make_connection(SignalEmitter& emitter)
    {
        emitter.connect<NewCommandInputEvent>(InternalEventType::new_command_input, [this](auto& e) {
            push(e.text);
        });
    }

    void push(const std::u32string& s)
    {
        {
            std::scoped_lock lock(mutex_);
            queue_.push(s);
        }
        cv_.notify_one();
    }

    void shutdown()
    {
        {
            std::scoped_lock lock(mutex_);
            shutdown_ = true;
        }
        cv_.notify_all();
    }

    /* This function may be called recursively */
    int wait_get(std::string& buf)
    {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || shutdown_; });

        if (shutdown_) {
            return -1;
        }

        buf = text::to_utf8(queue_.front());
        queue_.pop();
        return buf.length();
    }

    ~CommandPipe()
    {
        shutdown();
    }

    CommandPipe(CommandPipe&&) = delete;
    CommandPipe& operator=(CommandPipe&&) = delete;

    CommandPipe(const CommandPipe&) = delete;
    CommandPipe& operator=(const CommandPipe&) = delete;

private:
    std::condition_variable_any cv_;
    std::recursive_mutex mutex_;
    std::queue<std::u32string> queue_;
    bool shutdown_{false};
};

class TextSelection {
public:
    struct Anchor {
        size_t entry_id{-1UL};
        size_t frag_index;
        size_t column;

        bool operator<(const Anchor& other) const {
            if (entry_id != other.entry_id)
                return entry_id < other.entry_id;
            if (frag_index != other.frag_index)
                return frag_index < other.frag_index;
            return column < other.column;
        }

        bool operator==(const Anchor& other) const {
            return entry_id == other.entry_id &&
            frag_index == other.frag_index &&
            column == other.column;
        }
    };

    Anchor begin{};
    Anchor end{};

    void reset()
    {
        begin = {};
        end = {};
    }

    bool active() const {
        return begin.entry_id != -1UL && end.entry_id != -1UL;
    }

    std::vector<SDL_Rect> to_rects(const Font* font,
                                   std::span<const VisibleRow> rows) const
    {
        if (!active()) return {};

        // normalize so we always go from top to bottom
        auto [top, bottom] = std::minmax(begin, end);

        auto top_pos = std::make_pair(top.entry_id, top.frag_index);
        auto bottom_pos = std::make_pair(bottom.entry_id, bottom.frag_index);

        std::vector<SDL_Rect> rects;
        for (const auto &row : rows) {
            const auto &frag = row.frag;
            auto frag_pos = std::make_pair(row.entry_id, frag.index);

            // skip outside range
            if (frag_pos < top_pos || frag_pos > bottom_pos) continue;

            size_t frag_len = frag.text.size();
            size_t sel_start = 0;
            size_t sel_end = frag_len;

            // If this fragment is the top anchor fragment
            if (frag_pos == top_pos) {
                sel_start = std::min(top.column, frag_len);
            }

            // If this fragment is the bottom anchor fragment
            if (frag_pos == bottom_pos) {
                sel_end = std::min(bottom.column, frag_len);
            }

            if (sel_start >= sel_end) continue; // empty selection

            int x = int(sel_start * font->char_width);
            int w = int((sel_end - sel_start) * font->char_width);
            int y = row.coord.y;

            rects.push_back({.x = x,
                             .y = y,
                             .w = w,
                             .h = font->line_height_with_spacing()});
        }
        return rects;
    }

std::vector<std::u32string_view> get_selected_text(std::deque<TextEntry>& entries) const
{
    if (!active()) return {};

    // normalize so we always go from top to bottom
    const auto [top, bottom] = std::minmax(begin, end);

    std::vector<std::u32string_view> result;

    for (auto& entry : entries | std::views::reverse) {
        if (entry.id < top.entry_id || entry.id > bottom.entry_id)
            continue; // skip outside selection

        auto vec = get_selected_text_from_one(entry);
        if (!vec.empty()) {
            std::ranges::move(vec, std::back_inserter(result));
        }
    }

    return result;
}

std::vector<std::u32string_view> get_selected_text_from_one(TextEntry& entry) const
{
    const auto [top, bottom] = std::minmax(begin, end);

    std::vector<std::u32string_view> result;
    if (entry.id < top.entry_id || entry.id > bottom.entry_id)
        return result;// skip outside selection

    for (const auto& frag : entry.fragments()) {
        if (frag.index < top.frag_index || frag.index > bottom.frag_index)
            continue;

        size_t sel_start = 0;
        size_t sel_end = frag.text.size();

        bool is_top    = (entry.id == top.entry_id && frag.index == top.frag_index);
        bool is_bottom = (entry.id == bottom.entry_id && frag.index == bottom.frag_index);

        if (is_top) sel_start = std::min(top.column, frag.text.size());
        if (is_bottom) sel_end = std::min(bottom.column, frag.text.size());

        if (sel_start >= sel_end) continue; // empty selection

        result.push_back(frag.text.substr(sel_start, sel_end - sel_start));
    }
    return result;
}

};

class OutputPane : public Widget {
public:
    // Use deque to hold a stable reference.
    std::deque<TextEntry> entries;
    VisibleRowsCache visible_rows;
    Prompt prompt;
    Scrollbar* scrollbar{nullptr};
    // Scrollbar could be made optional.
    int scroll_offset { 0 };
    SDL_Rect content_rect;
    int scrollback_;
    int num_lines { 0 };
    bool depressed { false };
    TextSelection text_selection;
    ISlot* mouse_motion_slot { nullptr };

    explicit OutputPane(Widget* parent)
        : Widget(parent)
        , prompt(this)
    {
        set_scrollback(props().get(property::OUTPUT_SCROLLBACK).value_or(1000));

        prompt.connect<NewCommandInputEvent>(InternalEventType::new_command_input, [this](auto& e)
        {
            new_input(e.text);
        });

        prompt.connect<NewInputEvent>(InternalEventType::new_input, [this](auto& e)
        {
            new_input(e.text);
        });

        font->connect<FontSizeChangedEvent>(InternalEventType::font_size_changed, [this](auto& /*e*/)
        {
            resize(frame);
        });

        window.connect<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONDOWN, [this](auto& e) {
            on_SDL_MOUSEBUTTONDOWN(e);
        });

        window.connect<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONUP, [this](auto& e) {
            on_SDL_MOUSEBUTTONUP(e);
        });

        window.connect<SDL_MouseWheelEvent>(SDL_MOUSEWHEEL, [this](auto& e) {
            scroll(e.y);
        });

        mouse_motion_slot = window.connect_later<SDL_MouseMotionEvent>(SDL_MOUSEMOTION, [this](auto& e) {
            if (depressed) {
                end_text_selection(map_to({ e.x, e.y }, content_rect));

                if (e.y > this->frame.h) {
                    scroll(-1);
                } else if (e.y < 0) {
                    scroll(1);
                }
            }
        });

        window.connect<SDL_KeyboardEvent>(SDL_KEYDOWN, [this](auto& e) {
            on_SDL_KEYDOWN(e);
        });

        window.connect<SDL_TextInputEvent>(SDL_TEXTINPUT, [this](auto& /*e*/) {
            // When inputting into the prompt, we should keep anchored to the
            // bottom so the prompt is visible.
            // We also need to adjust the scrollbar range as the prompt may span
            // multiple lines.
            // TODO: maybe it should connect to prompt for this.
            set_scroll_offset(0);
            if (scrollbar)
                scrollbar->set_content_size(num_lines + prompt.entry.size());
        });
    }

    void set_scrollback(size_t scrollback)
    {
        scrollback_ = scrollback;
    }

    void apply_scrollbar(Scrollbar *s)
    {
        scrollbar = s;

        scrollbar->set_page_size(rows());
        scrollbar->set_content_size(1); // account for prompt

        scrollbar->connect<ValueChangedEvent>(InternalEventType::value_changed, [this](auto& e) {
            set_scroll_offset_from_scrollbar(e.value);
        });
    }

    int on_SDL_KEYDOWN(const SDL_KeyboardEvent& e)
    {
        auto sym = e.keysym.sym;
        switch (sym) {
        case SDLK_TAB:
        {
            std::thread cmdhelper([input = text::to_utf8(*prompt.input)]() {
                auto& core = DFHack::Core::getInstance();
                auto& con = SDLConsole::get_console();
                std::vector<std::string> possibles;

                core.getAutoCompletePossibles(input, possibles);
                if (possibles.empty()) { return; }

                if (possibles.size() == 1) {
                    SDLConsole::get_console().set_prompt_input(possibles[0]);
                } else {
                    std::string result = std::accumulate(
                        std::next(possibles.begin()), possibles.end(), possibles[0],
                            [](const std::string& a, const std::string& b) {
                            return a + " " + b;
                        });
                    con.save_prompt();
                    con.write_line(result);
                    con.restore_prompt();
                }
            });
            cmdhelper.detach();
        }
            break;
        /* copy */
        case SDLK_c:
            if (sdl_console::SDL_GetModState() & KMOD_CTRL) {
                copy_selected_text_to_clipboard();
            }
            break;

        /* paste */
        case SDLK_v:
            if (sdl_console::SDL_GetModState() & KMOD_CTRL) {
                prompt.put_input_from_clipboard();
            }
            break;

        case SDLK_PAGEUP:
            scroll(ScrollAction::page_up);
            break;

        case SDLK_PAGEDOWN:
            scroll(ScrollAction::page_down);
            break;

        case SDLK_RETURN:
        case SDLK_BACKSPACE:
        case SDLK_UP:
        case SDLK_DOWN:
        case SDLK_LEFT:
        case SDLK_RIGHT:
            set_scroll_offset(0);
            break;
        default: break;
        }
        return 0;
    }

    void on_SDL_MOUSEBUTTONDOWN(SDL_MouseButtonEvent& e)
    {
        if (!geometry::in_rect(e.x, e.y, content_rect)) {
            return;
        }

        if (e.button != SDL_BUTTON_LEFT) {
            return;
        }

        SDL_Point point = map_to({e.x, e.y}, content_rect);

        // TODO: cleanup text selection bidness, this is ugly.
        if (e.clicks == 1) {
            begin_text_selection(point);
        } else if (e.clicks == 2) {
            const auto* frag = find_fragment_at_y(point.y);
            if (frag) {
                auto wordpos = text::find_run(std::u32string(frag->text), get_column(point.x));

                auto get_x = [this](std::u32string::size_type pos, int fallback) -> int {
                    return (pos != std::u32string::npos) ? pos * font->char_width : fallback;
                };

                begin_text_selection({get_x(wordpos.first, content_rect.x), point.y});
                // wordpos is 0-based
                end_text_selection({get_x(wordpos.second+1, content_rect.w), point.y});
            }
        } else if (e.clicks == 3) {
            // NOTE: 0 for x doesn't work.
            begin_text_selection({content_rect.x, point.y});
            end_text_selection({content_rect.w, point.y});
        }

        depressed = true;
        mouse_motion_slot->connect();
    }

    void on_SDL_MOUSEBUTTONUP(SDL_MouseButtonEvent& /*e*/)
    {
        if (depressed) {
            sdl_console::SDL_CaptureMouse(SDL_FALSE);
            depressed = false;
            mouse_motion_slot->disconnect();
        }
    }

    void clear()
    {
        entries.clear();
        num_lines = 0;
        set_scroll_offset(0);
        if (scrollbar)
            scrollbar->set_content_size(1);
        text_selection.reset();
        emit_text_selection_changed();
        visible_rows.rebuild = true;
    }

    void set_scroll_offset(int v)
    {
        scroll_offset = v;
        if (scrollbar)
            scrollbar->scroll_to(v);
        visible_rows.rebuild = true;
    }

    void set_scroll_offset_from_scrollbar(int v)
    {
        scroll_offset = v;
        visible_rows.rebuild = true;
    }

    void begin_text_selection(const SDL_Point p)
    {
        text_selection.reset();
        auto* vr = visible_rows.find_row_at_y(font, p.y);
        if (!vr) return;
        auto col = get_column(p.x);
      //  col = std::clamp(col, 0UL, vr->frag.text.size() - 1);
        text_selection.begin = {.entry_id = vr->entry_id,
            .frag_index = vr->frag.index,
            .column = col};
        emit_text_selection_changed();
    }

    void end_text_selection(const SDL_Point p)
    {
        auto* vr = visible_rows.find_row_at_y(font, p.y);
        if (!vr) return;
        auto col = get_column(p.x);
        //col = std::clamp(col, std::size_t(0), vr->frag.text.size() - 1);
        text_selection.end = {.entry_id = vr->entry_id,
            .frag_index = vr->frag.index,
            .column = col};

        emit_text_selection_changed();
    }

    void emit_text_selection_changed()
    {
        static bool previous_state = false;
        bool has_selection = text_selection.active();

        if (has_selection != previous_state) {
            previous_state = has_selection;
            emit(TextSelectionChangedEvent{.selected = has_selection});
        }
    }

    void scroll(int y)
    {
        if (y > 0) {
            scroll(ScrollAction::up);
        } else if (y < 0) {
            scroll(ScrollAction::down);
        }
    }

    void scroll(ScrollAction sa)
    {
        int step = 0;
        switch (sa) {
        case ScrollAction::up:
            step = 1;
            break;
        case ScrollAction::down:
            step = -1;
            break;
        case ScrollAction::page_up:
            step = int(rows());
            break;
        case ScrollAction::page_down:
            step = -int(rows());
            break;
        }

        int max = (int)std::max(0UL, (num_lines + prompt.entry.size()) - rows());
        set_scroll_offset(std::clamp(scroll_offset + step, 0, max));
    }

    void resize(const SDL_Rect& rect) override
    {
        frame = rect;

        set_content_rect();
        prompt.resize(content_rect);

        num_lines = 0;
        for (auto& e : entries) {
            wrap_text(e);
        }

        visible_rows.rebuild = true;

        context.props.set(property::RT_OUTPUT_COLUMNS, columns());
        context.props.set(property::RT_OUTPUT_ROWS, rows());
    }

    /*
     * Adjust content_rect dimensions to align with margin and font properties.
     * For character alignment consistency, content_rect must be divisible
     * into rows and columns that match the font's fixed character dimensions.
     */
    void set_content_rect()
    {
        content_rect = frame;

        const int margin = 4; // // Margin around the viewport in px.

        // max width respect to font and margin
        const int max_width = content_rect.w - (margin * 2);
        const int wfit = (max_width / font->char_width) * font->char_width;

        // max height with respect to font and margin
        const int max_height = content_rect.h - (margin * 2);
        const int hfit = (max_height / font->line_height_with_spacing()) * font->line_height_with_spacing();

        content_rect.x = frame.x + margin;
        content_rect.y = frame.y + margin;
        content_rect.w = wfit;
        content_rect.h = hfit;
    }

    void new_output(const std::u32string& text, std::optional<SDL_Color> color)
    {
        create_entry(TextEntryType::output, text, color);
    }

    void new_input(const std::u32string& text)
    {
        auto both = prompt.prompt_text + text;
        create_entry(TextEntryType::input, both, std::nullopt);
    }

    void wrap_text(
        TextEntry& entry)
    {
        entry.wrap_text(font->char_width, content_rect.w);
        num_lines += entry.size();
        if (scrollbar)
            scrollbar->set_content_size(num_lines + prompt.entry.size());
        visible_rows.rebuild = true;
    }

    /*
     * Create a new entry which may span multiple rows and set it to be the head.
     * This function will automatically cycle-out entries when the number of rows
     * has reached the max specified by scrollbac_.
     */
    TextEntry& create_entry(const TextEntryType entry_type,
                            const std::u32string& text, std::optional<SDL_Color> color)
    {
        static size_t entry_id = 1; // 0 reserved for prompt

        TextEntry& entry = entries.emplace_front(entry_type, ++entry_id, text, color);

        /* When the list is too long, start chopping */
        if (num_lines > scrollback_) {
            num_lines -= entries.back().size();
            entries.pop_back();
            // FIXME: Cleanup anything that points to this
        }
        wrap_text(entry);
        return entry;
    }

    const TextEntry::Fragment* find_fragment_at_y(int y)
    {
        auto it = std::ranges::find_if(visible_rows.rows, [this, y](VisibleRow const& r) {
            return geometry::is_y_within_bounds(y, r.coord.y, font->line_height);
        });

        if (it != visible_rows.rows.end()) {
            return &it->frag;
        }

        return nullptr;
    }

    void copy_selected_text_to_clipboard()
    {
        std::u32string sep{U"\n"};
        std::u32string clipboard_text;

        auto vec = text_selection.get_selected_text(entries);
        auto pvec = text_selection.get_selected_text_from_one(prompt.entry);

        std::ranges::move(pvec, std::back_inserter(vec));

        if (vec.empty())
            return;

        clipboard_text = std::accumulate(std::next(vec.begin()), vec.end(),
                        std::u32string(vec[0]),
                        [&](std::u32string acc, std::u32string_view s) {
                            acc.append(sep);
                            acc.append(s);
                            return acc;
                        });

        sdl_console::SDL_SetClipboardText(text::to_utf8(clipboard_text).c_str());
    }

    size_t get_column(int x)
    {
        size_t column = x / font->char_width;
        return std::clamp(column, 0UL, columns());
    }

    size_t column_extent(int width)
    {
        size_t extent = width / font->char_width;
        return std::clamp(extent, 0UL, columns());
    }

    size_t columns()
    {
        return (float)content_rect.w / font->char_width;
    }

    size_t rows()
    {
        return (float)content_rect.h / font->line_height_with_spacing();
    }

#if 0
    void do_cmd_completion()
    {
        assert(cmd_completion_future);
        if (cmd_completion_future->valid()
                && cmd_completion_future->wait_for(std::chrono::milliseconds(0)) // 0 for don't block
                        == std::future_status::ready) {
            try {
                std::string result = cmd_completion_future->get();
                if (!result.empty())
                    prompt.set_input(text::from_utf8(result));
            } catch (const std::exception &e) {
                std::cerr << "SDLConsole: cmd_completion_future exception: " << e.what() << std::endl;
            }
            cmd_completion_future.reset();
        }
    }
#endif


    void render() override
    {
        // Clip and localize coordinates to content_rect.
        sdl_console::SDL_RenderSetViewport(renderer(), &content_rect);

        // TODO: make sure renderer supports blending else highlighting
        // will make the text invisible.
        render_selected_text();

        render_prompt_and_output();

        prompt.render_cursor();
        sdl_console::SDL_RenderSetViewport(renderer(), &window.frame);
    }

    void build_visible_prompt_and_output_rows()
    {
        visible_rows.rows.clear();
        visible_rows.rebuild = false;
        prompt.rebuild = false;
        prompt.cursor.rebuild = false;

        const int max_row = rows() + scroll_offset;
        int ypos = content_rect.h; // Start from the bottom
        int row_counter = 0;

        std::vector<VisibleRow> rows;

        auto prompt_lines = get_visible_entry_rows(prompt.entry, ypos, row_counter, max_row);
        prompt.update_cursor_geometry_for_render(prompt_lines);
        std::ranges::move(prompt_lines, std::back_inserter(visible_rows.rows));

        if (entries.empty()) {
            return;
        }

        for (auto& entry : entries) {
            if (row_counter > max_row) {
                break;
            }

            auto more = get_visible_entry_rows(entry, ypos, row_counter, max_row);
            std::ranges::move(more, std::back_inserter(visible_rows.rows));
        }
    }

    std::vector<VisibleRow> get_visible_entry_rows(TextEntry& entry, int& ypos, int& row_counter, int max_row)
    {
        std::vector<VisibleRow> vrows;

        for (auto& line : entry.fragments() | std::views::reverse) {
            row_counter++;
            if (row_counter <= scroll_offset) {
                continue;
            }

            if (row_counter > max_row) {
                break;
            }

            ypos -= font->line_height_with_spacing();

            vrows.push_back({.entry_id = entry.id,
                             .frag = line,
                             .coord = {0, ypos},
                             .gpv = font->get_glyph_layout(line.text, 0, ypos),
                             .color = entry.color_opt});
        }
        return vrows;
    }

    void render_prompt_and_output()
    {
        if (visible_rows.rebuild || prompt.rebuild_needed()) {
            build_visible_prompt_and_output_rows();
        }

        for (const auto& row : visible_rows.rows) {
            auto sc = font->set_color(row.color);
            font->render(row.gpv);
        }
    }

    void render_selected_text()
    {
        if (!text_selection.active()) return;

        auto rects = text_selection.to_rects(font, visible_rows.rows);

        set_draw_color(renderer(), colors::mediumgray);
        sdl_console::SDL_RenderFillRects(renderer(), rects.data(), rects.size());
        set_draw_color(renderer(), colors::darkgray);
    }

    OutputPane(const OutputPane&) = delete;
    OutputPane& operator=(const OutputPane&) = delete;
};


class MainWindow : public VBox {
public:
  //  std::unique_ptr<Toolbar> toolbar; // optional toolbar. XXX: implementation requires it
  //  std::unique_ptr<OutputPane> outpane;
    OutputPane* outpane{nullptr};
    bool has_focus{false};
    bool is_shown{true};
    bool is_minimized{false};

    using TicksT = decltype(sdl_console::SDL_GetTicks64());
    TicksT ms_per_frame{1000/20};

    explicit MainWindow(ImplContext& ctx)
        : VBox(ctx)
    {
        SDL_Window* h = create_window(ctx.props);
        SDL_Renderer* r = create_renderer(ctx.props, h);

        ctx.ui.window.init(h, r);

        if (ctx.ui.window.id == 0) {
            throw std::runtime_error("Failed to get window ID");
        }
        sdl_console::SDL_GetRendererOutputSize(renderer(), &frame.w, &frame.h);

        font = ctx.ui.window.font_loader.load("data/art/curses_640x300.png");
        if (!font) {
            throw(std::runtime_error("Error loading font"));
        }

        connect<SDL_WindowEvent>(SDL_WINDOWEVENT, [this](auto& e) {
            switch(e.event) {
            case SDL_WINDOWEVENT_RESIZED:
                resize({});
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                has_focus = false;
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                // TODO: Check if cursor is disabled first.
                SDL_ShowCursor(SDL_ENABLE);
                has_focus = true;
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                is_minimized = true;
            [[fallthrough]];
            case SDL_WINDOWEVENT_HIDDEN:
                is_shown = false;
                break;
            case SDL_WINDOWEVENT_SHOWN:
            case SDL_WINDOWEVENT_EXPOSED:
            case SDL_WINDOWEVENT_MAXIMIZED:
                is_shown = true;
                is_minimized = false;
                break;
            default: break;
            }

            set_ms_per_frame();
        });

        connect<SDL_MouseMotionEvent>(SDL_MOUSEMOTION, [this](auto& e) {
            context.ui.window.mouse_pos.x = e.x;
            context.ui.window.mouse_pos.y = e.y;
        });

        build_ui();
    }

    ~MainWindow() override = default;

    void build_ui()
    {
        auto& toolbar_hbox = add_child<HBox>();
        toolbar_hbox.layout.preferred_height = font->line_height_with_spacing() * 2;
        toolbar_hbox.layout.align = layout::Align::Top | layout::Align::Fill;
        toolbar_hbox.layout.stretch = 0;


        auto& toolbar = toolbar_hbox.add_child<Toolbar>();
        toolbar.layout.align = layout::Align::Fill | layout::Align::Top;
        toolbar.layout.stretch = 1;


        auto& outbox = add_child<HBox>();
        outbox.layout.stretch = 1;
        outbox.layout.margin = {4, 4, 4, 4};

        auto& output = outbox.add_child<OutputPane>();
        outpane = &output;
        output.layout.stretch = 1;
        output.layout.align = layout::Align::Fill;

        auto& scrollbar = outbox.add_child<Scrollbar>();
        scrollbar.layout.preferred_width = 16;
        scrollbar.layout.align = layout::Align::Right | layout::Align::Fill | layout::Align::Top;

        resize({0, 0, frame.w, frame.h});

        output.apply_scrollbar(&scrollbar); // Size of output area needs to be known before applying scrollbar

        Button& copy = *toolbar.add_button(U"Copy");
        copy.enabled = false;
        copy.connect<ClickedEvent>(InternalEventType::clicked, [&output](auto& /*e*/) {
            output.copy_selected_text_to_clipboard();
        });

        output.connect<TextSelectionChangedEvent>(InternalEventType::text_selection_changed, [&copy](auto& e) {
            copy.enabled = e.selected;
        });

        Button& paste = *toolbar.add_button(U"Paste");
        paste.connect<ClickedEvent>(InternalEventType::clicked, [&output](auto& /*e*/) {
            output.prompt.put_input_from_clipboard();
        });

        Button& font_inc = *toolbar.add_button(U"A+");
        font_inc.connect<ClickedEvent>(InternalEventType::clicked, [&output](auto& /*e*/) {
            output.font->incr_size();
        });

        Button& font_dec = *toolbar.add_button(U"A-");
        font_dec.connect<ClickedEvent>(InternalEventType::clicked, [&output](auto& /*e*/) {
            output.font->decr_size();
        });
    }

    void set_ms_per_frame()
    {
        constexpr TicksT fps_shown = 20;
        constexpr TicksT fps_minimized = 1;

        constexpr TicksT shown_ms = 1000 / fps_shown; // 20 fps;
        constexpr TicksT minimized_ms = 1000 / fps_minimized;

        ms_per_frame = is_minimized ? minimized_ms : shown_ms;
    }

    void render() override
    {
        if (!should_render()) return;

        // Should not fail unless OOM.
        sdl_console::SDL_RenderClear(renderer());
        // set background color

        // should not fail unless renderer is invalid
        //set_draw_color(renderer(), colors::darkgray);

        for (auto& child : children) {
            child->render();
        }

        set_draw_color(renderer(), colors::darkgray);

        sdl_console::SDL_RenderPresent(renderer());
    }

    bool should_render() const
    {
        static auto last_render_tick = SDL_GetTicks64();

        if (!is_shown) {
            return false;
        }

        auto current_tick = sdl_console::SDL_GetTicks64();
        auto rate = ms_per_frame;
        if (current_tick - last_render_tick >= rate) {
            last_render_tick = current_tick;
            return true;
        }

        return false;
    }

    void resize(const SDL_Rect& /*r*/) override
    {
        sdl_console::SDL_GetRendererOutputSize(renderer(), &frame.w, &frame.h);
        //toolbar->resize({ 0, 0, frame.w, font->line_height_with_spacing() * 2 });
        //outpane->resize({ 0, toolbar->frame.h, frame.w, frame.h - toolbar->frame.h });
        layout_children();
    }

    static SDL_Window* create_window(Property& props)
    {
        // Inform SDL to pass the mouse click event when switching between windows.
        SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

        auto title = props.get(property::WINDOW_MAIN_TITLE).value_or("DFHack Console");
        SDL_Rect create_rect = props.get(property::WINDOW_MAIN_RECT).value_or(
            SDL_Rect{SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480});
        //auto flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
        auto flags = SDL_WINDOW_RESIZABLE;

        SDL_Window* handle = sdl_tsd.CreateWindow(title.c_str(), create_rect.x, create_rect.y, create_rect.w, create_rect.h, flags);
        if (!handle) {
            throw std::runtime_error("Failed to create SDL window");
        }

        sdl_console::SDL_SetWindowMinimumSize(handle, 64, 48);
        return handle;
    }

    static SDL_Renderer* create_renderer(Property& props, SDL_Window* handle)
    {
        sdl_console::SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
        sdl_console::SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
        // Flags 0 instructs SDL to choose the default backend for the
        // host system. TODO: add config to force software rendering
        SDL_RendererFlags rflags = (SDL_RendererFlags)0;
        //SDL_RendererFlags rflags = (SDL_RendererFlags)SDL_RENDERER_SOFTWARE;
        SDL_Renderer* rend = sdl_tsd.CreateRenderer(handle, -1, rflags);
        SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
        sdl_console::SDL_RenderSetIntegerScale(rend, SDL_TRUE);
        return rend;
    }

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
};

Toolbar::Toolbar(Widget* parent)
    : Widget(parent)
{
    // Copy so that font size changes don't propogate to the toolbar.
    font = context.ui.window.font_loader.clone("toolbar", font);
};

void Toolbar::render()
{
    set_draw_color(renderer(), colors::gold);
    // Render bg
    // SDL_RenderFillRect(renderer(), &viewport);
    // Draw a border
    sdl_console::SDL_RenderDrawRect(renderer(), &frame);
    // Lay out horizontally
    for (auto& w : widgets) {
        w->render();
    }

    set_draw_color(renderer(), colors::darkgray);
}

void Toolbar::resize(const SDL_Rect& rect)
{
    frame = rect;
    layout_buttons();
}

Button* Toolbar::add_button(std::u32string text)
{
    auto button = std::make_unique<Button>(this, text);
    Button& b = *button.get();
    b.frame.h = frame.h;
    b.frame.y = frame.y;
    b.frame.w = b.label_rect.w + (font->char_width * 2);
    widgets.emplace_back(std::move(button));
    layout_buttons();
    return &b;
}

void Toolbar::layout_buttons()
{
    int margin_right = 0;
    int x = (frame.w - margin_right) - compute_widgets_startx();

    for (auto& w : widgets) {
        w->frame.x = x;
        x += w->frame.w;
    }

    for (auto& w : widgets) {
        w->resize(frame);
    }
}

int Toolbar::compute_widgets_startx()
{
    int x = 0;
    for (auto& w : widgets) {
        x += w->frame.w;
    }
    return x;
}

/*
 * Manages a centralized queue system designed for thread-safe handling
 * of external events.
 *
 * Previously, this class supported two distinct queues: one for SDL events and
 * another for API task events. However, it now only stores API tasks.
 *
 * A shared mutex and a dirty flag are used to facilitate synchronization and
 * integration with a condition variable (should one be required).
 * The dirty flag indicates whether any of the queues have items to process.
 */
class ExternalEventQueue {
    template <typename T>
    class Queue {
        friend class ExternalEventQueue;

    public:
        Queue() = default;

        void push(T event) {
            std::scoped_lock l(mutex_);
            events_.push_back(std::move(event));
            // relaxed should be fine here because of the mutex.
            dirty_.store(true, std::memory_order_relaxed);
        }

        std::vector<T> drain() {
            // quick check before locking
            if (!dirty_.load(std::memory_order_relaxed)) {
                return {};
            }

            std::scoped_lock l(mutex_);
            std::vector<T> out;
            out.swap(events_);
            dirty_.store(false, std::memory_order_relaxed);
            return out;
        }

        bool is_empty()
        {
            std::scoped_lock l(mutex_);
            return events_.empty();
        }

    private:
        std::vector<T> events_;
        std::mutex mutex_;
        std::atomic<bool> dirty_;
    };

public:
    using Task = std::function<void()>;
    Queue<Task> api_task;

    ExternalEventQueue() = default;

    void reset() {
        api_task.drain();
    }

    ExternalEventQueue(const ExternalEventQueue&) = delete;
    ExternalEventQueue& operator=(const ExternalEventQueue&) = delete;
};

#if 0
void render_texture(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const SDL_Rect& dst)
{
    sdl_console::SDL_RenderCopy(renderer, texture, nullptr, &dst);
}
#endif

int set_draw_color(SDL_Renderer* renderer, const SDL_Color& color)
{
    return sdl_console::SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

struct SDLConsole_private {
    Property props;
    std::weak_ptr<SDLConsole_impl> impl_weak;
    std::thread::id init_thread_id;
};

class SDLConsole_impl : public std::enable_shared_from_this<SDLConsole_impl> {
public:
    SDLConsole::State& state;
    SDLConsole_private& priv;
    CommandPipe command_pipe;
    ExternalEventQueue external_event_queue;
    ImplContext impl_context;
    MainWindow main_window;

    explicit SDLConsole_impl(SDLConsole* con)
    : state(con->state)
    , priv(*con->priv)
    , impl_context(priv.props)
    , main_window(impl_context)
    {
        command_pipe.make_connection(outpane().prompt);
    }

    OutputPane& outpane() const {
        return *main_window.outpane;
    }

    bool sdl_event_hook(SDL_Event& e)
    {
        // NOTE: Having focus is not enough to determine handling.
        // This window ID check is important for df to update its state
        // when moving between windows.
        if (e.type == SDL_WINDOWEVENT && e.window.windowID != main_window.context.ui.window.id) {
            return false;
        }

        if (e.type == SDL_WINDOWEVENT || main_window.has_focus) {
            emit_sdl_event(e);
            return true;
        }

        return false;
    }

    void update()
    {
        handle_tasks();

        main_window.render();
    }

    void shutdown()
    {
        state.set_state(SDLConsole::State::shutdown);
        command_pipe.shutdown();
    }

    ~SDLConsole_impl()
    {
        shutdown();
    }

private:
    void handle_tasks()
    {
        for (auto& t : external_event_queue.api_task.drain()) {
            t();
        }
    }

    void emit_sdl_event(SDL_Event& e)
    {
        main_window.emit(e);
    }
};

SDLConsole::SDLConsole()
{
    priv = std::make_unique<SDLConsole_private>();
    state.reset();
}

SDLConsole::~SDLConsole() = default;


/*
 * SDL events and video subsystems must be initialized
 * before this function is called.
 */
bool SDLConsole::init()
{
    // state is either active or shutting down
    if (!state.is_inactive()) { return true; }
    bool success = true;
    //std::cerr << "SDLConsole: init() from thread: " << std::this_thread::get_id() << '\n';
    priv->init_thread_id = std::this_thread::get_id();
    try {
        bind_sdl_symbols();
        impl = std::make_shared<SDLConsole_impl>(this);
        priv->impl_weak = impl;
        state.set_state(SDLConsole::State::active);
    } catch(std::runtime_error &e) {
        success = false;
        impl.reset();
        sdl_tsd.clear();
        state.reset();
        std::cerr << "SDLConsole: caught exception: " << e.what();
    }
    return success;
}

/* Line may be wrapped */
void SDLConsole::write_line(std::string line, SDL_Color color)
{
    write_line_(line, color);
}

void SDLConsole::write_line(std::string line)
{
    write_line_(line, std::nullopt);
}

void SDLConsole::write_line_(std::string& line, std::optional<SDL_Color> color)
{
    auto l = text::from_utf8(line);
    push_api_task([this, color, l = std::move(l)] {
        impl->outpane().new_output(l, color);
    });
}

int SDLConsole::get_columns()
{
    return priv->props.get(property::RT_OUTPUT_COLUMNS).value_or(-1);
}

int SDLConsole::get_rows()
{
    return priv->props.get(property::RT_OUTPUT_ROWS).value_or(-1);
}

SDLConsole& SDLConsole::set_mainwindow_create_rect(int w, int h, int x, int y)
{
    SDL_Rect rect{x, y, w, h};
    priv->props.set(property::WINDOW_MAIN_RECT, rect);
    return *this;
}

SDLConsole& SDLConsole::set_scrollback(int scrollback) {
    priv->props.set(property::OUTPUT_SCROLLBACK, scrollback);
    push_api_task([this, scrollback] {
        impl->outpane().set_scrollback(scrollback);
    });
    return *this;
}

SDLConsole& SDLConsole::set_prompt(const std::string& text)
{
    auto t = text::from_utf8(text);
    priv->props.set(property::PROMPT_TEXT, t);
    push_api_task([this, t = std::move(t)] {
        impl->outpane().prompt.set_prompt_text(t);
    });
    return *this;
}

std::string SDLConsole::get_prompt()
{
    return text::to_utf8(priv->props.get(property::PROMPT_TEXT).value_or(U"> "));
}

void SDLConsole::set_prompt_input(const std::string& text)
{
    push_api_task([this, t = text::from_utf8(std::move(text))] {
        impl->outpane().prompt.set_input(t);
    });
}

void SDLConsole::save_prompt()
{
    push_api_task([this] {
        impl->outpane().prompt.save();
    });
}

void SDLConsole::restore_prompt()
{
    push_api_task([this] {
        impl->outpane().prompt.restore();
    });
}

void SDLConsole::show_window() {
    push_api_task([this] {
        sdl_console::SDL_ShowWindow(impl->main_window.context.ui.window.handle);
    });
}

void SDLConsole::hide_window() {
    push_api_task([this] {
        sdl_console::SDL_HideWindow(impl->main_window.context.ui.window.handle);
    });
}

void SDLConsole::clear()
{
    push_api_task([this] {
        impl->outpane().clear();
    });
}

int SDLConsole::get_line(std::string& buf)
{
    if (auto I = std::weak_ptr<SDLConsole_impl>(impl).lock()) {
        return I->command_pipe.wait_get(buf);
    }
    return -1;
}

void SDLConsole::set_command_history(std::span<const std::string> entries)
{
    std::deque<std::u32string> my_entries;
    for (const auto& entry : entries) {
        my_entries.push_front(text::from_utf8(entry));
    }
    push_api_task([this, my_entries = std::move(my_entries)] {
        impl->outpane().prompt.set_command_history(my_entries);
    });
}

SDLConsole& SDLConsole::get_console()
{
    static SDLConsole instance;
    return instance;
}

// Callable from any thread.
void SDLConsole::shutdown()
{
    state.set_state(State::shutdown);
    if (auto I = std::weak_ptr<SDLConsole_impl>(impl).lock()) {
        I->shutdown();
    }
}

bool SDLConsole::destroy()
{
    if (!impl) {
        return true;
    }

    if (priv->init_thread_id != std::this_thread::get_id()) {
        std::cerr << "SDLConsole: destroy() called from non-init thread "
        << std::this_thread::get_id()
        << " (expected " << priv->init_thread_id << ")\n";
        return false;
    }

    // Kill our impl shared_ptr.
    impl.reset();
    // NOTE: The only other long living impl shared_ptr is get_line()
    // which runs on a separate thread and is commanded to close when shutdown() is called.
    while (!priv->impl_weak.expired()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // Cleanup thread specific resources.
    sdl_tsd.clear();
    // Back to inactive state.
    state.reset();
    return true;
}

// NOTE: Obtaining a shared_ptr would be wasteful here since
// this function is called by the same thread that manages impl's lifetime.
bool SDLConsole::sdl_event_hook(SDL_Event &e)
{
    if (impl) [[likely]]  {
        return impl->sdl_event_hook(e);
    }
    return false;
}

// NOTE: Obtaining a shared_ptr would be wasteful here since
// this function is called by the same thread that manages impl's lifetime.
void SDLConsole::update()
{
    if (impl) {
        impl->update();
    }
}

// Called from other threads.
template<typename F>
void SDLConsole::push_api_task(F&& func)
{
    static_assert(std::is_invocable_v<F>, "Callable must be invocable");

    /* We could use SDL_PushEvent() here. However, we may need to allocate
     * memory, which could result in memory leaks if the event is not received
     * for cleanup.
     */
    if (auto I = std::weak_ptr<SDLConsole_impl>(impl).lock()) {
        I->external_event_queue.api_task.push(std::forward<F>(func));
    }
}

} // end namespace sdl_console

// kate: replace-tabs on; indent-width 4;
