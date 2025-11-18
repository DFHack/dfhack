#pragma once

#include "SDLConsole_impl.h"
//#include <SDL.h>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>
#include "sdl_symbols.h"

namespace DFHack::SDLConsoleLib {

class Font;
struct GlyphRec {
    SDL_Rect rect {};
    Font* font { nullptr };
    int8_t bearing_y { 0 }; // For TTF
    uint8_t height { 0 };
    int8_t page_idx { 0 };
};

struct GlyphPosition {
    SDL_Rect src; // from the font atlas
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
    Int char_width { 0 };
    Int line_height { 0 };
    Int line_spacing { 0 };
    Int line_height_with_spacing { 0 };
    Int ascent { 0 };
};

class FontRenderer {
protected:
    explicit FontRenderer(SDL_Renderer* renderer)
        : renderer_(renderer)
    {
    }

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
    // int size_;

    //Font() = default;
    virtual ~Font() = default;
    virtual FontAtlas& atlas() = 0;
    virtual int size() const noexcept = 0;
    virtual bool set_size(int desired_px) = 0;
    virtual Texture to_texture(StringView text) = 0;
    virtual ScopedColor set_color(const SDL_Color& color) { return ScopedColor(*this, color); };
    virtual std::optional<ScopedColor> set_color(const std::optional<SDL_Color>& color) { return std::nullopt; };
    virtual void reset_color() { };
    virtual bool has_glyph(char32_t codepoint) const = 0;

    const FontMetrics& metrics() const noexcept
    {
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
    Font* fallback_ { nullptr };
};

class FontAtlas : public virtual FontRenderer {
public:
    Font& font;

    explicit FontAtlas(Font* font)
        : font(*font)
    {
    }

    virtual GlyphPosVector get_glyph_layout(StringView text, int x, int y) = 0;
    virtual void enlarge() = 0;
    virtual void shrink() = 0;
    virtual GlyphRec& get_glyph_rec(char32_t codepoint, Font::FallbackMode) = 0;
    virtual SDL_Texture* get_texture(int page_idx) = 0;

    void render(std::span<const GlyphPosition> vec) const;
    void render(StringView text, int x, int y);
    GlyphRec& get_glyph_rec(char32_t codepoint)
    {
        return get_glyph_rec(codepoint, Font::FallbackMode::Enabled);
    }
    static GlyphRec get_fallback_glyph_rec(Font& font, char32_t codepoint);
};

class DFBitmapFont : public Font, public FontAtlas {
#if 0 // NOLINT
    class ScopedColor {
public:
    explicit ScopedColor(BitmapFont* font);
    ScopedColor(BitmapFont* font, const SDL_Color& color);

    void set(const SDL_Color& color);
    ~ScopedColor();

    ScopedColor(const ScopedColor&) = delete;
    ScopedColor& operator=(const ScopedColor&) = delete;

private:
    BitmapFont* font_;
};
#endif

public:
    explicit DFBitmapFont(SDL_Renderer* renderer);

    static std::unique_ptr<DFBitmapFont> create(SDL_Renderer* renderer,
                                                const std::filesystem::path& path,
                                                int size);

    Texture to_texture(StringView text) override;

    int size() const noexcept override;

    bool set_size(int new_size) override;

    FontAtlas& atlas() override;

    std::optional<ScopedColor> set_color(const std::optional<SDL_Color>& color) override;
    ScopedColor set_color(const SDL_Color& color) override;

    GlyphPosVector get_glyph_layout(StringView text, int x, int y) override;

    void enlarge() override;
    void shrink() override;

    static bool is_ascii(char32_t codepoint) noexcept;

    bool has_glyph(char32_t codepoint) const override;

    SDL_Texture* get_texture(int page_idx) override;

    GlyphRec& get_glyph_rec(char32_t codepoint, FallbackMode fbm) override;

    DFBitmapFont(DFBitmapFont&& other) = delete;
    DFBitmapFont& operator=(DFBitmapFont&&) = delete;

    DFBitmapFont(const DFBitmapFont&) = delete;
    DFBitmapFont& operator=(const DFBitmapFont&) = delete;

private:
    Texture texture_;
    std::vector<GlyphRec> glyphs_;
    int orig_char_width { 0 };
    int orig_line_height { 0 };
    float scale_factor { 0 };
    std::unique_ptr<FontAtlas> atlas_;

    static constexpr int atlas_columns = 16;
    static constexpr int atlas_rows = 16;

    bool init(const std::filesystem::path& path);
    static std::vector<GlyphRec> build_glyph_rects(int sheet_w, int sheet_h,
                                                   int columns, int rows);
};
}
