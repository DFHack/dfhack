#include "df/viewscreen_joblistst.h"

struct do_job_now_hook : public df::viewscreen_joblistst {
    typedef df::viewscreen_joblistst interpose_base;

    bool handleInput(std::set<df::interface_key> *input) {
        if (input->count(interface_key::CUSTOM_N)) {
            df::job *job = vector_get(jobs, cursor_pos);
            if (job) {
                job->flags.bits.do_now = !job->flags.bits.do_now;
            }

            return true;
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input)) {
        if (!handleInput(input)) {
            INTERPOSE_NEXT(feed)(input);
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ()) {
        INTERPOSE_NEXT(render)();
        int x = 32;
        auto dim = Screen::getWindowSize();
        int y = dim.y - 2;
        bool do_now = false;

        df::job *job = vector_get(jobs, cursor_pos);
        if (job) {
            do_now = job->flags.bits.do_now;
        }

        OutputHotkeyString(x, y, (!do_now ? "Do job now!" : "Normal priority"),
            interface_key::CUSTOM_N, false, x, COLOR_WHITE, COLOR_LIGHTRED);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(do_job_now_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(do_job_now_hook, render);
