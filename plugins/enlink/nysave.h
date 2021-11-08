#ifndef UNKNBLCDR_NYSAVE
#define UNKNBLCDR_NYSAVE	

#include <string>

#include "loctools.h"

#include "df/global_objects.h"
#include "Core.h"
#include "modules/World.h"

namespace NySave
{
    inline std::string sanitize_name(const std::string &in)
    {
        std::string out(in);
        for (auto &ch : out)
        {
            if (!isalnum(ch) && ch != '-' && ch != '_')
            {
                ch = '_';
            }
        }
        return out;
    }

    std::string save_name(const std::string & name)
    //File associated with `name` in the save folder
    {
        std::string finalname = sanitize_name(name);
        return std::string("data/save/") + DFHack::World::ReadWorldFolder() + "/dfhack-" + finalname + ".nsv";
    }

    std::string current_name(const std::string & name)
    //File associated with `name` in the current folder
    {
        std::string finalname = sanitize_name(name);
        return std::string("data/save/current/dfhack-") + finalname + ".nsv";
    }

    template <class Stream, class ... Args>
    inline bool open_save_stream(const std::string &name, Stream & fstr, Args && ... args)
    //Opens `fstr` to a file associated with `name` in the save folder
    //Returns true on error.
    {
        if (!DFHack::Core::getInstance().isWorldLoaded())
        {
            return true;
        }
        if (fstr.is_open())
        {
            fstr.close();
        }
        fstr.open(save_name(name), std::forward<Args>(args)...);
        return !fstr.is_open();
    }

    template <class Stream, class ... Args>
    inline bool open_current_stream(const std::string &name, Stream & fstr, Args && ... args)
    //Opens `fstr` to a file associated with `name` in the current folder
    //Returns true on error.
    {
        if (!DFHack::Core::getInstance().isWorldLoaded())
        {
            return true;
        }
        if (fstr.is_open())
        {
            fstr.close();
        }
        fstr.open(current_name(name), std::forward<Args>(args)...);
        return !fstr.is_open();
    }

    inline bool can_get_site_string()
    {
        return (DFHack::World::isFortressMode() || DFHack::World::isAdventureMode()) && !!df::global::world;
    }

    inline std::string get_current_site_string()
    {
        if (can_get_site_string())
        {
            const int32_t res = LocTools::get_current_site();
            return std::to_string(res);
        }
        else
        {
            return std::string("nope");
        }
    }

    inline void get_possibly_visible_site_strings(std::vector<std::string> &str_v)
    {
        str_v.clear();
        std::vector<int32_t> v;
        LocTools::get_possibly_visible_sites(v);
        if (v.size() > 0)
        {
            str_v.reserve(v.size());
            for (const auto &it : v)
            {
                str_v.push_back(std::to_string(it));
            }
        }
        else
        {
            str_v.push_back("nope");
        }
    }
    

    template <class Stream, class ... Args>
    inline bool open_site_stream(const std::string &prefix, const std::string &sstr, Stream & fstr, Args && ... args)
    {
        return open_save_stream(prefix + "_" + sstr, fstr, std::forward<Args>(args)...);
    }

    template <class Stream, class ... Args>
    inline bool open_current_site_stream(const std::string &prefix, Stream & fstr, Args && ... args)
    //Opens `fstr` to a file associated with `name` in the current site
    //Returns true on error.
    {
        if (!can_get_site_string())
        {
            return true;
        }
        return open_site_stream(prefix, get_current_site_string(), fstr, std::forward<Args>(args)...);
    }

}

#endif
