local _ENV = mkmodule('plugins.logistics')

local argparse = require('argparse')
local gui = require('gui')
local overlay = require('plugins.overlay')
local utils = require('utils')
local widgets = require('gui.widgets')

-------------------------------------
-- logistics status console output
--

local function make_stat(desig, sp_number, stats, configs)
    local stat = {
        enabled=(configs[sp_number] ~= nil) and (configs[sp_number][desig] == 'true'),
        designated=stats[desig..'_designated'][sp_number] or 0,
        can_designate=stats[desig..'_can_designate'][sp_number] or 0,
        designatable=nil,
    }
    stat.designatable = stat.designated + stat.can_designate
    return stat
end

function getStockpileData()
    local stats, configs = logistics_getStockpileData()
    local data={sp_stats={}, sp_totals={}, sp_name_max_len=0}
    data.sp_totals = {
        melt={enabled_count=0, designated=0, designatable=0},
        trade={enabled_count=0, designated=0, designatable=0},
        dump={enabled_count=0, designated=0, designatable=0},
        train={enabled_count=0, designated=0, designatable=0},
        forbid={enabled_count=0, designated=0, designatable=0},
        claim={enabled_count=0}
    }
    local function inc_totals(total, sp)
        if sp.enabled then total.enabled_count = total.enabled_count + 1 end
        if nil == total.designated or nil == total.designatable then return end
        total.designated = total.designated + sp.designated
        total.designatable = total.designatable + sp.designatable
    end

    for i,bld in ipairs(df.global.world.buildings.other.STOCKPILE) do
        local sp_number, sp_name = bld.stockpile_number, bld.name
        local sort_key = tostring(sp_name):lower()
        if #sp_name == 0 then
            sp_name = ('Stockpile #%d'):format(sp_number)
            sort_key = ('stockpile #%09d'):format(sp_number)
        end
        data.sp_name_max_len = math.max(data.sp_name_max_len, #sp_name)
        table.insert(data.sp_stats, {
            sp_number=sp_number,
            sp_name=sp_name,
            sort_key=sort_key,
            melt=make_stat('melt', sp_number, stats, configs),
            trade=make_stat('trade', sp_number, stats, configs),
            dump=make_stat('dump', sp_number, stats, configs),
            train=make_stat('train', sp_number, stats, configs),
            forbid=make_stat('forbid', sp_number, stats, configs),
            claim=make_stat('claim', sp_number, stats, configs),
            melt_masterworks=(configs[sp_number] ~= nil) and (configs[sp_number].melt_masterworks == 'true'),
        })

        local j = i+1
        inc_totals(data.sp_totals.melt, data.sp_stats[j].melt)
        inc_totals(data.sp_totals.trade, data.sp_stats[j].trade)
        inc_totals(data.sp_totals.dump, data.sp_stats[j].dump)
        inc_totals(data.sp_totals.train, data.sp_stats[j].train)
        inc_totals(data.sp_totals.forbid, data.sp_stats[j].forbid)
        inc_totals(data.sp_totals.claim, data.sp_stats[j].claim)
    end

    table.sort(data.sp_stats, function(a, b) return a.sort_key < b.sort_key end)
    return data
end

local function print_stockpile_data(data)
    -- setup stockpile summary table size and format
    local totals_row_label = 'Total Stockpiled Items'
    local name_len = math.min(40, math.max(data.sp_name_max_len, #totals_row_label))

    -- --          sp.#  sp.name                    '  melt'     trade       dump      train      forbid  ' -- --
    local fmt =  ' %6s  %-' .. name_len .. 's  ' .. '%-6s%-11s  %-6s%-11s  %-6s%-11s  %-6s%-11s  %-6s%-11s' -- --
    local col_labels = fmt:format('number' , 'name',
    -- --                                           '  [x] ' '#### / ####'                                  -- --
                                                    '  melt','   items',
                                                    ' trade','   items',
                                                    '  dump','   items',
                                                    ' train','   items',
                                                    ' forbid','  items')
    -- --                                           '------' '-----------'                                  -- --
    local function uline(len) return ('-'):rep(len) end
    local ulines = fmt:format(uline(6), uline(name_len),
                                                    uline(6), uline(11),  -- melt
                                                    uline(6), uline(11),  -- trade
                                                    uline(6), uline(11),  -- dump
                                                    uline(6), uline(11),  -- train
                                                    uline(6), uline(11))  -- forbid
    -- --                                           '------' '-----------'                                  -- --
    local totals = data.sp_totals
    -- --                                           '  [#] ' '#### / ####'                                  -- --
    local tbl_ln_len = name_len + 105
    --  --------------------------------------------------------------------------------------------------  -- --
    local has_melt_mastworks, has_claim_enabled = false, false

    -- print stockpiles summary table header
    local function space(len) return (' '):rep(len) end
    print('Stockpiles' .. space(name_len) .. space(1) .. '[Enabled] Auto-Designations' .. space(11) .. 'Designated / Designatable Item Counts')
    print(uline(tbl_ln_len))
    print(col_labels)
    print(ulines)

    -- print stockpiles summary table data
    local function get_enab(stats, ch) return ('  [%s]'):format(stats.enabled and (ch or 'x') or ' ') end
    local function get_dstat(stats) return ('%4d / %-4d'):format(stats.designated, stats.designatable) end
    for _,sp in ipairs(data.sp_stats) do
        has_melt_mastworks = has_melt_mastworks or sp.melt_masterworks
        has_claim_enabled = has_claim_enabled or sp.claim.enabled
        local forbid_or_claim = (sp.claim.enabled and sp.claim) or (sp.forbid.enabled and sp.forbid) or sp.forbid
        print(fmt:format(sp.sp_number, sp.sp_name,
                get_enab(sp.melt, sp.melt_masterworks and 'M' or 'm'), get_dstat(sp.melt),
                get_enab(sp.trade,'t'), get_dstat(sp.trade),
                get_enab(sp.dump,'d'), get_dstat(sp.dump),
                get_enab(sp.train,'a'), get_dstat(sp.train),
                get_enab(forbid_or_claim, (sp.claim.enabled and 'C') or (sp.forbid.enabled and 'f')), get_dstat(sp.forbid)))
    end

    -- print stockpile summary table footer
    print(ulines)
    print(fmt:format('', totals_row_label,
            '', get_dstat(totals.melt),
            '', get_dstat(totals.trade),
            '', get_dstat(totals.dump),
            '', get_dstat(totals.train),
            '', get_dstat(totals.forbid)))
    print(uline(tbl_ln_len))
    if has_melt_mastworks or has_claim_enabled then
        print()
        if has_melt_mastworks then print('[M] in the "melt" column indicates that masterworks in the stockpile will be melted.') end
        if has_claim_enabled then print('[C] in the "forbid" column indicates that items in the stockpile will be claimed (unforbidden).') end
    end
end

local function print_status()
    -- print plugin enabled/disabled status
    print('logistics status')
    print((' - %sactively monitoring stockpiles and designating items')
            :format(isEnabled() and '' or 'not '))
    if df.global.gamemode ~= df.game_mode.DWARF or not dfhack.isMapLoaded() or not dfhack.isSiteLoaded() then
        return
    end
    print((' - %sauto-retraining partially trained animals')
            :format(logistics_getFeature('autoretrain') and '' or 'not '))

    -- print stockpile logistics configuration and summary
    local data = getStockpileData()
    print()
    if not data.sp_stats[1] then
        print 'No stockpiles configured'
    else
        print_stockpile_data(data)
    end

    -- print fortress logistics summary of all items
    local global_sp_stats = logistics_getGlobalCounts()
    print()
    print('Total fortress items:')
    print((' - Items designated for melting: %5d'):format(global_sp_stats.total_melt))
    print((' - Items designated for trading: %5d'):format(global_sp_stats.total_trade))
    print((' - Items designated for dumping: %5d'):format(global_sp_stats.total_dump))
    print((' - Animals designated for train: %5d'):format(global_sp_stats.total_train))  -- TODO: fix to include all animals designated for training
    print((' - Forbidden items             : %5d'):format(global_sp_stats.total_forbid)) -- TODO: this should exclude unreachable items
    print((' - All (unforbidden) items     : %5d'):format(global_sp_stats.total_claim))  -- TODO: same ^^ implement in logistics.cpp or use `item` script api?
end

--------------------------
-- configure stockpiles
--

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
                features.claim = features.claim or features.unforbid -- accept 'add unforbid' in addition to 'add claim'
                logistics_setStockpileConfig(config.stockpile_number,
                        features.melt and true or config.melt == 1,
                        features.trade and true or config.trade == 1,
                        features.dump and true or config.dump == 1,
                        features.train and true or config.train == 1,
                        (features.forbid and 1) or (features.claim and 2) or config.forbid,
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

------------------
-- command line
--

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
