#include "df/global_objects.h"
#include "modules/Gui.h"
#include "modules/Screen.h"
#include "df/enabler.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_titlest.h"

struct dwarfmode_pausing_fps_counter_hook : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    static const uint32_t history_length = 3;

    // whether init.txt have [FPS:YES]
    static bool init_have_fps_yes()
    {
        static bool first = true;
        static bool init_have_fps_yes;

        if (first && df::global::gps)
        {
            // if first time called, then display_frames is set iff init.txt have [FPS:YES]
            first = false;
            init_have_fps_yes = (df::global::gps->display_frames == 1);
        }
        return init_have_fps_yes;
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        if (!df::global::pause_state || !df::global::enabler || !df::global::world
            || !df::global::gps || !df::global::pause_state)
            return;

        // if init.txt does not have [FPS:YES] then dont show this FPS counter
        if (!dwarfmode_pausing_fps_counter_hook::init_have_fps_yes())
            return;

        static bool prev_paused = true;
        static uint32_t prev_clock = 0;
        static int32_t prev_frames = 0;
        static uint32_t elapsed_clock = 0;
        static uint32_t elapsed_frames = 0;
        static double history[history_length];

        if (prev_clock == 0)
        {
            // init
            for (int i = 0; i < history_length; i++)
                history[i] = 0.0;
        }

        // disable default FPS counter because it is rendered on top of this FPS counter.
        if (df::global::gps->display_frames == 1)
            df::global::gps->display_frames = 0;

        if (*df::global::pause_state)
            prev_paused = true;
        else
        {
            uint32_t clock = df::global::enabler->clock;
            int32_t frames = df::global::world->frame_counter;

            if (!prev_paused && prev_clock != 0
                && clock >= prev_clock && frames >= prev_frames)
            {
                // if we were previously paused, then dont add clock/frames,
                // but wait for the next time render is called.
                elapsed_clock += clock - prev_clock;
                elapsed_frames += frames - prev_frames;
            }

            prev_paused = false;
            prev_clock = clock;
            prev_frames = frames;

            // add FPS to history every second or after at least one frame.
            if (elapsed_clock >= 1000 && elapsed_frames >= 1)
            {
                double fps = elapsed_frames / (elapsed_clock / 1000.0);
                for (int i = history_length - 1; i >= 1; i--)
                    history[i] = history[i - 1];
                history[0] = fps;

                elapsed_clock = 0;
                elapsed_frames = 0;
            }
        }

        // average fps over a few seconds to stabilize the counter.
        double fps_sum = 0.0;
        int fps_count = 0;
        for (int i = 0; i < history_length; i++)
        {
            if (history[i] > 0.0)
            {
                fps_sum += history[i];
                fps_count++;
            }
        }

        double fps = fps_count == 0 ? 1.0 : fps_sum / fps_count;
        double gfps = df::global::enabler->calculated_gfps;

        std::stringstream fps_counter;
        fps_counter << "FPS:"
                    << setw(4) << fixed << setprecision(fps >= 1.0 ? 0 : 2) << fps
                    << " (" << gfps << ")";

        // show this FPS counter same as the default counter.
        int x = 10;
        int y = 0;
        OutputString(COLOR_WHITE, x, y, fps_counter.str(),
                     false, 0, COLOR_CYAN, false);
    }
};

struct title_pausing_fps_counter_hook : df::viewscreen_titlest {
    typedef df::viewscreen_titlest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        // if init.txt have FPS:YES then enable default FPS counter when exiting dwarf mode.
        // So it is enabled if starting adventure mode without exiting dwarf fortress.
        if (dwarfmode_pausing_fps_counter_hook::init_have_fps_yes()
            && df::global::gps && df::global::gps->display_frames == 0)
            df::global::gps->display_frames = 1;
    }
};


IMPLEMENT_VMETHOD_INTERPOSE(dwarfmode_pausing_fps_counter_hook, render);

IMPLEMENT_VMETHOD_INTERPOSE(title_pausing_fps_counter_hook, render);
