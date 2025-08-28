local _ENV = mkmodule('plugins.preserve-rooms')

local argparse = require('argparse')
local gui = require('gui')
local overlay = require('plugins.overlay')
local utils = require('utils')
local widgets = require('gui.widgets')

local GLOBAL_KEY = 'preserve-rooms'

DEBUG = DEBUG or false

------------------
-- public API
--

-- updated on world load
code_lookup = code_lookup or {}

function assignToRole(code, bld)
    local group_codes = code_lookup[code:lower()]
    if not group_codes then
        dfhack.printerr('Noble or administrator role not found: ' .. code)
    end
    preserve_rooms_assignToRole(group_codes, bld and bld.id or -1)
end

------------------
-- command line
--

local function print_status()
    local features, stats = preserve_rooms_getState()
    print('Features:')
    for feature,enabled in pairs(features) do
        print(('  %20s: %s'):format(feature, enabled))
    end
    print()
    print('Rooms reserved for traveling units:', stats.reservations)
    print('Managing assignments for noble rooms:', stats.nobles)

    if DEBUG then
        for k,v in pairs(preserve_rooms_getDebugState()) do
            local elems = v:split(',')
            if elems[1] == '' then
                table.remove(elems, 1)
            end
            print(('%s (%d):'):format(k, #elems))
            for _,elem in ipairs(elems) do
                print('  ' .. elem)
            end
            print()
        end
    end
end

local function do_set_feature(enabled, feature)
    if not preserve_rooms_setFeature(enabled, feature) then
        qerror(('unknown feature: "%s"'):format(feature))
    end
end

local function do_reset_feature(feature)
    if not preserve_rooms_resetFeatureState(feature) then
        qerror(('unknown feature: "%s"'):format(feature))
    end
end

function parse_commandline(args)
    local opts = {}
    local positionals = argparse.processArgsGetopt(args, {
        {'h', 'help', handler=function() opts.help = true end},
    })

    if opts.help or not positionals or positionals[1] == 'help' then
        return false
    end

    local command = table.remove(positionals, 1)
    if not command or command == 'status' then
        print_status()
    elseif command == 'now' then
        preserve_rooms_cycle()
    elseif command == 'enable' or command == 'disable' then
        do_set_feature(command == 'enable', positionals[1])
    elseif command == 'reset' then
        do_reset_feature(positionals[1])
    else
        return false
    end

    return true
end

----------------------
-- ReservedWidget
--

ReservedWidget = defclass(ReservedWidget, overlay.OverlayWidget)
ReservedWidget.ATTRS{
    desc='Shows whether a zone has been reserved for a unit or role.',
    default_enabled=true,
    default_pos={x=37, y=6},
    viewscreens={
        'dwarfmode/Zone/Some/Bedroom',
        'dwarfmode/Zone/Some/DiningHall',
        'dwarfmode/Zone/Some/Office',
        'dwarfmode/Zone/Some/Tomb',
    },
    frame={w=44, h=15},
    version=2,
}

new_world_loaded = true

local CONFLICTING_TOOLTIPS = utils.invert{
    df.main_hover_instruction.ZONE_REPAINT,
    df.main_hover_instruction.ZONE_SUSPEND,
    df.main_hover_instruction.ZONE_REMOVE_EXISTING,
    df.main_hover_instruction.ZONE_ASSIGN_LOCATION,
}

function ReservedWidget:init()
    self.code_to_idx = {}

    self:addviews{
        widgets.Panel{
            view_id='pause_mask',
            frame={t=3, l=0, w=4, h=3},
        },
        widgets.Panel{
            view_id='add_mask',
            frame={t=6, l=4, w=4, h=3},
        },
        widgets.Panel{
            frame={t=0, l=9},
            visible=function()
                local scr = dfhack.gui.getDFViewscreen(true)
                return not dfhack.gui.matchFocusString('dwarfmode/UnitSelector', scr) and
                    not dfhack.gui.matchFocusString('dwarfmode/LocationSelector', scr) and
                    not dfhack.gui.matchFocusString('dwarfmode/LocationDetails', scr)
            end,
            subviews={
                widgets.Panel{
                    visible=function()
                        return not preserve_rooms_isReserved() and preserve_rooms_getFeature('track-roles')
                    end,
                    subviews={
                        widgets.CycleHotkeyLabel{
                            view_id='role',
                            frame={t=1, l=1, r=1, h=2},
                            frame_background=gui.CLEAR_PEN,
                            label='Autoassign to holder of role:',
                            label_below=true,
                            key='CUSTOM_SHIFT_S',
                            options={{label='None', value={''}, pen=COLOR_YELLOW}},
                            on_change=function(group_codes)
                                self.subviews.list:setSelected(self.code_to_idx[group_codes[1]] or 1)
                                preserve_rooms_assignToRole(group_codes, -1)
                            end,
                        },
                        widgets.Panel{
                            view_id='hover_trigger',
                            frame={t=0, l=0, r=0, h=4},
                            frame_style=gui.FRAME_MEDIUM,
                            visible=true,
                        },
                        widgets.Panel{
                            view_id='hover_expansion',
                            frame_style=gui.FRAME_MEDIUM,
                            visible=false,
                            subviews={
                                widgets.Panel{
                                    frame={t=2},
                                    frame_background=gui.CLEAR_PEN,
                                },
                                widgets.List{
                                    view_id='list',
                                    frame={t=3, l=1},
                                    frame_background=gui.CLEAR_PEN,
                                    on_submit=function(idx)
                                        self.subviews.role:setOption(idx, true)
                                    end,
                                    choices={'None'},
                                },
                            },
                        },
                    },
                },
                widgets.Panel{
                    frame={t=0, h=5},
                    frame_style=gui.FRAME_MEDIUM,
                    frame_background=gui.CLEAR_PEN,
                    visible=preserve_rooms_isReserved,
                    subviews={
                        widgets.Label{
                            frame={t=0, l=0},
                            text={
                                'Reserved for traveling unit:', NEWLINE,
                                {gap=1, text=preserve_rooms_getReservationName, pen=COLOR_YELLOW},
                            },
                        },
                        widgets.HotkeyLabel{
                            frame={t=2, l=0},
                            key='CUSTOM_SHIFT_R',
                            label='Clear reservation',
                            on_activate=preserve_rooms_clearReservation,
                        },
                    },
                },
                widgets.HelpButton{
                    command='preserve-rooms',
                    visible=function()
                        return preserve_rooms_isReserved() or preserve_rooms_getFeature('track-roles')
                    end,
                },
            },
        },
    }
end

local mi = df.global.game.main_interface

function ReservedWidget:onInput(keys)
    if keys._MOUSE_L and
        (preserve_rooms_isReserved() or preserve_rooms_getRoleAssignment() ~= '')
    then
        if self.subviews.pause_mask:getMousePos() then return true end
        if self.subviews.add_mask:getMousePos() then return true end
    end
    if CONFLICTING_TOOLTIPS[mi.current_hover] then
        return false
    end
    if mi.entering_building_name and keys._STRING then
        return false
    end
    return ReservedWidget.super.onInput(self, keys)
end

function ReservedWidget:render(dc)
    if new_world_loaded then
        self:refresh_role_list()
        new_world_loaded = false
    end

    local code = preserve_rooms_getRoleAssignment()
    local role = self.subviews.role
    if code ~= role:getOptionValue()[1] then
        local idx = self.code_to_idx[code] or 1
        role:setOption(idx)
        self.subviews.list:setSelected(idx)
    end

    local hover_expansion = self.subviews.hover_expansion
    if hover_expansion.visible and not hover_expansion:getMouseFramePos() then
        hover_expansion.visible = false
    elseif self.subviews.hover_trigger:getMousePos() then
        hover_expansion.visible = true
    end

    if not CONFLICTING_TOOLTIPS[mi.current_hover] then
        ReservedWidget.super.render(self, dc)
    end
end

local function to_title_case(str)
    return dfhack.capitalizeStringWords(dfhack.lowerCp437(str:gsub('_', ' ')))
end

local function add_options(options, choices, codes)
    for _,group_codes in ipairs(codes) do
        local names = {}
        for _,code in ipairs(group_codes) do
            table.insert(names, to_title_case(code))
        end
        local name = table.concat(names, '/')
        table.insert(options, {label=name, value=group_codes, pen=COLOR_YELLOW})
        table.insert(choices, name)
    end
end

-- updated on world load
codes = codes or {}

function ReservedWidget:refresh_role_list()
    local options, choices = {{label='None', value={''}, pen=COLOR_YELLOW}}, {'None'}
    add_options(options, choices, codes)

    self.code_to_idx = {['']=1}
    for idx,group_codes in ipairs(codes) do
        self.code_to_idx[group_codes[1]] = idx + 1 -- offset for None option
    end

    self.subviews.role.options = options
    self.subviews.role:setOption(1)
    self.subviews.list:setChoices(choices, 1)
end

OVERLAY_WIDGETS = {
    reserved=ReservedWidget,
}

local function add_positions(positions, entity)
    if not entity then return end
    for _,position in ipairs(entity.positions.own) do
        positions[position.id] = {
            id=position.id,
            code=position.code,
            replaced_by=position.replaced_by,
            required_value=position.required_office + position.required_bedroom + position.required_dining + position.required_tomb,
        }
    end
end

local function get_codes(positions)
    -- group by promotion chain
    -- grouped[id] is the group of positions that can be (eventually) promoted to the id'd position
    local grouped = {}
    for id,position in pairs(positions) do
        if position.replaced_by == -1 then
            ensure_key(grouped, id)[id] = position
        else
            -- replaced-by links may be cyclic. this code will behave "incorrectly" in the event
            -- of a cycle but will not hang. a proper fix is still needed. see issue DFHack/dfhack#5538
            local parent = positions[position.replaced_by]
            local counter = 0
            while parent.replaced_by ~= -1 and counter < 10 do
                parent = positions[parent.replaced_by]
                counter = counter + 1
            end
            ensure_key(grouped, parent.id)[id] = position
        end
    end

    -- add reverse links for promotion chains and record whether it has a room requirement
    for _,group in pairs(grouped) do
        for _,position in pairs(group) do
            if position.replaced_by ~= -1 then
                group[position.replaced_by].prev = position.id
            end
        end
    end

    -- produce combined codes
    local diplay_codes = {}
    for id,group in pairs(grouped) do
        local group_codes = {}
        local position = group[id]
        while position do
            table.insert(group_codes, 1, position.code)
            group_codes.required_value = math.max(group_codes.required_value or 0, position.required_value)
            position=group[position.prev]
        end
        table.insert(diplay_codes, group_codes)
    end
    table.sort(diplay_codes, function(a, b)
        if a.required_value == b.required_value then
            return a[1] < b[1]
        end
        return a.required_value > b.required_value
    end)

    return diplay_codes
end

local function get_api_lookup_table(codes)
    local lookup = {}
    for _,group_codes in ipairs(codes) do
        for _,code in ipairs(group_codes) do
            lookup[code:lower()] = group_codes
        end
    end
    return lookup
end

dfhack.onStateChange[GLOBAL_KEY] = function(sc)
    if sc ~= SC_MAP_LOADED or not dfhack.world.isFortressMode() then
        return
    end
    local positions = {}
    add_positions(positions, df.historical_entity.find(df.global.plotinfo.civ_id));
    add_positions(positions, df.historical_entity.find(df.global.plotinfo.group_id));
    codes = get_codes(positions)
    code_lookup = get_api_lookup_table(codes)
    new_world_loaded = true
end

return _ENV
