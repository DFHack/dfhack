using namespace std;

struct advmode_contained_hook : df::viewscreen_layer_unit_actionst {
    typedef df::viewscreen_layer_unit_actionst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        auto old_reaction = cur_reaction;
        auto old_reagent = reagent;

        INTERPOSE_NEXT(feed)(input);

        if (cur_reaction && (cur_reaction != old_reaction || reagent != old_reagent))
        {
            old_reagent = reagent;

            // Skip reagents already contained by others
            while (reagent < (int)cur_reaction->reagents.size()-1)
            {
                if (!cur_reaction->reagents[reagent]->flags.bits.IN_CONTAINER)
                    break;
                reagent++;
            }

            if (old_reagent != reagent)
            {
                // Reproduces a tiny part of the orginal screen code
                choice_items.clear();

                auto preagent = cur_reaction->reagents[reagent];
                reagent_amnt_left = preagent->quantity;

                for (int i = held_items.size()-1; i >= 0; i--)
                {
                    if (!preagent->matchesRoot(held_items[i], cur_reaction->index))
                        continue;
                    if (linear_index(sel_items, held_items[i]) >= 0)
                        continue;
                    choice_items.push_back(held_items[i]);
                }

                layer_objects[6]->setListLength(choice_items.size());

                if (!choice_items.empty())
                {
                    layer_objects[4]->active = layer_objects[5]->active = false;
                    layer_objects[6]->active = true;
                }
                else if (layer_objects[6]->active)
                {
                    layer_objects[6]->active = false;
                    layer_objects[5]->active = true;
                }
            }
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(advmode_contained_hook, feed);
