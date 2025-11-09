#ifndef SDL_CONSOLE_INTERNAL
#define SDL_CONSOLE_INTERNAL

#include <string>
#include <optional>
#include <SDL.h>
#include <span>
#include <vector>
#include <filesystem>
#include <iostream>
#include <source_location>

namespace DFHack {
namespace sdl_console {

enum class Error {
    Internal,
    SDL,
    FreeType
};


template <Error S, typename T = int>
inline void log_error(std::string_view ctx, T err = {}, const std::source_location& loc = std::source_location::current());

template <Error S, typename T = int>
[[noreturn]] inline void fatal(std::string_view ctx, T err = {}, const std::source_location& loc = std::source_location::current());

using String = std::u32string;
using StringView = std::u32string_view;
using Char = char32_t;

using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;


template <typename T>
concept Integral = std::is_integral_v<T>;

// Clamp from min_val to upper bound to avoid overflow
template <Integral T, Integral U>
constexpr T clamp_max(U value, T min_val = 1)
{
    using Tmp = std::common_type_t<T, U>;
    Tmp tmp = value;

    if (tmp < min_val)
        return min_val;
    if (tmp > std::numeric_limits<T>::max())
        return std::numeric_limits<T>::max();
    return static_cast<T>(tmp);
}

namespace geometry {
    struct Size {
        int w { 0 };
        int h { 0 };
    };
}

namespace text {
size_t skip_wspace(StringView text, size_t pos) noexcept;

size_t skip_wspace_reverse(StringView text, size_t pos) noexcept;

size_t skip_graph(StringView text, size_t pos) noexcept;

size_t skip_graph_reverse(StringView text, size_t pos) noexcept;

size_t find_prev_word(StringView text, size_t pos) noexcept;

size_t find_next_word(StringView text, size_t pos) noexcept;

std::pair<size_t, size_t> find_wspace_run(StringView text, size_t pos) noexcept;


template <typename T>
std::pair<size_t, size_t> find_run_with_pred(StringView text, size_t pos, T&& pred);

/*
 * If pos falls on a word, returns range of word.
 * If pos falls on whitespace, returns range of whitespace.
 */
std::pair<size_t, size_t> find_run(StringView text, size_t pos) noexcept;
}

class Font;
struct GlyphRec {
    SDL_Rect rect {};
    Font* font { nullptr };
    int8_t bearing_y { 0 }; // For TTF
    uint8_t height { 0 };
    int8_t page_idx { 0 };
};

struct GlyphPosition {
    SDL_Rect src;   // from the font atlas
    SDL_Rect dst;
    SDL_Texture* texture;
};

using GlyphPosVector = std::vector<GlyphPosition>;

class ScopedColor {
public:
    ScopedColor(Font& font, const SDL_Color& color);
    ~ScopedColor();

    ScopedColor(const ScopedColor&) = delete;
    ScopedColor& operator=(const ScopedColor&) = delete;
    ScopedColor(ScopedColor&&) = delete;
    ScopedColor& operator=(ScopedColor&&) = delete;

private:
    Font& font_;
};

struct FontMetrics {
    using Int = int;
    Int char_width{0};
    Int line_height{0};
    Int line_spacing{0};
    Int line_height_with_spacing{0};
    Int ascent{0};
};

class FontRenderer {
protected:
    explicit FontRenderer(SDL_Renderer* renderer)
    : renderer_(renderer) {}

    SDL_Renderer* const renderer_;
};

class FontAtlas;
class Font : public virtual FontRenderer {
public:
    enum class FallbackMode {
        Enabled,
        Disabled
    };

    static constexpr int8_t size_change_delta { 2 };
    static constexpr int size_min { 12 };
    static constexpr int size_max { 32 };
    static constexpr int line_spacing { 4 };
    std::filesystem::path path;
    //int size_;

    explicit Font(std::filesystem::path path)
    : path(std::move(path))
    {
    }

    virtual ~Font() = default;
    virtual FontAtlas& atlas() = 0;
    virtual int size() const noexcept = 0;
    virtual bool set_size(int desired_px)  = 0;
    virtual Texture to_texture(StringView text) = 0;
    virtual ScopedColor set_color(const SDL_Color& color) { return ScopedColor(*this, color); };
    virtual std::optional<ScopedColor> set_color(const std::optional<SDL_Color>& color) { return std::nullopt; };
    virtual void reset_color() {};
    virtual bool has_glyph(char32_t codepoint) const = 0;

    const FontMetrics& metrics() const noexcept {
        return metrics_;
    }

    void set_fallback(Font* fallback) noexcept
    {
        fallback_ = fallback;
    }

    Font* fallback() const noexcept
    {
        return fallback_;
    }

    int line_height_with_spacing() const noexcept
    {
        return metrics_.line_height_with_spacing;
    }

    int char_width() const noexcept
    {
        return metrics_.char_width;
    }

    int line_height() const noexcept
    {
        return metrics_.line_height;
    }

    // Get the surface size of a text.
    // Mono-spaced faces have the equal widths and heights.
    geometry::Size size_text(const StringView s) const noexcept
    {
        return geometry::Size {
            .w = int(s.length()) * metrics_.char_width,
            .h = line_height_with_spacing()
        };
    }

protected:
    FontMetrics metrics_;

private:
    Font* fallback_{nullptr};
};

class FontAtlas : public virtual FontRenderer {
public:
    Font& font;

    explicit FontAtlas(Font* font) : font(*font) {}

    virtual GlyphPosVector get_glyph_layout(StringView text, int x, int y) = 0;
    virtual void enlarge() = 0;
    virtual void shrink() = 0;
    virtual GlyphRec& get_glyph_rec(char32_t codepoint, Font::FallbackMode) = 0;
    virtual SDL_Texture* get_texture(int page_idx) = 0;

    void render(std::span<const GlyphPosition> vec) const;
    void render(StringView text, int x, int y);
    GlyphRec& get_glyph_rec(char32_t codepoint) {
        return get_glyph_rec(codepoint, Font::FallbackMode::Enabled);
    }
    static GlyphRec get_fallback_glyph_rec(Font& font, char32_t codepoint);
};

} // namespace sdl_console
} // namespace DFHack
#endif
