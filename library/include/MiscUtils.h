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

#pragma once

#include "Export.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <vector>

#if defined(_MSC_VER)
    #define DFHACK_FUNCTION_SIG __FUNCSIG__
#elif defined(__GNUC__)
    #define DFHACK_FUNCTION_SIG __PRETTY_FUNCTION__
#else
    #define DFHACK_FUNCTION_SIG __func__
#endif

#ifdef _WIN32
// On x86 MSVC, __thiscall passes |this| in ECX. On x86_64, __thiscall is the
// same as the standard calling convention.
// See https://docs.microsoft.com/en-us/cpp/cpp/thiscall for more info.
#define THISCALL __thiscall
#else
// On other platforms, there's no special calling convention for calling member
// functions.
#define THISCALL
#endif

namespace DFHack {
    class color_ostream;
}

template <typename T>
void print_bits ( T val, std::ostream& out )
{
    std::stringstream strs;
    T n_bits = sizeof ( val ) * CHAR_BIT;
    int cnt;
    for ( unsigned i = 0; i < n_bits; ++i )
    {
        cnt = i/10;
        strs << cnt << " ";
    }
    strs << std::endl;
    for ( unsigned i = 0; i < n_bits; ++i )
    {
        cnt = i%10;
        strs << cnt << " ";
    }
    strs << std::endl;
    for ( unsigned i = 0; i < n_bits; ++i )
    {
        strs << "--";
    }
    strs << std::endl;
    for ( unsigned i = 0; i < n_bits; ++i )
    {
        strs<< !!( val & 1 ) << " ";
        val >>= 1;
    }
    strs << std::endl;
    out << strs.str();
}

/*
 * Binary search in vectors.
 */

template <typename FT>
int linear_index(const std::vector<FT> &vec, FT key)
{
    for (unsigned i = 0; i < vec.size(); i++)
        if (vec[i] == key)
            return i;
    return -1;
}

template <typename FT>
int linear_index(const std::vector<FT*> &vec, const FT &key)
{
    for (unsigned i = 0; i < vec.size(); i++)
        if (vec[i] && *vec[i] == key)
            return i;
    return -1;
}

template <typename FT>
int binsearch_index(const std::vector<FT> &vec, FT key, bool exact = true)
{
    // Returns the index of the value >= the key
    int min = -1, max = (int)vec.size();
    const FT *p = vec.data();
    for (;;)
    {
        int mid = (min + max)>>1;
        if (mid == min)
            return exact ? -1 : max;
        FT midv = p[mid];
        if (midv == key)
            return mid;
        else if (midv < key)
            min = mid;
        else
            max = mid;
    }
}

template <typename CT, typename FT>
int linear_index(const std::vector<CT*> &vec, FT CT::*field, FT key)
{
    for (unsigned i = 0; i < vec.size(); i++)
        if (vec[i]->*field == key)
            return i;
    return -1;
}

template <typename CT, typename FT, typename MT>
int binsearch_index(const std::vector<CT*> &vec, FT MT::*field, FT key, bool exact = true)
{
    // Returns the index of the value >= the key
    int min = -1, max = (int)vec.size();
    CT *const *p = vec.data();
    for (;;)
    {
        int mid = (min + max)>>1;
        if (mid == min)
            return exact ? -1 : max;
        FT midv = p[mid]->*field;
        if (midv == key)
            return mid;
        else if (midv < key)
            min = mid;
        else
            max = mid;
    }
}

template <typename CT>
inline int binsearch_index(const std::vector<CT*> &vec, typename CT::key_field_type key, bool exact = true)
{
    return CT::binsearch_index(vec, key, exact);
}

template <typename CT>
inline int binsearch_index(const std::vector<CT*> &vec, typename CT::key_pointer_type key, bool exact = true)
{
    return CT::binsearch_index(vec, key, exact);
}

template<typename FT, typename KT>
inline bool vector_contains(const std::vector<FT> &vec, KT key)
{
    return binsearch_index(vec, key) >= 0;
}

template<typename CT, typename FT>
inline bool vector_contains(const std::vector<CT*> &vec, FT CT::*field, FT key)
{
    return binsearch_index(vec, field, key) >= 0;
}

template<typename T>
inline T vector_get(const std::vector<T> &vec, unsigned idx, const T &defval = T())
{
    if (idx < vec.size())
        return vec[idx];
    else
        return defval;
}

template<typename T>
inline void vector_insert_at(std::vector<T> &vec, unsigned idx, const T &val)
{
    vec.insert(vec.begin()+idx, val);
}

template<typename T>
inline void vector_erase_at(std::vector<T> &vec, unsigned idx)
{
    if (idx < vec.size())
        vec.erase(vec.begin()+idx);
}

template<typename FT>
unsigned insert_into_vector(std::vector<FT> &vec, FT key, bool *inserted = NULL)
{
    unsigned pos = (unsigned)binsearch_index(vec, key, false);
    bool to_ins = (pos >= vec.size() || vec[pos] != key);
    if (inserted) *inserted = to_ins;
    if (to_ins)
        vector_insert_at(vec, pos, key);
    return pos;
}

template<typename CT, typename FT, typename MT>
unsigned insert_into_vector(std::vector<CT*> &vec, FT MT::*field, CT *obj, bool *inserted = NULL)
{
    unsigned pos = (unsigned)binsearch_index(vec, field, obj->*field, false);
    bool to_ins = (pos >= vec.size() || vec[pos] != obj);
    if (inserted) *inserted = to_ins;
    if (to_ins)
        vector_insert_at(vec, pos, obj);
    return pos;
}

template<typename FT>
bool erase_from_vector(std::vector<FT> &vec, FT key)
{
    int pos = binsearch_index(vec, key);
    vector_erase_at(vec, pos);
    return pos >= 0;
}

template<typename CT, typename FT>
bool erase_from_vector(std::vector<CT*> &vec, FT CT::*field, FT key)
{
    int pos = binsearch_index(vec, field, key);
    vector_erase_at(vec, pos);
    return pos >= 0;
}

template <typename CT, typename KT>
CT *binsearch_in_vector(const std::vector<CT*> &vec, KT value)
{
    int idx = binsearch_index(vec, value);
    return idx < 0 ? NULL : vec[idx];
}

template <typename CT, typename FT>
CT *binsearch_in_vector(const std::vector<CT*> &vec, FT CT::*field, FT value)
{
    int idx = binsearch_index(vec, field, value);
    return idx < 0 ? NULL : vec[idx];
}

/*
 * List
 */

template<typename Link>
Link *linked_list_append(Link *head, Link *tail)
{
    while (head->next)
        head = head->next;
    head->next = tail;
    tail->prev = head;
    return tail;
}

template<typename Link>
Link *linked_list_insert_after(Link *pos, Link *link)
{
    link->next = pos->next;
    if (pos->next)
        pos->next->prev = link;
    link->prev = pos;
    pos->next = link;
    return link;
}

/**
 * Returns true if the item with id idToRemove was found, deleted, and removed
 * from the list. Otherwise returns false.
 */
template<typename Link>
bool linked_list_remove(Link *head, int32_t idToRemove)
{
    for (Link *link = head; link; link = link->next)
    {
        if (!link->item || link->item->id != idToRemove)
            continue;

        link->prev->next = link->next;
        if (link->next)
            link->next->prev = link->prev;
        delete(link);
        return true;
    }
    return false;
}

template<typename T>
inline typename T::mapped_type map_find(
    const T &map, const typename T::key_type &key,
    const typename T::mapped_type &defval = typename T::mapped_type()
) {
    auto it = map.find(key);
    return (it == map.end()) ? defval : it->second;
}

template <class T, typename Fn>
static void for_each_(std::vector<T> &v, Fn func)
{
    std::for_each(v.begin(), v.end(), func);
}

template <class T, class V, typename Fn>
static void for_each_(std::map<T, V> &v, Fn func)
{
    std::for_each(v.begin(), v.end(), func);
}

template <class T, class V, typename Fn>
static void transform_(const std::vector<T> &src, std::vector<V> &dst, Fn func)
{
    std::transform(src.begin(), src.end(), std::back_inserter(dst), func);
}

DFHACK_EXPORT bool prefix_matches(const std::string &prefix, const std::string &key, std::string *tail = NULL);

template<typename T>
typename T::mapped_type findPrefixInMap(
    const T &table, const std::string &key,
    const typename T::mapped_type& defval = typename T::mapped_type()
) {
    auto it = table.lower_bound(key);
    if (it != table.end() && it->first == key)
        return it->second;
    if (it != table.begin()) {
        --it;
        if (prefix_matches(it->first, key))
            return it->second;
    }
    return defval;
}

#ifdef __GNUC__
#define VARIABLE_IS_NOT_USED __attribute__ ((unused))
#else
#define VARIABLE_IS_NOT_USED
#endif

template<class CT>
inline bool static_add_to_map(CT *pmap, typename CT::key_type key, typename CT::mapped_type value) {
    (*pmap)[key] = value;
    return true;
}

#define CONCAT_TOKENS2(a,b) a##b
#define CONCAT_TOKENS(a,b) CONCAT_TOKENS2(a,b)
#define DFHACK_STATIC_ADD_TO_MAP(pmap,key,value) \
    static bool VARIABLE_IS_NOT_USED CONCAT_TOKENS(static_add_to_map_,__LINE__)\
        = static_add_to_map(pmap,key,value)

/*
 * MISC
 */

DFHACK_EXPORT bool split_string(std::vector<std::string> *out,
                                const std::string &str, const std::string &separator,
                                bool squash_empty = false);
DFHACK_EXPORT std::string join_strings(const std::string &separator, const std::vector<std::string> &items);

template<typename T>
inline std::string join_strings(const std::string &separator, T &items) {
    std::stringstream ss;

    bool first = true;
    for (auto &item : items) {
        if (first)
            first = false;
        else
            ss << separator;
        ss << item;
    }

    return ss.str();
}

DFHACK_EXPORT std::string toUpper(const std::string &str);
DFHACK_EXPORT std::string toLower(const std::string &str);
DFHACK_EXPORT std::string to_search_normalized(const std::string &str);

static inline std::string int_to_string(const int n) {
    std::ostringstream ss;
    ss << n;
    return ss.str();
}

static inline int string_to_int(const std::string s, int default_ = 0) {
    try {
        return std::stoi(s);
    }
    catch (std::exception&) {
        return default_;
    }
}

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char x){ return !std::isspace(x); }));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](char x){ return !std::isspace(x); }).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

enum word_wrap_whitespace_mode {
    WSMODE_KEEP_ALL,
    WSMODE_COLLAPSE_ALL,
    WSMODE_TRIM_LEADING
};

DFHACK_EXPORT bool word_wrap(std::vector<std::string> *out,
                             const std::string &str,
                             size_t line_length = 80,
                             word_wrap_whitespace_mode mode = WSMODE_KEEP_ALL);

inline bool bits_match(unsigned required, unsigned ok, unsigned mask)
{
    return (required & mask) == (required & mask & ok);
}

template<typename T, typename T1, typename T2>
inline T clip_range(T a, T1 minv, T2 maxv) {
    if (a < minv) return minv;
    if (a > maxv) return maxv;
    return a;
}

DFHACK_EXPORT int random_int(int max);

/**
 * Returns the amount of milliseconds elapsed since the UNIX epoch.
 * Works on both windows and linux.
 * source: http://stackoverflow.com/questions/1861294/how-to-calculate-execution-time-of-a-code-snippet-in-c
 */
DFHACK_EXPORT uint64_t GetTimeMs64();

DFHACK_EXPORT std::string stl_sprintf(const char *fmt, ...) Wformat(printf,1,2);
DFHACK_EXPORT std::string stl_vsprintf(const char *fmt, va_list args) Wformat(printf,1,0);

// Conversion between CP437 and UTF-8
DFHACK_EXPORT std::string UTF2DF(const std::string &in);
DFHACK_EXPORT std::string DF2UTF(const std::string &in);
DFHACK_EXPORT std::string DF2CONSOLE(const std::string &in);
DFHACK_EXPORT std::string DF2CONSOLE(DFHack::color_ostream &out, const std::string &in);

DFHACK_EXPORT std::string cxx_demangle(const std::string &mangled_name, std::string *status_out);
