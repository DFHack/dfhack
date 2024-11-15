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
#include <stdbool.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>
#include <ranges>
#include <condition_variable>
#include <cmath>
#include <cwctype>
//#include <cuchar>

#include "modules/DFSDL.h"
#include "SDL_console.h"

using namespace DFHack;

namespace sdl_console {

// These macros to be removed.
#define CONSOLE_SYMBOL_ADDR(sym) ::sym
#define CONSOLE_DECLARE_SYMBOL(sym) decltype(sym)* sym = CONSOLE_SYMBOL_ADDR(sym)

CONSOLE_DECLARE_SYMBOL(SDL_CaptureMouse);
CONSOLE_DECLARE_SYMBOL(SDL_ConvertSurfaceFormat);
CONSOLE_DECLARE_SYMBOL(SDL_CreateRenderer);
CONSOLE_DECLARE_SYMBOL(SDL_CreateRGBSurface);
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
CONSOLE_DECLARE_SYMBOL(SDL_HideWindow);
CONSOLE_DECLARE_SYMBOL(SDL_iconv_string);
CONSOLE_DECLARE_SYMBOL(SDL_InitSubSystem);
CONSOLE_DECLARE_SYMBOL(SDL_MapRGB);
CONSOLE_DECLARE_SYMBOL(SDL_memset);
CONSOLE_DECLARE_SYMBOL(SDL_RenderClear);
CONSOLE_DECLARE_SYMBOL(SDL_RenderCopy);
CONSOLE_DECLARE_SYMBOL(SDL_RenderDrawRect);
CONSOLE_DECLARE_SYMBOL(SDL_RenderFillRect);
CONSOLE_DECLARE_SYMBOL(SDL_RenderPresent);
CONSOLE_DECLARE_SYMBOL(SDL_RenderSetIntegerScale);
CONSOLE_DECLARE_SYMBOL(SDL_RenderSetViewport);
CONSOLE_DECLARE_SYMBOL(SDL_PointInRect);
CONSOLE_DECLARE_SYMBOL(SDL_SetClipboardText);
CONSOLE_DECLARE_SYMBOL(SDL_SetColorKey);
CONSOLE_DECLARE_SYMBOL(SDL_SetEventFilter);
CONSOLE_DECLARE_SYMBOL(SDL_SetHint);
CONSOLE_DECLARE_SYMBOL(SDL_SetRenderDrawColor);
CONSOLE_DECLARE_SYMBOL(SDL_SetTextureBlendMode);
CONSOLE_DECLARE_SYMBOL(SDL_SetTextureColorMod);
CONSOLE_DECLARE_SYMBOL(SDL_SetWindowMinimumSize);
CONSOLE_DECLARE_SYMBOL(SDL_ShowWindow);
CONSOLE_DECLARE_SYMBOL(SDL_StartTextInput);
CONSOLE_DECLARE_SYMBOL(SDL_StopTextInput);
CONSOLE_DECLARE_SYMBOL(SDL_UpperBlit);
CONSOLE_DECLARE_SYMBOL(SDL_UpdateTexture);
CONSOLE_DECLARE_SYMBOL(SDL_QuitSubSystem);


namespace text {
#if 0
    // From Console-posix
    //! Convert a locale defined multibyte coding to UTF-32 string for easier
    //! character processing.
    // UNUSED
    std::u32string from_locale_mb(const std::string& str)
    {
        std::u32string rv;
        std::u32string::value_type ch;
        size_t pos = 0;
        ssize_t sz;
        std::mbstate_t state{};
        while ((sz = std::mbrtoc32(&ch,&str[pos], str.size() - pos, &state)) != 0) {
            if (sz == -1 || sz == -2)
                break;
            rv.push_back(ch);
            if (sz == -3) /* multi value character */
                continue;
            pos += sz;
        }
        return rv;
    }

    //! Convert a UTF-32 string back to locale defined multibyte coding.
    // UNUSED
    std::string to_locale_mb(const std::u32string& wstr)
    {
        std::stringstream ss{};
        char mb[MB_CUR_MAX];
        std::mbstate_t state{};
        const size_t err = -1;
        for (auto ch: wstr) {
            size_t sz = std::c32rtomb(mb, ch, &state);
            if (sz == err)
                break;
            ss.write(mb, sz);
        }
        return ss.str();
    }
#endif

    std::string to_utf8(const std::u32string& u32_string)
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

    std::u32string from_utf8(std::string u8_string)
    {
        char* conv = SDL_iconv_string("UTF-32LE", "UTF-8",
                                      u8_string.data(),
                                      u8_string.size() + 1);
        if (!conv)
            return U"?u8?";

        std::u32string result(reinterpret_cast<char32_t*>(conv));
        sdl_console::SDL_free(conv);
        return result;
    }

    size_t utf8_strlen(const char* str)
    {
        size_t count = 0;
        size_t i = 0;
        while (str[i]) {
            unsigned char byte = str[i];
            if ((byte & 0x80) == 0) {
                ++i;
            } else if ((byte & 0xE0) == 0xC0) {
                i += 2;
            } else if ((byte & 0xF0) == 0xE0) {
                i += 3;
            } else if ((byte & 0xF8) == 0xF0) {
                i += 4;
            } else {
                // Invalid byte
                ++i;
            }
            ++count;
        }
        return count;
    }

    std::pair<size_t, size_t> find_previous_word(const std::u32string& text, size_t pos) {
        if (pos == 0) return {std::u32string::npos, std::u32string::npos};

        pos--;
        while (pos > 0 && std::iswspace(text[pos])) {
            pos--;
        }
        size_t end = pos + 1;

        while (pos > 0 && !std::iswspace(text[pos - 1])) {
            pos--;
        }

        return {pos, end};
    }

    std::pair<size_t, size_t> find_next_word(const std::u32string& text, size_t pos) {
        while (pos < text.size() && std::iswspace(text[pos])) {
            pos++;
        }
        size_t start = pos;

        while (pos < text.size() && !std::iswspace(text[pos])) {
            pos++;
        }
        while (pos < text.size() && std::iswspace(text[pos])) {
            pos++;
        }

        return {start, pos};
    }

    bool is_newline(char32_t ch) {
        return ch == U'\n' || ch == U'\r';
    }

    bool is_whitespace(char32_t ch) {
        return ch == U' ' || ch == U'\t';
    }
}

namespace geometry {
void center_rect(SDL_Rect& r)
{
    r.x = r.x - r.w / 2;
    r.y = r.y - r.h / 2;
}

bool in_rect(int x, int y, SDL_Rect& r)
{
    return ((x >= r.x) && (x < (r.x + r.w)) && (y >= r.y) && (y < (r.y + r.h)));
}

bool in_rect(SDL_Point& p, SDL_Rect& r)
{
    return sdl_console::SDL_PointInRect(&p, &r);
}

} // geometry

namespace grid {
int floor_boundary(int position, int cell_size)
{
    return std::floor((float)(position) / cell_size) * cell_size;
}

int ceil_boundary(int position, int cell_size)
{
    return std::ceil((float)(position) / cell_size) * cell_size;
}
}

// For testing purposes, to be removed
static const std::unordered_map<char32_t, uint8_t> unicode_to_cp437 = {
    // Control characters and symbols
    /* NULL           */ { U'\u263A', 0x01 }, { U'\u263B', 0x02 }, { U'\u2665', 0x03 },
    { U'\u2666', 0x04 }, { U'\u2663', 0x05 }, { U'\u2660', 0x06 }, { U'\u2022', 0x07 },
    { U'\u25D8', 0x08 }, { U'\u25CB', 0x09 }, { U'\u25D9', 0x0A }, { U'\u2642', 0x0B },
    { U'\u2640', 0x0C }, { U'\u266A', 0x0D }, { U'\u266B', 0x0E }, { U'\u263C', 0x0F },

    { U'\u25BA', 0x10 }, { U'\u25C4', 0x11 }, { U'\u2195', 0x12 }, { U'\u203C', 0x13 },
    { U'\u00B6', 0x14 }, { U'\u00A7', 0x15 }, { U'\u25AC', 0x16 }, { U'\u21A8', 0x17 },
    { U'\u2191', 0x18 }, { U'\u2193', 0x19 }, { U'\u2192', 0x1A }, { U'\u2190', 0x1B },
    { U'\u221F', 0x1C }, { U'\u2194', 0x1D }, { U'\u25B2', 0x1E }, { U'\u25BC', 0x1F },

    // ASCII, no mapping needed

    // Extended Latin characters and others
    { U'\u2302', 0x7F },

    { U'\u00C7', 0x80 }, { U'\u00FC', 0x81 }, { U'\u00E9', 0x82 }, { U'\u00E2', 0x83 },
    { U'\u00E4', 0x84 }, { U'\u00E0', 0x85 }, { U'\u00E5', 0x86 }, { U'\u00E7', 0x87 },
    { U'\u00EA', 0x88 }, { U'\u00EB', 0x89 }, { U'\u00E8', 0x8A }, { U'\u00EF', 0x8B },
    { U'\u00EE', 0x8C }, { U'\u00EC', 0x8D }, { U'\u00C4', 0x8E }, { U'\u00C5', 0x8F },

    { U'\u00C9', 0x90 }, { U'\u00E6', 0x91 }, { U'\u00C6', 0x92 }, { U'\u00F4', 0x93 },
    { U'\u00F6', 0x94 }, { U'\u00F2', 0x95 }, { U'\u00FB', 0x96 }, { U'\u00F9', 0x97 },
    { U'\u00FF', 0x98 }, { U'\u00D6', 0x99 }, { U'\u00DC', 0x9A }, { U'\u00A2', 0x9B },
    { U'\u00A3', 0x9C }, { U'\u00A5', 0x9D }, { U'\u20A7', 0x9E }, { U'\u0192', 0x9F },

    { U'\u00E1', 0xA0 }, { U'\u00ED', 0xA1 }, { U'\u00F3', 0xA2 }, { U'\u00FA', 0xA3 },
    { U'\u00F1', 0xA4 }, { U'\u00D1', 0xA5 }, { U'\u00AA', 0xA6 }, { U'\u00BA', 0xA7 },
    { U'\u00BF', 0xA8 }, { U'\u2310', 0xA9 }, { U'\u00AC', 0xAA }, { U'\u00BD', 0xAB },
    { U'\u00BC', 0xAC }, { U'\u00A1', 0xAD }, { U'\u00AB', 0xAE }, { U'\u00BB', 0xAF },

    // Box drawing characters
    { U'\u2591', 0xB0 }, { U'\u2592', 0xB1 }, { U'\u2593', 0xB2 }, { U'\u2502', 0xB3 },
    { U'\u2524', 0xB4 }, { U'\u2561', 0xB5 }, { U'\u2562', 0xB6 }, { U'\u2556', 0xB7 },
    { U'\u2555', 0xB8 }, { U'\u2563', 0xB9 }, { U'\u2551', 0xBA }, { U'\u2557', 0xBB },
    { U'\u255D', 0xBC }, { U'\u255C', 0xBD }, { U'\u255B', 0xBE }, { U'\u2510', 0xBF },

    { U'\u2514', 0xC0 }, { U'\u2534', 0xC1 }, { U'\u252C', 0xC2 }, { U'\u251C', 0xC3 },
    { U'\u2500', 0xC4 }, { U'\u253C', 0xC5 }, { U'\u255E', 0xC6 }, { U'\u255F', 0xC7 },
    { U'\u255A', 0xC8 }, { U'\u2554', 0xC9 }, { U'\u2569', 0xCA }, { U'\u2566', 0xCB },
    { U'\u2560', 0xCC }, { U'\u2550', 0xCD }, { U'\u256C', 0xCE }, { U'\u2567', 0xCF },

    { U'\u2568', 0xD0 }, { U'\u2564', 0xD1 }, { U'\u2565', 0xD2 }, { U'\u2559', 0xD3 },
    { U'\u2558', 0xD4 }, { U'\u2552', 0xD5 }, { U'\u2553', 0xD6 }, { U'\u256B', 0xD7 },
    { U'\u256A', 0xD8 }, { U'\u2518', 0xD9 }, { U'\u250C', 0xDA }, { U'\u2588', 0xDB },
    { U'\u2584', 0xDC }, { U'\u258C', 0xDD }, { U'\u2590', 0xDE }, { U'\u2580', 0xDF },

    // Mathematical symbols and others
    { U'\u03B1', 0xE0 }, { U'\u00DF', 0xE1 }, { U'\u0393', 0xE2 }, { U'\u03C0', 0xE3 },
    { U'\u03A3', 0xE4 }, { U'\u03C3', 0xE5 }, { U'\u00B5', 0xE6 }, { U'\u03C4', 0xE7 },
    { U'\u03A6', 0xE8 }, { U'\u0398', 0xE9 }, { U'\u03A9', 0xEA }, { U'\u03B4', 0xEB },
    { U'\u221E', 0xEC }, { U'\u03C6', 0xED }, { U'\u03B5', 0xEE }, { U'\u2229', 0xEF },

    { U'\u2261', 0xF0 }, { U'\u00B1', 0xF1 }, { U'\u2265', 0xF2 }, { U'\u2264', 0xF3 },
    { U'\u2320', 0xF4 }, { U'\u2321', 0xF5 }, { U'\u00F7', 0xF6 }, { U'\u2248', 0xF7 },
    { U'\u00B0', 0xF8 }, { U'\u2219', 0xF9 }, { U'\u00B7', 0xFA }, { U'\u221A', 0xFB },
    { U'\u207F', 0xFC }, { U'\u00B2', 0xFD }, { U'\u25A0', 0xFE }, { U'\u00A0', 0xFF }
};

#if 0
class Logger {
public:
    explicit Logger(const std::string& prefix)
    : prefix_(prefix) {}

    void log_error(const std::string& message) {
        log("ERROR", message);
    }

    void log_status(const std::string& message) {
        log("STATUS", message);
    }

    void log_message(const std::string& message) {
        log("MESSAGE", message);
    }

private:
    // Log with a prefix (e.g., ERROR, STATUS) and include the app name
    void log(const std::string& level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex);

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::cerr << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
        << "] "
        << "[" << prefix_ << "] "
        << "[" << level << "] " << message << std::endl;
    }

    std::string prefix_;
    std::mutex mutex;
};
#endif

enum class ScrollDirection {
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
struct InternalEventType {
    enum Type : Uint32 {
        new_input_line = SDL_LASTEVENT + 1,
        clicked,
        font_size_changed,
        range_changed,
        value_changed,
        text_selection_changed,
    };
};

enum class TextEntryType {
    input,
    output
};

namespace colors {
    // Default palette. Needs more. Needs configurable.
    const SDL_Color white = { 255, 255, 255, 255 };
    const SDL_Color lightgray = { 211, 211, 211, 255 };
    const SDL_Color mediumgray = { 65, 65, 65, 255 };
    const SDL_Color charcoal = { 54, 69, 79, 255 };
    const SDL_Color darkgray = { 27, 27, 27, 255 };

    const SDL_Color mauve = { 100,68,84, 255};
    const SDL_Color gold = { 247,193,41, 255};
    const SDL_Color teal = {  94, 173, 146, 255};
}

void render_texture(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const SDL_Rect& dst);

int set_draw_color(SDL_Renderer*, const SDL_Color&);

class SDLThreadSpecificData {
public:
    using Texture = std::unique_ptr<SDL_Texture, decltype(sdl_console::SDL_DestroyTexture)>;
    using Renderer = std::unique_ptr<SDL_Renderer, decltype(sdl_console::SDL_DestroyRenderer)>;
    using Window = std::unique_ptr<SDL_Window, decltype(sdl_console::SDL_DestroyWindow)>;

    SDLThreadSpecificData()
    {
    }

    SDL_Texture* CreateTextureFromSurface(SDL_Renderer* r, SDL_Surface* s)
    {
        auto p = sdl_console::SDL_CreateTextureFromSurface(r, s);
        if (!p) return nullptr;
        textures_.emplace_back(make_unique_texture(p));
        return p;
    }

    SDL_Texture* CreateTexture(SDL_Renderer* r,
                                Uint32 format, int access,
                                int w, int h)
    {
        auto p = sdl_console::SDL_CreateTexture(r, format, access, w, h);
        if (!p) return nullptr;
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
        auto p = sdl_console::SDL_CreateRenderer(handle, index, flags);
        if (!p) return nullptr;
        renderers_.emplace_back(make_unique_renderer(p));
        return p;
    }

    SDL_Window* CreateWindow(const char *title,
                              int x, int y, int w, int h,
                              Uint32 flags)
    {
        auto p = sdl_console::SDL_CreateWindow(title, x, y, w, h, flags);
        if (!p) return nullptr;
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
        clear();
    }

private:
    Texture make_unique_texture(SDL_Texture* texture)
    {
        return Texture(texture, sdl_console::SDL_DestroyTexture);
    }

    Renderer make_unique_renderer(SDL_Renderer* renderer)
    {
        return Renderer(renderer, sdl_console::SDL_DestroyRenderer);
    }

    Window make_unique_window(SDL_Window* window)
    {
        return Window(window, sdl_console::SDL_DestroyWindow);
    }

    std::vector<Window> windows_;
    std::vector<Renderer> renderers_;
    std::vector<Texture> textures_;
};

static SDLThreadSpecificData sdl_tsd;

class ISlot {
public:
    virtual ~ISlot() = default;
    virtual void invoke(SDL_Event& event) = 0;
    virtual void disconnect() = 0;
    virtual void connect() = 0;
    virtual bool is_connected() = 0;
};

class ISignal {
public:
    virtual ~ISignal() = default;
    // todo for connect()
    virtual void disconnect(Uint32 event_type, ISlot* slot) = 0;
    virtual void reconnect(Uint32 event_type, ISlot* slot) = 0;
    virtual bool is_connected(Uint32 event_type, ISlot* slot) = 0;
};

template <typename EventType>
class Slot : public ISlot {
public:
    using Func = std::function<void(EventType&)>;

    Slot(ISignal& emitter, Uint32 event_type, Func& func)
        : emitter_(emitter)
        , event_type_(event_type)
        , func_(func)
    {
    }

    void invoke(SDL_Event& event) override
    {
        func_(get_event(event));
    }

    void disconnect() override
    {
        emitter_.disconnect(event_type_, this);
    }

    void connect() override
    {
        emitter_.reconnect(event_type_, this);
    }

    bool is_connected() override
    {
        return emitter_.is_connected(event_type_, this);
    }

    ~Slot() = default;

private:
    ISignal& emitter_;
    Uint32 event_type_;
    Func func_;

    EventType& get_event(SDL_Event& event)
    {
        // These branches are evaluated at compile time.
        if constexpr (std::is_same_v<EventType, SDL_KeyboardEvent>) {
            return event.key;
        } else if constexpr (std::is_same_v<EventType, SDL_MouseButtonEvent>) {
            return event.button;
        } else if constexpr (std::is_same_v<EventType, SDL_MouseMotionEvent>) {
            return event.motion;
        } else if constexpr (std::is_same_v<EventType, SDL_UserEvent>) {
            return event.user;
        } else if constexpr (std::is_same_v<EventType, SDL_TextInputEvent>) {
            return event.text;
        } else if constexpr (std::is_same_v<EventType, SDL_MouseWheelEvent>) {
            return event.wheel;
        } else if constexpr (std::is_same_v<EventType, SDL_WindowEvent>) {
            return event.window;
        } else {
            static_assert(std::is_same_v<EventType, void>, "Unsupported event type");
        }
    }
};

class SignalEmitter : public ISignal {
public:
    template <typename EventType>
    ISlot* connect(Uint32 event_type, typename Slot<EventType>::Func func)
    {
        auto slot = std::make_unique<Slot<EventType>>(*this, event_type, func);
        return slots_[event_type].emplace_back(std::move(slot)).get();
    }

    template <typename EventType>
    ISlot* connect_later(Uint32 event_type, typename Slot<EventType>::Func func)
    {
        auto slot = std::make_unique<Slot<EventType>>(*this, event_type, func);
        return disconnected_slots_[event_type].emplace_back(std::move(slot)).get();
    }

    void disconnect(Uint32 event_type, ISlot* slot) override
    {
        auto& disconnected_slots = disconnected_slots_[event_type];
        auto it = std::ranges::find_if(disconnected_slots, [slot](const std::unique_ptr<ISlot>& s) {
            return s.get() == slot;
        });

        if (it != disconnected_slots.end()) {
            disconnected_slots.emplace_back(std::move(*it));
            slots_[event_type].erase(it);
        }
    }

    void reconnect(Uint32 event_type, ISlot* slot) override
    {
        auto& disconnected_slots = disconnected_slots_[event_type];
        auto it = std::ranges::find_if(disconnected_slots, [slot](const std::unique_ptr<ISlot>& s) {
            return s.get() == slot;
        });

        if (it != disconnected_slots.end()) {
            slots_[event_type].emplace_back(std::move(*it));
            disconnected_slots.erase(it);
        }
    }

    bool is_connected(Uint32 event_type, ISlot* slot) override
    {
        return std::ranges::any_of(slots_[event_type], [slot](const std::unique_ptr<ISlot>& s) {
            return s.get() == slot;
        });
    }

    void emit(SDL_Event& event)
    {
        auto it = slots_.find(event.type);
        if (it != slots_.end()) {
            for (auto& slot : it->second) {
                slot->invoke(event);
            }
        }
    }

    void emit(InternalEventType::Type type)
    {
        auto e = make_sdl_user_event(type, nullptr);
        emit(e);
    }

    void emit(InternalEventType::Type type, void* data1)
    {
        auto e = make_sdl_user_event(type, data1);
        emit(e);
    }

    void clear()
    {
        slots_.clear();
        disconnected_slots_.clear();
    }

    static SDL_Event make_sdl_user_event(InternalEventType::Type type, void* data1)
    {
        SDL_Event event;
        sdl_console::SDL_zero(event);
        event.type = type;
        event.user.data1 = data1;
        return event;
    }

    template<typename T>
    static T copy_data1_from_userevent(SDL_UserEvent& e, std::optional<T> default_value = std::nullopt) {
        if (e.data1 == nullptr) {
            if (default_value) {
                return *default_value;
            }
            throw std::runtime_error("data is null and no default value provided");
        }

        T* value = static_cast<T*>(e.data1);
        return *value;
    }

private:
    using Container = std::vector<std::unique_ptr<ISlot>>;
    std::map<Uint32, Container> slots_;
    std::map<Uint32, Container> disconnected_slots_;
};

class Property {
    using Value = std::variant<std::string, std::u32string, int64_t, int, SDL_Rect>;
public:
    template <typename T>
    void set(const std::string& key, const T& value) {
        dirty_.store(true, std::memory_order_relaxed);
        std::scoped_lock l(m_);
        props_[key] = value;
        dirty_list_.push_back(key);
    }

    template <typename T>
    T get(const std::string& key, const T& default_value = T{}) {
        std::scoped_lock l(m_);
        auto it = props_.find(key);
        if (it == props_.end()) {
            return default_value;
        }

        if (std::holds_alternative<T>(it->second) == false) {
            return default_value; // TODO: log error
        }
        return std::get<T>(it->second);
    }

    template <typename T>
    void on_change(const std::string& key, T default_value, std::function<void(T)> fn) {
        std::scoped_lock l(m_);
        funcs_[key] = Func{
            [fn, default_value](const Value& new_value) {
                if (std::holds_alternative<T>(new_value)) {
                    fn(std::get<T>(new_value));
                } else {
                    fn(default_value);
                }
            },
            default_value
        };
        Value value = props_.count(key) ? props_[key] : Value{default_value};
        if (std::holds_alternative<T>(value)) {
            fn(std::get<T>(value));
        }
    }

    void update_props_if_needed() {
        if (!dirty_.load(std::memory_order_relaxed)) {
            return;
        }
        dirty_.store(false, std::memory_order_relaxed);
        std::scoped_lock l(m_);
        for (const auto& key : dirty_list_) {
            auto it = funcs_.find(key);
            if (it != funcs_.end()) {
                const Value& value = (props_.count(key) ? props_[key] : it->second.default_value);
                it->second.func(value);
            }
        }
        dirty_list_.clear();
    }

    // Just clear the function objects and dirty prop list
    // as they may have been invalidated.
    void reset()
    {
        funcs_.clear();
        dirty_list_.clear();
        dirty_.store(false, std::memory_order_relaxed);
    }

private:
    struct Func {
        std::function<void(const Value&)> func;
        Value default_value;
    };
    std::unordered_map<std::string, Func> funcs_;
    std::unordered_map<std::string, Value> props_;
    std::vector<std::string> dirty_list_;
    std::atomic<bool> dirty_;
    std::recursive_mutex m_;
};

namespace property {
    constexpr char WINDOW_MAIN_CREATE_RECT[] = "window.main.create.rect";
    constexpr char WINDOW_MAIN_TITLE[] = "window.main.title";

    constexpr char OUTPUT_SCROLLBACK[] = "output.scrollback";
    constexpr char RT_OUTPUT_ROWS[] = "rt.output.rows";
    constexpr char RT_OUTPUT_COLUMNS[] = "rt.output.columns";

    constexpr char PROMPT_TEXT[] = "prompt.text";
}

class Widget;
SDL_Texture* create_text_texture(Widget&, const std::u32string&, const SDL_Color&);

class TextEntry {
public:
    // A fragment is simply a chunk of text.
    struct Fragment {
        std::u32string_view text;
        size_t entry_offset; // position of fragment whin TextEntry
        size_t start_offset; // 0-based start position of this fragment
        size_t end_offset; // 0-based send position of this fragment
        SDL_Point coord {};

        Fragment(std::u32string_view text, size_t entry_offset, size_t start_offset, size_t end_offset)
        : text(text)
        , entry_offset(entry_offset)
        , start_offset(start_offset)
        , end_offset(end_offset) {};

        ~Fragment()
        {
        }

        Fragment(const Fragment&) = delete;
        Fragment& operator=(const Fragment&) = delete;
    };
    using Fragments = std::deque<Fragment>;

    TextEntryType type;
    // Unfragmented text.
    std::u32string text;
    SDL_Rect rect {};
    size_t size { 0 }; // # of fragments
    std::optional<SDL_Color> color_opt;

    TextEntry() {};

    ~TextEntry() {};

    TextEntry(TextEntryType type, const std::u32string& text)
        : type(type)
        , text(text) {};

    auto& add_fragment(std::u32string_view text, size_t start_offset, size_t end_offset)
    {
        return fragments_.emplace_back(text, size++, start_offset, end_offset);
    }

    void clear()
    {
        size = 0;
        fragments_.clear();
    }

    Fragments& fragments()
    {
        return fragments_;
    }

    void wrap_text(
        const int char_width,
        const int viewport_width)
    {
        // clear the fragments we're rebuilding.
        clear();

        struct Range {
            int start;
            int end;
        };

        int delim_idx = -1;
        int range_start_idx = 0;
        int text_idx = 0;
        std::vector<Range> ranges;

        auto close_fragment = [&](int end_idx) {
            ranges.emplace_back(range_start_idx, end_idx);
        };

        auto start_fragment = [&](int idx) {
            range_start_idx = idx + 1;
        };

        for (const auto& ch : text) {
            if (text::is_newline(ch)) {
                if (text_idx > range_start_idx) {
                    close_fragment(text_idx-1);  // Add fragment up to newline
                }
                start_fragment(text_idx);
                delim_idx = -1;
            } else if (text::is_whitespace(ch)) {
                delim_idx = text_idx;  // last space or tab character
            }

            if ((text_idx - range_start_idx + 1) * char_width >= viewport_width) {
                if (delim_idx != -1) {
                    close_fragment(delim_idx);  // Wrap at the last whitespace
                    start_fragment(delim_idx);
                    delim_idx = -1;
                } else {
                    close_fragment(text_idx);  // Wrap at current character
                    start_fragment(text_idx);
                }
            }

            text_idx++;
        }

        // Handle remaining text
        if (range_start_idx < (int)text.size()) {
            close_fragment(text.size() - 1);
        }

        for (const auto& range : ranges) {
            if (range.end >= range.start) { // guard against empty fragments for insurance
                std::u32string_view view(text);
                add_fragment(view.substr(range.start, range.end - range.start + 1), range.start, range.end);
            }
        }
    }

    std::optional<std::reference_wrapper<TextEntry::Fragment>> fragment_from_offset(size_t index)
    {
        for (auto& frag : fragments_) {
            if (index >= frag.start_offset && index <= frag.end_offset) {
                return frag;
            }
        }
        return std::nullopt;
    }

    TextEntry(const TextEntry&) = delete;
    TextEntry& operator=(const TextEntry&) = delete;

private:
    Fragments fragments_;
};

struct Glyph {
    SDL_Rect rect;
};

// XXX, TODO: cleanup.
class Font : public SignalEmitter {
    class ScopedColor {
    public:
        ScopedColor(Font* font) : font_(font) {}
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

    private:
        Font* font_;

        ScopedColor(const ScopedColor&) = delete;
        ScopedColor& operator=(const ScopedColor&) = delete;
    };

// FIXME: make members private and add accessors
public:
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    std::vector<Glyph> glyphs;
    int char_width;
    int line_height;
    int vertical_spacing;
    float scale_factor { 1 };
    int orig_char_width;
    int orig_line_height;
    int size_change_delta_ { 2 };

    Font(SDL_Renderer* renderer, SDL_Texture* texture, std::vector<Glyph>& glyphs, int char_width, int line_height)
        : renderer_(renderer)
        , texture_(texture)
        , glyphs(glyphs)
        , char_width(char_width)
        , line_height(line_height)
    {
        this->char_width = char_width;
        this->line_height = line_height;
        this->vertical_spacing = line_height * 0.5;
        orig_char_width = this->char_width;
        orig_line_height = this->line_height;
    }

    ~Font()
    {
    }

    std::optional<ScopedColor> set_color(std::optional<SDL_Color>& color)
    {
        if (!color.has_value()) return std::nullopt;
        return std::make_optional<ScopedColor>(this, color.value());
    }

    ScopedColor set_color(const SDL_Color& color)
    {
        return ScopedColor(this, color);
    }

    void render(const std::u32string_view& text, int x, int y)
    {
        for (auto& ch : text) {
            char32_t index;
            if (ch <= 127)
                index = ch;
            else {
                index = unicode_glyph_index(ch);
            }
            Glyph& g = glyphs[index];
            SDL_Rect dst = { x, y + (vertical_spacing / 2), (int)(g.rect.w * scale_factor), (int)((g.rect.h * scale_factor)) };
            x += g.rect.w * scale_factor;
            sdl_console::SDL_RenderCopy(renderer_, texture_, &g.rect, &dst);
        }
    }

    // Get the surface size of a text.
    // Mono-spaced faces have the equal widths and heights.
    void size_text(const std::u32string& s, int& w, int& h)
    {
        w = s.length() * char_width;
        h = line_height_with_spacing();
    }

    int line_height_with_spacing()
    {
        return line_height + vertical_spacing;
    }

    void incr_size()
    {
        change_size(size_change_delta_);
        emit(InternalEventType::font_size_changed);
    }

    void decr_size()
    {
        change_size(-size_change_delta_);
        emit(InternalEventType::font_size_changed);
    }

    char32_t unicode_glyph_index(const char32_t ch)
    {
        auto it = unicode_to_cp437.find(ch);
        if (it != unicode_to_cp437.end()) {
            return it->second;
        }
        return '?';
    }

    Font make_copy()
    {
        return *this;
    }

    Font(Font&& other) noexcept
        : renderer_(other.renderer_)
        , texture_(other.texture_)
        , glyphs(other.glyphs)
        , char_width(other.char_width)
        , line_height(other.line_height)
        , vertical_spacing(other.vertical_spacing)
        , scale_factor(other.scale_factor)
        , orig_char_width(other.orig_char_width)
        , orig_line_height(other.orig_line_height)
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

    // Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

private:
    void change_size(int delta) {
        scale_factor = (float)(char_width + delta) / orig_char_width;

        char_width = std::clamp(orig_char_width * scale_factor, 8.0f, 32.0f);
        line_height = std::clamp(orig_line_height * scale_factor, 8.0f, 32.0f);
    }

    Font(const Font& other)
    : renderer_(other.renderer_)
    , texture_(other.texture_)
    , glyphs(other.glyphs)
    , char_width(other.char_width)
    , line_height(other.line_height)
    , vertical_spacing(other.vertical_spacing)
    , scale_factor(other.scale_factor)
    , orig_char_width(other.orig_char_width)
    , orig_line_height(other.orig_line_height)
    {
    }
};

// This stuff needs reworked, I think.
using FontMap = std::map<std::pair<std::string, int>, Font>;
class FontLoader {
public:
    FontLoader(SDL_Renderer* renderer)
        : renderer_(renderer)
    {
    }

    virtual ~FontLoader() = default;

    virtual Font* open(const std::string& path, int size) = 0;

    Font* default_font()
    {
        return &fmap_.begin()->second;
    }

    Font* get_copy(std::string key, Font* font)
    {
        auto kp = std::make_pair(key, 0);
        auto result = fmap_.emplace(kp, font->make_copy());
        return &result.first->second;
    }

    FontLoader(const FontLoader&) = delete;
    FontLoader& operator=(const FontLoader&) = delete;

    FontLoader(FontLoader&& other) noexcept
        : fmap_(std::move(other.fmap_))
        , renderer_(other.renderer_)
        , textures_(std::move(other.textures_))
    {
    }

    FontLoader& operator=(FontLoader&& other) noexcept
    {
        if (this != &other) {
            fmap_ = std::move(other.fmap_);
            renderer_ = other.renderer_;
            textures_ = std::move(other.textures_);
        }
        return *this;
    }

protected:
    FontMap fmap_;
    SDL_Renderer* renderer_;
    std::vector<SDL_Texture*> textures_;
};

class BMPFontLoader : public FontLoader {
public:
    BMPFontLoader(SDL_Renderer* renderer)
        : FontLoader(renderer)
    {
    }

    ~BMPFontLoader()
    {
        for (auto tex : textures_) {
            sdl_tsd.DestroyTexture(tex);
        }
    }

    Font* open(const std::string& path, int size)
    {
        auto key = std::make_pair(path, size);
        auto it = fmap_.find(key);

        if (it != fmap_.end()) {
            return &it->second;
        }

        //SDL_Surface* surface = IMG_Load(path.c_str());
        SDL_Surface* surface = DFSDL::DFIMG_Load(path.c_str());
        if (surface == nullptr) {
            return nullptr;
        }

        //sdl_console::SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        //  FIXME: hardcoded magenta
        Uint32 bg_color = sdl_console::SDL_MapRGB(surface->format, 255, 0, 255);
        sdl_console::SDL_SetColorKey(surface, SDL_TRUE, bg_color);

        std::vector<Glyph> glyphs;
        // FIXME: magic numbers
        glyphs = build_glyph_rects(surface->pitch, surface->h, 16, 16);

        int width = surface->pitch;
        int height = surface->h;

        SDL_Surface* conv_surface = sdl_console::SDL_CreateRGBSurface(0, surface->pitch, surface->h, 32,
                                                                      0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
        if (!conv_surface)
            return nullptr;

        sdl_console::SDL_BlitSurface(surface, NULL, conv_surface, NULL);
        sdl_console::SDL_FreeSurface(surface);

        auto texture = sdl_tsd.CreateTextureFromSurface(renderer_, conv_surface);
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface Error: " << sdl_console::SDL_GetError() << std::endl;
            return nullptr;
        }
        sdl_console::SDL_FreeSurface(conv_surface);
        sdl_console::SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        textures_.push_back(texture);

        assert(width > 0);
        assert(height > 0);

        // FIXME: magic numbers
        auto result = fmap_.emplace(key, Font(renderer_, texture, glyphs, std::max(8, width/16), std::max(8, height/16)));
        return &result.first->second;
    }

    BMPFontLoader(const BMPFontLoader&) = delete;
    BMPFontLoader& operator=(const BMPFontLoader&) = delete;


    BMPFontLoader(BMPFontLoader&& other) noexcept = default;
    BMPFontLoader& operator=(BMPFontLoader&& other) noexcept = default;

private:
    std::vector<Glyph> build_glyph_rects(int sheet_w, int sheet_h, int columns, int rows)
    {
        int tile_w = sheet_w / columns;
        int tile_h = sheet_h / rows;
        int total_glyphs = rows * columns;

        std::vector<Glyph> glyphs;
        glyphs.reserve(rows * columns);
        for (int i = 0; i < total_glyphs; ++i) {
            int r = i / rows;
            int c = i % columns;
            Glyph glyph;
            glyph.rect = { tile_w * c, tile_h * r, tile_w, tile_h };
            glyphs.push_back(glyph);
        }
        return glyphs;
    }
};

class MainWindow;

/*
 * Shared context object for a window and its children, includes
 * resources and properties required for rendering and event handling.
 */
class WidgetContext {
public:
    SignalEmitter* global_emitter;
    Property &props;
    SDL_Window* window_handle;
    SDL_Renderer* renderer;
    Uint32 window_id{0};
    BMPFontLoader font_loader;
    SDL_Rect rect{};
    SDL_Point mouse_coord{};

    WidgetContext(SignalEmitter* emitter, Property& props, SDL_Window* h, SDL_Renderer* r)
    : global_emitter(emitter)
    , props(props)
    , window_handle(h)
    , renderer(r)
    , font_loader(r)
    {
        window_id = sdl_console::SDL_GetWindowID(window_handle);
        if (window_id == 0) {
            throw std::runtime_error("Failed to get window ID");
        }

        sdl_console::SDL_GetRendererOutputSize(renderer, &rect.w, &rect.h);
    }

    ~WidgetContext()
    {
    }

    WidgetContext(WidgetContext&& other) noexcept
    : global_emitter(other.global_emitter)
    , props(other.props)
    , window_handle(std::move(other.window_handle))
    , renderer(std::move(other.renderer))
    , window_id(other.window_id)
    , font_loader(std::move(other.font_loader)) // Move BMPFontLoader
    , rect(other.rect)
    , mouse_coord(other.mouse_coord)
    {
        other.global_emitter = nullptr;
        other.window_handle = nullptr;
        other.renderer = nullptr;
    }

#if 0
    WidgetContext& operator=(WidgetContext&& other) noexcept {
        if (this != &other) {
            global_emitter = other.global_emitter;
         //   props = other.props;
            window_handle = std::move(other.window_handle);
            renderer = std::move(other.renderer);
            window_id = other.window_id;
            font_loader = std::move(other.font_loader);
            rect = other.rect;
            mouse_coord = other.mouse_coord;

            //other.global_emitter = nullptr;
            other.window_handle = nullptr;
            other.renderer = nullptr;
        }
        return *this;
    }
#endif

    WidgetContext(const WidgetContext&) = delete;
    WidgetContext& operator=(const WidgetContext&) = delete;

    static WidgetContext create_main_window(Property& props, SignalEmitter* emitter)
    {
        auto title = props.get<std::string>(property::WINDOW_MAIN_TITLE, "SDL Console");
        SDL_Rect create_rect = props.get<SDL_Rect>(property::WINDOW_MAIN_CREATE_RECT,
                                                     SDL_Rect{SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480});
        auto flags = SDL_WINDOW_RESIZABLE;

        SDL_Window* handle = sdl_tsd.CreateWindow(title.c_str(), create_rect.x, create_rect.y, create_rect.w, create_rect.h, flags);
        if (!handle) {
            throw std::runtime_error("Failed to create SDL window");
        }

        sdl_console::SDL_SetWindowMinimumSize(handle, 64, 48);

        SDL_Renderer* renderer = create_renderer(props, handle);
        if (!renderer) {
            throw std::runtime_error("Failed to create SDL renderer");
        }

        return WidgetContext(emitter, props, handle, renderer);
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
        sdl_console::SDL_RenderSetIntegerScale(rend, SDL_TRUE);
        return rend;
    }
};

// TODO: needs work
class Widget : public SignalEmitter {
public:
    Widget* parent;
    Font* font;
    SDL_Rect viewport {};
    WidgetContext& context;

    Widget(Widget* parent)
        : parent(parent)
        , font(parent->font)
        , viewport(parent->viewport)
        , context(parent->context)
    {
    }

    Widget(Widget* parent, SDL_Rect viewport)
        : parent(parent)
        , font(parent->font)
        , viewport(viewport)
        , context(parent->context)
    {
    }

    // Constructor for Window
    Widget(WidgetContext& window_context)
        : parent(nullptr)
        , context(window_context)

    {
        auto bmpfont = context.font_loader.open("data/art/curses_640x300.png", 14);
        if (!bmpfont)
            throw(std::runtime_error("Error loading font"));
        font = bmpfont;
        viewport = context.rect;
    }

    SDL_Renderer* renderer()
    {
        return context.renderer;
    }

    SDL_Point mouse_coord()
    {
        return context.mouse_coord;
    }

    SDL_Point map_point_to_viewport(const SDL_Point& point)
    {
        return { point.x - viewport.x, point.y - viewport.y };
    }

    Property& props()
    {
        return context.props;
    }

    template <typename EventType, typename... Args>
    ISlot* connect_global(Args&&... args)
    {
        return context.global_emitter->connect<EventType>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void disconnect_global(Args&&... args)
    {
        context.global_emitter->disconnect(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void emit_global(Args&&... args)
    {
        context.global_emitter->emit(std::forward<Args>(args)...);
    }

    virtual void render() = 0;
    virtual void resize(const SDL_Rect& new_viewport) = 0;

    virtual ~Widget() { }

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
};

class Prompt : public Widget {
public:
    Prompt(Widget* parent)
        : Widget(parent)
    {
        input = &history.emplace_back(U"");

        props().on_change<std::u32string>(property::PROMPT_TEXT, U"> ", [this](const std::u32string& value) {
            set_prompt_text(value);
        });

        create_cursor_texture();

        connect_global<SDL_KeyboardEvent>(SDL_KEYDOWN, [this](SDL_KeyboardEvent& e) {
            on_SDL_KEYDOWN(e);
        });

        connect_global<SDL_TextInputEvent>(SDL_TEXTINPUT, [this](SDL_TextInputEvent& e) {
            put_input_at_cursor(text::from_utf8(e.text));
        });
    }

    ~Prompt()
    {
        //sdl_tsd.DestroyTexture(cursor_texture);
    }

    void create_cursor_texture()
    {
        cursor_texture = sdl_tsd.CreateTexture(renderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
        if (cursor_texture == nullptr)
            throw(std::runtime_error(sdl_console::SDL_GetError()));

        // FFFFFF = rgb white, 7F = 50% transparant
        Uint32 pixel = 0xFFFFFF7F;
        if (sdl_console::SDL_UpdateTexture(cursor_texture, NULL, &pixel, sizeof(Uint32)) != 0) {
            throw(std::runtime_error(sdl_console::SDL_GetError()));
        }
        // For transparancy
        sdl_console::SDL_SetTextureBlendMode(cursor_texture, SDL_BLENDMODE_BLEND);
    }

    /* OutputPane does this */
    void render() override
    {
    }

    void put_input_from_clipboard()
    {
        auto* str = sdl_console::SDL_GetClipboardText();
        if (*str != '\0')
            put_input_at_cursor(text::from_utf8(str));
        // Always free, even when empty.
        sdl_console::SDL_free(str);
    }

    void on_SDL_KEYDOWN(const SDL_KeyboardEvent& e)
    {
        // TODO: check if keysym.sym mapping is universally friendly
        auto sym = e.keysym.sym;
        switch (sym) {
        case SDLK_BACKSPACE:
            erase_input();
            break;

        case SDLK_UP:
            set_input_from_history(ScrollDirection::up);
            break;

        case SDLK_DOWN:
            set_input_from_history(ScrollDirection::down);
            break;

        case SDLK_LEFT:
            move_cursor_left();
            break;

        case SDLK_RIGHT:
            move_cursor_right();
            break;

        case SDLK_RETURN:
            new_input_line();

        case SDLK_HOME:
            cursor = 0;
            break;

        case SDLK_END:
            cursor = input->length();
            break;

        case SDLK_b:
            if (sdl_console::SDL_GetModState() & KMOD_CTRL) {
                auto ret = text::find_previous_word(*input, (size_t)cursor);
                if (ret.first != std::u32string::npos) {
                    cursor = ret.first;
                }
            }
            break;

        case SDLK_f:
            if (sdl_console::SDL_GetModState() & KMOD_CTRL) {
                auto ret = text::find_next_word(*input, (size_t)cursor);
                if (ret.second != std::u32string::npos) {
                    cursor = ret.second;
                }
            }
            break;
        }
    }

    void new_input_line()
    {
        emit(InternalEventType::new_input_line, input);

        input = &history.emplace_back(U"");
        history_index = history.size() - 1;

        cursor = 0;
        rebuild = true;
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
    void set_input_from_history(const ScrollDirection dir)
    {
        if (history.empty()) return;

        if (dir == ScrollDirection::up && history_index > 0) {
            history_index--;
        } else if (dir == ScrollDirection::down && history_index < (int)history.size() - 1) {
            history_index++;
        } else {
            return;
        }

        input = &history[history_index];
        cursor = input->length();
        rebuild = true;
    }

    void put_input_at_cursor(const std::u32string& str)
    {
        /* if cursor is at end of line, it's a simple concatenation */
        if (cursor == input->length()) {
            *input += str;
        } else {
            /* else insert text into line at cursor's index */
            input->insert(cursor, str);
        }
        cursor += str.length();
        rebuild = true;
    }

    void erase_input()
    {
        if (cursor == 0 || input->length() == 0)
            return;

        if (input->length() == cursor) {
            input->pop_back();
        } else {
            /* else shift the text from cursor left by one character */
            input->erase(cursor-1, 1);
        }
        cursor -= 1;
        rebuild = true;
    }

    void move_cursor_left()
    {
        if (cursor > 0) {
            cursor--;
        }
    }

    void move_cursor_right()
    {
        if (cursor < input->length()) {
            cursor++;
        }
    }

    void resize(const SDL_Rect& new_viewport) override
    {
        viewport = new_viewport;
        wrap_text();
    }

    void maybe_rebuild()
    {
        if (rebuild) {
            wrap_text();
            rebuild = false;
        }
    }

    void wrap_text()
    {
        entry.text = prompt_text + *input;
        entry.wrap_text(font->char_width, viewport.w);
    }

    std::optional<std::reference_wrapper<const TextEntry::Fragment>> find_fragment_at_y(int y)
    {
        for (auto& line : entry.fragments()) {
            if (y == line.coord.y) {
                return line;
            }
        }

        return std::nullopt;
    }

    void render_cursor(int scroll_offset)
    {
        if (entry.fragments().empty())
            return;

        /* cursor's position */
        auto cursor_pos = cursor + prompt_text.length();

        // If cursor is the head, nullopt will be returned as it falls outside
        // the fragment boundary. Maybe FIXME
        auto& line = [this, cursor_pos]()->auto& {
            if (auto line_opt = entry.fragment_from_offset(cursor_pos)) {
                return line_opt.value().get();
            } else {
                return entry.fragments().back();
            }
        }();

        int r = (entry.size - 1) - line.entry_offset;
        // scroll_offset starts at 0.
        if (scroll_offset > r) {
            return;
        }

        const auto lh = font->line_height_with_spacing();
        const auto cw = font->char_width;
        /*  full range of line + cursor */
        int cx = (cursor_pos - line.start_offset) * cw;
        int cy = line.coord.y;

        SDL_Rect rect = { cx, cy, cw, lh };
        /* Draw the cursor */
        // No, not for this, but maybe used to pen text
        // SDL_SetTextureBlendMode(cursor_texture, SDL_BLENDMODE_BLEND);
        // SDL_SetTextureAlphaMod(cursor_texture, 0.5 * 255);
        // SDL_SetTextureColorMod(cursor_texture, 255, 255, 255);
        render_texture(renderer(), cursor_texture, rect);
    }

    Prompt(const Prompt&) = delete;
    Prompt& operator=(const Prompt&) = delete;

    // Holds wrapped lines from input
    TextEntry entry;
    // The text of the prompt itself.
    std::u32string prompt_text;
    // The input portion of the prompt.
    std::u32string* input;
    // Prompt text/input/cursor was changed flag
    bool rebuild { true };
    size_t cursor { 0 }; // position of cursor within an entry
    // 1x1 texture stretched to font's single character dimensions
    SDL_Texture* cursor_texture;
    /*
     * For input history.
     * use deque to hold a stable reference.
     */
    std::deque<std::u32string> history;
    int history_index;
};

class Scrollbar : public Widget {
private:
    int page_size_;
    int content_size_ { 0 };
    int content_offset_ { 0 };
    bool depressed_ { false };
    SDL_Rect thumb_rect_ {};
    ISlot* mouse_motion_slot_ { nullptr };

public:
    Scrollbar(Widget* parent, int page_size)
        : Widget(parent)
        , page_size_(page_size)
    {
        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONDOWN, [this](SDL_MouseButtonEvent& e) {
            this->on_SDL_MOUSEBUTTONDOWN(e);
        });

        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONUP, [this](SDL_MouseButtonEvent& e) {
            this->on_SDL_MOUSEBUTTONUP(e);
        });

        mouse_motion_slot_ = context.global_emitter->connect_later<SDL_MouseMotionEvent>(SDL_MOUSEMOTION, [this](SDL_MouseMotionEvent& e) {
            if (!depressed_)
                return;

            content_offset_ = content_offset_from_track_position(e.y);
            set_thumb_position(e.y);
            emit(InternalEventType::value_changed, &content_offset_);
        });

        thumb_rect_ = viewport;
        set_thumb_height();
    }

    void resize(const SDL_Rect& new_viewport) override
    {
        viewport = new_viewport;
        thumb_rect_ = viewport;
        set_thumb_height();
        set_thumb_position(track_position_from_content_offset());
    }

    void set_page_size(int size)
    {
        page_size_ = size;
    }

    void set_content_size(int value)
    {
        content_size_ = value;
        set_thumb_height();
    }

    void set_content_offset(int value)
    {
        content_offset_ = value;
        set_thumb_position(track_position_from_content_offset());
    }

    void render() override
    {
        set_draw_color(renderer(), colors::gold);

        SDL_RenderDrawRect(renderer(), &viewport);

        set_draw_color(renderer(), colors::mauve);

        // FIXME: hardcoded magic
        SDL_Rect tr{thumb_rect_.x + 4, thumb_rect_.y + 4, thumb_rect_.w - 8, thumb_rect_.h - 8};
        SDL_RenderFillRect(renderer(), &tr);

        set_draw_color(renderer(), colors::darkgray);
    }

    ~Scrollbar()
    {
    }

    Scrollbar(const Scrollbar&) = delete;
    Scrollbar& operator=(const Scrollbar&) = delete;

private:
    void on_SDL_MOUSEBUTTONDOWN(SDL_MouseButtonEvent& e)
    {
        if (!geometry::in_rect(e.x, e.y, viewport)) {
            return;
        }

        if (!mouse_motion_slot_->is_connected()) {
            mouse_motion_slot_->connect();
        }

        depressed_ = true;
        set_thumb_position_from_track_click(e.y);
        content_offset_ = content_offset_from_track_position(e.y);
        emit(InternalEventType::value_changed, &content_offset_);
    }

    void on_SDL_MOUSEBUTTONUP(SDL_MouseButtonEvent& e)
    {
        if (depressed_) {
            depressed_ = false;
            mouse_motion_slot_->disconnect();
        }
    }

    int calculate_thumb_position(int target_y, int offset)
    {
        int track_start = viewport.y;
        int track_end = viewport.y + viewport.h;

        // Position with offset and constrain within track limits
        return std::clamp(target_y - offset, track_start, track_end - thumb_rect_.h);
    }

    void set_thumb_position(int y)
    {
        thumb_rect_.y = calculate_thumb_position(y,  thumb_rect_.h);
    }

    void set_thumb_position_from_track_click(int y)
    {
        // Center thumb on click
        thumb_rect_.y = calculate_thumb_position(y, thumb_rect_.h / 2);
    }

    void set_thumb_height()
    {
        if (content_size_ > 0) {
            float thumb_ratio = (float)(page_size_) / content_size_;
            thumb_rect_.h = std::clamp((int)std::round(thumb_ratio * viewport.h), 30, viewport.h);
        } else {
            // set thumb to the minimum height
            thumb_rect_.h = 30;
        }
    }

    int content_offset_from_track_position(int y)
    {
        int track_h = viewport.h;
        float y_ratio = (float)y / track_h;
        int val = static_cast<int>((1.0f - y_ratio) * content_size_);

        // Ensure the scroll offset does not go beyond the valid range
        val = std::clamp(val, 0, content_size_ - page_size_);
        return val;
    }

    int track_position_from_content_offset()
    {
        int track_h = viewport.h;

        if (content_size_ <= page_size_) {
            return viewport.y;
        }

        float value_ratio = (float)content_offset_ / (content_size_);
        int y = (int)((1.0f - value_ratio) * track_h);

        return y + viewport.y;
    }

};

class Button : public Widget {
public:
    Button(Widget* parent, std::u32string& label, SDL_Color color)
        : Widget(parent)
        , label(label)
    {
        compute_button_size();
        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONDOWN, [this](SDL_MouseButtonEvent& e) {
            on_SDL_MOUSEBUTTONDOWN(e);
        });

        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONUP, [this](SDL_MouseButtonEvent& e) {
            on_SDL_MOUSEBUTTONUP(e);
        });

        font->connect<SDL_UserEvent>(InternalEventType::font_size_changed, [this](SDL_UserEvent& e) {
            compute_button_size();
        });
    }

    void resize(const SDL_Rect& new_viewport) override
    {
        label_rect.x = viewport.x + (viewport.w / 2) - (label_rect.w / 2);
        label_rect.y = (viewport.h / 2) - (label_rect.h / 2);
    }

    void compute_button_size()
    {
        font->size_text(this->label, label_rect.w, label_rect.h);
        viewport.w = label_rect.w + (font->char_width * 2);
    }

    ~Button()
    {
    }

    void on_SDL_MOUSEBUTTONDOWN(SDL_MouseButtonEvent& e)
    {
        if (!geometry::in_rect(e.x, e.y, viewport)) {
            return;
        }
        depressed = true;
    }

    void on_SDL_MOUSEBUTTONUP(SDL_MouseButtonEvent& e)
    {
        if (!geometry::in_rect(e.x, e.y, viewport)) {
            if (depressed)
                depressed = false;
            return;
        }

        if (depressed) {
            emit(InternalEventType::clicked);
            depressed = false;
        }
    }

    void render() override
    {
        if (enabled) {
            SDL_Point coord = mouse_coord();
            if (depressed) {
                set_draw_color(renderer(), colors::teal);
                sdl_console::SDL_RenderFillRect(renderer(), &viewport);
                // SDL_RenderDrawRect(ui.renderer, &w.rect);
                set_draw_color(renderer(), colors::darkgray);
            } else if (geometry::in_rect(coord, viewport)) {
                set_draw_color(renderer(), colors::teal);
                sdl_console::SDL_RenderDrawRect(renderer(), &viewport);
                set_draw_color(renderer(), colors::darkgray);
            }
            font->render(label, label_rect.x, label_rect.y);
        } else {
            auto scoped_color = font->set_color(colors::mediumgray);
            font->render(label, label_rect.x, label_rect.y);
        }
    }

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    std::u32string label;
    SDL_Rect label_rect {};
    bool depressed { false };
    bool enabled { true };
};

class Toolbar : public Widget {
public:
    Toolbar(Widget* parent, SDL_Rect viewport);
    ~Toolbar() {};
    virtual void render() override;
    virtual void resize(const SDL_Rect& new_viewport) override;
    void layout_buttons();
    Button* add_button(std::u32string text);
    int compute_widgets_startx();
    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;
    // Should be changed to children and probably moved to base class
    std::deque<std::unique_ptr<Widget>> widgets;
};

class InputLinePipe {
public:
    InputLinePipe() = default;

    void make_connection(SignalEmitter& emitter)
    {
        emitter.connect<SDL_UserEvent>(InternalEventType::new_input_line, [this](SDL_UserEvent& e) {
            auto str = SignalEmitter::copy_data1_from_userevent<std::u32string>(e, U"");
            push(str);
        });
    }

    void push(std::u32string s)
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

    ~InputLinePipe()
    {
        shutdown();
    }

private:
    std::condition_variable_any cv_;
    std::recursive_mutex mutex_;
    std::queue<std::u32string> queue_;
    bool shutdown_{false};
};

class OutputPane : public Widget {
public:
    // Use deque to hold a stable reference.
    std::deque<TextEntry> entries;
    Prompt prompt;
    Scrollbar scrollbar;
    // Scrollbar could be made optional.
    int scroll_offset { 0 };
    SDL_Rect frame;
    int scrollback;
    int num_rows { 0 };
    bool depressed { false };
    SDL_Point mouse_motion_start { -1, -1 };
    SDL_Point mouse_motion_end { -1, -1 };
    ISlot* mouse_motion_slot { nullptr };
    std::vector<SDL_Rect> selected_rects;

    OutputPane(Widget* parent, SDL_Rect& viewport)
        : Widget(parent, viewport)
        , prompt(this)
        , scrollbar(this, rows())
    {
        resize(viewport);

        props().on_change<int>(property::OUTPUT_SCROLLBACK, 1000, [this](int v) {
            scrollback = v;
        });

        scrollbar.set_page_size(rows());
        scrollbar.set_content_size(1);

        prompt.connect<SDL_UserEvent>(InternalEventType::new_input_line, [this](SDL_UserEvent& e)
        {
            auto str = SignalEmitter::copy_data1_from_userevent<std::u32string>(e, U"");
            new_input(str);
        });

        font->connect<SDL_UserEvent>(InternalEventType::font_size_changed, [this](SDL_UserEvent& e)
        {
            resize(frame);
        });

        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONDOWN, [this](SDL_MouseButtonEvent& e) {
            on_SDL_MOUSEBUTTONDOWN(e);
        });

        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONUP, [this](SDL_MouseButtonEvent& e) {
            on_SDL_MOUSEBUTTONUP(e);
        });

        connect_global<SDL_MouseWheelEvent>(SDL_MOUSEWHEEL, [this](SDL_MouseWheelEvent& e) {
            scroll(e.y);
        });

        mouse_motion_slot = context.global_emitter->connect_later<SDL_MouseMotionEvent>(SDL_MOUSEMOTION, [this](SDL_MouseMotionEvent& e) {
            //if (!geometry::in_rect(e.x, e.y, this->viewport))
            //    return;
            if (depressed) {
                do_mouse_motion_end({ e.x, e.y });
                selected_rects = get_highlighted_rects();

                if (!selected_rects.empty()) {
                    bool change = true;
                    emit(InternalEventType::text_selection_changed, &change);
                }

                if (e.y > this->viewport.h) {
                    scroll(-1);
                } else if (e.y < 0) {
                    scroll(1);
                }
            }
        });

        connect_global<SDL_KeyboardEvent>(SDL_KEYDOWN, [this](SDL_KeyboardEvent& e) {
            on_SDL_KEYDOWN(e);
        });

        connect_global<SDL_TextInputEvent>(SDL_TEXTINPUT, [this](SDL_TextInputEvent& e) {
            scroll_offset = 0;
            emit(InternalEventType::value_changed, &scroll_offset);
        });

        scrollbar.connect<SDL_UserEvent>(InternalEventType::value_changed, [this](SDL_UserEvent& e) {
            scroll_offset = SignalEmitter::copy_data1_from_userevent<int>(e);
        });
    }

    int on_SDL_KEYDOWN(const SDL_KeyboardEvent& e)
    {
        auto sym = e.keysym.sym;
        switch (sym) {
        case SDLK_TAB:
          //  new_input_line(text::from_utf8("(tab)"));
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
            scroll(ScrollDirection::page_up);
            break;

        case SDLK_PAGEDOWN:
            scroll(ScrollDirection::page_down);
            break;

        case SDLK_RETURN:
        case SDLK_BACKSPACE:
        case SDLK_UP:
        case SDLK_DOWN:
        case SDLK_LEFT:
        case SDLK_RIGHT:
            set_scroll_offset(0);
            break;
        }
        return 0;
    }

    void on_SDL_MOUSEBUTTONDOWN(SDL_MouseButtonEvent& e)
    {
        if (!geometry::in_rect(e.x, e.y, viewport))
            return;

        if (e.button != SDL_BUTTON_LEFT) {
            return;
        }

        mouse_motion_end = { -1, -1 };
        selected_rects.clear();
        emit(InternalEventType::text_selection_changed);
        depressed = true;
        do_mouse_motion_start({ e.x, e.y });
        mouse_motion_slot->connect();
    }

    void on_SDL_MOUSEBUTTONUP(SDL_MouseButtonEvent& e)
    {
        if (depressed) {
            SDL_CaptureMouse(SDL_FALSE);
            depressed = false;
            mouse_motion_slot->disconnect();
        }
    }

    void clear()
    {
        entries.clear();
        num_rows = 0;
        set_scroll_offset(0);
        scrollbar.set_content_size(1);
        selected_rects.clear();
        emit(InternalEventType::text_selection_changed);
    }

    void set_scroll_offset(int v)
    {
        scroll_offset = v;
        scrollbar.set_content_offset(v);
    }

    void do_mouse_motion_start(const SDL_Point& point)
    {
        mouse_motion_start = map_point_to_viewport(point);
    }

    void do_mouse_motion_end(const SDL_Point& point)
    {
        mouse_motion_end = map_point_to_viewport(point);
    }

    void scroll(int y)
    {
        if (y > 0) {
            scroll(ScrollDirection::up);
        } else if (y < 0) {
            scroll(ScrollDirection::down);
        }
    }

    void scroll(ScrollDirection dir)
    {
        switch (dir) {
        case ScrollDirection::up:
            scroll_offset += 1;
            break;
        case ScrollDirection::down:
            scroll_offset -= 1;
            break;
        case ScrollDirection::page_up:
            scroll_offset += rows() / 2;
            break;
        case ScrollDirection::page_down:
            scroll_offset -= rows() / 2;
            break;
        }

        set_scroll_offset(std::min(std::max(0, scroll_offset), num_rows - 1));
    }

    void resize(const SDL_Rect& new_viewport) override
    {
        frame = new_viewport;
        viewport = new_viewport;
        // FIXME: magic numbers
        scrollbar.resize({ viewport.w - (8 * 2), viewport.y, (8 * 2), viewport.h });
        apply_margin_and_align_viewport();

        num_rows = 0;
        for (auto& e : entries) {
            wrap_text(e);
        }

        context.props.set<int>(property::RT_OUTPUT_COLUMNS, columns());
        context.props.set<int>(property::RT_OUTPUT_ROWS, rows());
    }

    /*
     * Adjust viewport dimensions to align with margin and font properties.
     * For character alignment consistency, the viewport must be divisible
     * into rows and columns that match the font's fixed character dimensions.
     */
    void apply_margin_and_align_viewport()
    {
         // (8px each side + 4px buffer tweak)
        const int scrollbar_space = (8 * 2) + 4;
        // Make room for scrollbar. TODO: needs layout framework
        // Deduct space on the right.
        viewport.w -= scrollbar_space;

        const int margin = 4; // // Margin around the viewport in px.

        // max width respect to font and margin
        const int max_width = viewport.w - (margin * 2);
        const int wfit = (max_width / font->char_width) * font->char_width;

        // max height with respect to font and margin
        const int max_height = viewport.h - (margin * 2);
        const int hfit = (max_height / font->line_height_with_spacing()) * font->line_height_with_spacing();

        viewport.x = frame.x + margin;
        viewport.y = frame.y + margin;
        viewport.w = wfit;
        viewport.h = hfit;
        // Prompt viewport is shared with this
        prompt.resize(viewport);
    }

    void new_output(const std::u32string& text, std::optional<SDL_Color> color)
    {
        TextEntry& entry = create_entry(TextEntryType::output, text, color);
        wrap_text(entry);
    }

    void new_input(const std::u32string& text)
    {
        auto both = prompt.prompt_text + text;
        auto& entry = create_entry(TextEntryType::input, both, std::nullopt);
        wrap_text(entry);
    }

    void wrap_text(
        TextEntry& entry)
    {
        entry.wrap_text(font->char_width, viewport.w);
        num_rows += entry.size;
        scrollbar.set_content_size(num_rows + 1);
    }

    /*
     * Create a new entry which may span multiple rows and set it to be the head.
     * This function will automatically cycle-out entries if the number of rows
     * has reached the max.
     */
    TextEntry& create_entry(const TextEntryType entry_type,
                            const std::u32string& text, std::optional<SDL_Color> color)
    {
        TextEntry& entry = entries.emplace_front(entry_type, text);
        entry.color_opt = color;

        /* When the list is too long, start chopping */
        if (num_rows > scrollback) {
            num_rows -= entries.back().size;
            entries.pop_back();
        }

        return entry;
    }

    std::optional<std::reference_wrapper<const TextEntry::Fragment>> find_fragment_at_y(int y)
    {
        for (auto& entry : entries) {
            for (auto& frag : entry.fragments()) {
                if (y == frag.coord.y) {
                    return frag;
                }
            }
        }
        return std::nullopt;
    }

    void copy_selected_text_to_clipboard()
    {
        char32_t sep = U'\n';
        std::u32string clipboard_text;

        for (const auto& rect : selected_rects) {
            auto frag_opt = find_fragment_at_y(rect.y);
            if (!frag_opt) {
                frag_opt = prompt.find_fragment_at_y(rect.y);
                if (!frag_opt)
                    continue;
            }

            const auto& frag = frag_opt.value().get();
            auto col = get_column(rect.x);

            if (col < frag.text.size()) {
                if (!clipboard_text.empty())
                    clipboard_text += sep;
                auto extent = column_extent(rect.w) + col;
                auto end_idx = std::min(extent, frag.text.size());
                clipboard_text += frag.text.substr(col, end_idx - col);
            }
        }
        sdl_console::SDL_SetClipboardText(text::to_utf8(clipboard_text).c_str());
    }

    size_t get_column(const int x)
    {
        return x / font->char_width;
    }

    size_t column_extent(int width)
    {
        return width / font->char_width;
    }

    int columns()
    {
        return (float)viewport.w / font->char_width;
    }

    int rows()
    {
        return (float)viewport.h / font->line_height_with_spacing();
    }

    void render() override
    {
        // SDL_RenderSetScale(renderer(), 1.2, 1.2);
        sdl_console::SDL_RenderSetViewport(renderer(), &viewport);
        prompt.maybe_rebuild();
        // TODO: make sure renderer supports blending else highlighting
        // will make the text invisible.
        render_highlight_selected_text();
        // SDL_SetTextureColorMod(font->texture, 0, 128, 0);
        render_prompt_and_output();
        // SDL_SetTextureColorMod(font->texture, 255, 255, 255);
        prompt.render_cursor(scroll_offset);
        sdl_console::SDL_RenderSetViewport(renderer(), &parent->viewport);
        scrollbar.render();
        // SDL_RenderSetScale(renderer(), 1.0, 1.0);
    }

    void render_prompt_and_output()
    {
        const int max_screen_row = rows() + scroll_offset;
        int ypos = viewport.h;
        int row_counter = 0;

        render_entry(prompt.entry, ypos, row_counter, max_screen_row);

        if (entries.empty())
            return;

        for (auto& entry : entries) {
            if (row_counter > max_screen_row) {
                break;
            }
            render_entry(entry, ypos, row_counter, max_screen_row);
        }
    }

    // FIXME: Position and rows to render calculations should be done elsewhere.
    void render_entry(TextEntry& entry, int& ypos, int& row_counter, int max_screen_row)
    {
        auto scoped_color = font->set_color(entry.color_opt);
        for (auto& row : entry.fragments() | std::views::reverse) {
            row_counter++;
            if (row_counter <= scroll_offset) {
                continue;
            } else if (row_counter > max_screen_row) {
                break;
            }

            ypos -= font->line_height_with_spacing();
            row.coord.y = ypos;
            font->render(row.text, row.coord.x, row.coord.y);
        }
    }

    void render_highlight_selected_text()
    {
        if (mouse_motion_end.y == -1)
            return;

        if (selected_rects.empty())
            return;

        set_draw_color(renderer(), colors::mediumgray);
        for (auto& rect : selected_rects) {

            sdl_console::SDL_RenderFillRect(renderer(), &rect);
        }
        set_draw_color(renderer(), colors::darkgray);
    }

    /*
     * FIXME: This function handles regions of text shown on screen.
     * TODO:  Support highlighting while scrolling (keep highlighted state when off screen).
     */

    std::vector<SDL_Rect> get_highlighted_rects()
    {
        const int char_width = font->char_width;
        const int line_height = font->line_height_with_spacing();
        const SDL_Point& selection_start = mouse_motion_start;
        const SDL_Point& selection_end = mouse_motion_end;

        // Calculate the start and end positions, snapping to line and character boundaries
        auto [top_point, bottom_point] = std::minmax({selection_start, selection_end},
                                            [](const SDL_Point& a, const SDL_Point& b) {
                                                return a.y < b.y;
                                            });

        auto top = grid::floor_boundary(top_point.y, line_height);
        auto bottom = grid::ceil_boundary(bottom_point.y, line_height);
        bool is_single_row = (bottom_point.y - top_point.y) <= line_height;

        int left;
        int right;
        if (is_single_row) {
            left = grid::floor_boundary(std::min(selection_start.x, selection_end.x), char_width);
            right = grid::ceil_boundary(std::max(selection_start.x, selection_end.x), char_width);
        } else {
            left = grid::floor_boundary(top_point.x, char_width);
            right = grid::ceil_boundary(bottom_point.x, char_width);
        }

        SDL_Rect current_rect = { left, top, (right - left), line_height };
        if (is_single_row)
            return { current_rect };

        int rows = std::ceil((float)(bottom - top) / line_height);
        std::vector<SDL_Rect> selected_rects;
        current_rect.w = viewport.w;
        selected_rects.push_back(current_rect);
        // Handle intermediate rows
        for (int i = 1; i < rows; ++i) {
            current_rect.x = 0;
            current_rect.y = top + i * line_height;
            current_rect.w = viewport.w;
            selected_rects.push_back(current_rect);
        }
        // Fill last row to end of selected text
        selected_rects.back().w = right;

        return selected_rects;
    }


    OutputPane(const OutputPane&) = delete;
    OutputPane& operator=(const OutputPane&) = delete;
};


class MainWindow : public Widget {
public:
    std::unique_ptr<Toolbar> toolbar; // optional toolbar. XXX: implementation requires it
    std::unique_ptr<OutputPane> outpane;
    bool has_focus{false};

    MainWindow(Property& props, WidgetContext& wctx)
        : Widget(wctx)
    {
        connect_global<SDL_WindowEvent>(SDL_WINDOWEVENT, [this](SDL_WindowEvent& e) {
            if (e.event == SDL_WINDOWEVENT_RESIZED) {
                resize({});
            } else if (e.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                has_focus = false;
            } else if (e.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                has_focus = true;
            }
        });

        connect_global<SDL_MouseMotionEvent>(SDL_MOUSEMOTION, [this](SDL_MouseMotionEvent& e) {
            context.mouse_coord.x = e.x;
            context.mouse_coord.y = e.y;
        });

        // TODO: make toolbar optional
        SDL_Rect tv = { 0, 0, viewport.w, font->line_height * 2 };
        toolbar = std::make_unique<Toolbar>(this, tv);

        SDL_Rect lv = { 0, toolbar->viewport.h, viewport.w, viewport.h - toolbar->viewport.h };
        outpane = std::make_unique<OutputPane>(this, lv);

        Button& copy = *toolbar->add_button(U"Copy");
        copy.enabled = false;
        copy.connect<SDL_UserEvent>(InternalEventType::clicked, [this](SDL_UserEvent& e) {
            outpane->copy_selected_text_to_clipboard();
        });
        outpane->connect<SDL_UserEvent>(InternalEventType::text_selection_changed, [&copy](SDL_UserEvent& e) {
            copy.enabled = SignalEmitter::copy_data1_from_userevent<bool>(e, false);
        });

        Button& paste = *toolbar->add_button(U"Paste");
        paste.connect<SDL_UserEvent>(InternalEventType::clicked, [this](SDL_UserEvent& e) {
            outpane->prompt.put_input_from_clipboard();
        });

        Button& font_inc = *toolbar->add_button(U"A+");
        font_inc.connect<SDL_UserEvent>(InternalEventType::clicked, [this](SDL_UserEvent& e) {
            outpane->font->incr_size();
        });

        Button& font_dec = *toolbar->add_button(U"A-");
        font_dec.connect<SDL_UserEvent>(InternalEventType::clicked, [this](SDL_UserEvent& e) {
            outpane->font->decr_size();
        });
    }

    ~MainWindow()
    {
    }

    void render() override {
        // Should not fail unless OOM.
        sdl_console::SDL_RenderClear(renderer());
        // set background color
        // should not fail unless renderer is invalid
        set_draw_color(renderer(), colors::darkgray);
        toolbar->render();
        outpane->render();

        sdl_console::SDL_RenderPresent(renderer());
    }

    void resize(const SDL_Rect& new_viewport) override
    {
        sdl_console::SDL_GetRendererOutputSize(renderer(), &viewport.w, &viewport.h);
        toolbar->resize({ 0, 0, viewport.w, font->line_height_with_spacing() * 2 });
        outpane->resize({ 0, toolbar->viewport.h, viewport.w, viewport.h - toolbar->viewport.h });
    }

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
};

Toolbar::Toolbar(Widget* parent, SDL_Rect viewport)
    : Widget(parent, viewport)
{
    font = context.font_loader.get_copy("toolbar", font);
};

void Toolbar::render()
{
    set_draw_color(renderer(), colors::gold);
    // Render bg
    // SDL_RenderFillRect(renderer(), &viewport);
    // Draw a border
    sdl_console::SDL_RenderDrawRect(renderer(), &viewport);
    // Lay out horizontally
    for (auto& w : widgets) {
        w->render();
    }

    set_draw_color(renderer(), colors::darkgray);
}

void Toolbar::resize(const SDL_Rect& new_viewport)
{
    viewport.w = new_viewport.w;
    layout_buttons();
}

Button* Toolbar::add_button(std::u32string text)
{
    auto button = std::make_unique<Button>(this, text, colors::white);
    Button& b = *button.get();
    b.viewport.h = viewport.h;
    b.viewport.y = 0;
    b.viewport.w = b.label_rect.w + (font->char_width * 2);
    widgets.emplace_back(std::move(button));
    layout_buttons();
    return &b;
}

void Toolbar::layout_buttons()
{
    int margin_right = 4;
    int x = (viewport.w - margin_right) - compute_widgets_startx();

    for (auto& w : widgets) {
        w->viewport.x = x;
        x += w->viewport.w;
    }

    for (auto& w : widgets) {
        w->resize(viewport);
    }
}

int Toolbar::compute_widgets_startx()
{
    int x = 0;
    for (auto& w : widgets) {
        x += w->viewport.w;
    }
    return x;
}

class ExternalEventQueue {
    template <typename T>
    class Queue {
        friend class ExternalEventQueue;

    public:
        explicit Queue(std::mutex& mutex)
        : mutex_(mutex) {}

        void push(T event) {
            std::scoped_lock l(mutex_);
            queue_.push(std::move(event));
        }

        std::optional<T> batch_pop() {
            if (!queue_.empty()) {
                T event = std::move(queue_.front());
                queue_.pop();
                return event;
            }
            return std::nullopt;
        }

        bool is_empty()
        {
            std::scoped_lock l(mutex_);
            return queue_.empty();
        }

        auto lock() {
            return std::scoped_lock(mutex_);
        }

        void drain() {
            std::scoped_lock l(mutex_);
            std::queue<T> empty;
            queue_.swap(empty);
        }

    private:
        std::queue<T> queue_;
        std::mutex& mutex_;
    };

public:
    ExternalEventQueue()
    : api_task(mutex) {}

    void reset() {
        api_task.drain();
    }

    ExternalEventQueue(const ExternalEventQueue&) = delete;
    ExternalEventQueue& operator=(const ExternalEventQueue&) = delete;

private:
    std::mutex mutex;
public:
    using ApiTask = std::function<void()>;
    Queue<ApiTask> api_task;
};

void render_texture(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const SDL_Rect& dst)
{
    sdl_console::SDL_RenderCopy(renderer, texture, NULL, &dst);
}

int set_draw_color(SDL_Renderer* renderer, const SDL_Color& color)
{
    return sdl_console::SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

struct SDLConsole_pshare {
    Property props;
    std::weak_ptr<SDLConsole_impl> impl_weak;
    //std::thread::id render_thread_id{0};
};

//static Uint32 render_frame_event_id{(Uint32)-1)};
class SDLConsole_impl : public std::enable_shared_from_this<SDLConsole_impl> {
public:
    Property& props;
    SDLConsole::State& state;
    SDLConsole_pshare& pshare;
    // For internal communication, mainly by widgets.
    SignalEmitter global_emitter;
    InputLinePipe input_pipe;
    ExternalEventQueue external_event_queue;
    WidgetContext widget_context;
    MainWindow main_window;

    SDLConsole_impl(SDLConsole* con)
    : props(con->pshare->props)
    , state(con->state)
    , pshare(*con->pshare)
    , widget_context(std::move(WidgetContext::create_main_window(props, &global_emitter)))
    , main_window(props, widget_context)
    {
        input_pipe.make_connection(outpane().prompt);
        state.set_state(SDLConsole::State::active);

#if 0
        render_frame_event_id = SDL_RegisterEvents(1);
        if (render_frame_event_id == (Uint32)-1)
            throw std::runtime_error("Failed to register SDL event");
        SDL_AddTimer(100, render_frame_timer, con);
#endif
    }

#if 0
    static Uint32 render_frame_timer(Uint32 period, void *data)
    {
        SDLConsole* con = static_cast<SDLConsole*>(data);
        if (!con->state.is_active()) return 0; // Cancel timer
        SDL_Event e;
        e.type = render_frame_event_id;
        SDL_PushEvent(&e);
        return 100;
    }
#endif

    OutputPane& outpane() {
        return *main_window.outpane.get();
    }

    bool sdl_event_hook(SDL_Event& e)
    {
        // NOTE: Having focus is not enough to determine handling.
        // This window ID check is important for df to receive click-through
        // events when moving between windows.
        if (e.type == SDL_WINDOWEVENT && e.window.windowID != main_window.context.window_id) {
            return false;
        } else if (e.type == SDL_WINDOWEVENT || main_window.has_focus) {
            emit_sdl_event(e);
            return true;
        }
        return false;
    }

    void update()
    {
        handle_events();
        props.update_props_if_needed();
        render_frame();
    }

    void shutdown()
    {
        state.set_state(SDLConsole::State::shutdown);
        props.reset();
        input_pipe.shutdown();
    }

    ~SDLConsole_impl()
    {
        shutdown();
    }

private:
    void handle_events()
    {
        {
            auto locker = external_event_queue.api_task.lock();
            while (auto task_opt = external_event_queue.api_task.batch_pop()) {
                task_opt.value()();
            }
        }
    }

    void render_frame()
    {
        main_window.render();
    }

    void emit_sdl_event(SDL_Event& e)
    {
        global_emitter.emit(e);
    }
};

SDLConsole::SDLConsole() : state()
{
    pshare = std::make_unique<SDLConsole_pshare>();
    state.set_state(State::inactive);
}

SDLConsole::~SDLConsole()  {}


/**
 * SDL events and video subsystems must be initialized
 * before this function is called.
 **/
bool SDLConsole::init()
{
    if (!state.is_inactive()) return true;
    bool success = true;
    std::cerr << "SDLConsole: init() from thread: " << std::this_thread::get_id() << std::endl;
    //pshare->render_thread_id = std::this_thread::get_id();
    try {
        impl = std::make_shared<SDLConsole_impl>(this);
        pshare->impl_weak = impl;
        // throw std::runtime_error("test");
    } catch(std::runtime_error &e) {
        success = false;
        impl.reset();
        sdl_tsd.clear();
        state.set_state(State::shutdown);
        std::cerr << "SDLConsole: caught exception: " << e.what();
    }
    return success;
}

void SDLConsole::reset()
{
    state.set_state(State::inactive);
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
    return pshare->props.get<int>(property::RT_OUTPUT_COLUMNS, 0);
}

int SDLConsole::get_rows()
{
    return pshare->props.get<int>(property::RT_OUTPUT_ROWS, 0);
}

SDLConsole& SDLConsole::set_mainwindow_create_rect(int w, int h, int x, int y)
{
    SDL_Rect rect{x, y, w, h};
    pshare->props.set<SDL_Rect>(property::WINDOW_MAIN_CREATE_RECT, rect);
    return *this;
}

SDLConsole& SDLConsole::set_scrollback(int scrollback) {
    pshare->props.set<int>(property::OUTPUT_SCROLLBACK, scrollback);
    return *this;
}

SDLConsole& SDLConsole::set_prompt(std::string text) {
    auto t = text::from_utf8(text);
    pshare->props.set<std::u32string>(property::PROMPT_TEXT, t);
    return *this;
}

void SDLConsole::show_window() {
    push_api_task([this] {
        sdl_console::SDL_ShowWindow(impl->main_window.context.window_handle);
    });
}

void SDLConsole::hide_window() {
    push_api_task([this] {
        sdl_console::SDL_HideWindow(impl->main_window.context.window_handle);
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
        return I->input_pipe.wait_get(buf);
    }
    return -1;
}

SDLConsole& SDLConsole::get_console()
{
    static SDLConsole instance;
    return instance;
}

// Can be called from any thread.
void SDLConsole::shutdown()
{
    std::cerr << "SDLConsole: shutdown() called from thread: " << std::this_thread::get_id() << std::endl;
    state.set_state(State::shutdown);
    if (auto I = std::weak_ptr<SDLConsole_impl>(impl).lock()) {
        I->shutdown();
    }
}

bool SDLConsole::destroy()
{
    std::cerr << "SDLConsole: destroy() from thread: " << std::this_thread::get_id() << std::endl;
    impl.reset();
    // NOTE: The only long living shared_ptr is get_line()
    // which is closed when shutdown() is called.
    while (!pshare->impl_weak.expired()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    sdl_tsd.clear();
    reset();
    return true;
}

bool SDLConsole::sdl_event_hook(SDL_Event &e)
{
    if (auto I = std::weak_ptr<SDLConsole_impl>(impl).lock()) {
        return I->sdl_event_hook(e);
    }
    return false;
}

void SDLConsole::update()
{
    if (auto I = std::weak_ptr<SDLConsole_impl>(impl).lock()) {
        I->update();
    }
}

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
