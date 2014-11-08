#include "df/meeting_diplomat_info.h"
#include "df/entity_sell_requests.h"
#include "df/viewscreen_topicmeeting_takerequestsst.h"

using namespace std;
using namespace DFHack;
using namespace df::enums;

struct takerequest_hook : df::viewscreen_topicmeeting_takerequestsst {
    typedef df::viewscreen_topicmeeting_takerequestsst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key>* input))
    {
        if (input->count(interface_key::CURSOR_RIGHT_FAST) ||
            input->count(interface_key::CURSOR_LEFT_FAST))
        {
            int delta = 0 + input->count(interface_key::CURSOR_RIGHT_FAST)
                          - input->count(interface_key::CURSOR_LEFT_FAST);
            vector<int8_t> &cur_priorities = meeting->sell_requests->priority[type_categories[type_idx]];
            for (size_t i = 0; i < cur_priorities.size(); i++)
            {
                cur_priorities[i] += delta;
                if (cur_priorities[i] > 4)
                    cur_priorities[i] = 4;
                if (cur_priorities[i] < 0)
                    cur_priorities[i] = 0;
            }
        }
        INTERPOSE_NEXT(feed)(input);
    }
    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        int x = 45, y = 23;
        OutputString(COLOR_LIGHTRED, x, y, "Shift+Left/Right");
        OutputString(COLOR_GREY, x, y, ": Adjust category");
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(takerequest_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(takerequest_hook, render);
