#include <stdexcept>

#include "df/viewscreen_createquotast.h"
#include "df/manager_order.h"

using df::global::world;

struct manager_quantity_hook : df::viewscreen_createquotast {
    typedef df::viewscreen_createquotast interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key>* input))
    {
        bool cancel = false;
        bool wanted_quantity = want_quantity;
        if (want_quantity)
        {
            for (auto it = input->begin(); it != input->end(); ++it)
            {
                char c = DFHack::Screen::keyToChar(*it);
                if (c >= '0' && c <= '9')
                {
                    cancel = true;
                    size_t len = strlen(str_filter);
                    if (len < 5)
                    {
                        str_filter[len] = c;
                        str_filter[len + 1] = '\0';
                    }
                }
            }
        }
        if (cancel)
            return;
        // Native feed() adds manager order, updates want_quantity, and removes SELECT from input
        int select = input->count(df::interface_key::SELECT);
        INTERPOSE_NEXT(feed)(input);
        if (wanted_quantity && select && strlen(str_filter) > 0)
        {
            df::manager_order* order = world->manager_orders[world->manager_orders.size() - 1];
            int16_t count = 1;
            try
            {
                count = std::stoi(str_filter);
            }
            catch (...) { }
            if (count < 1)
                count = 1;
            order->amount_total = count;
            order->amount_left = count;
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(manager_quantity_hook, feed);
