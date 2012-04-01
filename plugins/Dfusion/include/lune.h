#ifndef LUNE_H
#define LUNE_H


#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


#include "luaxx.hpp"
#include <string>

namespace lua
{
    class object
    {
        state &myst;
        int myref;
    public:
        object(state &myst):myst(myst)
        {
            myref=luaL_ref(myst,LUA_REGISTRYINDEX);
        }
        ~object()
        {
            luaL_unref(myst,LUA_REGISTRYINDEX,myref);
        }
        void Get()
        {
            lua_rawgeti(myst,LUA_REGISTRYINDEX,myref);
        }
        state &GetState(){return myst;};
    };

    class local_object
    {
        state myst;
        int myref;
        static object *mytbl;
    public:
        local_object(lua_State *L)
        {

            myst=state(L);
            //LOG<<"Creating local object...\n";
            //StackDump(L);
            if(!mytbl)
            {
                //LOG<<" Metable...\n";
                myst.newtable(); //2
                if(myst.newmetatable("WEAKTABLE"))//3
                {
                    //StackDump(L);
                    myst.push("kv"); //4
                    myst.setfield("__mode");//3
                    //LOG<<" Setting Metable...\n";
                    //StackDump(L);
                }
                //LOG<<" Attaching to holder...\n";

                //myst.setfield("__metatable");//2
                lua_setmetatable(myst,-1);
                mytbl=new object(myst);
                //StackDump(L);
                //LOG<<" Done Metatable...\n";
            }
            //StackDump(L);
            mytbl->Get();
            //LOG<<" Got my table...\n";
            //StackDump(L);
            myst.insert(-2);
            myref=luaL_ref(myst,-2);
            //LOG<<"Before pop:";
            //StackDump(L);
            myst.pop(1);
            GetTable();
            //LOG<<"========Done...\n"<<"Ref="<<myref<<"\n";
            //mytbl->Get();
            //StackDump(L);
            //LOG<<"===========================\n";
        }
        ~local_object()
        {
            //LOG<<"Deleting local object...\n";
            ReleaseTable();
        }
        void ReleaseTable()
        {
            mytbl->Get();
            int pos=myst.gettop();
            luaL_unref(myst,pos,myref);
            myst.remove(pos);
        }
        state GetState(){return myst;}
        void GetTable()
        {
            //LOG<<"Getting ref="<<myref<<"\n";
            //StackDump(myst);
            //LOG<<"Tbl preget\n";
            mytbl->Get();
            int pos=myst.gettop();
            //StackDump(myst);
            //LOG<<"Tbl get\n";
            //int pos=myst.gettop();
            lua_rawgeti(myst,pos,myref);
            //StackDump(myst);
            //LOG<<"Done\n";
            myst.remove(pos);
        }
    protected:

    };
};

template <typename T,bool GC=true>
class Lune
{
public:
    typedef struct
    {
        T *pT;
        int tableref;
    } userdataType;

    typedef int (T::*mfp)(lua_State *L);
    typedef struct
    {
        const char *name;
        mfp mfunc;
    } RegType;

    static void Register(lua_State *L)
    {
        lua_newtable(L);
        int methods = lua_gettop(L);

        luaL_newmetatable(L, T::className);
        int metatable = lua_gettop(L);

        // store method table in globals so that
        // scripts can add functions written in Lua.
        lua_pushvalue(L, methods);
        lua_setglobal(L, T::className);

        lua_pushliteral(L, "__metatable");
        lua_pushvalue(L, methods);
        lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

        lua_pushliteral(L, "__index");
        lua_pushcfunction(L, index_T);
        lua_settable(L, metatable);

        //lua_pushliteral(L, "__name");
        //lua_pushstring(L, T::className);
        //lua_settable(L, metatable);

        lua_pushliteral(L, "__newindex");
        lua_pushcfunction(L, newindex_T);
        lua_settable(L, metatable);

        lua_pushliteral(L, "__instances");
        lua_newtable(L);
        lua_settable(L, metatable);
        if(GC)
        {
            lua_pushliteral(L, "__gc");
            lua_pushcfunction(L, gc_T);
            lua_settable(L, metatable);
        }


        lua_newtable(L);                // metatable for method table
        int mt = lua_gettop(L);
        lua_pushliteral(L, "__call");
        lua_pushcfunction(L, new_T);
        lua_pushliteral(L, "new");
        lua_pushvalue(L, -2);           // dup new_T function
        lua_settable(L, methods);       // add new_T to method table
        lua_settable(L, mt);            // mt.__call = new_T
        lua_setmetatable(L, methods);
        //LOG<<"lune: registered class \""<<T::className<<"\"\n";
        // fill method table with methods from class T
        for (RegType *l = T::methods; l->name; l++)
        {
            /* edited by Snaily: shouldn't it be const RegType *l ... ? */
            lua_pushstring(L, l->name);
            lua_pushlightuserdata(L, (void*)l);
            lua_pushcclosure(L, thunk, 1);
            lua_settable(L, methods);
            //LOG<<"lune:    method \""<<l->name<<"\"\n";
        }

        lua_pop(L, 2);  // drop metatable and method table
    };
    static void GetTable(lua_State *L,T *p)
    {
        GetTableEx(L,p->GetTableId());
    }
    static void GetTableEx(lua_State *L,int id)
    {
        lua::state s(L);
        s.getmetatable(T::className);
        s.getfield("__instances");
        int ins=s.gettop();
        lua_rawgeti(L, ins, id);
        s.insert(-3);
        s.pop(2);
    }
    static T *check(lua_State *L, int narg)
    {
        userdataType *ud =
            static_cast<userdataType*>(luaL_checkudata(L, narg, T::className)); //TODO FIX THIs..
        //(lua_touserdata(L, narg));//
        if(!ud) luaL_error(L, "Bad argument %d: expected type %s", narg, T::className);
        return ud->pT;  // pointer to T object
    }
protected:
private:

    static int RegTable(lua_State *L)
    {
       // LOG<<"Regging....\n";
        //lua::StackDump(L);

        lua::state s(L);
        int ssize=s.gettop();
        //s.getglobal(T::className);
        s.getmetatable(T::className);
        s.getfield("__instances");
        int ins=s.gettop();
        s.newtable();
        int id=luaL_ref(L,ins);
        //LOG<<"After reg:\n";
        //lua::StackDump(L);
        s.settop(ssize);
        return id;
    }
    static void UnregTable(lua_State *L,int id)
    {
        lua::state s(L);
        s.getmetatable(T::className);
        s.getfield("__instances");
        int ins=s.gettop();
        //LOG<<"Unreg table id:"<<id<<"stack dump:\n";
        //lua::StackDump(L);
        luaL_unref(L,ins,id);
    }
    static int index_T(lua_State *L)  // calls with (table, key), return value
    {
        lua::state st(L);
        std::string key=st.as<std::string>(-1);
        T *p=check(L,1);
        GetTable(L,p);
        st.insert(-2);
        //LOG<<"Index:\n";
        //lua::StackDump(L);
        lua_rawget(L,-2); //try getting from normal table
        if(st.is<lua::nil>()) //failed
        {
            st.pop(2);
            st.getglobal(T::className); //try class tables then
            st.push(key);
            st.gettable();
        }
        return 1;
    }
    static int newindex_T(lua_State *L)
    {
        //LOG<<"New index....\n";
        //lua::StackDump(L);

        lua::state st(L);
        T *p=check(L,1);
        GetTable(L,p);
        //st.insert(-3);
        //LOG<<"Before set:\n";
        st.insert(-3);
        //lua::StackDump(L);
        lua_rawset(L,-3);
        return 0;
    }
    static int thunk(lua_State *L)
    {
        //LOG<<"Size of stack:"<<lua_gettop(L)<<"\n";
        //lua::StackDump(L);
        if(lua_gettop(L)<1)
            luaL_error(L,"Member function called without 'self'");
        //LOG<<"Size of stack after:"<<lua_gettop(L)<<"\n";
        // stack has userdata, followed by method args
        T *obj = check(L, 1);  // get 'self', or if you prefer, 'this'
        //T *obj=static_cast<userdataType*>(lua_touserdata(L,1))->pT;
        lua_remove(L, 1);  // remove self so member function args start at index 1
        // get member function from upvalue
        RegType *l = static_cast<RegType*>(lua_touserdata(L, lua_upvalueindex(1)));
        return (obj->*(l->mfunc))(L);  // call member function
    }
    static int gc_T(lua_State *L)
    {
        //lua_getfield(L,,"__ud");
        //LOG<<"Garbage collecting.\n";
        //lua::StackDump(L);
        userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
        T *obj = ud->pT;

        delete obj;  // call destructor for T objects
        UnregTable(L,ud->tableref);
        return 0;
    }
    static int new_T(lua_State *L)
    {
        //LOG<<"Pre build:"<<lua_gettop(L)<<"\n";
        //lua::StackDump(L);
        lua_remove(L, 1);   // use classname:new(), instead of classname.new()
        //lua_newtable(L);
        int id=RegTable(L);
        //LOG<<"Registred as:"<<id<<"\n";
        //int ssize=lua_gettop(L);
        T *obj = new T(L,id);  // call constructor for T objects
        lua_settop(L,0); //no need for parameters later.
        //LOG<<"Post build:"<<lua_gettop(L)<<"\t";
        //lua::StackDump(L);
        //LOG<<"TSOP\n";

        userdataType *ud =
            static_cast<userdataType*>(lua_newuserdata(L, sizeof(userdataType)));
        //lua::StackDump(L);
        luaL_getmetatable(L, T::className);  // lookup metatable in Lua registry
        lua_setmetatable(L,-2);
        //LOG<<"metatable set\n";

        //lua::StackDump(L);
        GetTable(L,obj);
        lua_pushliteral(L,"__obj");
        lua_pushvalue(L,-3);
        lua_settable(L,-3);
        lua_pop(L,1);
        //LOG<<"Object referenced\n";
        //lua::StackDump(L);
        //T *p = new(ud) T(L);  // call constructor for T objects
        //lua::StackDump(L);
        ud->pT = obj;  // store pointer to object in userdata
        ud->tableref=id;
        //luaL_getmetatable(L, T::className);  // lookup metatable in Lua registry
        //lua_setmetatable(L, tableindex);
        //lua::StackDump(L);
        //LOG<<"Push done\n";
        return 1;  // userdata containing pointer to T object
    }
    Lune() {}; //non constructable...
};
#define method(class, name) {#name, &class::name}
#define DEF_LUNE(class) static const char className[];\
                         static Lune<class>::RegType methods[];
#define DEF_LUNE_NOGC(class) static const char className[];\
                         static Lune<class,false>::RegType methods[];
#define IMP_LUNE(class,lua_name) const char class::className[]=#lua_name;
#define LUNE_METHODS_START(class) Lune<class>::RegType class::methods[] = {
#define LUNE_METHODS_START_NOGC(class) Lune<class,false>::RegType class::methods[] = {
#define LUNE_METHODS_END() {0,0}}
#endif // LUNE_H
