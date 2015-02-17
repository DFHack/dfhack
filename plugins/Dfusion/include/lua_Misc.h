#ifndef LUA_MISC_H
#define LUA_MISC_H

#include <map>

#include "Core.h"
#include <MemAccess.h>
#include "luamain.h"
#include "OutFile.h"
#include "LuaTools.h"

namespace lua
{

typedef std::map<std::string,void *> mapPlugs;

class PlugManager
{
    public:

        mapPlugs GetList(){return plugs;};
        uint32_t AddNewPlug(std::string name,uint32_t size,uint32_t loc=0);
        uint32_t FindPlugin(std::string name);

        static PlugManager &GetInst()
        {
            void *p;
            p=DFHack::Core::getInstance().GetData("dfusion_manager");
            if(p==0)
            {
                p=new PlugManager;
                DFHack::Core::getInstance().RegisterData(p,"dfusion_manager");
            }
            return *static_cast<PlugManager*>(p);
        };
    protected:
    private:
        PlugManager(){};
        mapPlugs plugs;
};

void RegisterMisc(lua::state &st);

}

#endif