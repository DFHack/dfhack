#include "font.h"
#include "modules/DFSDL.h"
#include "sdl_symbols.h"
#include <algorithm>
#include <unordered_map>

static const std::unordered_map<uint16_t, uint8_t> unicode_to_cp437 = {
    // Control characters and symbols
    /* NULL        */ { 0x263A, 0x01 },
    { 0x263B, 0x02 },
    { 0x2665, 0x03 },
    { 0x2666, 0x04 },
    { 0x2663, 0x05 },
    { 0x2660, 0x06 },
    { 0x2022, 0x07 },
    { 0x25D8, 0x08 },
    { 0x25CB, 0x09 },
    { 0x25D9, 0x0A },
    { 0x2642, 0x0B },
    { 0x2640, 0x0C },
    { 0x266A, 0x0D },
    { 0x266B, 0x0E },
    { 0x263C, 0x0F },

    { 0x25BA, 0x10 },
    { 0x25C4, 0x11 },
    { 0x2195, 0x12 },
    { 0x203C, 0x13 },
    { 0x00B6, 0x14 },
    { 0x00A7, 0x15 },
    { 0x25AC, 0x16 },
    { 0x21A8, 0x17 },
    { 0x2191, 0x18 },
    { 0x2193, 0x19 },
    { 0x2192, 0x1A },
    { 0x2190, 0x1B },
    { 0x221F, 0x1C },
    { 0x2194, 0x1D },
    { 0x25B2, 0x1E },
    { 0x25BC, 0x1F },

    // ASCII, no mapping needed

    // Extended Latin characters and others
    { 0x2302, 0x7F },

    { 0x00C7, 0x80 },
    { 0x00FC, 0x81 },
    { 0x00E9, 0x82 },
    { 0x00E2, 0x83 },
    { 0x00E4, 0x84 },
    { 0x00E0, 0x85 },
    { 0x00E5, 0x86 },
    { 0x00E7, 0x87 },
    { 0x00EA, 0x88 },
    { 0x00EB, 0x89 },
    { 0x00E8, 0x8A },
    { 0x00EF, 0x8B },
    { 0x00EE, 0x8C },
    { 0x00EC, 0x8D },
    { 0x00C4, 0x8E },
    { 0x00C5, 0x8F },

    { 0x00C9, 0x90 },
    { 0x00E6, 0x91 },
    { 0x00C6, 0x92 },
    { 0x00F4, 0x93 },
    { 0x00F6, 0x94 },
    { 0x00F2, 0x95 },
    { 0x00FB, 0x96 },
    { 0x00F9, 0x97 },
    { 0x00FF, 0x98 },
    { 0x00D6, 0x99 },
    { 0x00DC, 0x9A },
    { 0x00A2, 0x9B },
    { 0x00A3, 0x9C },
    { 0x00A5, 0x9D },
    { 0x20A7, 0x9E },
    { 0x0192, 0x9F },

    { 0x00E1, 0xA0 },
    { 0x00ED, 0xA1 },
    { 0x00F3, 0xA2 },
    { 0x00FA, 0xA3 },
    { 0x00F1, 0xA4 },
    { 0x00D1, 0xA5 },
    { 0x00AA, 0xA6 },
    { 0x00BA, 0xA7 },
    { 0x00BF, 0xA8 },
    { 0x2310, 0xA9 },
    { 0x00AC, 0xAA },
    { 0x00BD, 0xAB },
    { 0x00BC, 0xAC },
    { 0x00A1, 0xAD },
    { 0x00AB, 0xAE },
    { 0x00BB, 0xAF },

    // Box drawing characters
    { 0x2591, 0xB0 },
    { 0x2592, 0xB1 },
    { 0x2593, 0xB2 },
    { 0x2502, 0xB3 },
    { 0x2524, 0xB4 },
    { 0x2561, 0xB5 },
    { 0x2562, 0xB6 },
    { 0x2556, 0xB7 },
    { 0x2555, 0xB8 },
    { 0x2563, 0xB9 },
    { 0x2551, 0xBA },
    { 0x2557, 0xBB },
    { 0x255D, 0xBC },
    { 0x255C, 0xBD },
    { 0x255B, 0xBE },
    { 0x2510, 0xBF },

    { 0x2514, 0xC0 },
    { 0x2534, 0xC1 },
    { 0x252C, 0xC2 },
    { 0x251C, 0xC3 },
    { 0x2500, 0xC4 },
    { 0x253C, 0xC5 },
    { 0x255E, 0xC6 },
    { 0x255F, 0xC7 },
    { 0x255A, 0xC8 },
    { 0x2554, 0xC9 },
    { 0x2569, 0xCA },
    { 0x2566, 0xCB },
    { 0x2560, 0xCC },
    { 0x2550, 0xCD },
    { 0x256C, 0xCE },
    { 0x2567, 0xCF },

    { 0x2568, 0xD0 },
    { 0x2564, 0xD1 },
    { 0x2565, 0xD2 },
    { 0x2559, 0xD3 },
    { 0x2558, 0xD4 },
    { 0x2552, 0xD5 },
    { 0x2553, 0xD6 },
    { 0x256B, 0xD7 },
    { 0x256A, 0xD8 },
    { 0x2518, 0xD9 },
    { 0x250C, 0xDA },
    { 0x2588, 0xDB },
    { 0x2584, 0xDC },
    { 0x258C, 0xDD },
    { 0x2590, 0xDE },
    { 0x2580, 0xDF },

    // Mathematical symbols and others
    { 0x03B1, 0xE0 },
    { 0x00DF, 0xE1 },
    { 0x0393, 0xE2 },
    { 0x03C0, 0xE3 },
    { 0x03A3, 0xE4 },
    { 0x03C3, 0xE5 },
    { 0x00B5, 0xE6 },
    { 0x03C4, 0xE7 },
    { 0x03A6, 0xE8 },
    { 0x0398, 0xE9 },
    { 0x03A9, 0xEA },
    { 0x03B4, 0xEB },
    { 0x221E, 0xEC },
    { 0x03C6, 0xED },
    { 0x03B5, 0xEE },
    { 0x2229, 0xEF },

    { 0x2261, 0xF0 },
    { 0x00B1, 0xF1 },
    { 0x2265, 0xF2 },
    { 0x2264, 0xF3 },
    { 0x2320, 0xF4 },
    { 0x2321, 0xF5 },
    { 0x00F7, 0xF6 },
    { 0x2248, 0xF7 },
    { 0x00B0, 0xF8 },
    { 0x2219, 0xF9 },
    { 0x00B7, 0xFA },
    { 0x221A, 0xFB },
    { 0x207F, 0xFC },
    { 0x00B2, 0xFD },
    { 0x25A0, 0xFE },
    { 0x00A0, 0xFF },
};

namespace DFHack::SDLConsoleLib {
#if 0 // NOLINT
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

DFBitmapFont::DFBitmapFont(SDL_Renderer* renderer)
    : FontRenderer(renderer)
    , FontAtlas(this)
    , texture_(make_texture(nullptr))
{
}

std::unique_ptr<DFBitmapFont> DFBitmapFont::create(SDL_Renderer* renderer, const std::filesystem::path& path, int size)
{
    auto font = std::make_unique<DFBitmapFont>(renderer);

    if (!font->init(path))
        return nullptr;

    if (size != font->orig_line_height)
        font->set_size(size);

    return font;
}

Texture DFBitmapFont::to_texture(StringView text)
{
    return make_texture(nullptr);
}

int DFBitmapFont::size() const noexcept
{
    return metrics_.line_height;
}

bool DFBitmapFont::set_size(int new_size)
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

FontAtlas& DFBitmapFont::atlas()
{
    return *this;
}

std::optional<ScopedColor> DFBitmapFont::set_color(const std::optional<SDL_Color>& color)
{
    if (!color.has_value()) {
        return std::nullopt;
    }
    return std::make_optional<ScopedColor>(*this, color.value());
}

ScopedColor DFBitmapFont::set_color(const SDL_Color& color)
{
    return ScopedColor(*this, color);
}

GlyphPosVector DFBitmapFont::get_glyph_layout(const StringView text, int x, int y)
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
        g_pos.push_back({ .src = g.rect, .dst = dst, .texture = tex });
    }
    return g_pos;
}

void DFBitmapFont::enlarge()
{
    set_size(size() + Font::size_change_delta);
}

void DFBitmapFont::shrink()
{
    set_size(size() - Font::size_change_delta);
}

bool DFBitmapFont::is_ascii(char32_t codepoint) noexcept
{
    return codepoint < 128;
}

bool DFBitmapFont::has_glyph(char32_t codepoint) const
{
    if (is_ascii(codepoint))
        return true;
    return unicode_to_cp437.contains(codepoint);
}

SDL_Texture* DFBitmapFont::get_texture(int /*page_idx*/)
{
    return texture_.get();
}

GlyphRec& DFBitmapFont::get_glyph_rec(char32_t codepoint, FallbackMode fbm)
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

bool DFBitmapFont::init(const std::filesystem::path& path)
{
    auto surface = make_surface(DFSDL::DFIMG_Load(path.c_str()));
    if (surface == nullptr) {
        log_error<Error::SDL>("Failed to load cp437 bitmap");
        return false;
    }

    //  FIXME: hardcoded magenta
    // Make this keyed color transparent.
    uint32_t bg_color = SDLConsoleLib::SDL_MapRGB(surface->format, 255, 0, 255);
    if (SDLConsoleLib::SDL_SetColorKey(surface.get(), SDL_TRUE, bg_color))
        log_error<Error::SDL>("Failed to set color key"); // Continue anyway

    // Create a surface in ARGB8888 format, and replace the keyed color
    // with fully transparant pixels. This step completely removes the color.
    // NOTE: Do not use surface->pitch
    auto conv_surface = make_surface(SDLConsoleLib::SDL_CreateRGBSurfaceWithFormat(0, surface->w, surface->h, 32,
                                                                                   SDL_PixelFormatEnum::SDL_PIXELFORMAT_ARGB8888));

    if (!conv_surface) {
        log_error<Error::SDL>("Failed to create RGB surface");
        return false;
    }

    SDLConsoleLib::SDL_BlitSurface(surface.get(), nullptr, conv_surface.get(), nullptr);
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

    texture_ = make_texture(SDLConsoleLib::SDL_CreateTextureFromSurface(Font::renderer_, surface.get()));
    if (!texture_) {
        log_error<Error::SDL>("Failed to create texture from surface");
        return false;
    }
    SDLConsoleLib::SDL_SetTextureBlendMode(texture_.get(), SDL_BLENDMODE_BLEND);

    return true;
}

std::vector<GlyphRec> DFBitmapFont::build_glyph_rects(int sheet_w, int sheet_h,
                                                      int columns, int rows)
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
                .h = tile_h }, // Rectangle in pixel dimensions
            .height = uint8_t(tile_h)
        };
        glyphs.push_back(glyph);
    }
    return glyphs;
}

}
