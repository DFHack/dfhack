#include "sdl_symbols.h"
#include "text.h"

namespace DFHack::SDLConsoleLib::text {

    std::string to_utf8(const std::u32string_view u32_string)
    {
        char* conv = SDLConsoleLib::SDL_iconv_string("UTF-8", "UTF-32LE",
                                                     reinterpret_cast<const char*>(u32_string.data()),
                                                     (u32_string.size()) * sizeof(char32_t));
        if (!conv)
            return "?u8?";

        std::string result(conv);
        SDLConsoleLib::SDL_free(conv);
        return result;
    }

    std::u32string from_utf8(const std::string_view u8_string)
    {
        char* conv = SDLConsoleLib::SDL_iconv_string("UTF-32LE", "UTF-8",
                                                     u8_string.data(),
                                                     u8_string.size());
        if (!conv)
            return U"?u8?";

        std::u32string result(reinterpret_cast<char32_t*>(conv));
        SDLConsoleLib::SDL_free(conv);
        return result;
    }
}
