local _ENV = mkmodule('plugins.sort')

local gui = require('gui')
local overlay = require('plugins.overlay')
local utils = require('utils')
local widgets = require('gui.widgets')

local CH_UP = string.char(30)
local CH_DN = string.char(31)

local MELEE_WEAPON_SKILLS = {
    df.job_skill.AXE,
    df.job_skill.SWORD,
    df.job_skill.MACE,
    df.job_skill.HAMMER,
    df.job_skill.SPEAR,
    df.job_skill.MELEE_COMBAT, --Fighter
}

local RANGED_WEAPON_SKILLS = {
    df.job_skill.CROSSBOW,
    df.job_skill.RANGED_COMBAT,
}

local LEADERSHIP_SKILLS = {
    df.job_skill.MILITARY_TACTICS,
    df.job_skill.LEADERSHIP,
    df.job_skill.TEACHING,
}

local function sort_noop(a, b)
    -- this function is used as a marker and never actually gets called
    error('sort_noop should not be called')
end

local function sort_by_name_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local name1 = dfhack.TranslateName(dfhack.units.getVisibleName(unit1))
    local name2 = dfhack.TranslateName(dfhack.units.getVisibleName(unit2))
    return utils.compare_name(name1, name2)
end

local function sort_by_name_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    local name1 = dfhack.TranslateName(dfhack.units.getVisibleName(unit1))
    local name2 = dfhack.TranslateName(dfhack.units.getVisibleName(unit2))
    return utils.compare_name(name2, name1)
end

local function sort_by_migrant_wave_desc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    return utils.compare(unit2.id, unit1.id)
end

local function sort_by_migrant_wave_asc(unit_id_1, unit_id_2)
    if unit_id_1 == unit_id_2 then return 0 end
    local unit1 = df.unit.find(unit_id_1)
    local unit2 = df.unit.find(unit_id_2)
    if not unit1 then return -1 end
    if not unit2 then return 1 end
    return utils.compare(unit1.id, unit2.id)
end

local function get_skill(unit_id, skill, unit)
    unit = unit or df.unit.find(unit_id)
    return unit and
        unit.status.current_soul and
        utils.binsearch(unit.status.current_soul.skills, skill, 'id')
end

local function get_max_skill(unit_id, list)
    local unit = df.unit.find(unit_id)
    if not unit or not unit.status.current_soul then return end
    local max
    for _,skill in ipairs(list) do
        local s = get_skill(unit_id, skill, unit)
        if s then
            if not max then
                max = s
            else
                if max.rating == s.rating and max.experience < s.experience then
                    max = s
                elseif max.rating < s.rating then
                    max = s
                end
            end
        end
    end
    return max
end

local function make_sort_by_skill_list_desc(list)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        if unit_id_1 == -1 then return -1 end
        if unit_id_2 == -1 then return 1 end
        local s1 = get_max_skill(unit_id_1, list)
        local s2 = get_max_skill(unit_id_2, list)
        if s1 == s2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        if not s2 then return -1 end
        if not s1 then return 1 end
        if s1.rating ~= s2.rating then
            return utils.compare(s2.rating, s1.rating)
        end
        if s1.experience ~= s2.experience then
            return utils.compare(s2.experience, s1.experience)
        end
        return sort_by_name_desc(unit_id_1, unit_id_2)
    end
end

local function make_sort_by_skill_list_asc(list)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        if unit_id_1 == -1 then return -1 end
        if unit_id_2 == -1 then return 1 end
        local s1 = get_max_skill(unit_id_1, list)
        local s2 = get_max_skill(unit_id_2, list)
        if s1 == s2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        if not s2 then return 1 end
        if not s1 then return -1 end
        if s1.rating ~= s2.rating then
            return utils.compare(s1.rating, s2.rating)
        end
        if s1.experience ~= s2.experience then
            return utils.compare(s1.experience, s2.experience)
        end
        return sort_by_name_desc(unit_id_1, unit_id_2)
    end
end

local function make_sort_by_skill_desc(sort_skill)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        if unit_id_1 == -1 then return -1 end
        if unit_id_2 == -1 then return 1 end
        local s1 = get_skill(unit_id_1, sort_skill)
        local s2 = get_skill(unit_id_2, sort_skill)
        if s1 == s2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        if not s2 then return -1 end
        if not s1 then return 1 end
        if s1.rating ~= s2.rating then
            return utils.compare(s2.rating, s1.rating)
        end
        if s1.experience ~= s2.experience then
            return utils.compare(s2.experience, s1.experience)
        end
        return sort_by_name_desc(unit_id_1, unit_id_2)
    end
end

local function make_sort_by_skill_asc(sort_skill)
    return function(unit_id_1, unit_id_2)
        if unit_id_1 == unit_id_2 then return 0 end
        if unit_id_1 == -1 then return -1 end
        if unit_id_2 == -1 then return 1 end
        local s1 = get_skill(unit_id_1, sort_skill)
        local s2 = get_skill(unit_id_2, sort_skill)
        if s1 == s2 then return sort_by_name_desc(unit_id_1, unit_id_2) end
        if not s2 then return 1 end
        if not s1 then return -1 end
        if s1.rating ~= s2.rating then
            return utils.compare(s1.rating, s2.rating)
        end
        if s1.experience ~= s2.experience then
            return utils.compare(s1.experience, s2.experience)
        end
        return sort_by_name_desc(unit_id_1, unit_id_2)
    end
end

local SORT_FNS = {
    sort_by_any_melee_desc=make_sort_by_skill_list_desc(MELEE_WEAPON_SKILLS),
    sort_by_any_melee_asc=make_sort_by_skill_list_asc(MELEE_WEAPON_SKILLS),
    sort_by_any_ranged_desc=make_sort_by_skill_list_desc(RANGED_WEAPON_SKILLS),
    sort_by_any_ranged_asc=make_sort_by_skill_list_asc(RANGED_WEAPON_SKILLS),
    sort_by_leadership_desc=make_sort_by_skill_list_desc(LEADERSHIP_SKILLS),
    sort_by_leadership_asc=make_sort_by_skill_list_asc(LEADERSHIP_SKILLS),
    sort_by_axe_desc=make_sort_by_skill_desc(df.job_skill.AXE),
    sort_by_axe_asc=make_sort_by_skill_asc(df.job_skill.AXE),
    sort_by_sword_desc=make_sort_by_skill_desc(df.job_skill.SWORD),
    sort_by_sword_asc=make_sort_by_skill_asc(df.job_skill.SWORD),
    sort_by_mace_desc=make_sort_by_skill_desc(df.job_skill.MACE),
    sort_by_mace_asc=make_sort_by_skill_asc(df.job_skill.MACE),
    sort_by_hammer_desc=make_sort_by_skill_desc(df.job_skill.HAMMER),
    sort_by_hammer_asc=make_sort_by_skill_asc(df.job_skill.HAMMER),
    sort_by_spear_desc=make_sort_by_skill_desc(df.job_skill.SPEAR),
    sort_by_spear_asc=make_sort_by_skill_asc(df.job_skill.SPEAR),
    sort_by_crossbow_desc=make_sort_by_skill_desc(df.job_skill.CROSSBOW),
    sort_by_crossbow_asc=make_sort_by_skill_asc(df.job_skill.CROSSBOW),
}

-- ----------------------
-- SquadAssignmentOverlay
--

SquadAssignmentOverlay = defclass(SquadAssignmentOverlay, overlay.OverlayWidget)
SquadAssignmentOverlay.ATTRS{
    default_pos={x=-33, y=40},
    default_enabled=true,
    viewscreens='dwarfmode/UnitSelector/SQUAD_FILL_POSITION',
    frame={w=63, h=7},
    frame_style=gui.FRAME_PANEL,
    frame_background=gui.CLEAR_PEN,
}

function SquadAssignmentOverlay:init()
    self.dirty = true

    self:addviews{
        widgets.CycleHotkeyLabel{
            view_id='sort',
            frame={l=0, t=0, w=29},
            label='Sort by:',
            key='CUSTOM_SHIFT_S',
            options={
                {label='any melee skill'..CH_DN, value=SORT_FNS.sort_by_any_melee_desc, pen=COLOR_GREEN},
                {label='any melee skill'..CH_UP, value=SORT_FNS.sort_by_any_melee_asc, pen=COLOR_YELLOW},
                {label='any ranged skill'..CH_DN, value=SORT_FNS.sort_by_any_ranged_desc, pen=COLOR_GREEN},
                {label='any ranged skill'..CH_UP, value=SORT_FNS.sort_by_any_ranged_asc, pen=COLOR_YELLOW},
                {label='any leader skill'..CH_DN, value=SORT_FNS.sort_by_leadership_desc, pen=COLOR_GREEN},
                {label='any leader skill'..CH_UP, value=SORT_FNS.sort_by_leadership_asc, pen=COLOR_YELLOW},
                {label='name'..CH_DN, value=sort_by_name_desc, pen=COLOR_GREEN},
                {label='name'..CH_UP, value=sort_by_name_asc, pen=COLOR_YELLOW},
                {label='migrant wave'..CH_DN, value=sort_by_migrant_wave_desc, pen=COLOR_GREEN},
                {label='migrant wave'..CH_UP, value=sort_by_migrant_wave_asc, pen=COLOR_YELLOW},
                {label='axe skill'..CH_DN, value=SORT_FNS.sort_by_axe_desc, pen=COLOR_GREEN},
                {label='axe skill'..CH_UP, value=SORT_FNS.sort_by_axe_asc, pen=COLOR_YELLOW},
                {label='sword skill'..CH_DN, value=SORT_FNS.sort_by_sword_desc, pen=COLOR_GREEN},
                {label='sword skill'..CH_UP, value=SORT_FNS.sort_by_sword_asc, pen=COLOR_YELLOW},
                {label='mace skill'..CH_DN, value=SORT_FNS.sort_by_mace_desc, pen=COLOR_GREEN},
                {label='mace skill'..CH_UP, value=SORT_FNS.sort_by_mace_asc, pen=COLOR_YELLOW},
                {label='hammer skill'..CH_DN, value=SORT_FNS.sort_by_hammer_desc, pen=COLOR_GREEN},
                {label='hammer skill'..CH_UP, value=SORT_FNS.sort_by_hammer_asc, pen=COLOR_YELLOW},
                {label='spear skill'..CH_DN, value=SORT_FNS.sort_by_spear_desc, pen=COLOR_GREEN},
                {label='spear skill'..CH_UP, value=SORT_FNS.sort_by_spear_asc, pen=COLOR_YELLOW},
                {label='crossbow skill'..CH_DN, value=SORT_FNS.sort_by_crossbow_desc, pen=COLOR_GREEN},
                {label='crossbow skill'..CH_UP, value=SORT_FNS.sort_by_crossbow_asc, pen=COLOR_YELLOW},
            },
            initial_option=SORT_FNS.sort_by_any_melee_desc,
            on_change=self:callback('refresh_list', 'sort'),
        },
        widgets.EditField{
            view_id='search',
            frame={l=32, t=0},
            label_text='Search: ',
            on_char=function(ch) return ch:match('[%l _-]') end,
            on_change=function() self:refresh_list() end,
        },
        widgets.Panel{
            frame={t=2, l=0, r=0, b=0},
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='sort_any_melee',
                    frame={t=0, l=0, w=10},
                    options={
                        {label='any melee', value=sort_noop},
                        {label='any melee'..CH_DN, value=SORT_FNS.sort_by_any_melee_desc, pen=COLOR_GREEN},
                        {label='any melee'..CH_UP, value=SORT_FNS.sort_by_any_melee_asc, pen=COLOR_YELLOW},
                    },
                    initial_option=SORT_FNS.sort_by_any_melee_desc,
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_any_melee'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_any_ranged',
                    frame={t=0, l=13, w=11},
                    options={
                        {label='any ranged', value=sort_noop},
                        {label='any ranged'..CH_DN, value=SORT_FNS.sort_by_any_ranged_desc, pen=COLOR_GREEN},
                        {label='any ranged'..CH_UP, value=SORT_FNS.sort_by_any_ranged_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_any_ranged'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_leadership',
                    frame={t=0, l=27, w=11},
                    options={
                        {label='leadership', value=sort_noop},
                        {label='leadership'..CH_DN, value=SORT_FNS.sort_by_leadership_desc, pen=COLOR_GREEN},
                        {label='leadership'..CH_UP, value=SORT_FNS.sort_by_leadership_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_leadership'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_name',
                    frame={t=0, l=41, w=5},
                    options={
                        {label='name', value=sort_noop},
                        {label='name'..CH_DN, value=sort_by_name_desc, pen=COLOR_GREEN},
                        {label='name'..CH_UP, value=sort_by_name_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_name'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_migrant_wave',
                    frame={t=0, l=48, w=13},
                    options={
                        {label='migrant wave', value=sort_noop},
                        {label='migrant wave'..CH_DN, value=sort_by_migrant_wave_desc, pen=COLOR_GREEN},
                        {label='migrant wave'..CH_UP, value=sort_by_migrant_wave_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_migrant_wave'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_axe',
                    frame={t=2, l=2, w=4},
                    options={
                        {label='axe', value=sort_noop},
                        {label='axe'..CH_DN, value=SORT_FNS.sort_by_axe_desc, pen=COLOR_GREEN},
                        {label='axe'..CH_UP, value=SORT_FNS.sort_by_axe_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_axe'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_sword',
                    frame={t=2, l=9, w=6},
                    options={
                        {label='sword', value=sort_noop},
                        {label='sword'..CH_DN, value=SORT_FNS.sort_by_sword_desc, pen=COLOR_GREEN},
                        {label='sword'..CH_UP, value=SORT_FNS.sort_by_sword_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_sword'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_mace',
                    frame={t=2, l=18, w=5},
                    options={
                        {label='mace', value=sort_noop},
                        {label='mace'..CH_DN, value=SORT_FNS.sort_by_mace_desc, pen=COLOR_GREEN},
                        {label='mace'..CH_UP, value=SORT_FNS.sort_by_mace_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_mace'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_hammer',
                    frame={t=2, l=25, w=7},
                    options={
                        {label='hammer', value=sort_noop},
                        {label='hammer'..CH_DN, value=SORT_FNS.sort_by_hammer_desc, pen=COLOR_GREEN},
                        {label='hammer'..CH_UP, value=SORT_FNS.sort_by_hammer_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_hammer'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_spear',
                    frame={t=2, l=36, w=6},
                    options={
                        {label='spear', value=sort_noop},
                        {label='spear'..CH_DN, value=SORT_FNS.sort_by_spear_desc, pen=COLOR_GREEN},
                        {label='spear'..CH_UP, value=SORT_FNS.sort_by_spear_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_spear'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_crossbow',
                    frame={t=2, l=45, w=9},
                    options={
                        {label='crossbow', value=sort_noop},
                        {label='crossbow'..CH_DN, value=SORT_FNS.sort_by_crossbow_desc, pen=COLOR_GREEN},
                        {label='crossbow'..CH_UP, value=SORT_FNS.sort_by_crossbow_asc, pen=COLOR_YELLOW},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_crossbow'),
                },
            }
        },
    }
end

local function normalize_search_key(search_key)
    local out = ''
    for c in dfhack.toSearchNormalized(search_key):gmatch("[%w%s]") do
        out = out .. c:lower()
    end
    return out
end

local function filter_matches(unit_id, search)
    if unit_id == -1 then return true end
    local unit = df.unit.find(unit_id)
    if not unit then return true end
    local search_key = dfhack.TranslateName(dfhack.units.getVisibleName(unit))
    if unit.status.current_soul then
        for _,skill in ipairs(unit.status.current_soul.skills) do
            search_key = (search_key or '') .. ' ' .. (df.job_skill[skill.id] or '')
        end
    end
    return normalize_search_key(search_key):find(dfhack.toSearchNormalized(search))
end

local unit_selector = df.global.game.main_interface.unit_selector

-- this function uses the unused itemid and selected vectors to keep state,
-- taking advantage of the fact that they are reset by DF when the list of units changes
local function filter_vector(search, prev_search)
    local unid_is_filtered = #unit_selector.selected >= 0 and unit_selector.selected[0] ~= 0
    if #search == 0 or #unit_selector.selected == 0 then
        if not unid_is_filtered then
            -- we haven't modified the unid vector; nothing to do here
            return
        end
        -- restore the unid vector
        unit_selector.unid:assign(unit_selector.itemid)
        -- clear our "we meddled" flag
        unit_selector.selected[0] = 0
        return
    end
    if unid_is_filtered and search == prev_search then
        -- prev filter still stands
        return
    end
    if unid_is_filtered then
        -- restore the unid vector
        unit_selector.unid:assign(unit_selector.itemid)
    else
        -- save the unid vector and set our meddle flag
        unit_selector.itemid:assign(unit_selector.unid)
        unit_selector.selected[0] = 1
    end
    -- do the actual filtering
    for idx=#unit_selector.unid-1,0,-1 do
        if not filter_matches(unit_selector.unid[idx], search) then
            unit_selector.unid:erase(idx)
        end
    end
end

local SORT_WIDGET_NAMES = {
    'sort',
    'sort_any_melee',
    'sort_any_ranged',
    'sort_leadership',
    'sort_name',
    'sort_migrant_wave',
    'sort_axe',
    'sort_sword',
    'sort_mace',
    'sort_hammer',
    'sort_spear',
    'sort_crossbow',
}

function SquadAssignmentOverlay:refresh_list(sort_widget, sort_fn)
    sort_widget = sort_widget or 'sort'
    sort_fn = sort_fn or self.subviews.sort:getOptionValue()
    if sort_fn == sort_noop then
        self.subviews[sort_widget]:cycle()
        return
    end
    for _,widget_name in ipairs(SORT_WIDGET_NAMES) do
        self.subviews[widget_name]:setOption(sort_fn)
    end
    local search = self.subviews.search.text
    filter_vector(search, self.prev_search)
    self.prev_search = search
    utils.sort_vector(unit_selector.unid, nil, sort_fn)
end

function SquadAssignmentOverlay:onInput(keys)
    if keys._MOUSE_R_DOWN or
        keys._MOUSE_L_DOWN and not self:getMouseFramePos()
    then
        -- if any click is made outside of our window, we may need to refresh our list
        self.dirty = true
    end
    return SquadAssignmentOverlay.super.onInput(self, keys)
end

function SquadAssignmentOverlay:onRenderFrame(dc, frame_rect)
    SquadAssignmentOverlay.super.onRenderFrame(self, dc, frame_rect)
    if self.dirty then
        self:refresh_list()
        self.dirty = false
    end
end

OVERLAY_WIDGETS = {
    squad_assignment=SquadAssignmentOverlay,
}

--[[
local utils = require('utils')
local units = require('plugins.sort.units')
local items = require('plugins.sort.items')

orders = orders or {}
orders.units = units.orders
orders.items = items.orders

function parse_ordering_spec(type,...)
    local group = orders[type]
    if group == nil then
        dfhack.printerr('Invalid ordering class: '..tostring(type))
        return nil
    end

    local specs = table.pack(...)
    local rv = { }

    for _,spec in ipairs(specs) do
        local nil_first = false
        if string.sub(spec,1,1) == '<' then
            nil_first = true
            spec = string.sub(spec,2)
        end

        local reverse = false
        if string.sub(spec,1,1) == '>' then
            reverse = true
            spec = string.sub(spec,2)
        end

        local cm = group[spec]

        if cm == nil then
            dfhack.printerr('Unknown order for '..type..': '..tostring(spec))
            return nil
        end

        if nil_first or reverse then
            cm = copyall(cm)
            cm.nil_first = nil_first
            cm.reverse = reverse
        end

        rv[#rv+1] = cm
    end

    return rv
end

make_sort_order = utils.make_sort_order
]]

return _ENV
