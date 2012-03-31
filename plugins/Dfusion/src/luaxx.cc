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

#include <luaxx.hpp>
#include <iostream>
#define LOG std::cout
/** @file
 * Luaxx implementation file.
 */

namespace lua
{
    void StackDump(lua_State *L)
    {
        int i;
        int top = lua_gettop(L);
        for (i = 1; i <= top; i++)    /* repeat for each level */
        {
            int t = lua_type(L, i);
            LOG<<i<<":";
            switch (t)
            {

            case LUA_TSTRING:  /* strings */
                LOG<<"str ="<<lua_tostring(L, i)<<"\n";
                break;

            case LUA_TBOOLEAN:  /* booleans */
                LOG<<"bool="<<(lua_toboolean(L, i) ? "true" : "false")<<"\n";
                break;

            case LUA_TNUMBER:  /* numbers */
                LOG<<"num ="<<lua_tonumber(L, i)<<"\n";
                break;
            case LUA_TTABLE:

                LOG<<lua_typename(L, t);
                {
                    //LOG<<"PRE TOP:"<< lua_gettop(L)<<"\n";
                    lua_getglobal(L,"PrintTable");
                    //lua_insert(L,-2);
                    lua_pushvalue(L,i);
                    lua_pcall(L,1,0,0);
                    //LOG<<"POST TOP:"<< lua_gettop(L)<<"\n";
                }

                break;
            default:  /* other values */
                LOG<<lua_typename(L, t);
                {
                    //LOG<<"PRE TOP:"<< lua_gettop(L)<<"\n";
                    lua_getglobal(L,"tostring");
                    //lua_insert(L,-2);
                    lua_pushvalue(L,i);
                    lua_pcall(L,1,1,0);
                    LOG<<"=";
                    LOG<<lua_tostring(L,-1)<<"\n";
                    lua_pop(L,1);
                    //LOG<<"POST TOP:"<< lua_gettop(L)<<"\n";
                }

                break;

            }

        }
        LOG<<"\n";  /* end the listing */
        LOG<<"==============================\n";
    }
#undef LOG
   /// Construct our lua environment.
   state::state() : L(luaL_newstate()), managed(true) {
      if (L == NULL)
         throw bad_alloc("Error creating lua state");
   }

   /** Construct our lua environment from an existing lua_State.
    * @param L the existing state to use.
    *
    * This function differs from the normal constructor as it sets a flag
    * that prevents lua_close() from being called when this class is
    * destroyed.
    */
   state::state(lua_State* L) :
      L(L), managed(false) {
   }
   state& state::operator = (const state& t)
    {
             if(managed)
                lua_close(L);
             managed=false;
             L=t.L;
			 return *this;
        }
   /// Destroy our lua environment.
   state::~state() {
      if (managed)
         lua_close(L);
   }

   /** Push a nil onto the stack.
    * @returns a reference to this lua::state
    */
   state& state::push() {
      lua_pushnil(L);
      return *this;
   }

   /** Push a nil onto the stack.
    * @returns a reference to this lua::state
    */
   state& state::push(nil) {
      lua_pushnil(L);
      return *this;
   }

   /** Push a boolean onto the stack.
    * @param boolean the value to push
    * @returns a reference to this lua::state
    */
   state& state::push(bool boolean) {
      lua_pushboolean(L, boolean);
      return *this;
   }

   /** Push a C-style string onto the stack.
    * @param s the string to push
    * @param length the length of the string
    * @returns a reference to this lua::state
    */
   state& state::push(const char* s, size_t length) {
      lua_pushlstring(L, s, length);
      return *this;
   }

   /** Push a C-style string onto the stack.
    * @param s the string to push
    * @note This must be a '0' terminated string.
    * @returns a reference to this lua::state
    */
   state& state::push(const char* s) {
      lua_pushstring(L, s);
      return *this;
   }

   /** Push an std::string onto the stack.
    * @param s the string to push
    * @returns a reference to this lua::state
    */
   state& state::push(const std::string& s) {
      lua_pushlstring(L, s.c_str(), s.size());
      return *this;
   }

   /** Push an C function onto the stack.
    * @param f the function to push
    * @returns a reference to this lua::state
    */
   state& state::push(cfunction f) {
      lua_pushcfunction(L, f);
      return *this;
   }

   /** Create a new table on to the stack.
    * @see state::newtable()
    * @returns a reference to this lua::state
    */
   state& state::push(table) {
      lua_newtable(L);
      return *this;
   }

   /** Push a light userdatum on to the stack.
    * @param p the pointer to push
    * @returns a reference to this lua::state
    */
   state& state::push(void* p) {
      lua_pushlightuserdata(L, p);
      return *this;
   }

   /** Get the value at index as a string.
    * @param default_value this value is returned if the conversion fails
    * @param index the index to get
    * @note This function does \em not pop the value from the stack.
    *
    * @note lua::state::as(std::string()) will convert the value at the
    * indicated index to a string <em>on the stack</em>.  This can
    * confuse lua::state::next();
    *
    * @returns the indicated value from the stack or the default value if the
    * conversion fails
    */
   template<>
   std::string state::as(std::string default_value, int index) {
      if (lua_isstring(L, index))
      {
         size_t len;
         const char *str = lua_tolstring(L, index, &len);
         return std::string(str, len);
      }
      else
         return default_value;
   }

   /** Check an argument of the current function.
    * @param narg the argument number to check
    *
    * This function will throw a lua error if there is no argument at the
    * given position.
    *
    * @note This function is meant to be called from with in a lua::cfunction.
    * The error throw is internal to the lua interpreter.  When compiled as
    * C++, a C++ exception is thrown, so the stack is properly unwound.  This
    * exception is not meant to be caught.
    */
   state& state::check(int narg) {
      luaL_checkany(L, narg);
      return *this;
   }

#ifndef lua_Integer_int
   /** Check an argument of the current function.
    * @param i the int to hold the returned value
    * @param narg the argument number to check
    *
    * This function checks if the given argument number is an int.
    *
    * @note This function is meant to be called from with in a
    * lua::cfunction.  The error throw is internal to the lua intrepeter.
    * When compiled as C++, a C++ exception is thrown, so the stack is
    * properly unwound.  This exception is not meant to be caught.
    */
   state& state::check(int& i, int narg) {
      i = luaL_checkint(L, narg);
      return *this;
   }
#endif

   /** Check an argument of the current function.
    * @param i the lua::integer (lua_Integer) to hold the returned value
    * @param narg the argument number to check
    *
    * This function checks if the given argument number is an integer.
    *
    * @note This is different from lua::check(int(), ...).  It returns a
    * lua::integer (lua_Integer), which may not be an int.
    *
    * @note This function is meant to be called from with in a
    * lua::cfunction.  The error throw is internal to the lua intrepeter.
    * When compiled as C++, a C++ exception is thrown, so the stack is
    * properly unwound.  This exception is not meant to be caught.
    */
   state& state::check(integer& i, int narg) {
      i = luaL_checkinteger(L, narg);
      return *this;
   }

#ifndef lua_Integer_long
   /** Check an argument of the current function.
    * @param l the long to hold the returned value
    * @param narg the argument number to check
    *
    * This function checks if the given argument number is a long.
    *
    * @note This function is meant to be called from with in a
    * lua::cfunction.  The error throw is internal to the lua intrepeter.
    * When compiled as C++, a C++ exception is thrown, so the stack is
    * properly unwound.  This exception is not meant to be caught.
    */
   state& state::check(long& l, int narg) {
      l = luaL_checklong(L, narg);
      return *this;
   }
#endif

   /** Check an argument of the current function.
    * @param s the string to hold the returned value
    * @param narg the argument number to check
    *
    * This function checks if the given argument number is a string.
    *
    * @note This function is meant to be called from with in a
    * lua::cfunction.  The error throw is internal to the lua intrepeter.
    * When compiled as C++, a C++ exception is thrown, so the stack is
    * properly unwound.  This exception is not meant to be caught.
    */
   state& state::check(std::string& s, int narg) {
      const char* c;
      size_t l;
      c = luaL_checklstring(L, narg, &l);

      s.assign(c, l);

      return *this;
   }

   /** Check an argument of the current function.
    * @param n the lua::number (lua_Number) to hold the returned value
    * @param narg the argument number to check
    *
    * This function checks if the given argument number is a lua::number
    * (lua_Number, a double by default).
    *
    * @note This function is meant to be called from with in a lua::cfunction.
    * The error throw is internal to the lua interpreter.  When compiled as
    * C++, a C++ exception is thrown, so the stack is properly unwound.  This
    * exception is not meant to be caught.
    */
   state& state::check(number& n, int narg) {
      n = luaL_checknumber(L, narg);
      return *this;
   }

#if 0
   /** [specialization] Generate a Lua error (T = std::string).
    */
   template<>
   void state::error(const std::string& message) {
      push(message);
      lua_error(L);
   }
#endif

   /** Call a lua function.
    * @param nargs the number of args to pass to the function
    * @param nresults the number of values to return from the function
    * @param on_error A stack index where the error handling function is
    * stored.
    * @note The error handling function must be pushed in the stack
    * before the function to be called and its arguments.
    * @returns a reference to this lua::state
    */
   state& state::pcall(int nargs, int nresults, int on_error) {
      throw_error(lua_pcall(L, nargs, nresults, on_error));
      return *this;
   }

   /** Call a lua function in unprotected mode.
    * @param nargs the number of args to pass to the function
    * @param nresults the number of values to return from the function
    * stored.
    * @note If there is an error in the call the program will terminate.
    * @returns a reference to this lua::state
    */
   state& state::call(int nargs, int nresults) {
      lua_call(L, nargs, nresults);
      return *this;
   }

   /** Ensure the stack is at least the given size.
    * @param size the size to use
    *
    * If the stack is smaller than the given size, it will grow to the
    * specified size.
    *
    * @exception lua::exception Thrown if the operation fails.
    * @returns a reference to this lua::state
    */
   state& state::checkstack(int size) {
      if (!lua_checkstack(L, size))
         throw lua::exception("Error growing the stack");
      return *this;
   }

   /** Set a new index as the top of the stack.
    * @param index the index to use as the new top
    * @note If the previous top was higher than the new one, top values
    * are discarded.  Otherwise this function pushs nils on to the stack
    * to get the proper size.
    * @returns a reference to this lua::state
    */
   state& state::settop(int index) {
      lua_settop(L, index);
      return *this;
   }

   /** Get the number of elements in the stack.
    * @note This value is also the index of the top element.
    * @returns the number of elements in the stack
    */
   int state::gettop() {
      return lua_gettop(L);
   }

   /** Get the number of elements in the stack.
    * @note This value is also the index of the top element.
    * @returns the number of elements in the stack
    */
   int state::size() {
      return lua_gettop(L);
   }

   /** Check if the stack is empty.
    * @returns true if the stack is empty, false otherwise
    */
   bool state::empty() {
      return !lua_gettop(L);
   }

   /** Move the top element to the given index.
    * @param index the index to insert at
    * @note All elements on top of the given index are shifted up to open
    * space for this element.
    * @returns a reference to this lua::state
    */
   state& state::insert(int index) {
      lua_insert(L, index);
      return *this;
   }

   /** Replace the given index with the top element.
    * @param index the index to replae
    * @returns a reference to this lua::state
    */
   state& state::replace(int index) {
      lua_replace(L, index);
      return *this;
   }

   /** Remove the given index from the stack.
    * @param index the index to remove
    * @note Elements are shifted down to fill in the empty spot.
    * @returns a reference to this lua::state
    */
   state& state::remove(int index) {
      lua_remove(L, index);
      return *this;
   }

   /** Remove the given number of elemens from the stack.
    * @param elements the number of elements to remove
    * @returns a reference to this lua::state
    */
   state& state::pop(int elements) {
      lua_pop(L, elements);
      return *this;
   }

   /** Push a copy of the element at the given index to the top of the
    * stack.
    * @param index the index of the element to copy
    * @returns a reference to this lua::state
    */
   state& state::pushvalue(int index) {
      lua_pushvalue(L, index);
      return *this;
   }

   /** Create a new table on the stack.
    * @returns a reference to this lua::state
    */
   state& state::newtable() {
      lua_newtable(L);
      return *this;
   }

   /** Create a new metatable and add it to the registry.
    * @param tname the name to use for the new metatable in the registry
    *
    * This function creates a new metatable and adds it to the registry
    * with the given key.  This function also pushes the new metatable
    * onto the stack.
    *
    * @note Regardless of the return value, the new metatable is always pushed
    * on to the stack.
    *
    * @return true if a new metatable was created, false if the registry
    * alread has a key with the given name.
    */
   bool state::newmetatable(const std::string& tname) {
      return luaL_newmetatable(L, tname.c_str());
   }

   /** Create a new userdatum on the stack.
    * @param nbytes the size of the new userdatum
    * @return a pointer to the new userdatum
    */
   void* state::newuserdata(size_t nbytes) {
      return lua_newuserdata(L, nbytes);
   }

   /** Get a value from a table on the stack.
    * @param index the index the table is stored at
    *
    * This function gets a value from the table at the given index and
    * pushes it onto the stack.
    *
    * @note You should have already pushed the key used to reference this
    * value onto the stack before calling this function.
    *
    * @returns a reference to this lua::state
    */
   state& state::gettable(int index) {
      lua_gettable(L, index);
      return *this;
   }

   /** Get a value from a table on the stack.
    * @param k the key
    * @param index the index the table is stored at
    *
    * This function gets a value from the table at the given index and
    * pushes it onto the stack.
    *
    * @returns a reference to this lua::state
    */
   state& state::getfield(const std::string& k, int index) {
      lua_getfield(L, index, k.c_str());
      return *this;
   }

   /** Set a value in a table.
    * @param index the index the table is stored at
    *
    * This function sets a value in a table stored at the given index.
    *
    * @note The key and value to be used should have already been pushed
    * on the stack in that order.
    *
    * @returns a reference to this lua::state
    */
   state& state::settable(int index) {
      lua_settable(L, index);
      return *this;
   }

   /** Set a field in a table.
    * @param k the key
    * @param index the index the table is stored at
    *
    * This function sets a value in a table stored at the given index.
    *
    * @note The value to be used should be on the top of the stack.
    *
    * @returns a reference to this lua::state
    */
   state& state::setfield(const std::string& k, int index) {
      lua_setfield(L, index, k.c_str());
      return *this;
   }

   /** Get the metatable associated with the given registry entry.
    * @param tname the name in the registry
    *
    * This function gets the metatable associated with the given key in
    * the registry.  The resulting metatable is pushed onto the stack.
    *
    * @note This function uses luaL_getmetatable() internally.
    *
    * @returns a reference to this lua::state
    */
   state& state::getmetatable(const std::string& tname) {
      luaL_getmetatable(L, tname.c_str());
      return *this;
   }

   /** Get the metatable of the value at the given index.
    * @param index the index the value is stored at
    *
    * This function pushes on to the stack the metatabe of the value at
    * the given index.
    *
    * @note This function uses lua_getmetatable() internally.
    *
    * @returns false if the value at the given index does not have a
    * metatable or if the index is not valid
    */
   bool state::getmetatable(int index) {
      return lua_getmetatable(L, index);
   }

   /** Get the next key value pair from a table on the stack.
    * @param index the stack index the table is at
    *
    * This function pops a key from the stack and pushes the next key
    * value pair to the stack.  The key will be stored at index -2 and
    * the value will be at index -1.  The key is expected to be on the
    * top of the stack.
    *
    * @note While traversing a table, do not call
    * lua::state::to(std::string()) directly on a key, unless you know
    * that the key is actually a string.  lua::state::to(std::string())
    * changes the value at the given index; this confuses the next call
    * to lua::state::next().
    *
    * <strong>While Loop Example:</strong>
    * @code
    * while(L.next() != 0) {
    *    // do stuff
    *    L.pop();
    * }
    * @endcode
    *
    * <strong>For Loop Example:</strong>
    * @code
    * for(L.push(lua::nil()); L.next(); L.pop()) {
    *    // do stuff
    * }
    * @endcode
    *
    * @returns true as long as there are remaining items in the table
    */
   bool state::next(int index) {
      return lua_next(L, index);
   }

   /** Load a global symbol onto the stack.
    * @param name the name of the global to load
    *
    * This function loads a global symbol onto the stack from the lua
    * state.
    *
    * @returns a reference to this lua::state
    */
   state& state::getglobal(const std::string& name) {
      lua_getglobal(L, name.c_str());
      return *this;
   }

   /** Set a global symbol.
    * @param name the name of the global to set
    *
    * This function sets/creates a global symbol from the value above it
    * on the stack.
    *
    * @note You should have pushed the value of the symbol onto the stack
    * before calling this function.
    *
    * @returns a reference to this lua::state
    */
   state& state::setglobal(const std::string& name) {
      lua_setglobal(L, name.c_str());
      return *this;
   }

   /** Load a file as a Lua chunk.
    * @param filename the name of the file to load
    * @returns a reference to this lua::state
    */
   state& state::loadfile(const std::string& filename) {
      throw_error(luaL_loadfile(L, filename.c_str()));
      return *this;
   }

   /** Load a string as a Lua chunk.
    * @param s the string to load
    * @returns a reference to this lua::state
    */
   state& state::loadstring(const std::string& s) {
      throw_error(luaL_loadstring(L, s.c_str()));
      return *this;
   }

   /** Get the length of a value on the stack.
    * @param index the index the value is stored at
    * @returns the length of the indicated value
    */
   size_t state::objlen(int index) {
      return lua_rawlen(L, index);
   }

   /** Get the value at index as a bool.
    * @param boolean where to store the value
    * @param index the index to get
    * @note This function does \em not pop the value from the stack.
    * @todo Instead of throwing an exception here, we may just return an
    * error code.
    * @throws lua::bad_conversion if the value on the stack could not be
    * converted to the indicated type
    * @returns a reference to this lua::state
    */
   template<>
   state& state::to(bool& boolean, int index) {
      if (lua_isboolean(L, index))
         boolean = lua_toboolean(L, index);
      else
         throw bad_conversion("Cannot convert non 'boolean' value to bool");

      return *this;
   }

   /** Get the value at index as a string.
    * @param string where to store the value
    * @param index the index to get
    * @note This function does \em not pop the value from the stack.
    * @todo Instead of throwing an exception here, we may just return an
    * error code.
    *
    * @note lua::state::to(std::string()) will convert the value at the
    * indicated index to a string <em>on the stack</em>.  This can
    * confuse lua::state::next();
    *
    * @throws lua::bad_conversion if the value on the stack could not be
    * converted to the indicated type
    * @returns a reference to this lua::state
    */
   template<>
   state& state::to(std::string& string, int index) {
      if (lua_isstring(L, index))
      {
          size_t len;
          const char *str = lua_tolstring(L, index, &len);
          string.replace(0, std::string::npos, str, len);
      }
      else
         throw bad_conversion("Cannot convert value to string");

      return *this;
   }

   /** Get the value at index as a bool.
    * @param default_value this value is returned if the conversion fails
    * @param index the index to get
    * @note This function does \em not pop the value from the stack.
    * @returns the indicated value from the stack or the default value if the
    * conversion fails
    */
   template<>
   bool state::as(bool default_value, int index) {
      if (lua_isboolean(L, index))
         return lua_toboolean(L, index);
      else
         return default_value;
   }

   /** [specialization] Get the value at index as a bool (T = bool).
    */
   template<>
   bool state::as(int index) {
      if (lua_isboolean(L, index))
         return lua_toboolean(L, index);
      else
         throw bad_conversion("Cannot convert non 'boolean' value to bool");
   }

   /** [specialization] Get the value at index as a string (T = std::string).
    * @note lua::state::as(std::string()) will convert the value at the
    * indicated index to a string <em>on the stack</em>.  This can confuse
    * lua::state::next();
    */
   template<>
   std::string state::as(int index) {
      if (lua_isstring(L, index))
      {
          size_t len;
          const char *str = lua_tolstring(L, index, &len);
          return std::string(str, len);
      }
      else
         throw bad_conversion("Cannot convert value to string");
   }

   /** [specialization] Check if the given index is a nil (T = lua::nil).
    */
   template<>
   bool state::is<nil>(int index) {
      return lua_isnil(L, index);
   }

   /** [specialization] Check if the given index is a boolean (T = bool).
    */
   template<>
   bool state::is<bool>(int index) {
      return lua_isboolean(L, index);
   }

   /** [specialization] Check if the given index is a string (T = std::string).
    */
   template<>
   bool state::is<std::string>(int index) {
      return lua_isstring(L, index);
   }

   /** [specialization] Check if the given index is a table (T = lua::table).
    */
   template<>
   bool state::is<table>(int index) {
      return lua_istable(L, index);
   }

   /** [specialization] Check if the given index is a C function (T =
    * lua::cfunction).
    */
   template<>
   bool state::is<cfunction>(int index) {
      return lua_iscfunction(L, index);
   }

   /** [specialization] Check if the given index is a function (T =
    * lua::function).
    */
   template<>
   bool state::is<function>(int index) {
      return lua_isfunction(L, index);
   }

   /** [specialization] Check if the given index is userdata (T =
    * lua::userdata).
    */
   template<>
   bool state::is<userdata>(int index) {
      return lua_isuserdata(L, index);
   }

   /** [specialization] Check if the given index is light userdata (T =
    * lua::lightuserdata).
    */
   template<>
   bool state::is<lightuserdata>(int index) {
      return lua_islightuserdata(L, index);
   }

}

