#include "df/viewscreen_titlest.h"

using namespace DFHack;

struct title_start_rename_hook : df::viewscreen_titlest {
    typedef df::viewscreen_titlest interpose_base;
    typedef interpose_base::T_sel_subpage T_sel_subpage;
    static T_sel_subpage last_subpage;
    static bool in_rename;
    static bool rename_failed;
    static std::string entry;

    inline df::viewscreen_titlest::T_start_savegames *get_cur_save()
    {
        return vector_get(start_savegames, sel_submenu_line);
    }

    inline std::string full_save_dir(const std::string &region_name)
    {
        return std::string("data/save/") + region_name;
    }

    bool do_rename()
    {
        auto save = get_cur_save();
        if (!save)
            return false;
        if (Filesystem::isdir(full_save_dir(entry)))
            return false;
        if (rename(full_save_dir(save->save_dir).c_str(), full_save_dir(entry).c_str()) != 0)
            return false;
        save->save_dir = entry;
        entry = "";
        return true;
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        if (sel_subpage == T_sel_subpage::StartSelectWorld || sel_subpage == T_sel_subpage::StartSelectMode)
        {
            auto save = get_cur_save();
            if (save)
            {
                int x = 1, y = 7;
                OutputHotkeyString(x, y,
                    in_rename ? entry.c_str() : "Rename",
                    df::interface_key::CUSTOM_R,
                    false, 0,
                    rename_failed ? COLOR_LIGHTRED : COLOR_WHITE,
                    in_rename ? COLOR_RED : COLOR_LIGHTRED);
                if (in_rename)
                    OutputString(COLOR_LIGHTGREEN, x, y, "_");
            }
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        using namespace df::enums::interface_key;
        if (in_rename)
        {
            rename_failed = false;
            auto string_key = get_string_key(input);
            if (input->count(SELECT) && !entry.empty())
            {
                if (do_rename())
                    in_rename = false;
                else
                    rename_failed = true;
            }
            else if (input->count(STRING_A000))
            {
                if (!entry.empty())
                    entry.erase(entry.size() - 1);
            }
            else if (string_key != NONE)
            {
                entry += Screen::keyToChar(string_key);
            }
            else if (input->count(LEAVESCREEN) || (input->count(SELECT) && entry.empty()) ||
                input->count(STANDARDSCROLL_UP) || input->count(STANDARDSCROLL_DOWN))
            {
                entry = "";
                in_rename = false;
                std::set<df::interface_key> tmp;
                if (input->count(STANDARDSCROLL_UP))
                    tmp.insert(STANDARDSCROLL_UP);
                if (input->count(STANDARDSCROLL_DOWN))
                    tmp.insert(STANDARDSCROLL_DOWN);
                INTERPOSE_NEXT(feed)(&tmp);
            }
        }
        else if (input->count(CUSTOM_R))
        {
            in_rename = true;
        }
        else
        {
            INTERPOSE_NEXT(feed)(input);
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(title_start_rename_hook, render);
IMPLEMENT_VMETHOD_INTERPOSE(title_start_rename_hook, feed);

df::viewscreen_titlest::T_sel_subpage title_start_rename_hook::last_subpage =
    df::viewscreen_titlest::T_sel_subpage::None;
bool title_start_rename_hook::in_rename = false;
bool title_start_rename_hook::rename_failed = false;
std::string title_start_rename_hook::entry;
