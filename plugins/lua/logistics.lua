local _ENV = mkmodule('plugins.logistics')

local argparse = require('argparse')
local gui = require('gui')
local overlay = require('plugins.overlay')
local utils = require('utils')
local widgets = require('gui.widgets')

local function make_stat(name, stockpile_number, stats, configs)
    return {
        enabled=configs[stockpile_number] and configs[stockpile_number][name] == 'true',
        designated=stats[name..'_designated'][stockpile_number] or 0,
        can_designate=stats[name..'_can_designate'][stockpile_number] or 0,
    }
end

function getStockpileData()
    local stats, configs = logistics_getStockpileData()
    local data = {}
    for _,bld in ipairs(df.global.world.buildings.other.STOCKPILE) do
        local stockpile_number, name = bld.stockpile_number, bld.name
        local sort_key = tostring(name):lower()
        if #name == 0 then
            name = ('Stockpile #%d'):format(bld.stockpile_number)
            sort_key = ('stockpile #%09d'):format(bld.stockpile_number)
        end
        table.insert(data, {
            stockpile_number=stockpile_number,
            name=name,
            sort_key=sort_key,
            melt=make_stat('melt', stockpile_number, stats, configs),
            trade=make_stat('trade', stockpile_number, stats, configs),
            dump=make_stat('dump', stockpile_number, stats, configs),
            train=make_stat('train', stockpile_number, stats, configs),
            melt_masterworks=configs[stockpile_number] and configs[stockpile_number].melt_masterworks == 'true',
        })
    end
    table.sort(data, function(a, b) return a.sort_key < b.sort_key end)
    return data
end

local function print_stockpile_data(data)
    local name_len = 4
    for _,sp in ipairs(data) do
        name_len = math.min(40, math.max(name_len, #sp.name))
    end

    local has_melt_mastworks = false

    print('Designated/designatable items in stockpiles:')
    print()
    local fmt = '%6s  %-' .. name_len .. 's  %4s %10s  %5s %11s  %4s %10s  %5s %11s';
    print(fmt:format('number', 'name', 'melt', 'melt items', 'trade', 'trade items', 'dump', 'dump items', 'train', 'train items'))
    local function uline(len) return ('-'):rep(len) end
    print(fmt:format(uline(6), uline(name_len), uline(4), uline(10), uline(5), uline(11), uline(4), uline(10), uline(5), uline(11)))
    local function get_enab(stats, ch) return ('[%s]'):format(stats.enabled and (ch or 'x') or ' ') end
    local function get_dstat(stats) return ('%d/%d'):format(stats.designated, stats.designated + stats.can_designate) end
    for _,sp in ipairs(data) do
        has_melt_mastworks = has_melt_mastworks or sp.melt_masterworks
        print(fmt:format(sp.stockpile_number, sp.name, get_enab(sp.melt, sp.melt_masterworks and 'X'), get_dstat(sp.melt),
            get_enab(sp.trade), get_dstat(sp.trade), get_enab(sp.dump), get_dstat(sp.dump), get_enab(sp.train), get_dstat(sp.train)))
    end
    if has_melt_mastworks then
        print()
        print('An "X" in the "melt" column indicates that masterworks in the stockpile will be melted.')
    end
end

local function print_status()
    print(('logistics is %sactively monitoring stockpiles and marking items')
            :format(isEnabled() and '' or 'not '))

    if df.global.gamemode ~= df.game_mode.DWARF or not dfhack.isMapLoaded() or not dfhack.isSiteLoaded() then
        return
    end

    print()
    print(('%sutoretraining partially trained animals'):format(
        logistics_getFeature('autoretrain') and 'A' or 'Not a'))

    local data = getStockpileData()
    print()
    if not data[1] then
        print 'No stockpiles configured'
    else
        print_stockpile_data(data)
    end

    local global_stats = logistics_getGlobalCounts()
    print()
    print(('Total items marked for melting: %5d'):format(global_stats.total_melt))
    print(('Total items marked for trading: %5d'):format(global_stats.total_trade))
    print(('Total items marked for dumping: %5d'):format(global_stats.total_dump))
    print(('Total animals marked for training: %2d'):format(global_stats.total_train))
end

local function for_stockpiles(opts, fn)
    if not opts.sp then
        local selected_sp = dfhack.gui.getSelectedStockpile()
        if not selected_sp then qerror('please specify or select a stockpile') end
        fn(selected_sp.stockpile_number)
        return
    end
    for _,sp in ipairs(argparse.stringList(opts.sp)) do
        fn(sp)
    end
end

local function do_add_stockpile_config(features, opts)
    for_stockpiles(opts, function(sp)
        local configs = logistics_getStockpileConfigs(tonumber(sp) or sp)
        if not configs then
            dfhack.printerr('invalid stockpile: '..sp)
        else
            for _,config in ipairs(configs) do
                logistics_setStockpileConfig(config.stockpile_number,
                    features.melt or config.melt == 1,
                    features.trade or config.trade == 1,
                    features.dump or config.dump == 1,
                    features.train or config.train == 1,
                    not not opts.melt_masterworks)
            end
        end
    end)
end

local function do_clear_stockpile_config(all, opts)
    if all then
        logistics_clearAllStockpileConfigs()
        return
    end
    for_stockpiles(opts, function(sp)
        logistics_clearStockpileConfig(tonumber(sp) or sp)
    end)
end

local function do_set_feature(enabled, feature)
    if not logistics_setFeature(enabled, feature) then
        qerror(('unknown feature: "%s"'):format(feature))
    end
end

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    return argparse.processArgsGetopt(args, {
        {'h', 'help', handler=function() opts.help = true end},
        {'m', 'melt-masterworks', handler=function() opts.melt_masterworks = true end},
        {'s', 'stockpile', hasArg=true, handler=function(arg) opts.sp = arg end},
    })
end

function parse_commandline(args)
    local opts = {}
    local positionals = process_args(opts, args)

    if opts.help or not positionals then
        return false
    end

    local command = table.remove(positionals, 1)
    if not command or command == 'status' then
        print_status()
    elseif command == 'now' then
        logistics_cycle()
    elseif command == 'add' then
        do_add_stockpile_config(utils.invert(positionals), opts)
    elseif command == 'clear' then
        do_clear_stockpile_config(utils.invert(positionals).all, opts)
    elseif command == 'enable' or command == 'disable' then
        do_set_feature(command == 'enable', positionals[1])
    else
        return false
    end

    return true
end

---------------------------------
-- AutoretrainOverlay
--

AutoretrainOverlay = defclass(AutoretrainOverlay, overlay.OverlayWidget)
AutoretrainOverlay.ATTRS{
    desc='Adds toggle to pets screen for autoretraining partially trained pets.',
    default_pos={x=51, y=-4},
    default_enabled=true,
    viewscreens='dwarfmode/Info/CREATURES/PET',
    frame={w=36, h=5},
    frame_inset={l=0, r=0, t=2, b=0},
}

local function refresh_list()
    local pet_list = dfhack.gui.getWidget(
        df.global.game.main_interface.info.creatures, 'Tabs', 'Pets/Livestock', 0, 0)
    if pet_list then
        pet_list.sort_flags.NEEDS_RESORTED = true
    end
end

function AutoretrainOverlay:init()
    local panel = widgets.Panel{
        frame={t=0, h=3},
        frame_style=gui.FRAME_MEDIUM,
        frame_background=gui.CLEAR_PEN,
        subviews={
            widgets.ToggleHotkeyLabel{
                view_id='toggle',
                key='CUSTOM_CTRL_A',
                label='Autoretrain livestock:',
                on_change=function(val)
                    logistics_setFeature(val, 'autoretrain')
                    if (val) then refresh_list() end
                end,
            },
        },
    }
    self:addviews{
        panel,
        widgets.HelpButton{command='logistics'},
    }
end

function AutoretrainOverlay:render(dc)
    self.subviews.toggle:setOption(logistics_getFeature('autoretrain'))
    AutoretrainOverlay.super.render(self, dc)
end

function AutoretrainOverlay:preUpdateLayout(parent_rect)
    self.frame_inset.t = parent_rect.width >= 143 and 0 or 2
end

OVERLAY_WIDGETS = {autoretrain=AutoretrainOverlay}

return _ENV
