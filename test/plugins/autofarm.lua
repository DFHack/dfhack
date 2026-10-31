config.mode = 'fortress'
config.target = 'autofarm'

-- autofarm enabled state persists; leave it disabled
config.wrapper = function(test_fn)
    return dfhack.with_finalize(function()
        dfhack.run_command_silent('autofarm', 'disable')
    end, test_fn)
end

function test.status()
    local output = dfhack.run_command_silent('autofarm', 'status')
    expect.ne('', output)
end

function test.enable_disable()
    dfhack.run_command_silent('autofarm', 'enable')
    local output = dfhack.run_command_silent('autofarm', 'status')
    expect.str_find('Active', output)
    dfhack.run_command_silent('autofarm', 'disable')
    output = dfhack.run_command_silent('autofarm', 'status')
    expect.str_find('Stopped', output)
end

function test.runonce()
    local output, status = dfhack.run_command_silent('autofarm', 'runonce')
    expect.eq(0, status)
end

function test.unknown_arg_shows_usage()
    local output, status = dfhack.run_command_silent('autofarm', 'bogus')
    expect.true_(output:find('autofarm', 1, true) ~= nil or status ~= 0)
end

function test.threshold_rejects_bad_plant()
    local output, status = dfhack.run_command_silent('autofarm',
                                                   'threshold', '-1', '5')
    expect.true_(output:find('Cannot find plant', 1, true) ~= nil
                 or status ~= 0)
end

local function find_plant(id)
    for _,plant in ipairs(df.global.world.raws.plants.all) do
        if plant.id == id then return plant end
    end
end

function test.threshold_growth()
    local plugin = require('plugins.autofarm')
    -- quarry bushes have a stockpile LEAVES growth in vanilla
    local output, status = dfhack.run_command_silent('autofarm',
                                                     'threshold', '30',
                                                     'BUSH_QUARRY:LEAVES')
    expect.eq(0, status)
    local plant = find_plant('BUSH_QUARRY')
    local data = plugin.autofarm_getData()
    expect.eq(30, data.growth_thresholds[plant.index].LEAVES)
    plugin.autofarm_removeGrowthThreshold(plant.index, 'LEAVES')
    plugin.autofarm_removeThreshold(plant.index)
end

function test.threshold_rejects_bad_growth()
    local output, status = dfhack.run_command_silent('autofarm',
                                                   'threshold', '5',
                                                   'BUSH_QUARRY:NOPE')
    expect.true_(output:find('Cannot find plant or growth', 1, true) ~= nil
                 or status ~= 0)
    -- growths that are not stockpile-marked are also rejected
    output, status = dfhack.run_command_silent('autofarm',
                                             'threshold', '5',
                                             'BUCKWHEAT:FLOWERS')
    expect.true_(output:find('Cannot find plant or growth', 1, true) ~= nil
                 or status ~= 0)
end

function test.growth_threshold_funcs()
    local plugin = require('plugins.autofarm')
    local plant = find_plant('BUSH_QUARRY')
    plugin.autofarm_setGrowthThreshold(plant.index, 'LEAVES', 42)
    local data = plugin.autofarm_getData()
    expect.eq(42, data.growth_thresholds[plant.index].LEAVES)
    plugin.autofarm_removeGrowthThreshold(plant.index, 'LEAVES')
    data = plugin.autofarm_getData()
    expect.nil_(data.growth_thresholds[plant.index])
end

function test.farm_exclusion()
    local plugin = require('plugins.autofarm')
    expect.false_(plugin.autofarm_isFarmExcluded(-1))
    plugin.autofarm_setFarmExcluded(-1, true)
    expect.true_(plugin.autofarm_isFarmExcluded(-1))
    plugin.autofarm_setFarmExcluded(-1, false)
    expect.false_(plugin.autofarm_isFarmExcluded(-1))
end

function test.get_data()
    local data = require('plugins.autofarm').autofarm_getData()
    expect.eq('number', type(data.default_threshold))
    expect.eq('table', type(data.thresholds))
    expect.eq('table', type(data.growth_thresholds))
    expect.eq('table', type(data.last_counts))
    expect.eq('table', type(data.growth_counts))
    expect.eq('table', type(data.excluded_farms))
end
