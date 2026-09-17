config.mode = 'fortress'
config.target = 'seedwatch'

local seedwatch = require('plugins.seedwatch')

local saved_targets = {}
do
    local watch_map = seedwatch.seedwatch_getData()
    for k, v in pairs(watch_map) do
        saved_targets[k] = v
    end
end

config.wrapper = function(test_fn)
    return dfhack.with_finalize(function()
        -- restore original targets and zero out any watch entries that the
        -- test created for previously unwatched plants (there is no API to
        -- remove a watch entry entirely)
        local watch_map = seedwatch.seedwatch_getData()
        for k in pairs(watch_map) do
            if saved_targets[k] == nil then saved_targets[k] = 0 end
        end
        for k, v in pairs(saved_targets) do
            local plant = df.global.world.raws.plants.all[k]
            if plant then
                seedwatch.seedwatch_setTarget(plant.id, v)
            end
        end
    end, test_fn)
end

-- finds a seed-bearing non-tree plant, matching the set of plant ids that
-- seedwatch manages (see plugin_load_world_data in seedwatch.cpp)
local function get_seed_plant()
    local plants = df.global.world.raws.plants.all
    for i = 0, #plants - 1 do
        local plant = plants[i]
        if plant.material_defs.type[df.plant_material_def.seed] ~= -1 and
                not plant.flags.TREE then
            return i, plant.id
        end
    end
end

local function all_targets()
    local watch_map = seedwatch.seedwatch_getData()
    return watch_map
end

function test.status()
    expect.true_(seedwatch.parse_commandline('status'))
    expect.true_(seedwatch.parse_commandline())
end

function test.help()
    expect.false_(seedwatch.parse_commandline('help'))
    expect.false_(seedwatch.parse_commandline('-h'))
end

function test.set_all_targets()
    expect.true_(seedwatch.parse_commandline('all', '50'))
    local targets = all_targets()
    local count = 0
    for _ in pairs(targets) do count = count + 1 end
    expect.gt(count, 0)
    for _, v in pairs(targets) do
        expect.eq(50, v)
    end
end

function test.clear()
    expect.true_(seedwatch.parse_commandline('clear'))
    for _, v in pairs(all_targets()) do
        expect.eq(0, v)
    end
end

function test.set_single_plant()
    local idx, id = get_seed_plant()
    if not idx then return end
    expect.true_(seedwatch.parse_commandline(id, '17'))
    expect.eq(17, all_targets()[idx])
end

function test.target_floored()
    local idx, id = get_seed_plant()
    if not idx then return end
    expect.true_(seedwatch.parse_commandline(id, '9.9'))
    expect.eq(9, all_targets()[idx])
end

function test.unknown_plant()
    local idx = get_seed_plant()
    -- the C++ layer prints an error message but the command does not fail
    expect.true_(seedwatch.parse_commandline(
        'THIS_PLANT_DOES_NOT_EXIST', '5'))
    if idx then
        expect.ne(5, all_targets()[idx])
    end
end

function test.negative_target()
    local _, id = get_seed_plant()
    if not id then return end
    expect.error_match('non%-negative integer', function()
        seedwatch.parse_commandline(id, '-5') end)
end

function test.missing_target()
    expect.false_(seedwatch.parse_commandline('all'))
end
