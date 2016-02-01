#include "jsoncpp.h"
#pragma once

namespace Json {

    template <typename T> bool is (const Json::Value &val) { return false; }
    template <typename T> T as (const Json::Value &val);
    template <typename T> T get (const Json::Value &val, const std::string &key, const T &default_) { return default_; }

#define define_helpers(type, is_func, as_func) \
        template<> inline bool is<type> (const Json::Value &val) { return val.is_func(); } \
        template<> inline type as<type> (const Json::Value &val) { return val.as_func(); } \
        template<> inline type get<type> (const Json::Value &val, const std::string &key, const type &default_) \
            { const Json::Value &x = val[key]; return is<type>(x) ? as<type>(x) : default_; }
    define_helpers(bool,         isBool,    asBool);
    define_helpers(Json::Int,    isInt,     asInt);
    define_helpers(Json::UInt,   isUInt,    asUInt);
    define_helpers(Json::Int64,  isInt64,   asInt64);
    define_helpers(Json::UInt64, isUInt64,  asUInt64);
    define_helpers(float,        isDouble,  asFloat);
    define_helpers(double,       isDouble,  asDouble);
    define_helpers(std::string,  isString,  asString);
#undef define_helpers

    inline std::string toSimpleString (const Json::Value &val)
    {
        Json::FastWriter w;
        return w.write(val);
    }

}
