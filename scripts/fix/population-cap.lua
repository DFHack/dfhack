-- Communicates current population to mountainhomes to avoid cap overshooting.

-- The reason for population cap problems is that the population value it
-- is compared against comes from the last dwarven caravan that successfully
-- left for mountainhomes. This script instantly updates it.
-- Note that a migration wave can still overshoot the limit by 1-2 dwarves because
-- of the last migrant bringing his family. Likewise, king arrival ignores cap.

local args = {...}

local ui = df.global.ui
local ui_stats = ui.tasks
local civ = df.historical_entity.find(ui.civ_id)

if not civ then
    qerror('No active fortress.')
end

local civ_stats = civ.activity_stats

if not civ_stats then
    if args[1] ~= 'force' then
        qerror('No caravan report object; use "fix/population-cap force" to create one')
    end
    print('Creating an empty statistics structure...')
    civ.activity_stats = {
        new = true,
        created_weapons = { resize = #ui_stats.created_weapons },
        known_creatures1 = { resize = #ui_stats.known_creatures1 },
        known_creatures = { resize = #ui_stats.known_creatures },
        known_plants1 = { resize = #ui_stats.known_plants1 },
        known_plants = { resize = #ui_stats.known_plants },
    }
    civ_stats = civ.activity_stats
end

-- Use max to keep at least some of the original caravan communication idea
civ_stats.population = math.max(civ_stats.population, ui_stats.population)

print('Home civ notified about current population.')
