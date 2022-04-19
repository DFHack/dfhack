using namespace std;

static df::interface_key fast_trade_move_key(const set<df::interface_key> *input)
{
    return input->count(interface_key::CURSOR_DOWN_FAST)
         ? interface_key::STANDARDSCROLL_DOWN
         : input->count(interface_key::CURSOR_UP_FAST)
         ? interface_key::STANDARDSCROLL_UP
         : interface_key::NONE;
}

#define DEFINE_FAST_TRADE_INTERPOSE(is_trading)                             \
    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))   \
    {                                                                       \
        df::interface_key move_key;                                         \
        if ((is_trading) && (move_key = fast_trade_move_key(input)))        \
        {                                                                   \
            set<df::interface_key> tmp;                                     \
            tmp.insert(interface_key::SELECT);                              \
            INTERPOSE_NEXT(feed)(&tmp);                                     \
                                                                            \
            tmp.clear();                                                    \
            tmp.insert(move_key);                                           \
            INTERPOSE_NEXT(feed)(&tmp);                                     \
        }                                                                   \
        else                                                                \
            INTERPOSE_NEXT(feed)(input);                                    \
    }


struct fast_trade_assign_hook : df::viewscreen_layer_assigntradest {
    typedef df::viewscreen_layer_assigntradest interpose_base;

    DEFINE_FAST_TRADE_INTERPOSE(layer_objects[1]->active)
};

IMPLEMENT_VMETHOD_INTERPOSE(fast_trade_assign_hook, feed);

struct fast_trade_select_hook : df::viewscreen_tradegoodsst {
    typedef df::viewscreen_tradegoodsst interpose_base;
    DEFINE_FAST_TRADE_INTERPOSE(has_traders && !is_unloading && !in_edit_count)
};

IMPLEMENT_VMETHOD_INTERPOSE(fast_trade_select_hook, feed);
