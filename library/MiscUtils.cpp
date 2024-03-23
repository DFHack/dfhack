/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "Internal.h"
#include "Export.h"
#include "MiscUtils.h"
#include "ColorText.h"

#include "modules/DFSDL.h"

#ifndef LINUX_BUILD
// We don't want min and max macros
#define NOMINMAX
    #include <Windows.h>
#else
    #include <sys/time.h>
    #include <ctime>
    #include <cxxabi.h>
#endif

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <cstdlib>

#include <sstream>
#include <map>
#include <array>

int random_int(int max)
{
    return int(int64_t(rand()) * max / (int64_t(RAND_MAX) + 1));
}

std::string stl_sprintf(const char *fmt, ...) {
    va_list lst;
    va_start(lst, fmt);
    std::string rv = stl_vsprintf(fmt, lst);
    va_end(lst);
    return rv;
}

std::string stl_vsprintf(const char *fmt, va_list args) {
    /* Allow small (about single line) strings to be printed into stack memory
     * with a call to vsnprintf.
     */
    std::array<char,128> buf;
    va_list args2;
    va_copy(args2, args);
    int rsz = vsnprintf(&buf[0], buf.size(), fmt, args2);
    va_end(args2);
    if (rsz < 0)
        return std::string(); /* Error occurred */
    if (static_cast<unsigned>(rsz) < buf.size())
        return std::string(&buf[0], rsz); /* Whole string fits to a single line buffer */
    std::string rv;
    // Allocate enough memory for the output and null termination
    rv.resize(rsz);
    rsz = vsnprintf(&rv[0], rv.size()+1, fmt, args);
    if (rsz < static_cast<int>(rv.size()))
      rv.resize(std::max(rsz,0));
    return rv;
}

bool split_string(std::vector<std::string> *out,
                  const std::string &str, const std::string &separator, bool squash_empty)
{
    out->clear();

    size_t start = 0, pos;

    if (!separator.empty())
    {
        while ((pos = str.find(separator,start)) != std::string::npos)
        {
            if (pos > start || !squash_empty)
                out->push_back(str.substr(start, pos-start));
            start = pos + separator.size();
        }
    }

    if (start < str.size() || !squash_empty)
        out->push_back(str.substr(start));

    return out->size() > 1;
}

std::string join_strings(const std::string &separator, const std::vector<std::string> &items)
{
    std::stringstream ss;

    for (size_t i = 0; i < items.size(); i++)
    {
        if (i)
            ss << separator;
        ss << items[i];
    }

    return ss.str();
}

char toupper_cp437(char c)
{
    switch (c)
    {
        case (char)129: // 'ü'
            return (char)154; // 'Ü'
        case (char)164: // 'ñ'
            return (char)165; // 'Ñ'
        case (char)132: // 'ä'
            return (char)142; // 'Ä'
        case (char)134: // 'å'
            return (char)143; // 'Å'
        case (char)130: // 'é'
            return (char)144; // 'É'
        case (char)148: // 'ö'
            return (char)153; // 'Ö'
        case (char)135: // 'ç'
            return (char)128; // 'Ç'
        case (char)145: // 'æ'
            return (char)146; // 'Æ'
        default: // toupper consistently across locales
            return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
    }
}

char tolower_cp437(char c)
{
    switch (c)
    {
        case (char)154: // 'Ü'
            return (char)129; // 'ü'
        case (char)165: // 'Ñ'
            return (char)164; // 'ñ'
        case (char)142: // 'Ä'
            return (char)132; // 'ä'
        case (char)143: // 'Å'
            return (char)134; // 'å'
        case (char)144: // 'É'
            return (char)130; // 'é'
        case (char)153: // 'Ö'
            return (char)148; // 'ö'
        case (char)128: // 'Ç'
            return (char)135; // 'ç'
        case (char)146: // 'Æ'
            return (char)145; // 'æ'
        default: // tolower consistently across locales
            return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
    }
}

std::string toUpper_cp437(const std::string &str)
{
    std::string rv(str.size(),' ');
    for (unsigned i = 0; i < str.size(); ++i)
        rv[i] = toupper_cp437(str[i]);
    return rv;
}

std::string toLower_cp437(const std::string &str)
{
    std::string rv(str.size(),' ');
    for (unsigned i = 0; i < str.size(); ++i)
        rv[i] = tolower_cp437(str[i]);
    return rv;
}

static const char *normalized_table[256] = {
    //.0  .1    .2    .3    .4    .5    .6    .7    .8    .9    .A    .B    .C    .D    .E    .F
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 0.
    NULL, NULL, NULL, NULL, NULL,  "S", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 1.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 2.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 3.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 4.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 5.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 6.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // 7.
     "C",  "u",  "e",  "a",  "a",  "a",  "a",  "c",  "e",  "e",  "e",  "i",  "i",  "i",  "A",  "A", // 8.
     "E", "ae", "Ae",  "o",  "o",  "o",  "u",  "u",  "y",  "O",  "U",  "c",  "L",  "Y", NULL,  "f", // 9.
     "a",  "i",  "o",  "u",  "n",  "N",  "a",  "o", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // A.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // B.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // C.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // D.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // E.
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // F.
};

std::string to_search_normalized(const std::string &str)
{
    std::string result;
    result.reserve(str.size());
    for (char c : str)
    {
        const char *mapped = normalized_table[(uint8_t)c];
        if (mapped == NULL)
            result += tolower(c);
        else
            while (*mapped != '\0')
            {
                result += tolower(*mapped);
                ++mapped;
            }
    }

    return result;
}

std::string capitalize_string_words(const std::string& str)
{   // Cleaned up from g_src/basics.cpp, and returns new string
    std::string out = str;
    bool starting = true;
    int32_t bracket_count = 0;
    bool conf;

    for (size_t s = 0; s < out.length(); s++)
    {
        if (out[s] == '[') { ++bracket_count; continue; }
        else if (out[s] == ']') { --bracket_count; continue; }
        else if (bracket_count > 0) continue;

        conf = false;
        if (!starting)
        {
            if (out[s - 1] == ' ' || out[s - 1] == '\"')
                conf = true;
            // Discount single quote if it isn't preceded by space, comma, or nothing
            else if (out[s - 1] == '\'' && s >= 2 && (out[s - 2] == ' ' || out[s - 2] == ','))
                conf = true;
        }

        if (starting || conf)
        {   // Capitalize
            out[s] = toupper_cp437(out[s]);
            starting = false;
        }
    }

    return out;
}

bool word_wrap(std::vector<std::string> *out, const std::string &str, size_t line_length,
               word_wrap_whitespace_mode mode)
{
    if (line_length == 0)
        line_length = SIZE_MAX;

    std::string line;
    size_t break_pos = 0;
    bool ignore_whitespace = false;

    for (auto &c : str)
    {
        if (c == '\n')
        {
            out->push_back(line);
            line.clear();
            break_pos = 0;
            ignore_whitespace = (mode == WSMODE_TRIM_LEADING);
            continue;
        }

        if (isspace(c))
        {
            if (ignore_whitespace || (mode == WSMODE_COLLAPSE_ALL && break_pos == line.length()))
                continue;

            line.push_back((mode == WSMODE_COLLAPSE_ALL) ? ' ' : c);
            break_pos = line.length();
        }
        else
        {
            line.push_back(c);
            ignore_whitespace = false;
        }

        if (line.length() > line_length)
        {
            if (break_pos > 0)
            {
                // Break before last space, and skip that space
                out->push_back(line.substr(0, break_pos - 1));
            }
            else
            {
                // Single word is too long, just break it
                out->push_back(line.substr(0, line_length));
                break_pos = line_length;
            }
            line = line.substr(break_pos);
            break_pos = 0;
            ignore_whitespace = (mode == WSMODE_TRIM_LEADING);
        }
    }
    if (line.length())
        out->push_back(line);

    return true;
}

std::string grab_token_string_pos(const std::string& source, int32_t pos, char compc)
{   // Cleaned up from g_src/basics.cpp, return string instead of bool
    std::string out;

    // Go until you hit compc, ']', or the end
    for (auto s = source.begin() + pos; s < source.end(); ++s)
    {
        if (*s == compc || *s == ']')
            break;
        out += *s;
    }

    return out;
}

bool prefix_matches(const std::string &prefix, const std::string &key, std::string *tail)
{
    size_t ksize = key.size();
    size_t psize = prefix.size();
    if (ksize < psize || memcmp(prefix.data(), key.data(), psize) != 0)
        return false;
    if (tail)
        tail->clear();
    if (ksize == psize)
        return true;
    if (psize == 0 || prefix[psize-1] == '/')
    {
        if (tail) *tail = key.substr(psize);
        return true;
    }
    if (key[psize] == '/')
    {
        if (tail) *tail = key.substr(psize+1);
        return true;
    }
    return false;
}

#ifdef LINUX_BUILD // Linux
uint64_t GetTimeMs64()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t ret = tv.tv_usec;

    // Convert from micro seconds (10^-6) to milliseconds (10^-3)
    ret /= 1000;
    // Adds the seconds (10^0) after converting them to milliseconds (10^-3)
    ret += (tv.tv_sec * 1000);
    return ret;
}


#else // Windows
uint64_t GetTimeMs64()
{
    FILETIME ft;
    LARGE_INTEGER li;

    // Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC)
    // and copy it to a LARGE_INTEGER structure.
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    uint64_t ret = li.QuadPart;
    // Convert from file time to UNIX epoch time.
    ret -= 116444736000000000LL;
    // From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals
    ret /= 10000;

    return ret;
}
#endif

/* Character decoding */

// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

static const uint8_t utf8d[] = {
  // The first part of the table maps bytes to character classes that
  // to reduce the size of the transition table and create bitmasks.
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

  // The second part is a transition table that maps a combination
  // of a state of the automaton and a character class to a state.
   0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
  12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
  12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
  12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
  12,36,12,12,12,12,12,12,12,12,12,12,
};

static inline uint32_t
decode(uint32_t* state, uint32_t* codep, uint8_t byte) {
  uint32_t type = utf8d[byte];

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = utf8d[256 + *state + type];
  return *state;
}

/* Character encoding */

static inline int encode(uint8_t *out, uint16_t c) {
    if (c <= 0x7F)
    {
        out[0] = c;
        return 1;
    }
    else if (c <= 0x7FF)
    {
        out[0] = (0xC0 | (c >> 6));
        out[1] = (0x80 | (c & 0x3F));
        return 2;
    }
    else /*if (c <= 0xFFFF)*/
    {
        out[0] = (0xE0 | (c >> 12));
        out[1] = (0x80 | ((c >> 6) & 0x3F));
        out[2] = (0x80 | (c & 0x3F));
        return 3;
    }
}

/* CP437 */

static uint16_t character_table[256] = {
    0,      0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, //
    0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
    0x25BA, 0x25C4, 0x2195, 0x203C, 0xB6,   0xA7,   0x25AC, 0x21A8, //
    0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, //
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, //
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, //
    0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, //
    0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, //
    0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, //
    0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x2302,
    0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, //
    0xEA, 0xEB, 0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5,
    0xC9, 0xE6, 0xC6, 0xF4, 0xF6, 0xF2, 0xFB, 0xF9, //
    0xFF, 0xD6, 0xDC, 0xA2, 0xA3, 0xA5, 0x20A7, 0x192,
    0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA, //
    0xBF, 0x2310, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, //
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, //
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, //
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x3B1,  0xDF,   0x393,  0x3C0,  0x3A3,  0x3C3,  0xB5,   0x3C4, //
    0x3A6,  0x398,  0x3A9,  0x3B4,  0x221E, 0x3C6,  0x3B5,  0x2229,
    0x2261, 0xB1,   0x2265, 0x2264, 0x2320, 0x2321, 0xF7,   0x2248, //
    0xB0,   0x2219, 0xB7,   0x221A, 0x207F, 0xB2,   0x25A0, 0xA0
};

std::string DF2UTF(const std::string &in)
{
    std::string out;
    out.reserve(in.size());

    uint8_t buf[4];
    for (size_t i = 0; i < in.size(); i++)
    {
        int cnt = encode(buf, character_table[(uint8_t)in[i]]);
        out.append(&buf[0], &buf[cnt]);
    }

    return out;
}

std::string UTF2DF(const std::string &in)
{
    // Unicode to normal lookup table
    static std::map<uint32_t, char> ctable;

    if (ctable.empty())
    {
        for (uint16_t i = 0; i < 256; i++)
            if (character_table[i] != i)
                ctable[character_table[i]] = char(i);
    }

    // Actual conversion loop
    size_t size = in.size();
    std::string out(size, char(0));

    uint32_t codepoint = 0;
    uint32_t state = UTF8_ACCEPT, prev = UTF8_ACCEPT;
    uint32_t pos = 0;

    for (unsigned i = 0; i < size; prev = state, i++) {
        switch (decode(&state, &codepoint, uint8_t(in[i]))) {
        case UTF8_ACCEPT:
            if (codepoint < 256 && character_table[codepoint] == codepoint) {
                out[pos++] = char(codepoint);
            } else {
                char v = ctable[codepoint];
                out[pos++] = v ? v : '?';
            }
            break;

        case UTF8_REJECT:
            out[pos++] = '?';
            if (prev != UTF8_ACCEPT) --i;
            state = UTF8_ACCEPT;
            break;
        }
    }

    if (pos != size)
        out.resize(pos);
    return out;
}

DFHACK_EXPORT std::string DF2CONSOLE(const std::string &in)
{
    bool is_utf = false;
#ifdef LINUX_BUILD
    std::string locale = "";
    if (getenv("LANG"))
        locale += getenv("LANG");
    if (getenv("LC_CTYPE"))
        locale += getenv("LC_CTYPE");
    locale = toUpper_cp437(locale);
    is_utf = (locale.find("UTF-8") != std::string::npos) ||
             (locale.find("UTF8") != std::string::npos);
#endif
    return is_utf ? DF2UTF(in) : in;
}

DFHACK_EXPORT std::string DF2CONSOLE(DFHack::color_ostream &out, const std::string &in)
{
    return out.is_console() ? DF2CONSOLE(in) : in;
}

DFHACK_EXPORT std::string cxx_demangle(const std::string &mangled_name, std::string *status_out)
{
#ifdef __GNUC__
    int status;
    char *demangled = abi::__cxa_demangle(mangled_name.c_str(), nullptr, nullptr, &status);
    std::string out;
    if (demangled) {
        out = demangled;
        free(demangled);
    }
    if (status_out) {
        if (status == 0) *status_out = "success";
        else if (status == -1) *status_out = "memory allocation failure";
        else if (status == -2) *status_out = "invalid mangled name";
        else if (status == -3) *status_out = "invalid arguments";
        else *status_out = "unknown error";
    }
    return out;
#else
    if (status_out) {
        *status_out = "not implemented on this platform";
    }
    return "";
#endif
}
