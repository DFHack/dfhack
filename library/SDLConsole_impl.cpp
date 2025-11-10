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

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftlcdfil.h>

#include "SDL_pixels.h"
#include "Core.h"
#include "modules/DFSDL.h"
#include "SDLConsole.h"
#include "SDLConsole_impl.h"

namespace DFHack {
namespace sdl_console {
namespace {

// NOTE: SDL calls are prefixed with the sdl_console namespace to make it easy
// to switch namespaces later.
#define CONSOLE_SYMBOL_ADDR(sym) nullptr
#define CONSOLE_DECLARE_SYMBOL(sym) decltype(sym)* sym = CONSOLE_SYMBOL_ADDR(sym) // NOLINT

CONSOLE_DECLARE_SYMBOL(SDL_CaptureMouse);
CONSOLE_DECLARE_SYMBOL(SDL_ConvertSurfaceFormat);
CONSOLE_DECLARE_SYMBOL(SDL_CreateRenderer);
CONSOLE_DECLARE_SYMBOL(SDL_CreateRGBSurface);
CONSOLE_DECLARE_SYMBOL(SDL_CreateRGBSurfaceWithFormat);
CONSOLE_DECLARE_SYMBOL(SDL_CreateRGBSurfaceWithFormatFrom);
CONSOLE_DECLARE_SYMBOL(SDL_CreateTexture);
CONSOLE_DECLARE_SYMBOL(SDL_CreateTextureFromSurface);
CONSOLE_DECLARE_SYMBOL(SDL_CreateWindow);
CONSOLE_DECLARE_SYMBOL(SDL_DestroyRenderer);
CONSOLE_DECLARE_SYMBOL(SDL_DestroyTexture);
CONSOLE_DECLARE_SYMBOL(SDL_DestroyWindow);
CONSOLE_DECLARE_SYMBOL(SDL_free);
CONSOLE_DECLARE_SYMBOL(SDL_FreeSurface);
CONSOLE_DECLARE_SYMBOL(SDL_FillRect);
CONSOLE_DECLARE_SYMBOL(SDL_GetClipboardText);
CONSOLE_DECLARE_SYMBOL(SDL_GetError);
CONSOLE_DECLARE_SYMBOL(SDL_GetEventFilter);
CONSOLE_DECLARE_SYMBOL(SDL_GetModState);
CONSOLE_DECLARE_SYMBOL(SDL_GetRendererOutputSize);
CONSOLE_DECLARE_SYMBOL(SDL_GetWindowFlags);
CONSOLE_DECLARE_SYMBOL(SDL_GetWindowID);
CONSOLE_DECLARE_SYMBOL(SDL_GetWindowSize);
CONSOLE_DECLARE_SYMBOL(SDL_GetTicks64);
CONSOLE_DECLARE_SYMBOL(SDL_HideWindow);
CONSOLE_DECLARE_SYMBOL(SDL_iconv_string);
CONSOLE_DECLARE_SYMBOL(SDL_InitSubSystem);
CONSOLE_DECLARE_SYMBOL(SDL_MapRGB);
CONSOLE_DECLARE_SYMBOL(SDL_MapRGBA);
CONSOLE_DECLARE_SYMBOL(SDL_memset);
CONSOLE_DECLARE_SYMBOL(SDL_RenderClear);
CONSOLE_DECLARE_SYMBOL(SDL_RenderCopy);
CONSOLE_DECLARE_SYMBOL(SDL_RenderDrawRect);
CONSOLE_DECLARE_SYMBOL(SDL_RenderFillRect);
CONSOLE_DECLARE_SYMBOL(SDL_RenderFillRects);
CONSOLE_DECLARE_SYMBOL(SDL_RenderGetWindow);
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
CONSOLE_DECLARE_SYMBOL(SDL_SetRenderTarget);
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
        CONSOLE_ADD_SYMBOL(SDL_CreateRGBSurfaceWithFormatFrom),
        CONSOLE_ADD_SYMBOL(SDL_CreateTexture),
        CONSOLE_ADD_SYMBOL(SDL_CreateTextureFromSurface),
        CONSOLE_ADD_SYMBOL(SDL_CreateWindow),
        CONSOLE_ADD_SYMBOL(SDL_DestroyRenderer),
        CONSOLE_ADD_SYMBOL(SDL_DestroyTexture),
        CONSOLE_ADD_SYMBOL(SDL_DestroyWindow),
        CONSOLE_ADD_SYMBOL(SDL_free),
        CONSOLE_ADD_SYMBOL(SDL_FreeSurface),
        CONSOLE_ADD_SYMBOL(SDL_FillRect),
        CONSOLE_ADD_SYMBOL(SDL_GetClipboardText),
        CONSOLE_ADD_SYMBOL(SDL_GetError),
        CONSOLE_ADD_SYMBOL(SDL_GetEventFilter),
        CONSOLE_ADD_SYMBOL(SDL_GetModState),
        CONSOLE_ADD_SYMBOL(SDL_GetRendererOutputSize),
        CONSOLE_ADD_SYMBOL(SDL_GetWindowFlags),
        CONSOLE_ADD_SYMBOL(SDL_GetWindowID),
        CONSOLE_ADD_SYMBOL(SDL_GetWindowSize),
        CONSOLE_ADD_SYMBOL(SDL_GetTicks64),
        CONSOLE_ADD_SYMBOL(SDL_HideWindow),
        CONSOLE_ADD_SYMBOL(SDL_iconv_string),
        CONSOLE_ADD_SYMBOL(SDL_InitSubSystem),
        CONSOLE_ADD_SYMBOL(SDL_MapRGB),
        CONSOLE_ADD_SYMBOL(SDL_MapRGBA),
        CONSOLE_ADD_SYMBOL(SDL_memset),
        CONSOLE_ADD_SYMBOL(SDL_RenderClear),
        CONSOLE_ADD_SYMBOL(SDL_RenderCopy),
        CONSOLE_ADD_SYMBOL(SDL_RenderDrawRect),
        CONSOLE_ADD_SYMBOL(SDL_RenderFillRect),
        CONSOLE_ADD_SYMBOL(SDL_RenderFillRects),
        CONSOLE_ADD_SYMBOL(SDL_RenderGetWindow),
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
        CONSOLE_ADD_SYMBOL(SDL_SetRenderTarget),
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
            fatal<Error::Internal>("Failed to load sdl symbol", sym.name);
    }
}

#if 0 // NOLINT
std::string load_system_mono_font() {
    std::string font_path;
#ifdef __linux__
    if (!FcInit()) {
        std::cerr << "Failed to initialize Fontconfig\n";
        return "";
    }

    FcPattern* pat = FcPatternCreate();
    FcPatternAddInteger(pat, FC_SPACING, FC_MONO);
    FcConfigSubstitute(nullptr, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult result;
    FcPattern* font = FcFontMatch(nullptr, pat, &result);

    if (font) {
        FcChar8* file = nullptr;
        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
            font_path = reinterpret_cast<char*>(file);
        }
        FcPatternDestroy(font);
    }

    FcPatternDestroy(pat);
    FcFini();
#else
    std::cerr << "System font lookup not supported on this platform\n";
#endif
    return "";
}
#endif

} // anonymous namespace

template <Error S, typename T>
void log_error(std::string_view ctx, T err, const std::source_location& loc)
{
    std::filesystem::path p(loc.file_name());

    std::cerr << "SDLConsole [ERROR] " << ctx
    << " at " << p.filename().string()  << ":" << loc.line();

    if constexpr (S == Error::FreeType) {
        if (err) std::cerr << " (FT_Error: " << err << ")";
        std::cerr << " (FreeType)";
    }

    if constexpr (S == Error::SDL) {
        const char* sdl_err = sdl_console::SDL_GetError();
        if (sdl_err && *sdl_err) std::cerr << " (SDL_Error: " << sdl_err << ")";
        std::cerr << " (SDL)";
    }

    if constexpr (S == Error::Internal) {
        if (err) std::cerr << " (Error: " << err << ")";
        std::cerr << " (Internal)";
    }

    std::cerr << std::endl;
}

template <Error S, typename T>
[[noreturn]] void fatal(std::string_view ctx, T err, const std::source_location& loc)
{
    log_error<S>(ctx, err, loc);
    throw std::runtime_error(std::string(ctx));
}

using TicksT = decltype(sdl_console::SDL_GetTicks64());

namespace text {
    static std::string to_utf8(const std::u32string_view u32_string)
    {
        char* conv = sdl_console::SDL_iconv_string("UTF-8", "UTF-32LE",
                                                   reinterpret_cast<const char*>(u32_string.data()),
                                                   (u32_string.size()+1) * sizeof(char32_t));
        if (!conv)
            return "?u8?";

        std::string result(conv);
        sdl_console::SDL_free(conv);
        return result;
    }

    static String from_utf8(const std::string_view& u8_string)
    {
        char* conv = sdl_console::SDL_iconv_string("UTF-32LE", "UTF-8",
                                                   u8_string.data(),
                                                   u8_string.size() + 1);
        if (!conv)
            return U"?u8?";

        std::u32string result(reinterpret_cast<char32_t*>(conv));
        sdl_console::SDL_free(conv);
        return result;
    }

    static bool is_newline(Char ch) noexcept
    {
        return ch == U'\n' || ch == U'\r';
    }

    static bool is_wspace(Char ch) noexcept
    {
        return ch == U' ' || ch == U'\t';
    }

    template <typename T>
    std::pair<size_t, size_t> find_run_with_pred(const StringView text, size_t pos, T&& pred)
    {
        if (text.empty()) return { StringView::npos, StringView::npos };

        if (pos >= text.size()) return { StringView::npos, StringView::npos };

        const auto* left = text.begin() + pos;
        const auto* right = left;

        while (left != text.begin() && pred(*(left - 1)))
            --left;

        const auto* tok = right;
        while (right != text.end() && pred(*right))
            ++right;

        if (tok != right)
            --right;

        return {
            std::distance(text.begin(), left),
            std::distance(text.begin(), right)
        };
    }

    size_t skip_wspace(const StringView text, size_t pos) noexcept
    {
        if (text.empty())
            return 0;
        if (pos >= text.size())
            return text.size() - 1;

        auto sub = std::ranges::subrange(text.begin() + pos, text.end() - 1);
        return std::distance(text.begin(),
                             std::ranges::find_if_not(sub, is_wspace));
    }

    size_t skip_wspace_reverse(const StringView text, size_t pos) noexcept
    {
        if (text.empty()) return 0;
        if (pos >= text.size()) pos = text.size() - 1;

        const auto* it = text.begin() + pos;
        while (it != text.begin() && is_wspace(*it)) {
            --it;
        }
        return std::distance(text.begin(), it);
    }

    size_t skip_graph(const StringView text, size_t pos) noexcept
    {
        if (text.empty())
            return 0;
        if (pos >= text.size())
            return text.size() - 1;

        const auto sub = std::ranges::subrange(text.begin() + pos, text.end() - 1);
        return std::distance(text.begin(),
                             std::ranges::find_if(sub, is_wspace));
    }

    size_t skip_graph_reverse(const StringView text, size_t pos) noexcept
    {
        if (text.empty())
            return 0;
        if (pos >= text.size())
            pos = text.size() - 1;

        const auto* it = text.begin() + pos;
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
    size_t find_prev_word(const StringView text, size_t pos) noexcept
    {
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
    size_t find_next_word(const StringView text, size_t pos) noexcept
    {
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

    std::pair<size_t, size_t> find_wspace_run(const StringView text, size_t pos) noexcept
    {
        return find_run_with_pred(text, pos, [](Char ch) { return is_wspace(ch); });
    }

    std::pair<size_t, size_t> find_run(const StringView text, size_t pos) noexcept
    {
        if (text.empty() || pos >= text.size()) {
            return { String::npos, String::npos };
        }

        if (is_wspace(text[pos])) {
            return find_wspace_run(text, pos);
        }
        return find_run_with_pred(text, pos, [](Char ch) { return !is_wspace(ch); });
    }

    static size_t insert_at(String& text, size_t pos, const StringView str)
    {
        if (pos >= text.size()) {
            text += str;
        } else {
            text.insert(pos, str);
        }
        pos += str.length();
        return pos;
    }

    static size_t backspace(String& text, size_t pos)
    {
        if (pos == 0 || text.empty()) {
            return pos;
        }

        if (text.length() == pos) {
            text.pop_back();
        } else {
            /* else shift the text from cursor left by one character */
            text.erase(pos-1, 1);
        }

        return --pos;
    }
}

namespace clipboard {
    static String get_text()
    {
        String text;
        auto* str = sdl_console::SDL_GetClipboardText();
        if (*str != '\0') {
            text = text::from_utf8(str);
        }
        // Always free, even when empty.
        sdl_console::SDL_free(str);
        return text;
    }
}

namespace geometry {
#if 0 // NOLINT
void center_rect(SDL_Rect& r)
{
    r.x = r.x - r.w / 2;
    r.y = r.y - r.h / 2;
}
#endif

static bool in_rect(int x, int y, const SDL_Rect& r) noexcept
{
    return ((x >= r.x) && (x < (r.x + r.w)) && (y >= r.y) && (y < (r.y + r.h)));
}

static bool in_rect(const SDL_Point& p, const SDL_Rect& r) noexcept
{
    return bool(SDL_PointInRect(&p, &r));
}

static bool is_y_within(int y, int y_top, int height) noexcept
{
    return (y >= y_top && y <= y_top + height);
}
} // geometry

namespace grid {
    struct PixelExtent {
        int offset;
        int length;
    };

    static inline PixelExtent columns_to_pixel_extent(int start_col, int end_col, int col_width) noexcept
    {
        return { .offset = start_col * col_width,
                 .length = (end_col - start_col) * col_width };
    }

    static inline size_t column(int x, int col_width) noexcept
    {
        return x / col_width;
    }

    static inline int columns_to_pixels(int col, int col_width) noexcept {
        return col * col_width;
    }

    static inline int snap_to_column(int extent, int col_width) noexcept {
        return (extent / col_width) * col_width;
    }

} // grid

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
    Up,
    Down,
    PageUp,
    PageDown
};

enum class TextEntryType: std::uint8_t {
    Input,
    Output
};

namespace colors {
    // Default palette. Needs more. Needs configurable.
    constexpr SDL_Color white  { 255, 255, 255, 255 };
//    const SDL_Color lightgray  { 211, 211, 211, 255 };
    constexpr SDL_Color mediumgray  { 65, 65, 65, 255 };
//    const SDL_Color charcoal = { 54, 69, 79, 255 };
    constexpr SDL_Color darkgray  { 27, 27, 27, 255 };

    constexpr SDL_Color mauve { 100,68,84, 255};
    constexpr SDL_Color gold  { 247,193,41, 255};
    constexpr SDL_Color teal  {  94, 173, 146, 255};
}

static int set_draw_color(SDL_Renderer* const renderer, const SDL_Color& color) noexcept
{
    return sdl_console::SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

// Wrap a SDL_Surface with a unique_ptr.
// It is safe to call with nullptr
static Surface make_surface(SDL_Surface* s)
{
    return Surface(s, sdl_console::SDL_FreeSurface);
}

// Wrap a SDL_Texture with a unique_ptr.
// It is safe to call it with nullptr
static Texture make_texture(SDL_Texture* t)
{
    return Texture(t, sdl_console::SDL_DestroyTexture);
}

using Renderer = std::unique_ptr<SDL_Renderer, decltype(sdl_console::SDL_DestroyRenderer)>;
using Window = std::unique_ptr<SDL_Window, decltype(sdl_console::SDL_DestroyWindow)>;

static Renderer make_renderer(SDL_Renderer* renderer)
{
    return Renderer(renderer, sdl_console::SDL_DestroyRenderer);
}

static Window make_window(SDL_Window* window)
{
    return Window(window, sdl_console::SDL_DestroyWindow);
}

// TODO: Some events are actions and some are notifications. Group them.
using EventTypeId = uint32_t;
struct InternalEventType {
    enum Type : EventTypeId {
        SDLMouseButtonUp = 0,
        SDLMouseButtonDown,
        SDLTextInputEvent,
        SDLKeyDown,
        SDLMouseWheel,
        SDLMouseMotion,

        CommandSubmitted,
        InputSubmitted,
        Clicked,
        FontSizeChangeRequested,
        ValueChanged,
        TextSelectionChanged,
        FindTextQuery,
        FindTextMove,
        Paste,
        InputFocusChanged,
        WidgetDestroyed,

        NumEvents
    };
};

struct Event {
    Event() = default;
    // virtual ~Event() = default;
};

struct SDLMouseButtonUpEvent : Event {
    static constexpr EventTypeId type = InternalEventType::SDLMouseButtonUp;
    const SDL_MouseButtonEvent &button;
};

struct SDLMouseButtonDownEvent : Event {
    static constexpr EventTypeId type = InternalEventType::SDLMouseButtonDown;
    const SDL_MouseButtonEvent &button;
};

struct SDLTextInputEvent : Event {
    static constexpr EventTypeId type = InternalEventType::SDLTextInputEvent;
    const SDL_TextInputEvent &text;
};

struct SDLKeyDownEvent : Event {
    static constexpr EventTypeId type = InternalEventType::SDLKeyDown;
    const SDL_KeyboardEvent &key;
};

struct SDLMouseWheelEvent : Event {
    static constexpr EventTypeId type = InternalEventType::SDLMouseWheel;
    const SDL_MouseWheelEvent &wheel;
};

struct SDLMouseMotionEvent : Event {
    static constexpr EventTypeId type = InternalEventType::SDLMouseMotion;
    const SDL_MouseMotionEvent &motion;
};

struct CommandSubmittedEvent : Event {
    static constexpr EventTypeId type = InternalEventType::CommandSubmitted;
    const String command;
};

struct InputSubmittedEvent : Event {
    static constexpr EventTypeId type = InternalEventType::InputSubmitted;
    const String text;
};

struct ClickedEvent : Event {
    static constexpr EventTypeId type = InternalEventType::Clicked;
};

struct FontSizeChangeRequestedEvent : Event {
    static constexpr EventTypeId type = InternalEventType::FontSizeChangeRequested;
    const int delta { 0 };
};

template <typename T>
struct ValueChangedEvent : Event {
    static constexpr EventTypeId type = InternalEventType::ValueChanged;
    const T value;
};

struct TextSelectionChangedEvent : Event {
    static constexpr EventTypeId type = InternalEventType::TextSelectionChanged;
    const bool selected{};
};

struct FindTextQueryEvent : Event {
    static constexpr EventTypeId type = InternalEventType::FindTextQuery;
    const String query;
};

struct FindTextMoveEvent : Event {
    static constexpr EventTypeId type = InternalEventType::FindTextMove;
    const ScrollAction direction; // up or down
};

struct PasteEvent : Event {
    static constexpr EventTypeId type = InternalEventType::Paste;
    const String text;
};

struct InputFocusChangedEvent : Event {
    static constexpr EventTypeId type = InternalEventType::InputFocusChanged;
    const bool has_focus;
};

struct WidgetDestroyedEvent : Event {
    static constexpr EventTypeId type = InternalEventType::WidgetDestroyed;
    const void* addr;
};

/*
* A lightweight implementation inspired by Qt's signals and slots.
*
*/

class EventBus;
class ISlot {
public:
    virtual ~ISlot() = default;

    virtual bool invoke(Event& event) = 0;
    virtual EventTypeId event_type_id() = 0;

    virtual void disconnect() = 0;
    virtual void destroy() = 0;
    virtual void connect() = 0;
    virtual bool is_connected() = 0;

protected:
    friend class EventBus;
    // opaque connection identity. never dereference.
    void* sender{};
    // opaque connection identity. never dereference.
    void* receiver{};

    bool marked_destroyed{false};
    bool marked_connected{true};
};

// Base class for EventBus so Slot can use it before it's defined.
class ISignal {
public:
    virtual ~ISignal() = default;
    virtual void disconnect(ISlot* slot) = 0;
    virtual void reconnect(ISlot* slot) = 0;
};

template <typename E>
class Slot : public ISlot {
public:
    using Func = std::function<bool(E&)>;

    template <typename F>
    Slot(ISignal& emitter, void* sender, void* receiver, F&& func)
    : emitter_(emitter)
    , func_(std::forward<F>(func))
    {
        this->sender = sender;
        this->receiver = receiver;
    }

    bool invoke(Event& event) override
    {
        E& e = static_cast<E&>(event);
        return func_(e);
    }

    void disconnect() override { emitter_.disconnect(this); }
    void destroy() override { marked_destroyed = true; emitter_.disconnect(this); }
    void connect() override { emitter_.reconnect(this); }
    bool is_connected() override { return marked_connected; }
    EventTypeId event_type_id() override { return E::type; }

private:
    ISignal& emitter_;
    const Func func_;
};

class EventBus : public ISignal {
public:
    EventBus() {
        owned_.reserve(64); // rough estimate of total slots/connect()s
    }

    template <typename E, typename F>
    ISlot* connect(void* sender, void* receiver, F&& func)
    {
        auto slot = create_slot<E>(sender, receiver, std::forward<F>(func));
        slot->marked_connected = true;
        slots_.active.at(E::type).emplace_back(slot);
        return slot;
    }

    template <typename E, typename F>
    ISlot* connect_later(void* sender, void* receiver, F&& func)
    {
        auto slot = create_slot<E>(sender, receiver, std::forward<F>(func));
        slot->marked_connected = false;
        return slot;
    }

    void disconnect(ISlot* slot) override
    {
        slot->marked_connected = false;
        if (emit_depth_ == 0) {
            do_disconnect(slot);
        } else {
            slots_.dirty = true;
        }
    }

    void reconnect(ISlot* slot) override {
        slot->marked_connected = true;
        slots_.active.at(slot->event_type_id()).emplace_back(slot);
    }

    void invalidate_receiver_slots(void* receiver) noexcept {
        for (auto& slot_uptr : owned_) {
            ISlot* s = slot_uptr.get();
            if (s->receiver == receiver) {
                s->marked_connected = false;
                s->marked_destroyed = true;
            }
        }

        if (emit_depth_ == 0)
            destroy_slots_if_needed();
    }

    template <typename E>
    bool emit(E&& event, void* sender)
    {
        auto& slots = slots_.active.at(event.type);
        if (slots.empty())
            return false;

        bool handled = false;
        ++emit_depth_;
        for (auto* s : slots) {
            if (s->is_connected() && (s->sender == nullptr || s->sender == sender)) {
                if (s->invoke(event)) {
                    handled = true;
                    break;
                }
            }
        }
        --emit_depth_;

        if (slots_.dirty && emit_depth_ == 0)
            process_pending();

        return handled;
    }

    bool has_connection(EventTypeId type, void* receiver)
    {
        auto& slots = slots_.active.at(type);
        if (slots.empty())
            return false;

        for (auto* s : slots) {
            if (s->is_connected() && (s->receiver == receiver)) {
                return true;
            }
        }

        return false;
    }

    void clear() noexcept {
        SlotType e;
        slots_.active.swap(e);
        owned_.clear();
    }

private:
    std::vector<std::unique_ptr<ISlot>> owned_;

    using SlotType = std::array<std::deque<ISlot*>, InternalEventType::NumEvents> ;
    struct Slots {
        SlotType active;
        bool dirty{false};
        bool dirty_destroy{false};
    };
    Slots slots_;

    int emit_depth_{0};

    template<typename E, typename F>
    ISlot* create_slot(void* sender, void* receiver, F&& func) {
        auto slot = std::make_unique<Slot<E>>(*this, sender, receiver, std::forward<F>(func));
        ISlot* p = slot.get();
        owned_.emplace_back(std::move(slot));
        return p;
    }

    void destroy_slots_if_needed() noexcept {
        if (!slots_.dirty_destroy)
            return;

        slots_.dirty_destroy = false;
        auto& active = slots_.active;
        std::erase_if(owned_, [&active](const auto& slot_uptr) {
            ISlot* slot = slot_uptr.get();

            auto idx = slot->event_type_id();
            assert(idx < active.size());
            // Should never happen
            if (idx >= active.size()) {
                return false;
            }

            if (slot->marked_destroyed) {
                std::erase(active[idx], slot);
                return true;
            }
            return false;
        });
    }

    void process_pending()
    {
        slots_.dirty = false;
        destroy_slots_if_needed();

        auto& active = slots_.active;
        for (auto& slot_uptr : owned_) {
            ISlot* slot = slot_uptr.get();

            auto idx = slot->event_type_id();
            auto& vec = active.at(idx);

            if (slot->is_connected()) {
                // FIXME: This should not be necessary anymore since std::deque
                if (std::ranges::find(vec, slot) == vec.end())
                    vec.push_back(slot);
            } else {
                std::erase(vec, slot);
            }
        }
    }

    void do_disconnect(ISlot* slot)
    {
        slot->marked_connected = false;
        auto& vec = slots_.active.at(slot->event_type_id());
        std::erase(vec, slot);
    }
};

namespace property {
    // These must be set before SDLConsole::init()
    constexpr Property::Key<Rect> WINDOW_MAIN_RECT { "window.main.rect" };
    constexpr Property::Key<std::string> WINDOW_MAIN_TITLE { "window.main.title" };

    // Set any time.
    constexpr Property::Key<int> OUTPUT_SCROLLBACK { "output.scrollback" };

    constexpr Property::Key<String> PROMPT_TEXT { "prompt.text" };

    // Runtime information. Read only.
    constexpr Property::Key<int> RT_OUTPUT_ROWS { "rt.output.rows" };
    constexpr Property::Key<int> RT_OUTPUT_COLUMNS { "rt.output.columns" };
}

class TextEntry {
public:
    // A chunk of text within an entry
    struct Fragment {
        StringView text;
        size_t index;
        size_t start_offset; // 0-based absolute start position within entry text
        size_t end_offset; // 0-based absolutely end position within entry text

        Fragment(StringView text, size_t index, size_t start_offset, size_t end_offset)
        : text(text)
        , index(index)
        , start_offset(start_offset)
        , end_offset(end_offset) {};

        Fragment(const Fragment&) = delete;
        Fragment& operator=(const Fragment&) = delete;
    };
    using Fragments = std::deque<Fragment>;

    TextEntryType type { TextEntryType::Input }; // To differentiate output / input
    size_t id { 0 };
    String text; // whole raw text
    std::optional<SDL_Color> color_opt;

    TextEntry() = default;

    ~TextEntry() = default;

    TextEntry(TextEntryType type, size_t id, String text, std::optional<SDL_Color>& color)
        : type(type)
        , id(id)
        , text(std::move(text))
        , color_opt(color)
        {};

    auto& add_fragment(StringView text, size_t start_offset, size_t end_offset)
    {
        fragments_.emplace_back(text, fragments_.size(), start_offset, end_offset);
        return fragments_.back();
    }

    void clear() noexcept
    {
        fragments_.clear();
    }

    auto size() const noexcept
    {
        return fragments_.size();
    }

    const Fragments& fragments() const noexcept
    {
        return fragments_;
    }

    void wrap_text(int char_width, int viewport_width) {
        clear();

        int start_idx = 0;
        int delim_idx = -1;
        int idx = 0;

        auto close = [this, &start_idx](int end_idx) {
            if (end_idx < start_idx)
                return;

            add_fragment(StringView(text).substr(start_idx,
                                                 end_idx - start_idx + 1),
                         start_idx, end_idx);
        };

        auto open = [&start_idx](int new_start) {
            start_idx = new_start + 1;
        };

        for (auto ch : text) {
            if (text::is_newline(ch)) {
                close(idx - 1); // Up to newline
                open(idx); // Skip new line
                delim_idx = -1;
            } else if (text::is_wspace(ch)) {
                delim_idx = idx; // Last whitespace
            }

            if ((idx - start_idx + 1) * char_width >= viewport_width) {
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

    Fragment* fragment_from_offset(size_t index) noexcept
    {
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


ScopedColor::ScopedColor(Font& font, const SDL_Color& color)
: font_(font)
{
    //font_.set_color(color);
}

ScopedColor::~ScopedColor() {
    //font_.reset_color();
}

void FontAtlas::render(const std::span<const GlyphPosition> vec) const
{
    for (const auto& p : vec) {
        sdl_console::SDL_RenderCopy(renderer_, p.texture, &p.src, &p.dst);
    }
}

void FontAtlas::render(const StringView text, int x, int y)
{
    const GlyphPosVector g_pos = get_glyph_layout(text, x, y);
    for (const auto& p : g_pos) {
        sdl_console::SDL_RenderCopy(renderer_, p.texture, &p.src, &p.dst);
    }
}

// Must call has_glyph() first for this to work as expected
GlyphRec FontAtlas::get_fallback_glyph_rec(Font& font, char32_t codepoint)
{
    // Disable fallback to prevent circular recursion
    GlyphRec rec = font.atlas().get_glyph_rec(codepoint, Font::FallbackMode::Disabled);
    rec.font = &font;
    return rec;
}


// XXX, TODO: cleanup.
class DFBitmapFont : public Font, FontAtlas {
#if 0 //NOLINT
    class ScopedColor {
    public:
        explicit ScopedColor(BitmapFont* font) : font_(font) {}
        ScopedColor(BitmapFont* font, const SDL_Color& color)
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
        BitmapFont* font_;
    };
#endif

public:
    explicit DFBitmapFont(SDL_Renderer* renderer, const std::filesystem::path& path)
     : FontRenderer(renderer)
     , Font(path)
     , FontAtlas(this)
    {
    }

    static std::unique_ptr<DFBitmapFont> create(SDL_Renderer* renderer, const std::filesystem::path& path, int size)
    {
        auto font = std::make_unique<DFBitmapFont>(renderer, path);

        if (!font->init())
            return nullptr;

        if (size != font->orig_line_height)
            font->set_size(size);

        return font;
    }

    Texture to_texture(StringView text) override
    {
        return make_texture(nullptr);
    }

    int size() const noexcept override
    {
        return metrics_.line_height;
    }

    bool set_size(int new_size) override
    {
        new_size = std::clamp(new_size, Font::size_min, Font::size_max);

        scale_factor = float(new_size) / orig_line_height;

        int cw = orig_char_width * scale_factor;
        int lh = orig_line_height * scale_factor;
        int ls = Font::line_spacing;

        metrics_ = FontMetrics {
            .char_width = cw,
            .line_height = lh,
            .line_spacing = ls,
            .line_height_with_spacing = lh + ls,
            .ascent = 0
        };

        return true;
    }

    FontAtlas& atlas() override
    {
        return *this;
    }

    std::optional<ScopedColor> set_color(const std::optional<SDL_Color>& color) override
    {
        if (!color.has_value()) { return std::nullopt; }
        return std::make_optional<ScopedColor>(*this, color.value());
    }

    ScopedColor set_color(const SDL_Color& color) override
    {
        return ScopedColor(*this, color);
    }

    GlyphPosVector get_glyph_layout(const StringView text, int x, int y) override
    {
        GlyphPosVector g_pos;
        g_pos.reserve(text.size());
        int y_offset = metrics_.line_spacing / 2;
        for (const auto& ch : text) {
            const GlyphRec& g = FontAtlas::get_glyph_rec(ch);
            // If g is from a fallback, we can't use g.height
            // as it may not fit. Instead use line_height from metrics
            // and let SDL scale it.
            SDL_Rect dst = {
                .x = x,
                .y = y + y_offset,
                .w = metrics_.char_width,
                //.h = g.height // possibly from fallback font
                .h = metrics_.line_height // scale it up/down if needed
            };

            x += metrics_.char_width;

            SDL_Texture* tex = g.font ? g.font->atlas().get_texture(g.page_idx) : texture_.get();
            g_pos.push_back({.src = g.rect, .dst = dst, .texture = tex});
        }
        return g_pos;
    }

    void enlarge() override
    {
        set_size(size() + Font::size_change_delta);
    }

    void shrink() override
    {
        set_size(size() - Font::size_change_delta);
    }

    static bool is_ascii(char32_t codepoint) noexcept
    {
        return codepoint < 128;
    }

    bool has_glyph(char32_t codepoint) const override
    {
        if (is_ascii(codepoint))
            return true;
        return unicode_to_cp437.contains(codepoint);
    }

    SDL_Texture* get_texture(int /*page_idx*/) override
    {
        return texture_.get();
    }

    GlyphRec& get_glyph_rec(char32_t codepoint, FallbackMode fbm) override
    {
        if (is_ascii(codepoint))
            return glyphs_[codepoint];

        auto it = unicode_to_cp437.find(codepoint);
        if (it != unicode_to_cp437.end())
            return glyphs_[it->second];

        if (fbm == FallbackMode::Enabled
                && fallback() && fallback()->has_glyph(codepoint)) {
            glyphs_[codepoint] = get_fallback_glyph_rec(*fallback(), codepoint);
            return glyphs_[codepoint];
        }

        return glyphs_[0];
    }

    DFBitmapFont(DFBitmapFont&& other) = delete;
    DFBitmapFont& operator=(DFBitmapFont&&) = delete;

    DFBitmapFont(const DFBitmapFont&) = delete;
    DFBitmapFont& operator=(const DFBitmapFont&) = delete;

private:
    Texture texture_ { make_texture(nullptr) };
    std::vector<GlyphRec> glyphs_;
    int orig_char_width { 0 };
    int orig_line_height { 0 };
    float scale_factor { 1 };
    std::unique_ptr<FontAtlas> atlas_;

    static constexpr int atlas_columns { 16 };
    static constexpr int atlas_rows { 16 };

    bool init()
    {
        auto surface = make_surface(DFSDL::DFIMG_Load(path.c_str()));
        if (surface == nullptr) {
            log_error<Error::SDL>("Failed to load cp437 bitmap");
            return false;
        }

        //  FIXME: hardcoded magenta
        // Make this keyed color transparent.
        uint32_t bg_color = sdl_console::SDL_MapRGB(surface->format, 255, 0, 255);
        if (sdl_console::SDL_SetColorKey(surface.get(), SDL_TRUE, bg_color))
            log_error<Error::SDL>("Failed to set color key"); // Continue anyway

        // Create a surface in ARGB8888 format, and replace the keyed color
        // with fully transparant pixels. This step completely removes the color.
        // NOTE: Do not use surface->pitch
        auto conv_surface = make_surface(sdl_console::SDL_CreateRGBSurfaceWithFormat(0, surface->w, surface->h, 32,
                                                                                     SDL_PixelFormatEnum::SDL_PIXELFORMAT_ARGB8888));

        if (!conv_surface) {
            log_error<Error::SDL>("Failed to create RGB surface");
            return false;
        }

        sdl_console::SDL_BlitSurface(surface.get(), nullptr, conv_surface.get(), nullptr);
        surface = std::move(conv_surface);

        int char_width = surface->w / atlas_columns;
        int line_height = surface->h / atlas_rows;

        orig_char_width = char_width;
        orig_line_height = line_height;
        // FIXME: magic numbers
        int line_spacing = Font::line_spacing;

        metrics_ = FontMetrics {
            .char_width = char_width,
            .line_height = line_height,
            .line_spacing = line_spacing,
            .line_height_with_spacing = line_height + line_spacing,
            .ascent = 0
        };

        glyphs_ = build_glyph_rects(surface->w, surface->h, atlas_columns, atlas_rows);

        texture_ = make_texture(sdl_console::SDL_CreateTextureFromSurface(Font::renderer_, surface.get()));
        if (!texture_ ) {
            log_error<Error::SDL>("Failed to create texture from surface");
            return false;
        }
        sdl_console::SDL_SetTextureBlendMode(texture_.get(), SDL_BLENDMODE_BLEND);

        return true;
    }

    static std::vector<GlyphRec> build_glyph_rects(int sheet_w, int sheet_h, int columns, int rows)
    {
        int tile_w = sheet_w / columns;
        int tile_h = sheet_h / rows;
        int total_glyphs = rows * columns;

        std::vector<GlyphRec> glyphs;
        glyphs.reserve(total_glyphs);

        for (int i = 0; i < total_glyphs; ++i) {
            int r = i / columns;
            int c = i % columns;
            GlyphRec glyph = {
                .rect = {
                    .x = tile_w * c,
                    .y = tile_h * r,
                    .w = tile_w,
                    .h = tile_h
                }, // Rectangle in pixel dimensions
                .height = uint8_t(tile_h)
            };
            glyphs.push_back(glyph);
        }
        return glyphs;
    }
};

namespace freetype {
    static Surface blit_bitmap_to_surface(const FT_Bitmap& bmp)
    {
        int h = bmp.rows;
        int w = bmp.width;

        auto surf = make_surface(sdl_console::SDL_CreateRGBSurfaceWithFormat(0, w, h,
                                                                             32, SDL_PIXELFORMAT_RGBA32));
        if (!surf)
            return surf;

        auto bpp = surf->format->BytesPerPixel;

        for (auto y : std::views::iota(0, h)) {
            auto* row = reinterpret_cast<uint8_t*>(surf->pixels) + (y * surf->pitch);
            for (auto x : std::views::iota(0, w)) {
                auto a = bmp.buffer[(y * bmp.pitch) + x];

                auto* pixel = reinterpret_cast<uint32_t*>(row + (x * bpp));
                *pixel = sdl_console::SDL_MapRGBA(surf->format, 255, 255, 255, a);
            }
        }
        return surf;
    }

    static FT_Library get_ft_library() {
        static FT_Library ft = nullptr;
        static std::once_flag inited;
        std::call_once(inited, []{
            if (FT_Init_FreeType(&ft)) {
                ft = nullptr;
            }
        });
        return ft;
    }
}

class TrueTypeFontAtlas : public FontAtlas {
public:
    TrueTypeFontAtlas(Font& font, SDL_Renderer* renderer, FT_Face face)
    : FontRenderer(renderer)
    , FontAtlas(&font)
    , face_(face)
    , size_(font.size())
    {
    }

    static std::unique_ptr<TrueTypeFontAtlas> create(Font& font, SDL_Renderer* renderer, FT_Face face)
    {
        auto atlas = std::make_unique<TrueTypeFontAtlas>(font, renderer, face);

        if (!atlas->init()) {
            log_error<Error::FreeType>("Failed to initialize atlas");
            return nullptr;
        }
        return atlas;
    }

    void enlarge() override
    {
        resize_and_build_atlas(font.size() + Font::size_change_delta);
    }

    void shrink() override
    {
        resize_and_build_atlas(font.size() - Font::size_change_delta);
    }

    void resize_and_build_atlas(int new_size)
    {
        font.set_size(new_size);
        cache_cell_size();
        build_atlas();
    }

    GlyphPosVector get_glyph_layout(const StringView text, int x, int y) override
    {
        GlyphPosVector g_pos;
        g_pos.reserve(text.size());

        int pen_x = x;
        int pen_y = y;

        for (char32_t codepoint : text) {
            auto& g = FontAtlas::get_glyph_rec(codepoint);

            int y_offset = font.metrics().ascent - g.bearing_y;

            bool is_fb = g.font != nullptr;

            SDL_Rect src = g.rect;
            SDL_Rect dst = {
                .x = pen_x,
                .y = pen_y + y_offset + (font.metrics().line_spacing / 2),
                //.w = src.w,
                .w = is_fb ? font.char_width() : src.w, // FIXME: only for bitmap fallback
                .h = is_fb ? font.line_height() : g.height // FIXME: only for bitmap fallback
            };

            pen_x += font.char_width();

            SDL_Texture* tex = g.font ? g.font->atlas().get_texture(g.page_idx) : pages.at(g.page_idx).texture.get();

            g_pos.push_back({
                .src = src,
                .dst = dst,
                .texture = tex
            });
        }

        return g_pos;
    }

    bool has_glyph_rec(char32_t codepoint) const {
        return glyphs_.contains(codepoint);
    }

private:
    FT_Face face_ { nullptr };
    int size_ { 0 };
    std::unordered_map<char32_t, GlyphRec> glyphs_;
    static constexpr geometry::Size atlas_size { .w = 256, .h = 256 };
    int cell_width { 0 };
    int cell_height { 0 };
    static constexpr int cell_padding { 1 };

    struct AtlasPage {
        Texture texture;
        int next_free_cell{0};
    };

    struct Cell {
        SDL_Rect rect { -1, -1, -1, -1 };
        int8_t page_idx { -1 };

        constexpr bool is_valid() const noexcept
        {
            return rect.x >= 0 && rect.y >= 0 && rect.w > 0 && rect.h > 0 && page_idx >= 0;
        }

        static constexpr Cell invalid() noexcept { return {}; }
    };

    std::vector<AtlasPage> pages;

    bool init()
    {
        return build_atlas();
    }

    int max_cells() const noexcept
    {
        return (atlas_size.w / cell_width) * (atlas_size.h / cell_height);
    }

    void cache_cell_size() noexcept
    {
        auto [w, h] = get_cell_size();
        cell_width = w;
        cell_height = h;
    }

    bool can_hold_cell() const noexcept
    {
        auto [w, h] = get_cell_size();
        return w < atlas_size.w && h < atlas_size.h;
    }

    std::pair<int, int> get_cell_size() const noexcept
    {
        auto w = font.char_width() + cell_padding;
        auto h = font.line_height() + cell_padding;
        return std::make_pair(w, h);
    }

    bool build_atlas()
    {
        if (!can_hold_cell()) {
            log_error<Error::Internal>("Atlas can't hold any cells");
            return false;
        }
        cache_cell_size();

        pages.clear();
        glyphs_.clear();
        create_page();

        std::vector<Surface> surfaces;
        auto get_new_surface = [&surfaces]() {
            auto s = make_surface(sdl_console::SDL_CreateRGBSurfaceWithFormat(0,
                                                                              atlas_size.w, atlas_size.h,
                                                                              32, SDL_PIXELFORMAT_RGBA32));
            auto ptr = s.get();
            surfaces.push_back(std::move(s));
            return ptr;
        };
        SDL_Surface *curr = get_new_surface();
        if (!curr) {
            log_error<Error::SDL>("Failed to create surface");
            return false;
        }
        // not defined glyph
        load_char_to_atlas(0, curr);

        int ascii_start = 32;
        int ascii_end = 127;
        for(auto codepoint : std::views::iota(ascii_start, ascii_end)) {
            if (pages.back().next_free_cell >= max_cells()) {
                curr = get_new_surface();
                if (!curr) {
                    log_error<Error::SDL>("Failed to create surface");
                    return false;
                }
            }
            load_char_to_atlas(codepoint, curr);
        }

        if (pages.size() != surfaces.size()) {
            log_error<Error::Internal>("BUG: pages.size != surfaces.size");
            return false;
        }

        int idx = 0;
        for (auto& surf : surfaces) {
            auto tex = make_texture(sdl_console::SDL_CreateTextureFromSurface(renderer_, surf.get()));

            if (!tex) {
                log_error<Error::SDL>("Failed to create texture");
                return false;
            }

            if (sdl_console::SDL_SetRenderTarget(renderer_, pages[idx].texture.get())) {
                log_error<Error::SDL>("Failed to set render target");
                return false;
            }

            if (sdl_console::SDL_RenderCopy(renderer_, tex.get(), nullptr, nullptr)) {
                log_error<Error::SDL>("Failed to render copy");
                return false;
            }

            idx++;
        }
        assert(idx > 0);

        sdl_console::SDL_SetRenderTarget(renderer_, nullptr);
        return true;
    }

    void load_char_to_atlas(char32_t codepoint, SDL_Surface* atlas_surface)
    {
        auto* slot = load_ft_glyph_slot(codepoint);
        if (!slot)
            return;

        auto cell = advance_to_next_free_cell();
        if (!cell.is_valid())
            return;

        auto glyph_surf = freetype::blit_bitmap_to_surface(slot->bitmap);
        if (!glyph_surf)
            return;

        SDL_BlitSurface(glyph_surf.get(), nullptr, atlas_surface, &cell.rect);

        glyphs_[codepoint] = {
            .rect = cell.rect,
            .bearing_y = decltype(GlyphRec::bearing_y)(slot->bitmap_top),
            .height = decltype(GlyphRec::height)(slot->bitmap.rows),
            .page_idx = last_page_idx()
        };
    }

    SDL_Texture* get_texture(int page_idx) override
    {
        return pages.at(page_idx).texture.get();
    }

    FT_GlyphSlot load_ft_glyph_slot(char32_t codepoint)
    {
        // If codepoint isn't mapped in FT_Face, returns 0 for undefined
        // We check for that here otherwise FT_Load_Char() will generate the undefined glyph
        auto idx = FT_Get_Char_Index(face_, codepoint);
        if (idx == 0 && codepoint != 0)
            return nullptr;

        if (FT_Load_Char(face_, codepoint, FT_LOAD_RENDER))
            return nullptr;

        return face_->glyph;
    }

    bool load_char_to_atlas(char32_t codepoint)
    {
        auto* slot = load_ft_glyph_slot(codepoint);
        if (!slot)
            return false;

        auto cell = advance_to_next_free_cell();
        if (!cell.is_valid())
            return false;

        auto surface = freetype::blit_bitmap_to_surface(slot->bitmap);
        if (!surface)
            return false;

        auto glyph_tex = make_texture(sdl_console::SDL_CreateTextureFromSurface(renderer_, surface.get()));
        if (!glyph_tex)
            return false;

        sdl_console::SDL_SetRenderTarget(renderer_, pages.back().texture.get());
        sdl_console::SDL_RenderCopy(renderer_, glyph_tex.get(), nullptr, &cell.rect);
        sdl_console::SDL_SetRenderTarget(renderer_, nullptr);

        glyphs_[codepoint] = {
            .rect = cell.rect,
            .bearing_y = decltype(GlyphRec::bearing_y)(slot->bitmap_top),
            .height = decltype(GlyphRec::height)(slot->bitmap.rows),
            .page_idx = last_page_idx()
        };

        return true;
    }

    GlyphRec& get_glyph_rec(char32_t codepoint, Font::FallbackMode fbm) override
    {
        if (has_glyph_rec(codepoint)) {
            return glyphs_[codepoint];
        }

        if (load_char_to_atlas(codepoint)) {
            return glyphs_[codepoint];
        }

        if (fbm == Font::FallbackMode::Enabled
                && font.fallback() && font.fallback()->has_glyph(codepoint)) {
            glyphs_[codepoint] = get_fallback_glyph_rec(*font.fallback(), codepoint);
            glyphs_[codepoint].bearing_y = font.metrics().ascent; // FIXME: only for bitmap fallback
            return glyphs_[codepoint];
        }

        glyphs_[codepoint] = glyphs_[0];
        return glyphs_[0];
    }

    // Returns the next free cell, building a new page if necessary.
    // Otherwise returns Cell::invalid() on failure to create a new page
    Cell advance_to_next_free_cell() // NOLINT
    {
        auto& page = pages.back();
        const int glyph_idx = page.next_free_cell++;

        // if full, make a new page
        if (glyph_idx >= max_cells()) {
            if (!create_page())
                return Cell::invalid();
            return advance_to_next_free_cell();
        }

        const int cols = atlas_size.w / cell_width;
        const int gx = glyph_idx % cols;
        const int gy = glyph_idx / cols;

        auto c = Cell {
            .rect = {
                .x = gx * cell_width,
                .y = gy * cell_height,
                .w = font.char_width(),
                .h = font.line_height()
            },
            .page_idx = last_page_idx()
        };

        return c;
    }

    int8_t last_page_idx() const noexcept
    {
        return pages.size() - 1;
    }

    bool create_page()
    {
        auto tex = make_texture(sdl_console::SDL_CreateTexture(renderer_,
                                                               SDL_PIXELFORMAT_RGBA32,
                                                               SDL_TEXTUREACCESS_TARGET,
                                                               atlas_size.w, atlas_size.h));

        if (tex == nullptr)
            return false;

        sdl_console::SDL_SetTextureBlendMode(tex.get(), SDL_BLENDMODE_BLEND);
        pages.push_back({.texture = std::move(tex), .next_free_cell = 0});

        return true;
    }
};

class TrueTypeFont : public Font {
public:
    explicit TrueTypeFont(SDL_Renderer* renderer, const std::filesystem::path& path)
        : FontRenderer(renderer)
        , Font(path)
    {
    }

    static std::unique_ptr<TrueTypeFont> create(SDL_Renderer* renderer, const std::filesystem::path& path, int size)
    {
        auto font = std::make_unique<TrueTypeFont>(renderer, path);
        FT_Library ftl = freetype::get_ft_library();
        if (!ftl) {
            log_error<Error::FreeType>("Failed to load freetype library");
            return nullptr;
        }

        if (FT_Error err = FT_New_Face(ftl, path.c_str(), 0, &font->face_)) {
            log_error<Error::FreeType>("Failed to create font face", err);
            return nullptr;
        }

        if (!font->set_size(size)) {
            log_error<Error::FreeType>("Failed to set font size");
            return nullptr;
        }

        font->atlas_ = TrueTypeFontAtlas::create(*font, renderer, font->face_);
        if (!font->atlas_)
            return nullptr;
        return font;
    }

    FontAtlas& atlas() override
    {
        return *atlas_;
    }

    // Returns true on success, otherwise false.
    bool set_size(int desired_px) override
    {
        // Anything less than 12px high is far too small for non-square fonts.
        desired_px = std::clamp(desired_px, 12, 32);
        auto* window = sdl_console::SDL_RenderGetWindow(renderer_);
        if (!window) {
            log_error<Error::SDL>("Failed to get renderer window");
            return false;
        }

        int logical_height = 0;
        sdl_console::SDL_GetWindowSize(window, nullptr, &logical_height);
        if (logical_height <= 0) {
            log_error<Error::SDL>("Failed to get window size");
            return false;
        }

        int pixel_height = 0;
        sdl_console::SDL_GetRendererOutputSize(renderer_, nullptr, &pixel_height);
        if (pixel_height <= 0) {
            log_error<Error::SDL>("Invalid output size");
            return false;
        }

        auto scale = float(pixel_height) / logical_height;

        const int scaled = int(std::round(desired_px * scale));
        if (FT_Error err = FT_Set_Pixel_Sizes(face_, 0, scaled)) {
            log_error<Error::FreeType>("Failed to set font size", err);
            return false;
        }

        const int char_width = [this]() {
            if (FT_Load_Char(face_, 'M', FT_LOAD_BITMAP_METRICS_ONLY) == 0) {
                return face_->glyph->advance.x >> 6;
            }
            // 'M' was missing
            return face_->size->metrics.max_advance >> 6;
        }();

        if (char_width <= 0) {
            log_error<Error::FreeType>("bad char_width");
            return false;
        }

        const int mh = face_->size->metrics.height >> 6;
        const int line_height = mh > 0 ? mh : (face_->size->metrics.ascender - face_->size->metrics.descender) >> 6;

        if (line_height <= 0) {
            log_error<Error::FreeType>("bad line_height");
            return false;
        }

        const int ascent = face_->size->metrics.ascender >> 6;
        const int spacing = Font::line_spacing;

        metrics_ = FontMetrics{
            .char_width = char_width,
            .line_height = line_height,
            .line_spacing = spacing,
            .line_height_with_spacing = clamp_max<FontMetrics::Int>(line_height + spacing),
            .ascent = ascent
        };

        return true;
    }

    int size() const noexcept override
    {
        // TODO: cache this
        return face_->size->metrics.y_ppem;
    }

    Texture to_texture(StringView text) override
    {
        if (text.empty())
            return make_texture(nullptr);

        auto width = metrics_.char_width * text.size();
        auto target = make_surface(sdl_console::SDL_CreateRGBSurfaceWithFormat(0, width,
                                                                               metrics_.line_height, 32,
                                                                               SDL_PIXELFORMAT_RGBA32));
        if (!target)
            return make_texture(nullptr);

        sdl_console::SDL_FillRect(target.get(), nullptr, sdl_console::SDL_MapRGBA(target->format, 0, 0, 0, 0));

        int pen_x = 0;

        for (char32_t c : text) {
            if (FT_Load_Char(face_, c,  FT_LOAD_RENDER))
                continue;

            FT_GlyphSlot slot = face_->glyph;
            auto glyph_surf = freetype::blit_bitmap_to_surface(slot->bitmap);
            if (!glyph_surf)
                continue;

            int y_offset = metrics_.ascent - slot->bitmap_top;

            SDL_Rect dst = {
                .x = pen_x,
                .y = y_offset + (metrics_.line_spacing / 2),
                .w = metrics_.char_width,
                .h = int(slot->bitmap.rows)
            };

            SDL_BlitSurface(glyph_surf.get(), nullptr, target.get(), &dst);
            pen_x += metrics_.char_width;
        }

        auto tex = make_texture(sdl_console::SDL_CreateTextureFromSurface(renderer_, target.get()));
        return tex;
    }

    bool has_glyph(char32_t codepoint) const override
    {
        return FT_Get_Char_Index(face_, codepoint) != 0;
    }

protected:
    FT_Face face_ { nullptr };

private:
    std::unique_ptr<TrueTypeFontAtlas> atlas_;
};


class FontLoader {
    using FontMap = std::map<std::string, std::unique_ptr<Font>>;
public:
    static constexpr std::string_view default_key = "default";
    static constexpr std::string_view default_atlas_key = "default_atlas";

    explicit FontLoader(SDL_Renderer* r) : renderer_(r) { };
    ~FontLoader() = default;

    template <typename FontType>
    Font* load_font(const std::string& key, const std::string& path, int size)
    {
        if (fmap_.contains(key))
            return fmap_[key].get();

        auto font = FontType::create(renderer_, path, size);
        if (!font)
            return nullptr;

        auto* ptr = font.get();
        fmap_[key] = std::move(font);
        return ptr;
    }

    Font* load_bitmap(const std::string& key, const std::string& path, int size)
    {
        return load_font<DFBitmapFont>(key, path, size);
    }

    Font* load_truetype(const std::string& key, const std::string& path, int size)
    {
        return load_font<TrueTypeFont>(key, path, size);
    }

    Font* get_default()
    {
        const std::string key { default_key };
        if (fmap_.contains(key))
            return fmap_[key].get();
        return nullptr;
    }

    // No copies
    FontLoader(const FontLoader&) = delete;
    FontLoader& operator=(const FontLoader&) = delete;

    FontLoader(FontLoader&& other) noexcept = delete;
    FontLoader& operator=(FontLoader&& other) noexcept = delete;


private:
    FontMap fmap_;
    SDL_Renderer* const renderer_;
};

class Widget;

struct SDLWindowResources {
    const uint32_t id;
    Window const window; // order matters. window is destroyed last.
    Renderer const renderer;

    SDLWindowResources(Window w, Renderer r, uint32_t id)
    : id(id)
    , window(std::move(w))
    , renderer(std::move(r))
    { }
};

/*
* Shared context object for a window and its children.
*
*/
struct WidgetContext {
    struct WidgetState {
        Widget* hovered { nullptr };
        Widget* focused { nullptr };
        Widget* input { nullptr };

        void clear_if(Widget* w) noexcept
        {
            if (hovered == w)
                hovered = nullptr;

            if (focused == w)
                focused = nullptr;

            if (input == w)
                input = nullptr;
        }
    };

    Property& props;
    EventBus& event_bus;
    SDLWindowResources& sdl_window;
    FontLoader font_loader;
    WidgetState widget;
    TicksT now_tick;

    explicit WidgetContext(Property& p,
                         EventBus& em,
                         SDLWindowResources& sdl_window)
        : props(p)
        , event_bus(em)
        , sdl_window(sdl_window)
        , font_loader(sdl_window.renderer.get())
    { };

    WidgetContext(WidgetContext&&) = delete;
    WidgetContext& operator=(WidgetContext&&) = delete;

    WidgetContext(const WidgetContext&) = delete;
    WidgetContext& operator=(const WidgetContext&) = delete;
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

static bool has_align(Align a, Align flag) {
    return (uint8_t(a) & uint8_t(flag)) != 0;
}

struct Margin {
    int left { 0 };
    int top { 0 };
    int right { 0 };
    int bottom { 0 };
};

struct LayoutSpec {
    Margin margin;
    geometry::Size size;
    uint8_t stretch { 0 }; // relative flex factor (0 = fixed)
    Align align = Align::Fill;

    void apply_margins(SDL_Rect& r) const
    {
        r.x += margin.left;
        r.y += margin.top;
        r.w -= margin.left + margin.right;
        r.h -= margin.top  + margin.bottom;
    }
};

// Safe guard frames from 0 width and height
static void ensure_frame(SDL_Rect& r)
{
    r.w = std::max(r.w, 1);
    r.h = std::max(r.h, 1);
}

} // namespace layout

class Widget {
public:
    Widget* parent { nullptr };
    Font* font { nullptr };
    // Give sane default. Width and Height should be at least 1.
    SDL_Rect frame { .x = 0, .y = 0, .w = 1, .h = 1 };
    WidgetContext& context;
    Widget& window;

    layout::LayoutSpec layout_spec;

    std::vector<std::unique_ptr<Widget>> children;

    explicit Widget(Widget* parent)
        : parent(parent)
        , font(parent->font)
        , frame(parent->frame)
        , context(parent->context)
        , window(parent->window)
    {
    }

    Widget(Widget* parent, Font* font)
    : parent(parent)
    , font(font)
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
    explicit Widget(WidgetContext& ctx)
        : context(ctx)
        , window(*this)

    {
    }

    SDL_Renderer* renderer() const noexcept
    {
        return context.sdl_window.renderer.get();
    }

    static SDL_Point map_to(const SDL_Point& point, const SDL_Rect& rect) noexcept
    {
        return { point.x - rect.x, point.y - rect.y };
    }

    Property& props() const noexcept
    {
        return context.props;
    }

    virtual void render() = 0;

    virtual void layout_children() {};

    virtual geometry::Size preferred_size() const { return {.w = 1, .h = 1}; }

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

    static Widget* find_widget_at(Widget* root, int x, int y) noexcept
    {
        if (!root)
            return nullptr;

        if (!geometry::in_rect({x, y}, root->frame))
            return nullptr;

        for (const auto& child_uptr : std::views::reverse(root->children)) {
            Widget* child = child_uptr.get();
            if (Widget* hit = find_widget_at(child, x, y))
                return hit;
        }

        return root;
    }

    void take_input_focus()
    {
        auto* prev = context.widget.input;
        if (prev == this)
            return;

        if (prev)
            prev->emit(InputFocusChangedEvent { .has_focus = false });

        context.widget.input = this;
        emit(InputFocusChangedEvent { .has_focus = true });
    }

    virtual ~Widget() noexcept {
        context.event_bus.invalidate_receiver_slots(this);

        context.widget.clear_if(this);

        if (this != &window) {
            context.event_bus.emit(WidgetDestroyedEvent{.addr = this}, nullptr);
        }
    }

    template <typename E>
    bool emit(E&& event) {
        return context.event_bus.emit(std::forward<E>(event), this);
    }

    Widget(Widget&&) = delete;
    Widget& operator=(Widget&&) = delete;

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

protected:
    // Should remain protected to guard against misuse  .
    // 1. 'this' registers the object with the event bus.
    // 2. Objects that call connect() must call invalidate_receiver_slots() before destruction.
    template <typename E, typename F>
    ISlot* connect(void* sender, F&& func) {
        return context.event_bus.connect<E>(sender, this, std::forward<F>(func));
    }
};

struct VerticalAxis {
    static constexpr int& major(SDL_Rect& r) noexcept { return r.h; }   // height
    static constexpr int& minor(SDL_Rect& r) noexcept { return r.w; } // width

    static constexpr const int& major(const SDL_Rect& r) noexcept { return r.h; }
    static constexpr const int& minor(const SDL_Rect& r) noexcept { return r.w; }

    static constexpr int& start(SDL_Rect& r) noexcept { return r.y; }
    static constexpr const int& start(const SDL_Rect& r) noexcept { return r.y; }

    static constexpr int& minor_start(SDL_Rect& r) noexcept { return r.x; }
    static constexpr const int& minor_start(const SDL_Rect& r) noexcept { return r.x; }

    static constexpr int& size(layout::LayoutSpec& s) noexcept { return s.size.h; }
    static constexpr const int& size(const layout::LayoutSpec& s) noexcept { return s.size.h; }

    static constexpr int& size(SDL_Rect& r) noexcept { return r.h; }
    static constexpr const int& size(const SDL_Rect& r) noexcept { return r.h; }

    static constexpr int& margin_start(layout::Margin& m) noexcept { return m.top; }
    static constexpr int& margin_end(layout::Margin& m) noexcept { return m.bottom; }

    static constexpr int& minor_margin_start(layout::Margin& m) noexcept { return m.left; }
    static constexpr int& minor_margin_end(layout::Margin& m) noexcept { return m.right; }

    static constexpr layout::Align center_flag() noexcept { return layout::Align::VCenter; }
    static constexpr layout::Align end_flag() noexcept { return layout::Align::Bottom; }
};

struct HorizontalAxis {
    static constexpr int& major(SDL_Rect& r) noexcept { return r.w; }
    static constexpr int& minor(SDL_Rect& r) noexcept { return r.h; }

    static constexpr const int& major(const SDL_Rect& r) noexcept { return r.w; }
    static constexpr const int& minor(const SDL_Rect& r) noexcept { return r.h; }

    static constexpr int& start(SDL_Rect& r) noexcept { return r.x; }
    static constexpr const int& start(const SDL_Rect& r) noexcept { return r.x; }

    static constexpr int& minor_start(SDL_Rect& r) noexcept { return r.y; }
    static constexpr const int& minor_start(const SDL_Rect& r) noexcept { return r.y; }

    static constexpr int& size(layout::LayoutSpec& s) noexcept { return s.size.w; }
    static constexpr const int& size(const layout::LayoutSpec& s) noexcept { return s.size.w; }

    static constexpr int& size(SDL_Rect& r) noexcept { return r.w; }
    static constexpr const int& size(const SDL_Rect& r) noexcept { return r.w; }

    static constexpr int& margin_start(layout::Margin& m) noexcept { return m.left; }
    static constexpr int& margin_end(layout::Margin& m) noexcept { return m.right; }

    static constexpr int& minor_margin_start(layout::Margin& m) noexcept { return m.top; }
    static constexpr int& minor_margin_end(layout::Margin& m) noexcept { return m.bottom; }

    static constexpr layout::Align center_flag() noexcept { return layout::Align::HCenter; }
    static constexpr layout::Align end_flag() noexcept { return layout::Align::Right; }
};

template <typename Axis>
class BoxLayout : public Widget {
public:
    int spacing = 0;

    explicit BoxLayout(Widget* parent)
    : Widget(parent)
    {}

    void render() override
    {
        for (auto& child : children)
            child->render();
    }

    void layout_children() override
    {
        if (children.empty())
            return;

        int total_stretch = 0;
        int total_fixed = 0;

        auto& margin = layout_spec.margin;

        const int avail_major = Axis::major(frame) - (Axis::margin_start(margin) + Axis::margin_end(margin));
        const int avail_minor = Axis::minor(frame) - (Axis::minor_margin_start(margin) + Axis::minor_margin_end(margin));

        for (auto& child : children) {
            auto& spec = child->layout_spec;
            if (spec.stretch > 0)
                total_stretch += spec.stretch;
            else
                total_fixed += (Axis::size(spec) > 0 ? Axis::size(spec)
                                                     : Axis::major(child->frame));
        }

        total_fixed += spacing * (std::ssize(children) - 1);
        const int available = std::max(avail_major - total_fixed, 1);
        int offset = Axis::start(frame) + Axis::margin_start(margin);

        int idx = 0;
        for (auto& child_uptr : children) {
            auto& child = *child_uptr.get();
            SDL_Rect r = child.frame;

            Axis::minor(r) = avail_minor;
            Axis::minor_start(r) = Axis::minor_start(frame);

            int major_size = 1;
            if (child.layout_spec.stretch > 0)
                major_size = available * child.layout_spec.stretch / total_stretch;
            else if (Axis::size(child.layout_spec) > 0)
                major_size = Axis::size(child.layout_spec);

            Axis::major(r) = major_size;
            Axis::start(r) = offset;
            offset += major_size;

            if (idx < std::ssize(children) - 1)
                offset += spacing;

            child.layout_spec.apply_margins(r);

            layout::ensure_frame(r);

            apply_alignment(r, child.layout_spec.align);

            child.resize(r);
        }
    }

    constexpr void apply_alignment(SDL_Rect& r, layout::Align align) noexcept
    {
        if (align == layout::Align::Fill)
            return;

        auto& margin = layout_spec.margin;

        const int avail = Axis::size(frame) - (Axis::margin_start(margin) + Axis::margin_end(margin));

        if (layout::has_align(align, Axis::center_flag())) {
            Axis::start(r) = Axis::start(frame) + Axis::margin_start(margin)
                                                + (avail - Axis::size(r)) / 2;
        } else if (layout::has_align(align, Axis::end_flag())) {
            Axis::start(r) = Axis::start(frame) + avail - Axis::size(r)
                                                        - Axis::margin_end(margin);
        } else {
            Axis::start(r) = Axis::start(frame) + Axis::margin_start(margin);
        }
    }

};

using VBox = BoxLayout<VerticalAxis>;
using HBox = BoxLayout<HorizontalAxis>;

class Cursor {
public:
    enum class DrawStyle { Fill, Outline };

    SDL_Rect rect;
    DrawStyle style{DrawStyle::Fill};
    bool visible{true}; // false when scrolling away from the line it sits on
    bool rebuild{true};

    explicit Cursor(SDL_Renderer* renderer, const TicksT& ticks_ref)
    : renderer_(renderer), now_tick(ticks_ref)
    {
    }

    void update_blink() noexcept
    {
        if (now_tick - last_blink_time_ >= blink_interval_ms) {
            blink_on_ = !blink_on_;
            visible = blink_on_;
            last_blink_time_ = now_tick;
        }
    }

    void pause_blink() noexcept
    {
        visible = true;
        auto now = SDL_GetTicks64();
        last_blink_time_ = now;
    }

    int position() const noexcept
    {
        return position_;
    }

    void render() const noexcept
    {
        if (!visible) {
            return;
        }
        SDL_Color c = colors::white;
        c.a = 128;
        set_draw_color(renderer_, c); // 128 - about 50% transparant

        if (style == DrawStyle::Fill)
            SDL_RenderFillRect(renderer_, &rect);
        else
            SDL_RenderDrawRect(renderer_, &rect);

        c.a = 255;
        set_draw_color(renderer_, c);
    }

    Cursor& operator=(size_t position) noexcept
    {
        position_ = position;
        rebuild = true;
        return *this;
    }

    Cursor& operator++() noexcept
    {
        ++position_;
        rebuild = true;
        return *this;
    }

    Cursor& operator--() noexcept
    {
        if (position_ > 0) {
            --position_;
            rebuild = true;
        }
        return *this;
    }

    Cursor& operator+=(std::size_t n) noexcept
    {
        position_ += n;
        rebuild = true;
        return *this;
    }

    Cursor& operator-=(std::size_t n) noexcept
    {
        position_ = (n > position_) ? 0 : position_ - n;
        rebuild = true;
        return *this;
    }

    Cursor(const Cursor&) = delete;
    Cursor& operator=(const Cursor&) = delete;
    Cursor(Cursor&&) = delete;
    Cursor& operator=(Cursor&&) = delete;

private:
    SDL_Renderer* const renderer_;
    const TicksT& now_tick;
    size_t position_ { 0 };
    uint64_t last_blink_time_{0};
    bool blink_on_{false};
    static constexpr uint32_t blink_interval_ms{500};
};

class SingleLineEdit : public Widget {
public:
    int max_visible_chars { 20 };

    explicit SingleLineEdit(Widget* parent)
    : Widget(parent), cursor_(renderer(), context.now_tick)
    {
        connect<SDLTextInputEvent>(this, [this](auto& e) {
            const auto str = text::from_utf8(e.text.text);
            insert_at_cursor(str);
            cursor_.pause_blink();
            return true;
        });

        connect<SDLKeyDownEvent>(this, [this](auto& e) {
            const auto sz = text_.size();
            on_key_down(e.key);
            if (sz != text_.size())
                emit(ValueChangedEvent<String>{.value = text_});
            cursor_.pause_blink();
            return true;
        });

        connect<PasteEvent>(this, [this](auto& e) {
            insert_at_cursor(e.text);
            cursor_.pause_blink();
            return true;
        });

        visible_text_ = text_;
    }

    void set_text(StringView t)
    {
        text_ = t;
        cursor_ = text_.length();
        scroll();
        rebuild_cursor();
        emit(ValueChangedEvent<String>{.value = text_});
    }

    auto get_text()
    {
        return text_;
    }

    void insert_at_cursor(StringView new_text)
    {
        cursor_ = text::insert_at(text_, cursor_.position(), new_text);
        scroll();
        rebuild_cursor();
        emit(ValueChangedEvent<String>{.value = text_});
    }

    void resize(const SDL_Rect& r) override {
        frame = r;
        compute_text_baseline();
        text_start_x_ = frame.x + font->char_width();
        max_visible_chars = (frame.w / font->char_width()) - 2;
        set_visible_text();
        rebuild_cursor();
    }

    void render() override
    {
        set_draw_color(renderer(), {0, 0, 0, 255});
        SDL_RenderFillRect(renderer(), &frame);

        if (context.widget.input == this) {
            set_draw_color(renderer(), colors::gold);
            SDL_RenderDrawRect(renderer(), &frame);

            if (cursor_.rebuild) {
                rebuild_cursor();
            }

            cursor_.update_blink();

            cursor_.rect = {
                .x = cursor_.rect.x,
                .y = text_baseline_y_,
                .w = 2,
                .h = font->line_height_with_spacing()
            };
            cursor_.render();
        }

        SDL_Point pos{ text_start_x_, text_baseline_y_ };
        font->atlas().render(visible_text_, pos.x, pos.y);
    }

    geometry::Size preferred_size() const noexcept override {
        geometry::Size sz;
        sz.w = grid::columns_to_pixels(max_visible_chars, font->char_width()) + 4;
        sz.h = font->line_height_with_spacing() * 2;
        return sz;
    }

private:
    String text_;
    String visible_text_;
    Cursor cursor_;
    int text_baseline_y_ { 0 };
    int text_start_x_ { 0 };
    int scroll_chars_ { 0 };

    void backspace()
    {
        cursor_ = text::backspace(text_, cursor_.position());
    }

    void compute_text_baseline() noexcept
    {
        int available = frame.h - 4;
        text_baseline_y_ = frame.y + 2 + (available - font->line_height()) / 2;
    }

    void rebuild_cursor() noexcept
    {
        cursor_.rect.x = text_start_x_ + grid::columns_to_pixels(cursor_.position() - scroll_chars_, font->char_width());
        cursor_.rebuild = false;
    }

    void scroll()
    {
        if (cursor_.position() < scroll_chars_) {
            scroll_chars_ = cursor_.position(); // scroll left
        } else if (cursor_.position() >= scroll_chars_ + max_visible_chars) {
            scroll_chars_ = cursor_.position() - max_visible_chars + 1; // scroll right
        }

        set_visible_text();
    }

    void set_visible_text()
    {
        const int text_len = std::ssize(text_);
        const int visible_len = std::min(max_visible_chars, text_len);

        scroll_chars_ = std::clamp(scroll_chars_, 0, std::max(0, text_len - visible_len));
        visible_text_ = text_.substr(scroll_chars_, visible_len);
    }

    void on_key_down(const SDL_KeyboardEvent& e)
    {
        // TODO: check if keysym.sym mapping is universally locale friendly
        const auto sym = e.keysym.sym;
        switch (sym) {
        case SDLK_BACKSPACE:
            backspace();
            break;

        case SDLK_LEFT:
            --cursor_;
            break;

        case SDLK_RIGHT:
            if (cursor_.position() < std::ssize(text_))
                ++cursor_;
            break;

        case SDLK_HOME:
            cursor_ = 0;
            break;

        case SDLK_END:
            cursor_ = std::ssize(text_);
            break;

        case SDLK_v: // FIXME: belongs in prompt
            if (sdl_console::SDL_GetModState() & KMOD_CTRL) {
                String cbtext = clipboard::get_text();
                if (cbtext.size()) {
                    insert_at_cursor(cbtext);
                }
            }
            break;
        default:;
        }

        scroll();
    }
};

struct ViewPortRow {
    size_t entry_id;
    const TextEntry::Fragment& frag;
    SDL_Point coord;
    GlyphPosVector gpv;
    std::optional<SDL_Color> color;
};

struct ViewPort {
    std::vector<ViewPortRow> rows;
    bool rebuild { true };

    void clear() noexcept
    {
        rows.clear();
    }

    const ViewPortRow* find_row_at_y(Font& font, int y) const noexcept
    {
        auto it = std::ranges::find_if(rows, [&font, y](ViewPortRow const& r) {
            return geometry::is_y_within(y, r.coord.y, font.line_height_with_spacing());
        });

        if (it != rows.end()) {
            return &(*it);
        }

        return nullptr;
    }
};

class Prompt : public Widget {
public:
    // Holds wrapped lines from input
    TextEntry entry;
    // For input history.
    // use deque to hold a stable reference.
    std::deque<String> history;
    Cursor cursor;
    // The text of the prompt itself.
    String prompt_text;
    // The input portion of the prompt.
    String* input;
    String saved_input;
    int history_index { 0 };
    bool rebuild { true };

    explicit Prompt(Widget* parent)
        : Widget(parent)
        , cursor(renderer(), context.now_tick)
    {
        input = &history.emplace_back(U"");

        set_prompt_text(props().get(property::PROMPT_TEXT).value_or(U"> "));

        connect<SDLKeyDownEvent>(this, [this](auto& e) {
            on_key_down(e.key);
            return true;
        });

        connect<SDLTextInputEvent>(this, [this](auto& e) {
            insert_at_cursor(text::from_utf8(e.text.text));
            return true;
        });
    }

    //~Prompt() override = default;

    /* OutputPane does this */
    void render() override
    {
    }

    void on_key_down(const SDL_KeyboardEvent& e)
    {
        // TODO: check if keysym.sym mapping is universally locale friendly
        const auto sym = e.keysym.sym;
        switch (sym) {
        case SDLK_BACKSPACE:
            backspace();
            break;

        case SDLK_UP:
            set_input_from_history(ScrollAction::Up);
            break;

        case SDLK_DOWN:
            set_input_from_history(ScrollAction::Down);
            break;

        case SDLK_LEFT:
            move_cursor_left();
            break;

        case SDLK_RIGHT:
            move_cursor_right();
            break;

        case SDLK_RETURN:
            submit_command();
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

    void set_command_history(std::deque<String> saved_history)
    {
        std::swap(history, saved_history);
        input = &history.emplace_back(U"");
        history_index = history.size() - 1;
        cursor = 0;
        rebuild = true;
    }

    void submit_command()
    {
        emit(CommandSubmittedEvent { .command = *input });

        // If empty, log an empty line? But don't add it to history.
        if (input->empty()) {
            return;
        }

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
        emit(InputSubmittedEvent { .text = *input });
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

    void set_prompt_text(const StringView value)
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

        if (sa == ScrollAction::Up && history_index > 0) {
            history_index--;
        } else if (sa == ScrollAction::Down && history_index < std::ssize(history) - 1) {
            history_index++;
        } else {
            return;
        }

        input = &history.at(history_index);
        cursor = input->length();
        wrap_text();
    }

    void insert_at_cursor(const StringView str)
    {
        cursor = text::insert_at(*input, cursor.position(), str);
        wrap_text();
    }

    void set_input(const StringView str)
    {
        *input = str;
        cursor = str.length();
        wrap_text();
    }

    void backspace()
    {
        cursor = text::backspace(*input, cursor.position());
        wrap_text();
    }

    void move_cursor_left() noexcept
    {
        if (cursor.position() > 0) {
            --cursor;
        }
    }

    void move_cursor_right() noexcept
    {
        if (cursor.position() < std::ssize(*input)) {
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
        entry.wrap_text(font->char_width(), frame.w);
        rebuild = true;
    }

    void update_cursor_geometry(std::span<const ViewPortRow> vrows)
    {
        if (entry.fragments().empty()) { // Shouldn't happen'
            return;
        }

        // cursor's starting position
        const auto cursor_pos = cursor.position() + prompt_text.length();

        auto get_cursor_line = [&cursor_pos, this]() -> const TextEntry::Fragment* {
            if (cursor_pos >= entry.text.length()) {
                return &entry.fragments().back();
            }
            // else find the line containing the cursor
            return entry.fragment_from_offset(cursor_pos);
        };

        const auto* line = get_cursor_line();
        if (!line)
            return; // Shouldn't happen

        const auto it = std::ranges::find_if(vrows, [line](ViewPortRow const& r) {
            return &r.frag == line;
        });

        if (it == vrows.end()) {
            cursor.visible = false;
            return;
        }

        cursor.visible = true;
        const ViewPortRow& vr = *it;

        auto lh = font->line_height_with_spacing();
        auto cw = font->char_width();
        auto cx = int(cursor_pos - line->start_offset) * cw;
        auto cy = vr.coord.y;

        cursor.rect = { .x = cx,
                        .y = cy,
                        .w = cw,
                        .h = lh };
    }

    void render_cursor() const noexcept
    {
        cursor.render();
    }

    bool rebuild_needed() const noexcept
    {
        return rebuild || cursor.rebuild;
    }

    Prompt(Prompt&&) = delete;
    Prompt& operator=(Prompt&&) = delete;

    Prompt(const Prompt&) = delete;
    Prompt& operator=(const Prompt&) = delete;
};

class Scrollbar : public Widget {
public:
    explicit Scrollbar(Widget* parent)
        : Widget(parent)
    {
        connect<SDLMouseButtonDownEvent>(this, [this](auto& e) {
            return on_mouse_button_down(e.button);
        });

        connect<SDLMouseButtonUpEvent>(this, [this](auto& e) {
            return on_mouse_button_up(e.button);
        });

        mouse_motion_slot_ = context.event_bus.connect_later<SDLMouseMotionEvent>(&window, this, [this](auto& e) {
            return on_mouse_motion(e.motion);
        });

        thumb_.set_rect(frame);
        set_thumb_height();
    }

    void resize(const SDL_Rect& r) noexcept override
    {
        frame = r;
        thumb_.set_rect(frame);
        resize_thumb();
    }

    void set_page_size(size_t size) noexcept
    {
        page_size_ = size;
        resize_thumb();
    }

    void set_content_size(size_t size) noexcept
    {
        content_size_ = size;
        resize_thumb();
    }

    void scroll_to(size_t position) noexcept
    {
        scroll_offset_ = position;
        move_thumb_to(track_position_from_scroll_offset());
    }

    void render() noexcept override
    {
        set_draw_color(renderer(), track_color);

        SDL_RenderDrawRect(renderer(), &frame);

        set_draw_color(renderer(), Thumb::color);

        SDL_Rect margined_thumb = {
            thumb_.rect.x + 4,
            thumb_.rect.y + 4,
            thumb_.rect.w - 8,
            thumb_.rect.h - 8
        };

        SDL_RenderFillRect(renderer(), &margined_thumb);

        //set_draw_color(renderer(), colors::darkgray);
    }

    Scrollbar(Scrollbar&&) = delete;
    Scrollbar& operator=(Scrollbar&&) = delete;

    Scrollbar(const Scrollbar&) = delete;
    Scrollbar& operator=(const Scrollbar&) = delete;

private:
    static constexpr SDL_Color track_color { colors::gold };

    struct Thumb {
        SDL_Rect rect{};
        static constexpr SDL_Color color { colors::mauve };

        void set_rect(SDL_Rect& r)
        {
            rect = r;
        }
    };

    int page_size_; // Height of visible area.
    int content_size_ { 0 }; // Total height of content.
    int scroll_offset_ { 0 };
    bool depressed_ { false };
    ISlot* mouse_motion_slot_ { nullptr };
    Thumb thumb_;

    void resize_thumb() noexcept
    {
        set_thumb_height();
        move_thumb_to(track_position_from_scroll_offset());
    }

    bool on_mouse_button_down(auto& e)
    {
        if (!mouse_motion_slot_->is_connected()) {
            mouse_motion_slot_->connect();
        }

        depressed_ = true;
        scroll_offset_ = scroll_offset_from_y(e.y);
        move_thumb_to(track_position_from_scroll_offset());
        emit(ValueChangedEvent<int>{.value = scroll_offset_});
        return true;
    }

    bool on_mouse_button_up(auto& /*e*/)
    {
        if (depressed_) {
            depressed_ = false;
            mouse_motion_slot_->disconnect();
            return true;
        }
        return false;
    }

    bool on_mouse_motion(auto& e) noexcept
    {
        if (!depressed_) { return false; }

        scroll_offset_ = scroll_offset_from_y(e.y);
        move_thumb_to(track_position_from_scroll_offset());

        emit(ValueChangedEvent<int>{.value = scroll_offset_});
        return true;
    }

    int calculate_thumb_position(int target_y) const noexcept
    {
        int track_top = frame.y;
        int track_bot = frame.y + frame.h;

        // Position with offset and constrain within track limits
        return std::clamp(target_y - thumb_.rect.h, track_top, track_bot - thumb_.rect.h);
    }

    void move_thumb_to(int y) noexcept
    {
        thumb_.rect.y = calculate_thumb_position(y);
    }

    void set_thumb_height() noexcept
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

    int scroll_offset_from_y(int y) const noexcept
    {
        const int track_range = frame.h - thumb_.rect.h;
        const int content_range = content_size_ - page_size_;

        if (track_range <= 0 || content_range <= 0)
            return 0;

        // Track position is aligns with the middle of the thumb.
        int thumb_mid_y = y - (thumb_.rect.h / 2);
        thumb_mid_y = std::clamp(thumb_mid_y, frame.y, frame.y + track_range);

        const float pos_ratio = (float)(thumb_mid_y - frame.y) / track_range;
        const int offset = (int)((1.0F - pos_ratio) * content_range);

        // Ensure the scroll offset does not go beyond the valid range
        return std::clamp(offset, 0, content_range);
    }

    int track_position_from_scroll_offset() const noexcept
    {
        if (content_size_ <= page_size_ || content_size_ == 0) {
            return frame.y;
        }

        float offset_ratio = (float)scroll_offset_ / (content_size_);
        int pos = (int)((1.0F - offset_ratio) * frame.h);

        return pos + frame.y;
    }

};

class Button : public Widget {
public:
    String label;
    SDL_Rect label_rect {};
    bool depressed { false };
    bool enabled { true };

    Button(Widget* parent, StringView label)
        : Widget(parent)
        , label(label)
    {
        connect<SDLMouseButtonDownEvent>(this, [this](auto& e) {
            depressed = true;
            return true;
        });

        connect<SDLMouseButtonUpEvent>(this, [this](auto& e) {
            depressed = false;
            if (!geometry::in_rect(e.button.x, e.button.y, frame))
                return false;

            emit(ClickedEvent{});
            return true;
        });
    }

    void resize(const SDL_Rect& r) override
    {
        frame = r;

        auto sz = font->size_text(label);
        label_rect.w = sz.w;
        label_rect.h = sz.h;

        // Center label within the frame
        label_rect.x = frame.x + (frame.w - label_rect.w) / 2;
        label_rect.y = frame.y + (frame.h - label_rect.h) / 2;
    }

    void render() override
    {
        if (!enabled) {
            auto scoped_color = font->set_color(colors::mediumgray);
            font->atlas().render(label, label_rect.x, label_rect.y);
            return;
        }

        int label_offset = 0;

        if (depressed) {
            label_offset = 1; // click effect
            set_draw_color(renderer(), colors::teal);
            SDL_RenderFillRect(renderer(), &frame);
        } else if (context.widget.hovered == this) {
            set_draw_color(renderer(), colors::teal);
            SDL_RenderDrawRect(renderer(), &frame);
        }

        font->atlas().render(label,
                     label_rect.x + label_offset,
                     label_rect.y + label_offset);
    }

    geometry::Size preferred_size() const override {
        auto sz = font->size_text(label);
        return { .w = sz.w + (font->char_width() * 2),
                 .h = sz.h + (font->line_height_with_spacing()) };
    }

    Button(Button&&) = delete;
    Button& operator=(Button&&) = delete;

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;
};

class Toolbar : public Widget {
public:
    explicit Toolbar(Widget* parent);
    ~Toolbar() override = default;
    void render() override;
    void resize(const SDL_Rect& rect) override;
    void layout_buttons();
    Button* add_button(const String& text);
    int compute_widgets_startx();
    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;
};

class CommandPipe {
public:
    CommandPipe() = default;

    void make_connection(EventBus& emitter, Widget* w)
    {
        emitter.connect<CommandSubmittedEvent>(w, this, [this](auto& e) {
            push(e.command);
            return false;
        });
    }

    void push(const String& s)
    {
        {
            std::scoped_lock lock(mutex_);
            queue_.push(s);
        }
        cv_.notify_one();
    }

    void shutdown() noexcept
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
    std::queue<String> queue_;
    std::condition_variable_any cv_;
    std::recursive_mutex mutex_;
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
                                   std::span<const ViewPortRow> rows) const
    {
        if (!active()) return {};

        // normalize so we always go from top to bottom
        const auto [top, bottom] = std::minmax(begin, end);

        const auto top_pos = std::make_pair(top.entry_id, top.frag_index);
        const auto bottom_pos = std::make_pair(bottom.entry_id, bottom.frag_index);

        std::vector<SDL_Rect> rects;
        for (const auto &row : rows) {
            const auto &frag = row.frag;
            const auto frag_pos = std::make_pair(row.entry_id, frag.index);

            // skip outside range
            if (frag_pos < top_pos || frag_pos > bottom_pos) continue;

            const size_t frag_len = frag.text.size();
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

            const auto [x, w] = grid::columns_to_pixel_extent(sel_start, sel_end, font->char_width());

            rects.push_back({.x = x,
                             .y = row.coord.y,
                             .w = w,
                             .h = font->line_height_with_spacing()});
        }
        return rects;
    }

std::vector<StringView> get_selected_text(std::deque<TextEntry>& entries) const
{
    if (!active()) return {};

    // normalize so we always go from top to bottom
    const auto [top, bottom] = std::minmax(begin, end);

    std::vector<StringView> result;

    for (const auto& entry : entries | std::views::reverse) {
        if (entry.id < top.entry_id || entry.id > bottom.entry_id)
            continue; // skip outside selection

        auto vec = get_selected_text_from_one(entry);
        if (!vec.empty()) {
            std::ranges::move(vec, std::back_inserter(result));
        }
    }

    return result;
}

std::vector<StringView> get_selected_text_from_one(const TextEntry& entry) const
{
    const auto [top, bottom] = std::minmax(begin, end);

    std::vector<StringView> result;
    if (entry.id < top.entry_id || entry.id > bottom.entry_id)
        return result;// skip outside selection

    for (const auto& frag : entry.fragments()) {
        if (frag.index < top.frag_index || frag.index > bottom.frag_index)
            continue;

        size_t frag_len = frag.text.size();
        size_t sel_start = 0;
        size_t sel_end = frag_len;

        bool is_top    = (entry.id == top.entry_id && frag.index == top.frag_index);
        bool is_bottom = (entry.id == bottom.entry_id && frag.index == bottom.frag_index);

        if (is_top) sel_start = std::min(top.column, frag_len);
        if (is_bottom) sel_end = std::min(bottom.column, frag_len);

        if (sel_start >= sel_end) continue; // empty selection
        // safe: sel_start checked < frag_len
        result.push_back(frag.text.substr(sel_start, sel_end - sel_start));
    }
    return result;
}

};

class TextFinder {
public:
    String needle;
    struct LineKey {
        size_t entry_id;
        size_t frag_index;

        bool operator==(const LineKey& other) const noexcept {
            return entry_id == other.entry_id && frag_index == other.frag_index;
        }
    };
    struct Match {
        LineKey line_key;
        size_t start_col;  // column in fragment
        size_t end_col;    // column in fragment
    };

    const Match* next() noexcept
    {
        if (matches.empty()) return nullptr;
        current_idx = (current_idx + 1) % matches.size();
        return &matches[current_idx];
    }

    const Match* prev() noexcept
    {
        if (matches.empty()) return nullptr;
        current_idx = (current_idx > 0 ? current_idx - 1 : matches.size() - 1) % matches.size();
        return &matches[current_idx];
    }

    const LineKey* next_line() noexcept
    {
        if (line_matches.empty()) return nullptr;
        current_line_idx = (current_line_idx + 1) % line_matches.size();
        return &line_matches[current_line_idx];
    }

    const LineKey* prev_line() noexcept
    {
        if (line_matches.empty()) return nullptr;
        current_line_idx = (current_line_idx > 0 ? current_line_idx - 1 : line_matches.size() - 1) % line_matches.size();
        return &line_matches[current_line_idx];
    }

    const Match* current() const noexcept
    {
        return matches.empty() ? nullptr : &matches[current_idx];
    }

    void clear() noexcept
    {
        needle.clear();
        current_idx = 0;
        current_line_idx = 0;
        matches.clear();
        line_matches.clear();
    }
    bool empty() const noexcept { return matches.empty(); }

    bool is_active() const noexcept { return !needle.empty(); }

    void find(std::deque<TextEntry>& entries,
              const StringView needle)
    {
        clear();
        if (needle.empty()) return;
        this->needle = needle;

        for (auto& entry : entries) {
            std::vector<LineKey> entry_buf;
            size_t pos = 0;
            while ((pos = entry.text.find(needle, pos)) != String::npos) {
                size_t match_start = pos;
                size_t match_end = pos + needle.size();

                for (const auto& frag : entry.fragments()) {
                    if (frag.end_offset <= match_start) continue;  // fragment ends before match
                    if (frag.start_offset >= match_end) break;     // fragment starts after match

                    size_t local_start = std::max(match_start, frag.start_offset) - frag.start_offset;
                    size_t local_end   = std::min(match_end, frag.end_offset) - frag.start_offset;

                    LineKey key{.entry_id = entry.id, .frag_index = frag.index};
                    matches.push_back({key, local_start, local_end});

                    if (entry_buf.empty() || entry_buf.back() != key)
                        entry_buf.push_back(key);
                }

                pos = match_end; // advance past current match
            }

            if (!entry_buf.empty()) {
                std::ranges::reverse(entry_buf);
                line_matches.insert(line_matches.end(),
                                    entry_buf.begin(), entry_buf.end());
            }

        }
    }

    const std::vector<Match>& all_matches() const noexcept { return matches; }

    SDL_Rect active_rect(const Font* font,
                         std::span<const ViewPortRow> rows) const noexcept
    {
        if (line_matches.empty()) return {};

        const auto& active_line = line_matches[current_line_idx];
        for (const auto& row : rows) {
            LineKey key{.entry_id = row.entry_id, .frag_index = row.frag.index};
            if (key == active_line) {
                return {
                    .x = row.coord.x,
                    .y = row.coord.y,
                    .w = grid::columns_to_pixels(row.frag.text.size(), font->char_width()),
                    .h = font->line_height_with_spacing()
                };
            }
        }
        return {};
    }

    std::vector<SDL_Rect> to_rects(const Font* font,
                                   std::span<const ViewPortRow> rows) const
    {
        std::vector<SDL_Rect> rects;
        for (const auto& m : matches) {
            for (const auto& row : rows) {
                LineKey key { .entry_id = row.entry_id, .frag_index = row.frag.index };
                if (key != m.line_key)
                    continue;

                auto [x, w] = grid::columns_to_pixel_extent(m.start_col, m.end_col, font->char_width());
                rects.push_back({ .x = x,
                                  .y = row.coord.y,
                                  .w = w,
                                  .h = font->line_height_with_spacing() });
            }
        }
        return rects;
    }

private:
    std::vector<Match> matches;
    std::vector<LineKey> line_matches;
    size_t current_idx{0};
    size_t current_line_idx{0};
};

class TextFinderWidget : public Widget {
    HBox* layout_ptr;

public:
    std::function<void(TextFinderWidget*)> on_close;
    SingleLineEdit *input{nullptr};

    explicit TextFinderWidget(Widget* parent, Font* f)
    : Widget(parent, f)
    {
        auto& layout = add_child<HBox>();
        layout_ptr = &layout;

     //   auto& spacer = layout.add_child<HBox>();
      //  spacer.layout_spec.stretch = 1;

        geometry::Size sz;
        auto& edit = layout.add_child<SingleLineEdit>();
        edit.max_visible_chars = 30;
        edit.layout_spec.stretch = 0;
        sz = edit.preferred_size();
        edit.layout_spec.size.w = sz.w;
        edit.layout_spec.size.h = sz.h;
        connect<ValueChangedEvent<String>>(&edit, [this](ValueChangedEvent<String>& e) {
            window.emit(FindTextQueryEvent{.query = e.value});
            return true;
        });
        input = &edit;

        auto& next = layout.add_child<Button>(U"next");
        sz = next.preferred_size();
        next.layout_spec.size.w = sz.w;
        next.layout_spec.size.h = sz.h;
        connect<ClickedEvent>(&next, [this](auto&) -> bool {
            window.emit(FindTextMoveEvent{.direction = ScrollAction::Up});
            return true;
        });

        auto& prev = layout.add_child<Button>(U"prev");
        sz = prev.preferred_size();
        prev.layout_spec.size.w = sz.w;
        prev.layout_spec.size.h = sz.h;
        connect<ClickedEvent>(&prev, [this](auto&) -> bool {
            window.emit(FindTextMoveEvent{.direction = ScrollAction::Down});
            return true;
        });

        auto&x = layout.add_child<Button>(U"x"); // close
        sz = x.preferred_size();
        x.layout_spec.size.w = sz.w;
        x.layout_spec.size.h = sz.h;
        connect<ClickedEvent>(&x, [this](auto&) -> bool {
            if (on_close)
                on_close(this);
            return true;
        });

        layout.layout_children();

        connect<SDLTextInputEvent>(this, [&edit](auto& e) {
            edit.emit(e);
            return true;
        });

        connect<SDLKeyDownEvent>(this, [&edit](auto& e) {
            edit.emit(e);
            return true;
        });
    }

    void render() override {
        set_draw_color(renderer(), colors::darkgray);
        sdl_console::SDL_RenderFillRect(renderer(), &frame); // paint a background layer to cover output history
        layout_ptr->render();
    }

    void resize(const SDL_Rect& r) override {
        // We want this widget to snap to the top right corner
        // of the history, and only take up the necessary width.
        // NOTE: It shouldn't be necessary to fetch the parent's frame
        // here (outputpane). this->frame should already be set to it.
        int fw = 0;
        // Let some history show to the left of this widget.
        for(auto& child : layout_ptr->children) {
            fw += child->frame.w;
        }

        const int fh = font->line_height_with_spacing() * 2;
        const auto& pf = parent->frame;

        frame.x = pf.x + pf.w - fw;
        frame.y = pf.y + 2;
        frame.w = fw;
        frame.h = fh;

        layout_ptr->frame = frame;
        layout_ptr->layout_children();
    }
};

class OutputPane : public Widget {
public:
    // Use deque to hold a stable reference.
    std::deque<TextEntry> entries;
    ViewPort viewport;
    Prompt prompt;
    Scrollbar* scrollbar{nullptr};
    int scroll_offset { 0 };
    SDL_Rect content_frame;
    int scrollback_;
    int total_lines { 0 };
    bool depressed { false };
    TextSelection text_selection;
    TextFinder text_finder;
    ISlot* mouse_motion_slot { nullptr };

    explicit OutputPane(Widget* parent, Font* font)
        : Widget(parent, font)
        , prompt(this)
    {
        set_scrollback(props().get(property::OUTPUT_SCROLLBACK).value_or(1000));

        make_connections();
    }

    void make_connections()
    {
        connect<CommandSubmittedEvent>(&prompt, [this](auto& e)
        {
            log_input(e.command);
            return false;
        });

        connect<InputSubmittedEvent>(&prompt, [this](auto& e)
        {
            log_input(e.text);
            return true;
        });

        connect<FontSizeChangeRequestedEvent>(nullptr, [this](auto& e)
        {
            std::cerr << "font size: w:" << font->char_width() << ", h:" << font->line_height() << "\n";
            if (e.delta > 0)
                font->atlas().enlarge();
            else if (e.delta < 0)
                font->atlas().shrink();
            resize(frame);
            return false;;
        });

        connect<FindTextQueryEvent>(&window, [this](auto& e)
        {
            text_finder.find(entries, e.query);
            return true;
        });

        connect<FindTextMoveEvent>(&window, [this](auto& e) {
            const TextFinder::LineKey* lk = e.direction == ScrollAction::Up ? text_finder.next_line()
                                                                            : text_finder.prev_line();

            if (!lk)
                return true;

            int target_line = 0;
            for (auto& entry : entries) {
                if (entry.id == lk->entry_id) {
                    target_line += lk->frag_index;
                    break;
                }
                target_line += int(entry.size());
            }

            int scroll = target_line - (rows() / 2);
            set_scroll_offset(scroll);

            return true;
        });

        connect<SDLMouseButtonDownEvent>(this, [this](auto& e) {
            on_mouse_button_down(e.button);
            return true;
        });

        connect<SDLMouseButtonUpEvent>(this, [this](auto& e) {
            on_mouse_button_up(e.button);
            return true;
        });

        connect<SDLMouseWheelEvent>(&window, [this](auto& e) {
            scroll(e.wheel.y);
            return true;
        });

        connect<PasteEvent>(this, [this](auto& e) {
            prompt.insert_at_cursor(e.text);
            return true;
        });

        connect<InputFocusChangedEvent>(this, [this](auto& e) {
            prompt.cursor.style = e.has_focus ? Cursor::DrawStyle::Fill : Cursor::DrawStyle::Outline;
            return true;
        });

        mouse_motion_slot = context.event_bus.connect_later<SDLMouseMotionEvent>(&window, this, [this](auto& e) {
            if (depressed) {
                end_text_selection(map_to({ e.motion.x, e.motion.y }, content_frame));

                if (e.motion.y > this->frame.h) {
                    scroll(-1);
                } else if (e.motion.y < 0) {
                    scroll(1);
                }

                return true;
            }
            return false;
        });

        connect<SDLKeyDownEvent>(this, [this](auto& e) {
            prompt.emit(e);
            on_key_down(e.key);
            return true;
        });

        connect<SDLTextInputEvent>(this, [this](auto& e) {
            prompt.emit(e);
            // When inputting into the prompt, we should keep anchored to the
            // bottom so the prompt is visible.
            // We also need to adjust the scrollbar range as the prompt may span
            // multiple lines.
            // TODO: maybe it should connect to prompt for this.
            set_scroll_offset(0);
            if (scrollbar)
                scrollbar->set_content_size(total_lines + prompt.entry.size());

            return true;
        });
    }

    void set_scrollback(size_t scrollback) noexcept
    {
        scrollback_ = scrollback;
    }

    void apply_scrollbar(Scrollbar *s)
    {
        scrollbar = s;

        scrollbar->set_page_size(rows());
        scrollbar->set_content_size(1); // account for prompt

        connect<ValueChangedEvent<int>>(scrollbar, [this](auto& e) {
            set_scroll_offset_from_scrollbar(e.value);
            return true;
        });
    }

    int on_key_down(const SDL_KeyboardEvent& e)
    {
        auto sym = e.keysym.sym;
        switch (sym) {
        case SDLK_TAB: // FIXME: belongs in prompt
        {
            std::thread cmdhelper([input = text::to_utf8(*prompt.input)]() {
                auto& core = DFHack::Core::getInstance();
                auto& con = SDLConsole::get_console();
                std::vector<std::string> possibles;

                core.getAutoCompletePossibles(input, possibles);
                if (possibles.empty()) {
                    return;
                }

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
        } break;
        /* copy */
        case SDLK_c: // FIXME: belongs in prompt
            if (sdl_console::SDL_GetModState() & KMOD_CTRL) {
                copy_selected_text_to_clipboard();
            }
            break;

        /* paste */
        case SDLK_v: // FIXME: belongs in prompt
            if (sdl_console::SDL_GetModState() & KMOD_CTRL) {
                String text = clipboard::get_text();
                if (text.size())
                    prompt.insert_at_cursor(text);
            }
            break;

        case SDLK_PAGEUP:
            scroll(ScrollAction::PageUp);
            break;

        case SDLK_PAGEDOWN:
            scroll(ScrollAction::PageDown);
            break;

        case SDLK_RETURN:
        case SDLK_BACKSPACE:
        case SDLK_UP:
        case SDLK_DOWN:
        case SDLK_LEFT:
        case SDLK_RIGHT:
            set_scroll_offset(0);
            break;
        default:
            break;
        }
        return 0;
    }

    void on_mouse_button_down(const SDL_MouseButtonEvent& e)
    {
        if (e.button != SDL_BUTTON_LEFT) {
            return;
        }

        SDL_Point point = map_to({e.x, e.y}, content_frame);

        if (e.clicks == 1) {
            begin_text_selection(point);
        } else if (e.clicks == 2) {
            const auto* frag = find_fragment_at_y(point.y);
            if (frag) {
                auto wordpos = text::find_run(String(frag->text), grid::column(point.x, font->char_width()));

                auto get_x = [this](String::size_type pos, int fallback) -> int {
                    return (pos != String::npos) ? grid::columns_to_pixels(pos, font->char_width())
                                                 : fallback;
                };

                begin_text_selection({get_x(wordpos.first, 0), point.y});
                // wordpos is 0-based
                end_text_selection({get_x(wordpos.second+1, content_frame.w), point.y});
            }
        } else if (e.clicks == 3) {
            begin_text_selection({0, point.y});
            end_text_selection({content_frame.w, point.y});
        }

        depressed = true;
        mouse_motion_slot->connect();
    }

    void on_mouse_button_up(auto& /*e*/)
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
        total_lines = 0;
        set_scroll_offset(0);
        if (scrollbar)
            scrollbar->set_content_size(1);
        text_selection.reset();
        emit_text_selection_changed();
        viewport.rebuild = true;
    }

    void set_scroll_offset(int v)
    {
        int max = std::max(0, total_lines + int(prompt.entry.size()) - rows());
        scroll_offset = std::clamp(v, 0, max);

        if (scrollbar)
            scrollbar->scroll_to(scroll_offset);
        viewport.rebuild = true;
    }

    void set_scroll_offset_from_scrollbar(int v) noexcept
    {
        scroll_offset = v;
        viewport.rebuild = true;
    }

    void begin_text_selection(const SDL_Point p)
    {
        text_selection.reset();
        mark_text_selection(p, text_selection.begin);
    }

    void end_text_selection(const SDL_Point p)
    {
        mark_text_selection(p, text_selection.end);
    }

    void mark_text_selection(const SDL_Point p, TextSelection::Anchor& anchor)
    {
        const auto* vr = viewport.find_row_at_y(*font, p.y);
        if (!vr) return;
        auto col = grid::column(p.x, font->char_width());

        anchor = {
            .entry_id = vr->entry_id,
            .frag_index = vr->frag.index,
            .column = col
        };

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
            scroll(ScrollAction::Up);
        } else if (y < 0) {
            scroll(ScrollAction::Down);
        }
    }

    void scroll(ScrollAction sa)
    {
        int step = 0;
        switch (sa) {
        case ScrollAction::Up:
            step = 1;
            break;
        case ScrollAction::Down:
            step = -1;
            break;
        case ScrollAction::PageUp:
            step = rows();
            break;
        case ScrollAction::PageDown:
            step = -rows();
            break;
        }

        set_scroll_offset(scroll_offset + step);
    }

    void resize(const SDL_Rect& rect) override
    {
        frame = rect;

        align_content_rect();
        prompt.resize(content_frame);

        total_lines = 0;
        for (auto& e : entries) {
            wrap_text(e);
        }

        viewport.rebuild = true;

        context.props.set(property::RT_OUTPUT_COLUMNS, columns());
        context.props.set(property::RT_OUTPUT_ROWS, rows());

        if (scrollbar)
            scrollbar->set_page_size(rows());

        if (text_finder.is_active())
            text_finder.find(entries, String(text_finder.needle));
    }

    /*
    * Adjust content_rect dimensions to align with font properties.
    * For character alignment consistency, content_rect must be divisible
    * into rows and columns that match the font's fixed character dimensions.
    */
    void align_content_rect() noexcept
    {
        content_frame = frame;

        content_frame.w = grid::snap_to_column(content_frame.w, font->char_width());
        content_frame.h = grid::snap_to_column(content_frame.h, font->line_height_with_spacing());

        layout::ensure_frame(content_frame);
    }

    void log_output(const StringView text, std::optional<SDL_Color> color)
    {
        create_entry(TextEntryType::Output, text, color);
    }

    void log_input(const StringView text)
    {
        // Concat because the prompt can change
        auto both = prompt.prompt_text;
        both.append(text);
        create_entry(TextEntryType::Input, both, std::nullopt);
    }

    void wrap_text(
        TextEntry& entry)
    {
        entry.wrap_text(font->char_width(), content_frame.w);
        total_lines += entry.size();
        if (scrollbar)
            scrollbar->set_content_size(total_lines + prompt.entry.size());
        viewport.rebuild = true;
    }

    /*
    * Create a new entry which may span multiple rows and set it to be the head.
    * This function will automatically cycle-out entries when the number of rows
    * has reached the max specified by scrollbac_.
    */
    TextEntry& create_entry(const TextEntryType entry_type,
                            const StringView text, std::optional<SDL_Color> color)
    {
        static size_t entry_id = 1; // 0 reserved for prompt

        TextEntry& entry = entries.emplace_front(entry_type, ++entry_id, String(text), color);

        /* When the list is too long, start chopping */
        if (total_lines > scrollback_) {
            total_lines -= entries.back().size();
            entries.pop_back();
            // FIXME: Cleanup anything that points to this
        }
        wrap_text(entry);
        return entry;
    }

    const TextEntry::Fragment* find_fragment_at_y(int y) const noexcept
    {
        auto it = std::ranges::find_if(viewport.rows, [this, y](ViewPortRow const& r) {
            return geometry::is_y_within(y, r.coord.y, font->line_height());
        });

        if (it != viewport.rows.end()) {
            return &it->frag;
        }

        return nullptr;
    }

    void copy_selected_text_to_clipboard()
    {
        String sep{U"\n"};
        String clipboard_text;

        auto vec = text_selection.get_selected_text(entries);
        auto pvec = text_selection.get_selected_text_from_one(prompt.entry);

        std::ranges::move(pvec, std::back_inserter(vec));

        if (vec.empty())
            return;

        clipboard_text = std::accumulate(std::next(vec.begin()), vec.end(),
                        String(vec[0]),
                        [&](String acc, StringView s) {
                            acc.append(sep);
                            acc.append(s);
                            return acc;
                        });

        sdl_console::SDL_SetClipboardText(text::to_utf8(clipboard_text).c_str());
    }

    int columns() const noexcept
    {
        assert(font->char_width() > 0);
        return content_frame.w / font->char_width();
    }

    int rows() const noexcept
    {
        assert(font->line_height_with_spacing() > 0);
        return content_frame.h / font->line_height_with_spacing();
    }

    void render() override
    {
        // Clip and localize coordinates to content_rect.
        sdl_console::SDL_RenderSetViewport(renderer(), &content_frame);

        // TODO: make sure renderer supports blending else highlighting
        // will make the text invisible. Should always be the case.
        render_selected_text_highlights();

        render_text_finder_highlights();

        render_viewport();

        prompt.render_cursor();
        sdl_console::SDL_RenderSetViewport(renderer(), &window.frame);
    }

    void build_viewport()
    {
        viewport.clear();
        viewport.rebuild = false;
        prompt.rebuild = false;
        prompt.cursor.rebuild = false;

        const int max_row = rows() + scroll_offset;
        int ypos = content_frame.h; // Start from the bottom
        int row_counter = 0;

        std::vector<ViewPortRow> rows;

        auto prompt_lines = get_viewport_rows(prompt.entry, ypos, row_counter, max_row);
        prompt.update_cursor_geometry(prompt_lines);
        std::ranges::move(prompt_lines, std::back_inserter(viewport.rows));

        for (auto& entry : entries) {
            if (row_counter > max_row) {
                break;
            }

            auto more = get_viewport_rows(entry, ypos, row_counter, max_row);
            std::ranges::move(more, std::back_inserter(viewport.rows));
        }
    }

    std::vector<ViewPortRow> get_viewport_rows(const TextEntry& entry, int& ypos, int& row_counter, int max_row)
    {
        std::vector<ViewPortRow> vrows;

        for (const auto& line : entry.fragments() | std::views::reverse) {
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
                            .gpv = font->atlas().get_glyph_layout(line.text, 0, ypos),
                            .color = entry.color_opt});
        }
        return vrows;
    }

    void render_viewport()
    {
        if (viewport.rebuild || prompt.rebuild_needed()) {
            build_viewport();
        }

        for (const auto& row : viewport.rows) {
            auto sc = font->set_color(row.color);
            font->atlas().render(row.gpv);
        }

        prompt.render_cursor();
    }

    void render_selected_text_highlights()
    {
        if (!text_selection.active()) return;

        auto rects = text_selection.to_rects(font, viewport.rows);

        set_draw_color(renderer(), colors::mediumgray);
        sdl_console::SDL_RenderFillRects(renderer(), rects.data(), rects.size());
        //set_draw_color(renderer(), colors::darkgray);
    }

    void render_text_finder_highlights()
    {
        if (text_finder.empty()) return;

        set_draw_color(renderer(), {0, 0, 255, 200});

        auto lr = text_finder.active_rect(font, viewport.rows);
        sdl_console::SDL_RenderFillRect(renderer(), &lr);

        set_draw_color(renderer(), SDL_Color{50, 100, 200, 200});
        auto fr = text_finder.to_rects(font, viewport.rows);
        sdl_console::SDL_RenderFillRects(renderer(), fr.data(), fr.size());
        //set_draw_color(renderer(), colors::darkgray);
    }

    OutputPane(const OutputPane&) = delete;
    OutputPane& operator=(const OutputPane&) = delete;
};


class MainWindow : public Widget {
public:
    std::unique_ptr<VBox> layout;
    OutputPane* outpane { nullptr };
    std::vector<std::unique_ptr<Widget>> overlays;
    bool has_focus { false };
    bool is_shown { true };
    bool is_minimized { false };

    TicksT frame_refresh_rate{1000/20};

    explicit MainWindow(WidgetContext& ctx)
        : Widget(ctx)
    {
        sdl_console::SDL_GetRendererOutputSize(renderer(), &frame.w, &frame.h);
        // If either of these are true, something very bad happened with SDL.
        // Possible reasons: broken renderer, platform/sdl bugs.
        if (frame.w <= 0 || frame.h <= 0) {
            fatal<Error::SDL>("SDL_GetRendererOutputSize() returned invalid size");
        }

        font = ctx.font_loader.load_bitmap("default", "data/art/curses_640x300.png", 8);
        if (!font) {
            throw(std::runtime_error("Error loading font"));
        }

        //font->set_fallback(ctx.font_loader.load_truetype("default_atlas", "DejaVuSansMono.ttf", 16));

        set_frame_refresh_rate();
        layout = std::make_unique<VBox>(this);

        connect<SDLMouseMotionEvent>(this, [this](auto& e) {
            int x = e.motion.x;
            int y = e.motion.y;

            Widget* prev_hover = context.widget.hovered;
            Widget* new_hover  = nullptr;

            if (!overlays.empty())
                new_hover = find_widget_at(overlays.back().get(), x, y);

            if (!new_hover)
                new_hover = find_widget_at(layout.get(), x, y);

            if (new_hover == prev_hover)
                return false;

            context.widget.hovered = new_hover;
            return false;
        });


        connect<SDLKeyDownEvent>(this, [this](auto& e) {
            if (auto* input = context.widget.input) {
                input->emit(e);
                return false;
            }

            if (auto* focused = context.widget.focused) {
                // Try to locate a widget that accepts keyboard input.
                // dispatch_event() returns false when top level widgets
                // like the toolbar or scrollbar have focus.
                if (dispatch_event(focused, e))
                    return false;
            }

            outpane->emit(e);
            return false;
        });

        connect<SDLMouseButtonDownEvent>(this, [this](auto& e) {
            Widget* target{nullptr};

            if (!overlays.empty()) {
                target = find_widget_at(overlays.back().get(), e.button.x, e.button.y);
            }

            if (!target) {
                target = find_widget_at(layout.get(), e.button.x, e.button.y);
            }

            // No takers. set to default.
            if (!target)
                target = outpane;

            context.widget.focused = target;
            if (context.event_bus.has_connection(SDLTextInputEvent::type, target)) {
                target->take_input_focus();
            }
            target->emit(e);
            return false;
        });

        connect<SDLMouseButtonUpEvent>(this, [this](auto& e) {
            if (auto* focused = context.widget.focused)
                focused->emit(e);

            return false;
        });

        connect<SDLTextInputEvent>(this, [this](auto& e) {
            if (auto* input = context.widget.input)
                input->emit(e);

            return true;
        });

        connect<WidgetDestroyedEvent>(nullptr, [this](auto& e) {
            if (e.addr == outpane)
                return true;

            if (outpane == nullptr)
                return true;

            if (nullptr == context.widget.input) {
                outpane->take_input_focus();
            }

            if (nullptr == context.widget.focused)
                context.widget.focused = outpane;

            return true;
        });

        build_ui();

        context.widget.input = outpane;
        context.widget.focused = outpane;
    }

    void dispatch_sdl_event(const SDL_Event& e)
    {
        switch (e.type) {
        case SDL_MOUSEMOTION:
            emit(SDLMouseMotionEvent { .motion = e.motion });
            break;
        case SDL_MOUSEBUTTONDOWN:
            emit(SDLMouseButtonDownEvent { .button = e.button });
            break;
        case SDL_MOUSEBUTTONUP:
            emit(SDLMouseButtonUpEvent { .button = e.button });
            break;
        case SDL_MOUSEWHEEL:
            emit(SDLMouseWheelEvent { .wheel = e.wheel });
            break;
        case SDL_WINDOWEVENT:
            on_sdl_window_event(e.window);
            break;
        case SDL_TEXTINPUT:
            emit(SDLTextInputEvent { .text = e.text });
            break;
        case SDL_KEYDOWN:
            emit(SDLKeyDownEvent { .key = e.key });
            break;
        default:;
        }
    }

    void on_sdl_window_event(const SDL_WindowEvent& e)
    {
        switch (e.event) {
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
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            is_shown = false;
            break;
        case SDL_WINDOWEVENT_SHOWN:
        case SDL_WINDOWEVENT_EXPOSED:
        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
            is_shown = true;
            is_minimized = false;
            break;
        default:;
        }
    }

    ~MainWindow() override
    {
        context.event_bus.clear();
    }

    template <typename EventType>
    bool dispatch_event(Widget* start, EventType& e) {
        if (!start)
            return false;

        auto handled = false;
        // Infinite recursion without w != this
        for (Widget* w = start; w && w != this; w = w->parent) {
            if (w->emit(e)) {
                handled = true;
                break;
            }
        }
        return handled;
    }

    void build_ui()
    {
        auto& toolbar_hbox = layout->add_child<HBox>();
        toolbar_hbox.layout_spec.size.h = font->line_height_with_spacing() * 2;
        toolbar_hbox.layout_spec.stretch = 0;

        auto& toolbar = toolbar_hbox.add_child<Toolbar>();
        toolbar.layout_spec.stretch = 1;

        auto& outbox = layout->add_child<HBox>();
        outbox.layout_spec.stretch = 1;
        outbox.layout_spec.margin = {4, 4, 4, 4};

        //auto out_font = context.font_loader.load_truetype("default_atlas", "DejaVuSansMono.ttf", 16);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                vbZ
        auto out_font = context.font_loader.load_bitmap("default_atlas", "data/art/curses_640x300.png", 8);
        //out_font->set_fallback(context.font_loader.load_bitmap("default", "data/art/curses_640x300.png", 8));

        auto& output = outbox.add_child<OutputPane>(out_font);
        outpane = &output;
        output.layout_spec.stretch = 1;
        output.layout_spec.margin = {4, 4, 4, 4};

        auto& scrollbar = outbox.add_child<Scrollbar>();
        scrollbar.layout_spec.size.w = 16;

        resize({0, 0, frame.w, frame.h});

        output.apply_scrollbar(&scrollbar); // Size of output area needs known before applying scrollbar

        Button& copy = *toolbar.add_button(U"Copy");
        copy.enabled = false;
        connect<ClickedEvent>(&copy, [&output](auto& /*e*/) {
            output.copy_selected_text_to_clipboard();
            return true;
        });

        connect<TextSelectionChangedEvent>(&output, [&copy](auto& e) {
            copy.enabled = e.selected;
            return true;
        });

        Button& paste = *toolbar.add_button(U"Paste");
        connect<ClickedEvent>(&paste, [this](auto& /*e*/) {
            String text = clipboard::get_text();

            if (text.size() && context.widget.input)
                context.widget.input->emit(PasteEvent{ .text = text });
            return true;
        });

        Button& find = *toolbar.add_button(U"Find");
        connect<ClickedEvent>(&find, [this, &output](auto& /*e*/) {
            static String saved_needle;

            if (has_overlay<TextFinderWidget>())
                return true;

            auto find_widget = std::make_unique<TextFinderWidget>(&output, window.font);
            find_widget->resize({});
            find_widget->input->take_input_focus();
            find_widget->input->set_text(saved_needle);

            find_widget->on_close = [this, &output](TextFinderWidget* fw) {
                saved_needle = fw->input->get_text();
                output.text_finder.clear();
                std::erase_if(overlays, [fw](const auto& o) {
                    return o.get() == fw;
                });
            };

            overlays.push_back(std::move(find_widget));
            return true;
        });

        Button& enlarge_font = *toolbar.add_button(U"A+");
        connect<ClickedEvent>(&enlarge_font, [&output](auto& /*e*/) {
            output.emit(FontSizeChangeRequestedEvent { .delta = 1 });
            return true;
        });

        Button& shrink_font = *toolbar.add_button(U"A-");
        connect<ClickedEvent>(&shrink_font, [&output](auto& /*e*/) {
            output.emit(FontSizeChangeRequestedEvent { .delta = -1 });
            return true;
        });
    }

    void set_frame_refresh_rate()
    {
        constexpr TicksT fps_shown = 20;
        constexpr TicksT fps_minimized = 1;

        constexpr TicksT shown_ms = 1000 / fps_shown;
        constexpr TicksT minimized_ms = 1000 / fps_minimized;

        frame_refresh_rate = is_minimized ? minimized_ms : shown_ms;
    }

    void render() override
    {
        if (!should_render()) return;

        // Should not fail unless OOM.
        sdl_console::SDL_RenderClear(renderer());

        for (auto& child : layout->children) {
            child->render();
        }

        for (auto& w : overlays) {
            w->render();
        }

        set_draw_color(renderer(), colors::darkgray);

        sdl_console::SDL_RenderPresent(renderer());
    }

    // Rate limit rendering if needed
    bool should_render() const
    {
        static auto last_render_tick = SDL_GetTicks64();

        if (!is_shown) {
            return false;
        }

        auto current_tick = context.now_tick;
        auto rate = frame_refresh_rate;
        if (current_tick - last_render_tick >= rate) {
            last_render_tick = current_tick;
            return true;
        }

        return false;
    }

    void resize(const SDL_Rect& /*r*/) override
    {
        sdl_console::SDL_GetRendererOutputSize(renderer(), &frame.w, &frame.h);
        layout::ensure_frame(frame);
        layout->frame = frame;
        layout->layout_children();

        for (auto& w : overlays) {
            // Give overlays with parents such as the FindWidget
            // the same size as their parent
            if (w->parent)
                w->resize(w->parent->frame);
        }
    }

    // NOTE: If more windows are ever added, this needs revisited.
    static SDLWindowResources create_window(Property& props)
    {
        // Inform SDL to pass the mouse click event when switching between windows.
        SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

        auto title = props.get(property::WINDOW_MAIN_TITLE).value_or("DFHack Console");
        Rect create_rect = props.get(property::WINDOW_MAIN_RECT).value_or(
            Rect{SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480});
        //auto flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
        auto flags = SDL_WINDOW_RESIZABLE;

        auto window = make_window(sdl_console::SDL_CreateWindow(title.c_str(),
                                                                create_rect.x, create_rect.y,
                                                                create_rect.w, create_rect.h,
                                                                flags));
        if (!window) {
            fatal<Error::SDL>("Failed to create window");
        }

        sdl_console::SDL_SetWindowMinimumSize(window.get(), 64, 48);

        auto id = sdl_console::SDL_GetWindowID(window.get());
        if (id == 0)
            fatal<Error::SDL>("Failed to get window ID");

        auto rend = make_renderer(create_renderer(props, window.get()));
        return SDLWindowResources(std::move(window), std::move(rend), id);
    }

    static SDL_Renderer* create_renderer(Property& props, SDL_Window* handle)
    {
        sdl_console::SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
        sdl_console::SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
        // Flags 0 instructs SDL to choose the default backend for the
        // host system. TODO: add config to force software rendering
        auto rflags = (SDL_RendererFlags)0;
        //SDL_RendererFlags rflags = (SDL_RendererFlags)SDL_RENDERER_SOFTWARE;
        SDL_Renderer* rend = sdl_console::SDL_CreateRenderer(handle, -1, rflags);
        if (!rend)
            fatal<Error::SDL>("Failed to create renderer");
        SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
        sdl_console::SDL_RenderSetIntegerScale(rend, SDL_TRUE);
        return rend;
    }

    bool has_overlay_of_type(const std::type_info& ti) const {
        return std::ranges::any_of(overlays, [&](const auto& o) {
            return typeid(*o) == ti;
        });
    }

    template <typename T>
    bool has_overlay() const {
        return has_overlay_of_type(typeid(T));
    }

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
};

Toolbar::Toolbar(Widget* parent)
    : Widget(parent)
{
};

void Toolbar::render()
{
    set_draw_color(renderer(), colors::gold);
    sdl_console::SDL_RenderDrawRect(renderer(), &frame);
    // Lay out horizontally
    for (auto& w : children) {
        w->render();
    }

    //set_draw_color(renderer(), colors::darkgray);
}

void Toolbar::resize(const SDL_Rect& rect)
{
    frame = rect;
    layout_buttons();
}

Button* Toolbar::add_button(const String& text)
{
    auto button = std::make_unique<Button>(this, text);
    Button& b = *button.get();
    b.frame.y = frame.y;
    auto sz = b.preferred_size();
    b.frame.w = sz.w;
    b.frame.h = sz.h;
    children.emplace_back(std::move(button));
    layout_buttons();
    return &b;
}

void Toolbar::layout_buttons()
{
    int x = frame.w - compute_widgets_startx();

    for (auto& w : children) {
        w->frame.x = x;
        w->resize(w->frame);
        x += w->frame.w;
    }
}

int Toolbar::compute_widgets_startx()
{
    int x = 0;
    for (auto& w : children) {
        x += w->frame.w;
    }
    return x;
}

/*
* A centralized queue system intended for thread-safe handling
* of external events.
*
* Previously, this class supported two distinct queues: one for SDL events and
* another for API task events. However, it now only stores API tasks.
*
*/
class ExternalEventQueue {
    template <typename T>
    class Queue {

    public:
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

class SDLConsole_session : public std::enable_shared_from_this<SDLConsole_session> {
public:
    // Order matters.
    // SDL_Renderer and SDL_Window must be destroyed after all other sdl resources SDL_Texture, etc
    SDLWindowResources sdl_window;

    EventBus event_bus;
    CommandPipe command_pipe;
    ExternalEventQueue external_event_queue;

    WidgetContext widget_context;
    MainWindow main_window;
    bool shutdown_requested { false };

    explicit SDLConsole_session(Property& props)
    : sdl_window(MainWindow::create_window(props))
    , widget_context(props, event_bus, sdl_window)
    , main_window(widget_context)
    {
        command_pipe.make_connection(event_bus, &outpane().prompt);
    }

    OutputPane& outpane() const noexcept {
        return *main_window.outpane;
    }

    bool sdl_event_hook(const SDL_Event& e)
    {
        // NOTE: Having focus is not enough to determine handling.
        // This window ID check is important for df to update its state
        // when moving between windows.
        if (e.type == SDL_WINDOWEVENT && e.window.windowID != sdl_window.id) {
            return false;
        }

        if (e.type == SDL_WINDOWEVENT || main_window.has_focus) {
            main_window.dispatch_sdl_event(e);
            return true;
        }

        return false;
    }

    void update()
    {
        // TODO: move to main_window?
        widget_context.now_tick = sdl_console::SDL_GetTicks64();

        handle_tasks();

        main_window.render();
    }

    void shutdown()
    {
        shutdown_requested = true;
        command_pipe.shutdown();
    }

    ~SDLConsole_session() = default;

private:
    void handle_tasks()
    {
        for (auto& t : external_event_queue.api_task.drain()) {
            t();
        }
    }
};

SDLConsole::SDLConsole() = default;

SDLConsole::~SDLConsole() = default;

bool SDLConsole::init_session()
{
    if (impl)
        return true;
    bool success = true;
    //std::cerr << "SDLConsole: init() from thread: " << std::this_thread::get_id() << '\n';
    init_thread_id = std::this_thread::get_id();
    try {
        bind_sdl_symbols();
        impl = std::make_shared<SDLConsole_session>(props);
        impl_weak = impl;
        was_shutdown_.store(false);
    } catch(std::runtime_error &e) {
        success = false;
        impl.reset();
        was_shutdown_.store(true);
        log_error<Error::Internal>("Caught exception", e.what());
    }
    return success;
}

/* Line may be wrapped */
void SDLConsole::write_line(const std::string_view line, SDL_Color color)
{
    write_line_(line, color);
}

void SDLConsole::write_line(const std::string_view line)
{
    write_line_(line, std::nullopt);
}

void SDLConsole::write_line_(const std::string_view line, std::optional<SDL_Color> color)
{
    auto l = text::from_utf8(line);
    push_api_task([this, color, l = std::move(l)] {
        impl->outpane().log_output(l, color);
    });
}

int SDLConsole::get_columns()
{
    return props.get(property::RT_OUTPUT_COLUMNS).value_or(-1);
}

int SDLConsole::get_rows()
{
    return props.get(property::RT_OUTPUT_ROWS).value_or(-1);
}

SDLConsole& SDLConsole::set_mainwindow_create_rect(int w, int h, int x, int y)
{
    Rect rect{.x = x, .y = y, .w = w, .h = h};
    props.set(property::WINDOW_MAIN_RECT, rect);
    return *this;
}

SDLConsole& SDLConsole::set_scrollback(int scrollback) {
    props.set(property::OUTPUT_SCROLLBACK, scrollback);
    push_api_task([this, scrollback] {
        impl->outpane().set_scrollback(scrollback);
    });
    return *this;
}

SDLConsole& SDLConsole::set_prompt(const std::string_view text)
{
    auto t = text::from_utf8(text);
    props.set(property::PROMPT_TEXT, t);
    push_api_task([this, t = std::move(t)] {
        impl->outpane().prompt.set_prompt_text(t);
    });
    return *this;
}

std::string SDLConsole::get_prompt()
{
    return text::to_utf8(props.get(property::PROMPT_TEXT).value_or(U"> "));
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
        sdl_console::SDL_ShowWindow(impl->sdl_window.window.get());
    });
}

void SDLConsole::hide_window() {
    push_api_task([this] {
        sdl_console::SDL_HideWindow(impl->sdl_window.window.get());
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
    if (auto I = impl_weak.lock()) {
        return I->command_pipe.wait_get(buf);
    }
    return 0;
}

void SDLConsole::set_command_history(std::span<const std::string> entries)
{
    if (entries.empty())
        return;

    std::deque<String> my_entries;
    for (const auto& entry : entries) {
        my_entries.push_front(text::from_utf8(entry));
    }
    push_api_task([this, my_entries = std::move(my_entries)] {
        impl->outpane().prompt.set_command_history(my_entries);
    });
}

SDLConsole& SDLConsole::get_console() noexcept
{
    static SDLConsole instance;
    return instance;
}

void SDLConsole::shutdown_session() noexcept
{
    if (auto I = std::weak_ptr<SDLConsole_session>(impl).lock()) {
        I->shutdown();
    }
}

bool SDLConsole::destroy_session() noexcept
{
    if (!impl) {
        return true;
    }

    //if (on_destroy_session)
    //    on_destroy_session();

    if (init_thread_id != std::this_thread::get_id()) {
        std::ostringstream oss;
        oss << "destroy() called from non-init thread "
        << std::this_thread::get_id() << " (expected " << init_thread_id << ")";

        log_error<Error::Internal>(oss.str());
        return false;
    }

    // This should have already been called,
    // But do it again. It's absolutely critical as get_line() must be notified.
    impl->shutdown();

    // Kill our impl shared_ptr.
    impl.reset();
    was_shutdown_.store(true);

    // NOTE: The only other long living impl shared_ptr is get_line()
    // which runs on a separate thread (fiothread) and is commanded to close when shutdown() is called.
    // Purposefuly not using is_active() here.

    // Do we need to spin wait for it to finish?
    // Only seems necessary if we intend to start a new session immediately.
    while (!impl_weak.expired()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // Back to inactive state.
    return true;
}

// NOTE:
// This function must be called by the thread that called init().
// Should probably always be the case, but still may need a guard as done in destroy().
bool SDLConsole::sdl_event_hook(const SDL_Event &e)
{
    if (impl) {
        if (impl->shutdown_requested) [[unlikely]] {
            destroy_session();
            return false;
        }

        return impl->sdl_event_hook(e);
    }
    return false;
}

void SDLConsole::update()
{
    if (impl) [[likely]] {
        impl->update();
    }
}

// Called by other threads.
template<typename F>
void SDLConsole::push_api_task(F&& func)
{
    static_assert(std::is_invocable_v<F>, "Callable must be invocable");

    /* We could use SDL_PushEvent() here. However, we may need to allocate
    * memory, which could result in memory leaks if the event is not received
    * for cleanup.
    */
    if (auto I = impl_weak.lock()) {
        I->external_event_queue.api_task.push(std::forward<F>(func));
    }
}

bool SDLConsole::is_active()
{
    return !impl_weak.expired();
}

} // end namespace sdl_console
} // DFHack

// kate: replace-tabs on; indent-width 4;
