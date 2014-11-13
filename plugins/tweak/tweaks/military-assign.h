using namespace DFHack;
using namespace df::enums;
using Screen::Pen;

struct military_assign_hook : df::viewscreen_layer_militaryst {
    typedef df::viewscreen_layer_militaryst interpose_base;

    inline bool inPositionsMode() {
        return page == Positions && !(in_create_squad || in_new_squad);
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (inPositionsMode() && !layer_objects[0]->active)
        {
            auto pos_list = layer_objects[1];
            auto plist = layer_objects[2];
            auto &cand = positions.candidates;

            // Save the candidate list and cursors
            std::vector<df::unit*> copy = cand;
            int cursor = plist->getListCursor();
            int pos_cursor = pos_list->getListCursor();

            INTERPOSE_NEXT(feed)(input);

            if (inPositionsMode() && !layer_objects[0]->active)
            {
                bool is_select = input->count(interface_key::SELECT);

                // Resort the candidate list and restore cursor
                // on add to squad OR scroll in the position list.
                if (!plist->active || is_select)
                {
                    // Since we don't know the actual sorting order, preserve
                    // the ordering of the items in the list before keypress.
                    // This does the right thing even if the list was sorted
                    // with sort-units.
                    std::set<df::unit*> prev, next;
                    prev.insert(copy.begin(), copy.end());
                    next.insert(cand.begin(), cand.end());
                    std::vector<df::unit*> out;

                    // (old-before-cursor) (new) |cursor| (old-after-cursor)
                    for (int i = 0; i < cursor && i < (int)copy.size(); i++)
                        if (next.count(copy[i])) out.push_back(copy[i]);
                    for (size_t i = 0; i < cand.size(); i++)
                        if (!prev.count(cand[i])) out.push_back(cand[i]);
                    int new_cursor = out.size();
                    for (int i = cursor; i < (int)copy.size(); i++)
                        if (next.count(copy[i])) out.push_back(copy[i]);

                    cand.swap(out);
                    plist->setListLength(cand.size());
                    if (new_cursor < (int)cand.size())
                        plist->setListCursor(new_cursor);
                }

                // Preserve the position list index on remove from squad
                if (pos_list->active && is_select)
                    pos_list->setListCursor(pos_cursor);
            }
        }
        else
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        if (inPositionsMode())
        {
            auto plist = layer_objects[2];
            int x1 = plist->getX1(), y1 = plist->getY1();
            int x2 = plist->getX2(), y2 = plist->getY2();
            int i1 = plist->getFirstVisible(), i2 = plist->getLastVisible();
            int si = plist->getListCursor();

            for (int y = y1, i = i1; i <= i2; i++, y++)
            {
                auto unit = vector_get(positions.candidates, i);
                if (!unit || unit->military.squad_id < 0)
                    continue;

                for (int x = x1; x <= x2; x++)
                {
                    Pen cur_tile = Screen::readTile(x, y);
                    if (!cur_tile.valid()) continue;
                    cur_tile.fg = (i == si) ? COLOR_BROWN : COLOR_GREEN;
                    Screen::paintTile(cur_tile, x, y);
                }
            }
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(military_assign_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(military_assign_hook, render);
