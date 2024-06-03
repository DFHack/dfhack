// Workaround for DF bug #6273 - adjust all custom reactions to produce GLOVES items in sets with correct handedness
// It also analyzes the body plan of the unit performing the reaction, so Antmen will get 4 gloves instead of 2

// If a reaction tries to produce either 1 glove or 2 gloves, it will produce a single set
// Otherwise, it will multiply the number of products by the number of hands

#include "df/body_part_raw.h"
#include "df/caste_body_info.h"
#include "df/reaction_product_itemst.h"
#include "df/unit.h"

struct reaction_gloves_hook : df::reaction_product_itemst {
    typedef df::reaction_product_itemst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, produce, (df::unit *unit,
        std::vector<df::reaction_product*> *out_products, std::vector<df::item*> *out_items,
        std::vector<df::reaction_reagent*> *in_reag, std::vector<df::item*> *in_items,
        int32_t quantity, df::job_skill skill, int32_t quality,
        df::historical_entity *entity, df::world_site *site, std::vector<df::item *> *improv_items))
    {
        if (item_type != df::item_type::GLOVES) {
            INTERPOSE_NEXT(produce)(unit, out_products, out_items, in_reag, in_items, quantity, skill, quality, entity, site, improv_items);
            return;
        }

        // Examine creator unit's body plan, see how many hands it has
        // Count left hands and right hands, as well as "neutral" hands for compatibility
        int num_hands = 0, num_left = 0, num_right = 0;
        for (auto part : unit->body.body_plan->body_parts) {
            if (part->flags.is_set(df::body_part_raw_flags::GRASP)) {
                num_hands++;
                if (part->flags.is_set(df::body_part_raw_flags::LEFT))
                    num_left++;
                if (part->flags.is_set(df::body_part_raw_flags::RIGHT))
                    num_right++;
            }
        }

        std::vector<df::item*> out_items_temp;
        int old_count = count;

        // If the reaction product's count is set to 1 or 2, set it to the number of hands
        // Otherwise, *multiply* it by the number of hands (and multiply the left/right counts accordingly)
        if (count <= 2)
            count = num_hands;
        else {
            num_left *= count;
            num_right *= count;
            count *= num_hands;
        }

        INTERPOSE_NEXT(produce)(unit, out_products, &out_items_temp, in_reag, in_items, quantity, skill, quality, entity, site, improv_items);
        count = old_count;

        // If the reaction was somehow asked to produce multiple sets (due to excess inputs), multiply the outputs too
        num_left *= quantity;
        num_right *= quantity;

        // Iterate across the output gloves, set their handedness, then append them to the actual out_items list for DF
        for (auto out_item : out_items_temp) {
            // Do left gloves first, then right gloves, then "neutral" ones last
            // This is important for the "createitem" plugin, which contains similar workaround logic
            if (num_left > 0) {
                out_item->setGloveHandedness(1);
                --num_left;
            } else if (num_right > 0) {
                out_item->setGloveHandedness(2);
                --num_right;
            }
            out_items->push_back(out_item);
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(reaction_gloves_hook, produce);
