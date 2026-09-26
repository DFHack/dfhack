config.mode = 'fortress'
config.target = 'logistics'

local logistics = require('plugins.logistics')

local saved_autoretrain = logistics.logistics_getFeature('autoretrain')
local saved_configs = {}

config.wrapper = function(test_fn)
    return dfhack.with_finalize(function()
        logistics.logistics_setFeature(saved_autoretrain, 'autoretrain')
        for sp_number, configs in pairs(saved_configs) do
            for _,c in ipairs(configs) do
                logistics.logistics_setStockpileConfig(
                    c.stockpile_number, c.melt == 1, c.trade == 1,
                    c.dump == 1, c.train == 1, c.forbid,
                    c.melt_masterworks == 1)
            end
        end
    end, test_fn)
end

local function snapshot_config(sp_number)
    if saved_configs[sp_number] then return end
    saved_configs[sp_number] =
        logistics.logistics_getStockpileConfigs(sp_number) or {}
end

local function get_first_stockpile()
    local stockpiles = df.global.world.buildings.other.STOCKPILE
    if #stockpiles == 0 then return nil end
    return stockpiles[0].stockpile_number
end

local function get_stats_for(sp_number)
    for _,sp in ipairs(logistics.getStockpileData().sp_stats) do
        if sp.sp_number == sp_number then return sp end
    end
end

function test.help()
    expect.false_(logistics.parse_commandline{'help'})
    expect.false_(logistics.parse_commandline{'-h'})
end

function test.status()
    expect.true_(logistics.parse_commandline{})
    expect.true_(logistics.parse_commandline{'status'})
end

function test.unrecognized_command()
    expect.false_(logistics.parse_commandline{'bogus'})
end

function test.autoretrain_toggle()
    expect.true_(logistics.parse_commandline{'enable', 'autoretrain'})
    expect.true_(logistics.logistics_getFeature('autoretrain'))
    expect.true_(logistics.parse_commandline{'disable', 'autoretrain'})
    expect.false_(logistics.logistics_getFeature('autoretrain'))
end

function test.unknown_feature()
    expect.error_match('unknown feature', function()
        logistics.parse_commandline{'enable', 'bogus_feature'} end)
end

function test.missing_feature()
    expect.error(function()
        logistics.parse_commandline{'enable'} end)
end

function test.getStockpileData()
    local data = logistics.getStockpileData()
    expect.eq(#df.global.world.buildings.other.STOCKPILE,
              #data.sp_stats)
    -- totals are the sum of the per-stockpile stats
    for _,desig in ipairs{'melt', 'trade', 'dump', 'train', 'forbid'} do
        local designated, designatable = 0, 0
        for _,sp in ipairs(data.sp_stats) do
            designated = designated + sp[desig].designated
            designatable = designatable + sp[desig].designatable
        end
        expect.eq(designated, data.sp_totals[desig].designated)
        expect.eq(designatable, data.sp_totals[desig].designatable)
    end
end

function test.add_and_clear_config()
    local sp_number = get_first_stockpile()
    if not sp_number then return end
    snapshot_config(sp_number)
    expect.true_(logistics.parse_commandline{
        'add', 'melt', '-s', tostring(sp_number)})
    local sp = get_stats_for(sp_number)
    expect.true_(sp.melt.enabled)
    expect.false_(sp.trade.enabled)
    expect.true_(logistics.parse_commandline{
        'add', 'trade', 'dump', '-s', tostring(sp_number)})
    sp = get_stats_for(sp_number)
    expect.true_(sp.melt.enabled)
    expect.true_(sp.trade.enabled)
    expect.true_(sp.dump.enabled)
    expect.true_(logistics.parse_commandline{
        'clear', '-s', tostring(sp_number)})
    sp = get_stats_for(sp_number)
    expect.false_(sp.melt.enabled)
    expect.false_(sp.trade.enabled)
    expect.false_(sp.dump.enabled)
end

function test.add_invalid_stockpile()
    expect.printerr_match('invalid stockpile', function()
        logistics.parse_commandline{'add', 'melt', '-s', '999999'} end)
end

function test.cycle()
    -- a cycle can designate items if any stockpile configs are enabled;
    -- only run it when the fort has none so no state is mutated
    for _,desig in ipairs{'melt', 'trade', 'dump', 'train', 'forbid', 'claim'} do
        if logistics.getStockpileData().sp_totals[desig].enabled_count > 0 then
            return
        end
    end
    expect.true_(logistics.parse_commandline{'now'})
end
