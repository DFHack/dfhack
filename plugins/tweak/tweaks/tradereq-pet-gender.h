#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/entity_sell_category.h"
#include "df/historical_entity.h"
#include "df/viewscreen_topicmeeting_takerequestsst.h"

using namespace DFHack;

using df::global::world;

struct pet_gender_hook : df::viewscreen_topicmeeting_takerequestsst {
    typedef df::viewscreen_topicmeeting_takerequestsst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        if (type_categories[type_idx] == df::entity_sell_category::Pets)
        {
            df::historical_entity* entity = df::historical_entity::find(meeting->civ_id);
            vector<int32_t>& races = entity->resources.animals.pet_races;
            vector<int16_t>& castes = entity->resources.animals.pet_castes;
            for (int i = (good_idx / 17) * 17, y = 4; i < (good_idx / 17) * 17 + 17 && i < races.size(); i++, y++) {
                int x = 30 + 1 + world->raws.creatures.all[races[i]]->caste[castes[i]]->caste_name[0].size();
                bool male = (bool)world->raws.creatures.all[races[i]]->caste[castes[i]]->gender;
                OutputString((i == good_idx) ? COLOR_WHITE : COLOR_GREY,
                    x, y, male ? "\013" : "\014");
            }
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(pet_gender_hook, render);
