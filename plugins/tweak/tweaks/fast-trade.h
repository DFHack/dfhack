using namespace std;

struct fast_trade_assign_hook : df::viewscreen_layer_assigntradest {
    typedef df::viewscreen_layer_assigntradest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (layer_objects[1]->active && input->count(interface_key::CURSOR_DOWN_FAST))
        {
            set<df::interface_key> tmp; tmp.insert(interface_key::SELECT);
            INTERPOSE_NEXT(feed)(&tmp);
            tmp.clear(); tmp.insert(interface_key::STANDARDSCROLL_DOWN);
            INTERPOSE_NEXT(feed)(&tmp);
        }
        else if (layer_objects[1]->active && input->count(interface_key::CURSOR_UP_FAST))
        {
            set<df::interface_key> tmp; tmp.insert(interface_key::SELECT);
            INTERPOSE_NEXT(feed)(&tmp);
            tmp.clear(); tmp.insert(interface_key::STANDARDSCROLL_UP);
            INTERPOSE_NEXT(feed)(&tmp);
        }
        else
            INTERPOSE_NEXT(feed)(input);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(fast_trade_assign_hook, feed);

struct fast_trade_select_hook : df::viewscreen_tradegoodsst {
    typedef df::viewscreen_tradegoodsst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (!(is_unloading || !has_traders || in_edit_count)
            && (input->count(interface_key::CURSOR_DOWN_FAST) || input->count(interface_key::CURSOR_UP_FAST)))
        {
            set<df::interface_key> tmp; tmp.insert(interface_key::SELECT);
            INTERPOSE_NEXT(feed)(&tmp);
            if (in_edit_count)
                INTERPOSE_NEXT(feed)(&tmp);
            tmp.clear();
            if (input->count(interface_key::CURSOR_DOWN_FAST)) {
                tmp.insert(interface_key::STANDARDSCROLL_DOWN);
            } else if (input->count(interface_key::CURSOR_UP_FAST)) {
                tmp.insert(interface_key::STANDARDSCROLL_UP);
            }
            INTERPOSE_NEXT(feed)(&tmp);
        }
        else
            INTERPOSE_NEXT(feed)(input);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(fast_trade_select_hook, feed);

