local _ENV = mkmodule('plugins.logistics')

local argparse = require('argparse')
local gui = require('gui')
local overlay = require('plugins.overlay')
local utils = require('utils')
local widgets = require('gui.widgets')

local function make_stat(name, stockpile_number, stats, configs)
    return {
        enabled=(configs[stockpile_number] ~= nil) and (configs[stockpile_number][name] == 'true'),
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
            forbid=make_stat('forbid', stockpile_number, stats, configs),
            claim=make_stat('claim', stockpile_number, stats, configs),
            melt_masterworks=(configs[stockpile_number] ~= nil) and (configs[stockpile_number].melt_masterworks == 'true'),
        })
    end
    table.sort(data, function(a, b) return a.sort_key < b.sort_key end)
    return data
end

local function print_stockpile_data(data)
    local name_len = 22 -- "Total Stockpiled Items"
    for _,sp in ipairs(data) do
        name_len = math.min(40, math.max(name_len, #sp.name))
    end

    local function uline(len) return ('-'):rep(len) end
    local function space(len) return (' '):rep(len) end
    print('Stockpiles' .. space(name_len) .. space(1) .. '[Enabled] Auto-Designations' .. space(11) .. 'Designated / Designatable Item Counts')
    --          sp.#  __sp.name____________  __melt__   _trade__   __dump__   _train__   _forbid
    local fmt = ' %6s  %-' .. name_len .. 's  %-6s%-11s  %-6s%-11s  %-6s%-11s  %-6s%-11s  %-6s%-11s';
    print(uline(name_len) .. uline(105))
    print(fmt:format(
            'number' , 'name',
        --  '  [x]  ', '#### / ####'
            '  melt' , '   items',
            ' trade' , '   items',
            '  dump' , '   items',
            ' train' , '   items',
            ' forbid', '  items'))
    print(fmt:format(
            uline(6), uline(name_len),
            uline(6), uline(11),  -- melt
            uline(6), uline(11),  -- trade
            uline(6), uline(11),  -- dump
            uline(6), uline(11),  -- train
            uline(6), uline(11))) -- forbid
    local has_melt_mastworks, has_claim_enabled = false, false
    local function get_enab(stats, ch) return ('  [%s]'):format(stats.enabled and (ch or 'x') or ' ') end
    local function get_dstat(stats) return ('%4d / %-4d'):format(stats.designated, stats.designated + stats.can_designate) end
    local sp_totals = {
            melt_e = 0, melt_d = 0, melt_dable = 0,
            trade_e = 0, trade_d = 0, trade_dable = 0,
            dump_e = 0, dump_d = 0, dump_dable = 0,
            train_e = 0, train_d = 0, train_dable = 0,
            forbid_e = 0, forbid_d = 0, forbid_dable = 0}
    for _,sp in ipairs(data) do
        if (sp.melt_masterworks ~= nil) then has_melt_mastworks = has_melt_mastworks or sp.melt_masterworks end -- TODO: initialize sp.melt_masterworks in logistics.cpp
        has_claim_enabled = has_claim_enabled or sp.claim.enabled
        local forbid_or_claim = (sp.claim.enabled and sp.claim) or (sp.forbid.enabled and sp.forbid) or sp.forbid
        print(fmt:format(sp.stockpile_number, sp.name,
                get_enab(sp.melt, sp.melt_masterworks and 'M' or 'm'), get_dstat(sp.melt),
                get_enab(sp.trade,'t'), get_dstat(sp.trade),
                get_enab(sp.dump,'d'), get_dstat(sp.dump),
                get_enab(sp.train,'a'), get_dstat(sp.train),
                get_enab(forbid_or_claim, (sp.claim.enabled and 'C') or (sp.forbid.enabled and 'f')), get_dstat(sp.forbid)))
        sp_totals.melt_d = sp_totals.melt_d + sp.melt.designated
        sp_totals.melt_dable = sp_totals.melt_dable + sp.melt.designated + sp.melt.can_designate
        if sp.trade.enabled then sp_totals.trade_e = sp_totals.trade_e + 1 end
        sp_totals.trade_d = sp_totals.trade_d + sp.trade.designated
        sp_totals.trade_dable = sp_totals.trade_dable + sp.trade.designated + sp.trade.can_designate
        if sp.dump.enabled then sp_totals.dump_e = sp_totals.dump_e + 1 end
        sp_totals.dump_d = sp_totals.dump_d + sp.dump.designated
        sp_totals.dump_dable = sp_totals.dump_dable + sp.dump.designated + sp.dump.can_designate
        if sp.train.enabled then sp_totals.train_e = sp_totals.train_e + 1 end
        sp_totals.train_d = sp_totals.train_d + sp.train.designated
        sp_totals.train_dable = sp_totals.train_dable + sp.train.designated + sp.train.can_designate
        if sp.forbid.enabled or sp.claim.enabled then sp_totals.forbid_e = sp_totals.forbid_e + 1 end
        sp_totals.forbid_d = sp_totals.forbid_d + sp.forbid.designated
        sp_totals.forbid_dable = sp_totals.forbid_dable + sp.forbid.designated + sp.forbid.can_designate
    end
    print(fmt:format(
            uline(6), uline(name_len),
            uline(6), uline(11),  -- melt
            uline(6), uline(11),  -- trade
            uline(6), uline(11),  -- dump
            uline(6), uline(11),  -- train
            uline(6), uline(11))) -- forbid
    local function get_sp_totals(d, dable) return ('%4d / %-4d'):format(d, dable) end
    print(fmt:format('', 'Stockpiled Item Totals',
            '', get_sp_totals(sp_totals.melt_d, sp_totals.melt_dable),
            '', get_sp_totals(sp_totals.trade_d, sp_totals.trade_dable),
            '', get_sp_totals(sp_totals.dump_d, sp_totals.dump_dable),
            '', get_sp_totals(sp_totals.train_d, sp_totals.train_dable),
            '', get_sp_totals(sp_totals.forbid_d, sp_totals.forbid_dable)))
    print(uline(name_len) .. uline(105))
    if has_melt_mastworks or has_claim_enabled then
        print()
        if has_melt_mastworks then print('[M] in the "melt" column indicates that masterworks in the stockpile will be melted.') end
        if has_claim_enabled then print('[C] in the "forbid" column indicates that items in the stockpile will be claimed (unforbidden).') end
    end
end

local function print_status()
    print('logistics status')
    print((' - %sactively monitoring stockpiles and designating items')
            :format(isEnabled() and '' or 'not '))

    if df.global.gamemode ~= df.game_mode.DWARF or not dfhack.isMapLoaded() or not dfhack.isSiteLoaded() then
        return
    end

    print((' - %sautoretraining partially trained animals'):format(
        logistics_getFeature('autoretrain') and '' or 'not '))

    local data = getStockpileData()
    print()
    if not data[1] then
        print 'No stockpiles configured'
    else
        print_stockpile_data(data)
    end

    local global_stats = logistics_getGlobalCounts()
    print()
    print('Total reachable items:')
    print((' - Items designated for melting: %5d'):format(global_stats.total_melt))
    print((' - Items designated for trading: %5d'):format(global_stats.total_trade))
    print((' - Items designated for dumping: %5d'):format(global_stats.total_dump))
    print((' - Animals designated for train: %5d'):format(global_stats.total_train))  -- TODO: unlike the others, this is limited to animals in stockpiles
    print((' - Forbidden items             : %5d'):format(global_stats.total_forbid)) -- TODO: this should exclude unreachable items and buildings
    print((' - All (unforbidden) items     : %5d'):format(global_stats.total_claim))  -- TODO: same ^^
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
                if (features.forbid) then config.forbid = 1
                elseif (features.claim) then config.forbid = 2
                end
                logistics_setStockpileConfig(config.stockpile_number,
                        features.melt or config.melt == 1,
                        features.trade or config.trade == 1,
                        features.dump or config.dump == 1,
                        features.train or config.train == 1,
                        config.forbid,
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
