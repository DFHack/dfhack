/* vim: set et sw=3 tw=0 fo=croqlaw cino=t0:
 *
 * Luaxx, the C++ Lua wrapper library.
 * Copyright (c) 2006-2008 Matthew Nicholson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef LUAXX_H
#define LUAXX_H

#define lua_Integer_long 1
#define lua_Integer_int 1


#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


#include <string>
#include <vector>
#include <new>
#include <exception>

/** @file
 * Luaxx header file.
 */

/** @mainpage Luaxx
 *
 * Luaxx is a thin wrapper around the Lua C API.  The wrapper adds some
 * convenience functions and integrates well with modern C++.
 *
 * Luaxx is not designed like toLua, instead Luaxx is more of a 1 to 1
 * logical mapping of the lua API in C++.  For example: in C you would write
 * 'lua_pushnumber(L, 3)', in C++ with Luaxx you would write
 * 'L.push(3)'.
 *
 * Every thing is contained in the 'lua' namespace and exceptions are thrown
 * when a lua API function returns an error.  Most of the functionality is
 * contained in the lua::state class, which can be passed directly to lua C API
 * functions (the compiler will automatically use the internal lua_State
 * pointer).  See the documentation for that class for more information.
 */

namespace lua
{
    void StackDump(lua_State *L);
   /** A generic lua exception.
    */
   class exception : public std::exception {
      public:
         /// Constructor.
         exception() : std::exception() { }
         /// Constructor.
         explicit exception(const char* desc) : std::exception(), description(desc) { }
         virtual ~exception() throw() { }
         /** Get a description of the error.
          * @returns a C-string describing the error
          */
         virtual const char* what() const throw() {
            return description.c_str();
         }
      private:
         std::string description;
   };

   /** A lua runtime error.
    * This is thrown when there was an error executing some lua code.
    * @note This is not an std::runtime error.
    */
   class runtime_error : public exception {
      public:
         /// Constructor.
         runtime_error() : exception() { }
         /// Constructor.
         explicit runtime_error(const char* desc) : exception(desc) { }
         virtual ~runtime_error() throw() { }
   };

   /** A syntax error.
    */
   class syntax_error : public exception {
      public:
         /// Constructor.
         syntax_error() : exception() { }
         /// Constructor.
         explicit syntax_error(const char* desc) : exception(desc) { }
         virtual ~syntax_error() throw() { }
   };

   /** An error loading a lua file.
    * This is thrown when a call to lua::loadfile failed because the file could
    * not be opened or read.
    */
   class file_error : public exception {
      public:
         /// Constructor.
         file_error() : exception() { }
         /// Constructor.
         explicit file_error(const char* desc) : exception(desc) { }
         virtual ~file_error() throw() { }
   };

   /** A memory allocation error.
    */
   class bad_alloc : public exception, std::bad_alloc {
      public:
         /// Constructor.
         bad_alloc() : lua::exception(), std::bad_alloc() { }
         /// Constructor.
         explicit bad_alloc(const char* desc) : lua::exception(desc), std::bad_alloc() { }
         virtual ~bad_alloc() throw() { }
   };

   /** An error converting a lua type.
    */
   class bad_conversion : public exception {
      public:
         /// Constructor.
         bad_conversion() : exception() { }
         /// Constructor.
         explicit bad_conversion(const char* desc) : exception(desc) { }
         virtual ~bad_conversion() throw() { }
   };

   /// A Lua table (this class does not have any data).
   class table { };
   /// A Lua nil (this class does not have any data).
   class nil { };
   /// A lua function (not a cfunction).
   class function { };
   /// A lua userdatum
   class userdata { };
   /// A lua light userdatum
   class lightuserdata { };

   typedef lua_CFunction cfunction;  ///< A cfunction on the lua statck
   typedef lua_Integer integer;      ///< The default lua integer type
   typedef lua_Number number;        ///< The default lua number type
   typedef lua_Reader reader;        ///< The type of function used by lua_load
   const int multiret = LUA_MULTRET; ///< LUA_MULTIRET

   /** This is the Luaxx equivalent of lua_State.
    * The functions provided by this class, closely resemble those of the Lua C
    * API.
    */
    void StackDump(lua_State *L);
   class state {
      public:
         state();
         state(lua_State* L);
         state(const state& t)
         {
             managed=false;
             L=t.L;
         }
         state& operator = (const state& t);
         ~state();

         operator lua_State*();

         state& push();
         state& push(nil);
         state& push(bool boolean);
         template<typename T> state& push(T number);
         state& push(const char* s, size_t length);
         state& push(const char* s);
         state& push(const std::string& s);
         state& push(cfunction f);
         state& push(table);
         state& push(void* p);
         template<typename T> state& pushlightuserdata(T p);

         template<typename T> state& to(T& number, int index = -1);
         template<typename T> state& touserdata(T& p, int index = -1);

         template<typename T> T as(T default_value, int index = -1);
         template<typename T> T as(int index = -1);
         template<typename T> T vpop()
         {
             T ret;
             ret=as<T>();
             pop();
             return ret;
         }
         template<typename T> bool is(int index = -1);

         state& check(int narg);
#ifndef lua_Integer_int
         state& check(int& i, int narg);
#endif
         state& check(integer& i, int narg);
#ifndef lua_Integer_long
         state& check(long& l, int narg);
#endif
         state& check(std::string& s, int narg);
         state& check(number& n, int narg);

         template<typename msg_t> void error(msg_t message);
#if 0
         template<> void error(const std::string& message);
#endif

         state& pcall(int nargs = 0, int nresults = 0, int on_error = 0);
         state& call(int nargs = 0, int nresults = 0);

         state& checkstack(int size);
         state& settop(int index);
         int gettop();
         int size();
         bool empty();

         state& insert(int index);
         state& replace(int index);
         state& remove(int index);
         state& pop(int elements = 1);

         state& pushvalue(int index);

         state& newtable();
         bool newmetatable(const std::string& tname);
         template<typename userdata_t> userdata_t* newuserdata();
         void* newuserdata(size_t nbytes);

         state& gettable(int index = -2);
         state& getfield(const std::string& k, int index = -1);
         state& settable(int index = -3);
         state& setfield(const std::string& k, int index = -2);
         state& getmetatable(const std::string& tname);
         bool getmetatable(int index);

         bool next(int index = -2);

         state& getglobal(const std::string& name);
         state& setglobal(const std::string& name);

         state& loadfile(const std::string& filename);
         state& loadstring(const std::string& s);

         template<typename iterator_t> state& load(iterator_t begin, iterator_t end);

         size_t objlen(int index = -1);

      private:
         lua_State* L;
         bool managed;



         int throw_error(int code);
   };

   // template functions

   /** Push a number onto the stack.
    * @tparam T the numeric type to push (should be automatically determined,
    * if there is no specialization for the desired type, lua_pushnumber() will
    * be used, and may fail)
    * @param number the number to push
    * @returns a reference to this lua::state
    */
   template<typename T>
   state& state::push(T number) {
      lua_pushnumber(L, number);
      return *this;
   }

   /** Push a light userdatum on to the stack.
    * @tparam T the type of data to push (should be automatically determined)
    * @param p the pointer to push
    * @returns a reference to this lua::state
    */
   template<typename T>
   state& state::pushlightuserdata(T p) {
      lua_pushlightuserdata(L, p);
      return *this;
   }

   /** Check if the given index is of the given type (defaults to using
    * lua_isnumber()).
    * @tparam T the type to check for (the default, if no specializations
    * match, does lua_isnumber())
    * @param index the index to check
    * @note the default version (used if no specialization is matched) will
    * check if the given value is a number
    * @returns whether the value at the given index is a nil
    */
   template<typename T>
   bool state::is(int index) {
      return lua_isnumber(L, index);
   }

   /** Get the value at index as the given numeric type.
    * @tparam T they numeric type to static_cast<T>() the numeric value on the
    * stack to
    * @param number where to store the value
    * @param index the index to get
    * @note This function does \em not pop the value from the stack.
    * @todo Instead of throwing an exception here, we may just return an
    * error code.
    * @throws lua::bad_conversion if the value on the stack could not be
    * converted to the indicated type
    * @returns a reference to this lua::state
    */
   template<typename T>
   state& state::to(T& number, int index) {
      if (lua_isnumber(L, index))
         number = static_cast<T>(lua_tonumber(L, index));
      else
         throw bad_conversion("Cannot convert non 'number' value to number");

      return *this;
   }

   /** Get the value at index as (light) userdata.
    * @tparam T the type of data pointed to (pointer is returned as
    * reinterpret_cast<T>())
    * @param p the pointer to store the value in
    * @param index the index to get
    * @note This function does \em not pop the value from the stack.
    * @todo Instead of throwing an exception here, we may just return an
    * error code.
    * @throws lua::bad_conversion if the value on the stack could not be
    * converted to the indicated type
    * @returns a reference to this lua::state
    */
   template<typename T>
   state& state::touserdata(T& p, int index) {
      if (lua_isuserdata(L, index))
         p = reinterpret_cast<T>(lua_touserdata(L, index));
      else
         throw bad_conversion("Cannot convert non 'userdata' or 'lightuserdata' value to userdata");

      return *this;
   }

   /** Get the value at index as the given type.
    * @tparam T the type to retrieve the value on the stack as (the default
    * template function uses lua_tonumber(), specializations may cause
    * different behavior)
    * @param default_value this value is returned if the conversion fails
    * @param index the index to get
    * @note This function does \em not pop the value from the stack.
    * @returns the indicated value from the stack or the default value if
    * the conversion fails
    */
   template<typename T>
   T state::as(T default_value, int index) {
      if (lua_isnumber(L, index))
         return static_cast<T>(lua_tonumber(L, index));
      else
         return default_value;
   }

   /** Get the value at index as the given type.
    * @tparam T the expected type of the value
    * @param index the index to get
    *
    * @note This function does \em not pop the value from the stack.
    * @note The default version of this function uses lua_tonumber() but
    * specializations may cause different behavior.
    *
    * @todo Instead of throwing an exception here, we may just return an
    * error code.
    *
    * @throws lua::bad_conversion if the value on the stack could not be
    * converted to the indicated type
    *
    * This function will return the value on the stack as the given type.  If
    * the value is not of the given type <em>no conversion will be
    * performed</em> and lua::bad_conversion will be thrown.  There are some
    * exceptions to this rule, for example, numbers will be converted to
    * strings and vice-versa (conversion is only performed if the matching
    * lua_is*() function returns true).  The state::to() function should be
    * used to perform automatic conversion.
    *
    * @returns the indicated value as the given type if possible
    */
   template<typename T>
   T state::as(int index) {
      if (lua_isnumber(L, index))
         return static_cast<T>(lua_tonumber(L, index));
      else
         throw bad_conversion("Cannot convert non 'number' value to number");
   }

   /** Create a new userdatum on the stack.
    * @tparam userdata_t the type of the userdata (will be passed to sizeof())
    *
    * This function creates a new userdatum on the stack the size of
    * userdata_t and return a pointer to it.
    * @returns a pointer to the new userdatum
    */
   template<typename userdata_t>
   userdata_t* state::newuserdata() {
      return reinterpret_cast<userdata_t*>(lua_newuserdata(L, sizeof(userdata_t)));
   }

   /** Generate a Lua error.
    * @tparam msg_t the type of error message data (should be automatically
    * determined)
    * @param message the error message/value
    * @note This function is used to raise errors from lua::cfunctions.
    * @note This function never returns, instead it throws an exception
    * caught by the intepreter.
    */
   template<typename msg_t>
   void state::error(msg_t message) {
      push(message);
      lua_error(L);
   }

   /** Load a sequence of data as a Lua chunk.
    * @tparam iterator_t the type of iterator to use (should be automatically
    * determined)
    * @param begin an iterator to the start of the sequence
    * @param end an iterator to the end of the sequence (one past the
    * end)
    *
    * This function takes a sequence of data and attempts to convert it
    * into a Lua chunk.  The type of data passed must be able to be
    * converted into an 8-bit char.
    *
    * @note This function should automatically detect if the data is text
    * or binary.
    *
    * @returns a reference to this lua::state
    */
   template<typename iterator_t>
   state& state::load(iterator_t begin, iterator_t end) {
      // convert the data to characters
      std::vector<char> chunk(begin, end);

      // Here we use the address of the first element of our vector.
      // This works because the data in std::vectors is contiguous.
      throw_error(luaL_loadbuffer(L, &(*chunk.begin()), chunk.size(), NULL));
      return *this;
   }


   // template specializations
   template<> state& state::to(bool& boolean, int index);
   template<> state& state::to(std::string& string, int index);

   template<> bool state::as(bool default_value, int index);
   template<> std::string state::as(std::string default_value, int index);
   template<> bool state::as(int index);
   template<> std::string state::as(int index);

   template<> bool state::is<nil>(int index);
   template<> bool state::is<bool>(int index);
   template<> bool state::is<std::string>(int index);
   template<> bool state::is<table>(int index);
   template<> bool state::is<cfunction>(int index);
   template<> bool state::is<function>(int index);
   template<> bool state::is<userdata>(int index);
   template<> bool state::is<lightuserdata>(int index);


   // inline functions

   /** Convert a lua::state to a lua_State*.
    * This operator allows lua::state to behave like a lua_State
    * pointer.
    *
    * @note This should be used as a last result to interoperate with C
    * code.  This may be removed in future versions of Luaxx.
    */
   inline state::operator lua_State*() {
      return L;
   }

   /** Throws exceptions for error return codes.
    * @param code the return code
    *
    * This function throws an exception based on the error it was passed.
    * If it is passed a 0 it will not throw anything.
    *
    * @todo In the future this function may check an exception mask
    * before throwing an error.
    *
    * @returns the code it was passed
    */
   inline int state::throw_error(int code) {
      std::string error;

      // below, we package lua errors into exceptions
      switch (code) {
         case 0:
            break;
         case LUA_ERRSYNTAX:
            to(error).pop();
            throw syntax_error(error.c_str());
         case LUA_ERRMEM:
            to(error).pop();
            throw bad_alloc(error.c_str());
         case LUA_ERRRUN:
            to(error).pop();
            throw runtime_error(error.c_str());
         case LUA_ERRFILE:
            to(error).pop();
            throw file_error(error.c_str());
         default:
            to(error).pop();
            throw exception(error.c_str());
      }
      return code;
   }

}

#endif
