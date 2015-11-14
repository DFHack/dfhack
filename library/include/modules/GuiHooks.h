#pragma once
#include <vector>
namespace DFHack {
    #define GUI_HOOK_DECLARE(name, rtype, args) DFHACK_EXPORT extern DFHack::GuiHooks::Hook<rtype args> name
    #define GUI_HOOK_DEFINE(name, base_func) DFHack::GuiHooks::Hook<decltype(base_func)> name(base_func)
    #define GUI_HOOK_TOP(name) name.top()
    #define GUI_HOOK_CALLBACK(hook, name, callback) DFHack::GuiHooks::Hook<decltype(callback)>::Callback name(&hook, callback)
    namespace GuiHooks {
        template <typename T_func>
        class Hook {
            typedef Hook<T_func> T_hook;
            friend class Callback;
            T_func *base_func;
            std::vector<T_func*> funcs;
            void add(T_func *func)
            {
                if (std::find(funcs.begin(), funcs.end(), func) == funcs.end())
                    funcs.push_back(func);
            }
            void remove(T_func *func)
            {
                auto it = std::find(funcs.begin(), funcs.end(), func);
                if (it != funcs.end())
                    funcs.erase(it);
            }
        public:
            Hook(T_func* base) : base_func(base)
            { }
            T_func* top()
            {
                return funcs.empty() ? base_func : funcs[funcs.size() - 1];
            }
            T_func* next(T_func* cur)
            {
                if (funcs.size())
                {
                    auto it = std::find(funcs.begin(), funcs.end(), cur);
                    if (it != funcs.end() && it != funcs.begin())
                        return *(it - 1);
                }
                return base_func;
            }

            class Callback {
                T_func *func;
                T_hook *hook;
                bool enabled;
            public:
                Callback(T_hook *hook, T_func *func) : hook(hook), func(func)
                { }
                ~Callback()
                {
                    disable();
                }
                inline T_func *next() { return hook->next(func); }
                void apply (bool enable)
                {
                    if (enable)
                        hook->add(func);
                    else
                        hook->remove(func);
                    enabled = enable;
                }
                inline void enable() { apply(true); }
                inline void disable() { apply(false); }
                inline bool is_enabled() { return enabled; }
                inline void toggle() { apply(!enabled); }
            };
        };
    }
}
