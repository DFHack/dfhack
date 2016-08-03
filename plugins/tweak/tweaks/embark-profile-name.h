#include "df/viewscreen_setupdwarfgamest.h"
using namespace DFHack;
struct embark_profile_name_hook : df::viewscreen_setupdwarfgamest {
    typedef df::viewscreen_setupdwarfgamest interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        int ch = -1;
        for (auto it = input->begin(); ch == -1 && it != input->end(); ++it)
            ch = Screen::keyToChar(*it);
        if (in_save_profile && ch >= 32 && ch <= 126)
        {
            profile_name.push_back((char)ch);
        }
        else
        {
            if (input->count(df::interface_key::LEAVESCREEN))
                input->insert(df::interface_key::SETUPGAME_SAVE_PROFILE_ABORT);
            INTERPOSE_NEXT(feed)(input);
        }
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(embark_profile_name_hook, feed);
