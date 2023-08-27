-- Adjusts properties of caravans and provides overlays for enhanced trading
--@ module = true

local movegoods = reqscript('internal/caravan/movegoods')
local trade = reqscript('internal/caravan/trade')
local tradeagreement = reqscript('internal/caravan/tradeagreement')

dfhack.onStateChange.caravanTradeOverlay = function(code)
    if code == SC_WORLD_UNLOADED then
        trade.trader_selected_state = {}
        trade.broker_selected_state = {}
        trade.handle_ctrl_click_on_render = false
        trade.handle_shift_click_on_render = false
    end
end

OVERLAY_WIDGETS = {
    trade=trade.TradeOverlay,
    tradebanner=trade.TradeBannerOverlay,
    tradeagreement=tradeagreement.TradeAgreementOverlay,
    movegoods=movegoods.MoveGoodsOverlay,
    assigntrade=movegoods.AssignTradeOverlay,
}

INTERESTING_FLAGS = {
    casualty = 'Casualty',
    hardship = 'Encountered hardship',
    seized = 'Goods seized',
    offended = 'Offended'
}
local caravans = df.global.plotinfo.caravans

local function caravans_from_ids(ids)
    if not ids or #ids == 0 then
        return caravans
    end

    local c = {} --as:df.caravan_state[]
    for _,id in ipairs(ids) do
        id = tonumber(id)
        if id then
            c[id] = caravans[id]
        end
    end
    return c
end

function bring_back(car)
    if car.trade_state ~= df.caravan_state.T_trade_state.AtDepot then
        car.trade_state = df.caravan_state.T_trade_state.Approaching
    end
end

local commands = {}

function commands.list()
    for id, car in pairs(caravans) do
        print(dfhack.df2console(('%d: %s caravan from %s'):format(
            id,
            df.creature_raw.find(df.historical_entity.find(car.entity).race).name[2], -- adjective
            dfhack.TranslateName(df.historical_entity.find(car.entity).name)
        )))
        print('  ' .. (df.caravan_state.T_trade_state[car.trade_state] or ('Unknown state: ' .. car.trade_state)))
        print(('  %d day(s) remaining'):format(math.floor(car.time_remaining / 120)))
        for flag, msg in pairs(INTERESTING_FLAGS) do
            if car.flags[flag] then
                print('  ' .. msg)
            end
        end
    end
end

function commands.extend(days, ...)
    days = tonumber(days or 7) or qerror('invalid number of days: ' .. days) --luacheck: retype
    for id, car in pairs(caravans_from_ids{...}) do
        car.time_remaining = car.time_remaining + (days * 120)
        bring_back(car)
    end
end

function commands.happy(...)
    for id, car in pairs(caravans_from_ids{...}) do
        -- all flags default to false
        car.flags.whole = 0
        bring_back(car)
    end
end

function commands.leave(...)
    for id, car in pairs(caravans_from_ids{...}) do
        car.trade_state = df.caravan_state.T_trade_state.Leaving
    end
end

local function isDisconnectedPackAnimal(unit)
    if unit.following then
        local dragger = unit.following
        return (
            unit.relationship_ids[ df.unit_relationship_type.Dragger ] == -1 and
            dragger.relationship_ids[ df.unit_relationship_type.Draggee ] == -1
        )
    end
end

local function getPrintableUnitName(unit)
    local visible_name = dfhack.units.getVisibleName(unit)
    local profession_name = dfhack.units.getProfessionName(unit)
    if visible_name.has_name then
        return ('%s (%s)'):format(dfhack.TranslateName(visible_name), profession_name)
    end
    return profession_name  -- for unnamed animals
end

local function rejoin_pack_animals()
    print('Reconnecting disconnected pack animals...')
    local found = false
    for _, unit in pairs(df.global.world.units.active) do
        if unit.flags1.merchant and isDisconnectedPackAnimal(unit) then
            local dragger = unit.following
            print(('  %s  <->  %s'):format(
                dfhack.df2console(getPrintableUnitName(unit)),
                dfhack.df2console(getPrintableUnitName(dragger))
            ))
            unit.relationship_ids[ df.unit_relationship_type.Dragger ] = dragger.id
            dragger.relationship_ids[ df.unit_relationship_type.Draggee ] = unit.id
            found = true
        end
    end
    if (found) then
        print('All pack animals reconnected.')
    else
        print('No disconnected pack animals found.')
    end
end

function commands.unload()
    rejoin_pack_animals()
end

function commands.help()
    print(dfhack.script_help())
end

function main(args)
    local command = table.remove(args, 1) or 'list'
    if commands[command] then
        commands[command](table.unpack(args))
    else
        commands.help()
        print()
        qerror("No such command: " .. command)
    end
end

if not dfhack_flags.module then
    main{...}
end
