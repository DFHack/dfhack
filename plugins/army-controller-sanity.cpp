#include "Debug.h"
#include "PluginManager.h"

#include "df/world.h"
#include "df/army_controller.h"
#include "df/army.h"
#include "df/historical_entity.h"
#include "df/unit.h"
#include "df/global_objects.h"

#include <unordered_set>

DFHACK_PLUGIN("army-controller-sanity");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(army_controller_next_id);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(pause_state);

namespace DFHack {
    DBG_DECLARE(army_controller_sanity, log, DebugCategory::LWARNING);
}

namespace {
    bool checkArmyControllerSanity()
    {
        // army controllers are found:
        //   in viewscreen_worldst (army_controller (list), last_hover_ac) (not checked)
        //   in entitst (army_controller (list))
        //   in armyst (controller)
        //   in unitst (army_controller)
        //
        // master list is in army_controller_handlerst

        static size_t last_ac_vec_size = 0;
        static int last_army_controller_next_id = 0;

        if (last_army_controller_next_id == *df::global::army_controller_next_id &&
            last_ac_vec_size == df::global::world->army_controllers.all.size())
            return true;

        std::unordered_set<df::army_controller*> ac_set{};

        for (auto ac : df::global::world->army_controllers.all)
        {
            ac_set.insert(ac);
        }

        bool ok = true;

        for (auto ent : df::global::world->entities.all)
        {
            for (auto ac : ent->army_controllers)
            {
                if (ac_set.count(ac) == 0) {
                    WARN(log).print("acValidationError: Bad controller %p found in entity id %d\n", ac, ent->id);
                    ok = false;
                }
            }
        }

        for (auto ar : df::global::world->armies.all)
        {
            auto ac = ar->controller;
            if (ac && ac_set.count(ac) == 0) {
                WARN(log).print("acValidationError: Bad controller %p found in army id %d\n", ac, ar->id);
                ok = false;
            }
            else if (ac && ac->id != ar->controller_id)
            {
                WARN(log).print("acValidationError: controller %p id mismatch (%d != %d) in army %d\n", ac, ar->controller_id, ac->id, ar->id);
                ok = false;
            }
            else if (!ac && ar->controller_id != -1)
            {
                WARN(log).print("acValidationError: army %d has nonzero controller %d but controller pointer is null\n", ar->id, ar->controller_id);
                ok = false;
            }
        }

        for (auto un : df::global::world->units.all)
        {
            auto ac = un->enemy.army_controller;
            if (ac && ac_set.count(ac) == 0) {
                WARN(log).print("acValidationError: Bad controller %p found in unit id %d\n", ac, un->id);
                ok = false;
            }
            else if (ac && ac->id != un->enemy.army_controller_id)
            {
                WARN(log).print("acValidationError: controller %p id mismatch (%d != %d) in unit %d\n", ac, un->enemy.army_controller_id, ac->id, un->id);
                ok = false;
            }
            else if (!ac && un->enemy.army_controller_id != -1)
            {
                WARN(log).print("acValidationError: unit %d has has nonzero controller %d but controller pointer is null\n", un->id, un->enemy.army_controller_id);
                ok = false;
            }
        }

        last_army_controller_next_id = *df::global::army_controller_next_id;
        last_ac_vec_size = df::global::world->army_controllers.all.size();

        INFO(log).print("acValidation: controller count = %ld, next id = %d, season tick count = %d\n",
            last_ac_vec_size, last_army_controller_next_id, *df::global::cur_year_tick);

        return ok;
    }
}

DFhackCExport DFHack::command_result plugin_init(DFHack::color_ostream& out, std::vector <DFHack::PluginCommand>& commands)
{
    return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result plugin_enable(DFHack::color_ostream& out, bool enable)
{
    is_enabled = enable;
    return DFHack::CR_OK;
}

DFhackCExport DFHack::command_result plugin_onupdate(DFHack::color_ostream& out)
{
    if (is_enabled)
    {
        bool ok = checkArmyControllerSanity();
        if (!ok) {
            ERR(log).print("Army controller sanity check failed! Game pause forced.\n");
            *df::global::pause_state = true;
            is_enabled = false;
        }
    }
    return DFHack::CR_OK;
}
