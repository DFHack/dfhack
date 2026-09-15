#ifndef TRANS_HASH_H
#define TRANS_HASH_H

namespace
{
    template<typename ... Bases>
    struct overload : Bases ...
    {
        using is_transparent = void;
        using Bases::operator() ...;
    };

    struct char_pointer_hash
    {
        auto operator()(const char* ptr) const noexcept
        {
            return std::hash<std::string_view>{}(ptr);
        }
    };

    using transparent_string_hash = overload<
        std::hash<std::string>,
        std::hash<std::string_view>,
        char_pointer_hash
    >;
}

#endif
