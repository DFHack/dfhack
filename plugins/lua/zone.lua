local _ENV = mkmodule('plugins.zone')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local CH_UP = string.char(30)
local CH_DN = string.char(31)
local CH_MALE = string.char(11)
local CH_FEMALE = string.char(12)

local STATUS = {
    NONE={label='Unknown', value=0},
    PASTURED_HERE={label='Pastured here', value=1},
    PASTURED_ELSEWHERE={label='Pastured elsewhere', value=2},
    RESTRAINT={label='On restraint', value=3},
    BUILT_CAGE={label='On display in cage', value=4},
    ITEM_CAGE={label='In movable cage', value=5},
    ROAMING={label='Roaming', value=6},
}
local STATUS_REVMAP = {}
for k, v in pairs(STATUS) do
    STATUS_REVMAP[v.value] = k
end

local DISPOSITION = {
    NONE={label='Unknown', value=0},
    PET={label='Pet', value=1},
    TAME={label='Domesticated', value=2},
    TRAINED={label='Partially trained', value=3},
    WILD_TRAINABLE={label='Wild (trainable)', value=4},
    WILD_UNTRAINABLE={label='Wild (untrainable)', value=5},
    HOSTILE={label='Hostile', value=6},
}
local DISPOSITION_REVMAP = {}
for k, v in pairs(DISPOSITION) do
    DISPOSITION_REVMAP[v.value] = k
end

-- -------------------
-- Pasture
--

local STATUS_COL_WIDTH = 18
local DISPOSITION_COL_WIDTH = 18
local GENDER_COL_WIDTH = 6
local SLIDER_LABEL_WIDTH = math.max(STATUS_COL_WIDTH, DISPOSITION_COL_WIDTH) + 4
local SLIDER_WIDTH = 48

Pasture = defclass(Pasture, widgets.Window)
Pasture.ATTRS {
    frame_title='Assign units to pasture',
    frame={w=6+SLIDER_WIDTH*2, h=47},
    resizable=true,
    resize_min={h=27},
}

local function sort_noop(a, b)
    -- this function is used as a marker and never actually gets called
    error('sort_noop should not be called')
end

local function sort_base(a, b)
    if a.data.race == b.data.race then
        return a.data.gender < b.data.gender
    end
    return a.data.race < b.data.race
end

local function sort_by_name_desc(a, b)
    if a.search_key == b.search_key then
        return sort_base(a, b)
    end
    return a.search_key < b.search_key
end

local function sort_by_name_asc(a, b)
    if a.search_key == b.search_key then
        return sort_base(a, b)
    end
    return a.search_key > b.search_key
end

local function sort_by_gender_desc(a, b)
    if a.data.gender == b.data.gender then
        return sort_base(a, b)
    end
    return a.data.gender < b.data.gender
end

local function sort_by_gender_asc(a, b)
    if a.data.gender == b.data.gender then
        return sort_base(a, b)
    end
    return a.data.gender > b.data.gender
end

local function sort_by_disposition_desc(a, b)
    if a.data.disposition == b.data.disposition then
        return sort_base(a, b)
    end
    return a.data.disposition < b.data.disposition
end

local function sort_by_disposition_asc(a, b)
    if a.data.disposition == b.data.disposition then
        return sort_base(a, b)
    end
    return a.data.disposition > b.data.disposition
end

local function sort_by_status_desc(a, b)
    if a.data.status == b.data.status then
        return sort_base(a, b)
    end
    return a.data.status < b.data.status
end

local function sort_by_status_asc(a, b)
    if a.data.status == b.data.status then
        return sort_base(a, b)
    end
    return a.data.status > b.data.status
end

function Pasture:init()
    self:addviews{
        widgets.CycleHotkeyLabel{
            view_id='sort',
            frame={l=0, t=0, w=26},
            label='Sort by:',
            key='CUSTOM_SHIFT_S',
            options={
                {label='status'..CH_DN, value=sort_by_status_desc},
                {label='status'..CH_UP, value=sort_by_status_asc},
                {label='disposition'..CH_DN, value=sort_by_disposition_desc},
                {label='disposition'..CH_UP, value=sort_by_disposition_asc},
                {label='gender'..CH_DN, value=sort_by_gender_desc},
                {label='gender'..CH_UP, value=sort_by_gender_asc},
                {label='name'..CH_DN, value=sort_by_name_desc},
                {label='name'..CH_UP, value=sort_by_name_asc},
            },
            initial_option=sort_by_status_desc,
            on_change=self:callback('refresh_list', 'sort'),
        },
        widgets.EditField{
            view_id='search',
            frame={l=35, t=0},
            label_text='Search: ',
            on_char=function(ch) return ch:match('[%l -]') end,
        },
        widgets.Panel{
            frame={t=2, l=0, w=SLIDER_WIDTH, h=4},
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='min_status',
                    frame={l=0, t=0, w=SLIDER_LABEL_WIDTH},
                    label='Min status:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_Z',
                    key='CUSTOM_SHIFT_X',
                    options={
                        {label=STATUS.PASTURED_HERE.label, value=STATUS.PASTURED_HERE.value},
                        {label=STATUS.PASTURED_ELSEWHERE.label, value=STATUS.PASTURED_ELSEWHERE.value},
                        {label=STATUS.RESTRAINT.label, value=STATUS.RESTRAINT.value},
                        {label=STATUS.BUILT_CAGE.label, value=STATUS.BUILT_CAGE.value},
                        {label=STATUS.ITEM_CAGE.label, value=STATUS.ITEM_CAGE.value},
                        {label=STATUS.ROAMING.label, value=STATUS.ROAMING.value},
                    },
                    initial_option=STATUS.PASTURED_HERE.value,
                    on_change=function(val)
                        if self.subviews.max_status:getOptionValue() < val then
                            self.subviews.max_status:setOption(val)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='max_status',
                    frame={r=1, t=0, w=SLIDER_LABEL_WIDTH},
                    label='Max status:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_Q',
                    key='CUSTOM_SHIFT_W',
                    options={
                        {label=STATUS.PASTURED_HERE.label, value=STATUS.PASTURED_HERE.value},
                        {label=STATUS.PASTURED_ELSEWHERE.label, value=STATUS.PASTURED_ELSEWHERE.value},
                        {label=STATUS.RESTRAINT.label, value=STATUS.RESTRAINT.value},
                        {label=STATUS.BUILT_CAGE.label, value=STATUS.BUILT_CAGE.value},
                        {label=STATUS.ITEM_CAGE.label, value=STATUS.ITEM_CAGE.value},
                        {label=STATUS.ROAMING.label, value=STATUS.ROAMING.value},
                    },
                    initial_option=STATUS.ROAMING.value,
                    on_change=function(val)
                        if self.subviews.min_status:getOptionValue() > val then
                            self.subviews.min_status:setOption(val)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.RangeSlider{
                    frame={l=0, t=3},
                    num_stops=6,
                    get_left_idx_fn=function()
                        return self.subviews.min_status:getOptionValue()
                    end,
                    get_right_idx_fn=function()
                        return self.subviews.max_status:getOptionValue()
                    end,
                    on_left_change=function(idx) self.subviews.min_status:setOption(idx, true) end,
                    on_right_change=function(idx) self.subviews.max_status:setOption(idx, true) end,
                },
            },
        },
        widgets.Panel{
            frame={t=7, l=0, w=SLIDER_WIDTH, h=4},
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='min_disposition',
                    frame={l=0, t=0, w=SLIDER_LABEL_WIDTH},
                    label='Min disposition:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_C',
                    key='CUSTOM_SHIFT_V',
                    options={
                        {label=DISPOSITION.PET.label, value=DISPOSITION.PET.value},
                        {label=DISPOSITION.TAME.label, value=DISPOSITION.TAME.value},
                        {label=DISPOSITION.TRAINED.label, value=DISPOSITION.TRAINED.value},
                        {label=DISPOSITION.WILD_TRAINABLE.label, value=DISPOSITION.WILD_TRAINABLE.value},
                        {label=DISPOSITION.WILD_UNTRAINABLE.label, value=DISPOSITION.WILD_UNTRAINABLE.value},
                        {label=DISPOSITION.HOSTILE.label, value=DISPOSITION.HOSTILE.value},
                    },
                    initial_option=DISPOSITION.PET.value,
                    on_change=function(val)
                        if self.subviews.max_disposition:getOptionValue() < val then
                            self.subviews.max_disposition:setOption(val)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='max_disposition',
                    frame={r=1, t=0, w=SLIDER_LABEL_WIDTH},
                    label='Max disposition:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_E',
                    key='CUSTOM_SHIFT_R',
                    options={
                        {label=DISPOSITION.PET.label, value=DISPOSITION.PET.value},
                        {label=DISPOSITION.TAME.label, value=DISPOSITION.TAME.value},
                        {label=DISPOSITION.TRAINED.label, value=DISPOSITION.TRAINED.value},
                        {label=DISPOSITION.WILD_TRAINABLE.label, value=DISPOSITION.WILD_TRAINABLE.value},
                        {label=DISPOSITION.WILD_UNTRAINABLE.label, value=DISPOSITION.WILD_UNTRAINABLE.value},
                        {label=DISPOSITION.HOSTILE.label, value=DISPOSITION.HOSTILE.value},
                    },
                    initial_option=DISPOSITION.HOSTILE.value,
                    on_change=function(val)
                        if self.subviews.min_disposition:getOptionValue() > val then
                            self.subviews.min_disposition:setOption(val)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.RangeSlider{
                    frame={l=0, t=3},
                    num_stops=6,
                    get_left_idx_fn=function()
                        return self.subviews.min_disposition:getOptionValue()
                    end,
                    get_right_idx_fn=function()
                        return self.subviews.max_disposition:getOptionValue()
                    end,
                    on_left_change=function(idx) self.subviews.min_disposition:setOption(idx, true) end,
                    on_right_change=function(idx) self.subviews.max_disposition:setOption(idx, true) end,
                },
            },
        },
        widgets.Panel{
            frame={t=3, l=SLIDER_WIDTH+2, r=0, h=3},
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='egg',
                    frame={l=0, t=0, w=23},
                    key_back='CUSTOM_SHIFT_B',
                    key='CUSTOM_SHIFT_N',
                    label='Egg layers:',
                    options={
                        {label='Include', value='include', pen=COLOR_GREEN},
                        {label='Only', value='only', pen=COLOR_YELLOW},
                        {label='Exclude', value='exclude', pen=COLOR_RED},
                    },
                    initial_option='include',
                    on_change=self:callback('refresh_list'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='graze',
                    frame={l=0, t=2, w=20},
                    key_back='CUSTOM_SHIFT_T',
                    key='CUSTOM_SHIFT_Y',
                    label='Grazers:',
                    options={
                        {label='Include', value='include', pen=COLOR_GREEN},
                        {label='Only', value='only', pen=COLOR_YELLOW},
                        {label='Exclude', value='exclude', pen=COLOR_RED},
                    },
                    initial_option='include',
                    on_change=self:callback('refresh_list'),
                },
            },
        },
        widgets.Panel{
            view_id='list_panel',
            frame={t=12, l=0, r=0, b=4},
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='sort_status',
                    frame={t=0, l=0, w=7},
                    options={
                        {label='status', value=sort_noop},
                        {label='status'..CH_DN, value=sort_by_status_desc},
                        {label='status'..CH_UP, value=sort_by_status_asc},
                    },
                    initial_option=sort_by_status_desc,
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_status'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_disposition',
                    frame={t=0, l=STATUS_COL_WIDTH+2, w=12},
                    options={
                        {label='disposition', value=sort_noop},
                        {label='disposition'..CH_DN, value=sort_by_disposition_desc},
                        {label='disposition'..CH_UP, value=sort_by_disposition_asc},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_disposition'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_gender',
                    frame={t=0, l=STATUS_COL_WIDTH+2+DISPOSITION_COL_WIDTH+2, w=7},
                    options={
                        {label='gender', value=sort_noop},
                        {label='gender'..CH_DN, value=sort_by_gender_desc},
                        {label='gender'..CH_UP, value=sort_by_gender_asc},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_gender'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_name',
                    frame={t=0, l=STATUS_COL_WIDTH+2+DISPOSITION_COL_WIDTH+2+GENDER_COL_WIDTH+2, w=5},
                    options={
                        {label='name', value=sort_noop},
                        {label='name'..CH_DN, value=sort_by_name_desc},
                        {label='name'..CH_UP, value=sort_by_name_asc},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_name'),
                },
                widgets.FilteredList{
                    view_id='list',
                    frame={l=0, t=2, r=0, b=0},
                    on_submit=self:callback('toggle_item'),
                    on_submit2=self:callback('toggle_range'),
                    on_select=self:callback('select_item'),
                },
            }
        },
        widgets.HotkeyLabel{
            frame={l=0, b=2},
            label='Assign all/none',
            key='CUSTOM_CTRL_A',
            on_activate=self:callback('toggle_visible'),
            auto_width=true,
        },
        widgets.WrappedLabel{
            frame={b=0, l=0, r=0},
            text_to_wrap='Click to assign/unassign to current pasture. Shift click to assign/unassign a range.',
        },
    }

    -- replace the FilteredList's built-in EditField with our own
    self.subviews.list.list.frame.t = 0
    self.subviews.list.edit.visible = false
    self.subviews.list.edit = self.subviews.search
    self.subviews.search.on_change = self.subviews.list:callback('onFilterChange')

    self.subviews.list:setChoices(self:get_choices())
end

function Pasture:refresh_list(sort_widget, sort_fn)
    sort_widget = sort_widget or 'sort'
    sort_fn = sort_fn or self.subviews.sort:getOptionValue()
    if sort_fn == sort_noop then
        self.subviews[sort_widget]:cycle()
        return
    end
    for _,widget_name in ipairs{'sort', 'sort_status', 'sort_disposition', 'sort_gender', 'sort_name'} do
        self.subviews[widget_name]:setOption(sort_fn)
    end
    local list = self.subviews.list
    local saved_filter = list:getFilter()
    list:setFilter('')
    list:setChoices(self:get_choices(), list:getSelected())
    list:setFilter(saved_filter)
end

local function make_search_key(data)
    local out = ''
    for c in data.desc:gmatch("[%w%s]") do
        out = out .. c:lower()
    end
    return out
end

local function make_choice_text(data)
    return {
        {width=STATUS_COL_WIDTH, text=function() return STATUS[STATUS_REVMAP[data.status]].label end},
        {gap=2, width=DISPOSITION_COL_WIDTH, text=function() return DISPOSITION[DISPOSITION_REVMAP[data.disposition]].label end},
        {gap=2, width=GENDER_COL_WIDTH, text=data.gender == 0 and CH_FEMALE or CH_MALE},
        {gap=2, text=data.desc},
    }
end

local function get_unit_description(unit, raw)
    local race = dfhack.units.isChild(unit) and raw.general_child_name[0] or raw.caste[unit.caste].caste_name[0]
    local name = dfhack.TranslateName(dfhack.units.getVisibleName(unit))
    if name and #name > 0 then
        name = ('%s, %s'):format(name, race)
    else
        name = race
    end
    if #unit.syndromes.active > 0 then
        for _, unit_syndrome in ipairs(unit.syndromes.active) do
            local syndrome = df.syndrome.find(unit_syndrome.type)
            if not syndrome then goto continue end
            for _, effect in ipairs(syndrome.ce) do
                if df.creature_interaction_effect_display_namest:is_instance(effect) then
                    return name .. ' ' .. effect.name
                end
            end
            ::continue::
        end
    end
    return name
end

local function get_cage_ref(unit)
    return dfhack.units.getGeneralRef(unit, df.general_ref_type.CONTAINED_IN_ITEM)
end

local function get_built_cage(item_cage)
    if not item_cage then return end
    local built_cage_ref = dfhack.items.getGeneralRef(item_cage, df.general_ref_type.BUILDING_HOLDER)
    if not built_cage_ref then return end
    local built_cage = df.building.find(built_cage_ref.building_id)
    if not built_cage then return end
    if built_cage:getType() == df.building_type.Cage then
        return built_cage
    end
end

local function get_status(unit)
    local assigned_pasture_ref = dfhack.units.getGeneralRef(unit, df.general_ref_type.BUILDING_CIVZONE_ASSIGNED)
    if assigned_pasture_ref then
        if df.global.game.main_interface.civzone.cur_bld.id == assigned_pasture_ref.building_id then
            return STATUS.PASTURED_HERE.value
        else
            return STATUS.PASTURED_ELSEWHERE.value
        end
    end
    if dfhack.units.getGeneralRef(unit, df.general_ref_type.BUILDING_CHAIN) then
        return STATUS.RESTRAINT.value
    end
    local cage_ref = get_cage_ref(unit)
    if cage_ref then
        if get_built_cage(df.item.find(cage_ref.item_id)) then
            return STATUS.BUILT_CAGE.value
        else
            return STATUS.ITEM_CAGE.value
        end
    end
    return STATUS.ROAMING.value
end

local function get_disposition(unit)
    local disposition = DISPOSITION.NONE
    if dfhack.units.isPet(unit) then
        disposition = DISPOSITION.PET
    elseif dfhack.units.isDomesticated(unit) then
        disposition = DISPOSITION.TAME
    elseif dfhack.units.isTame(unit) then
        disposition = DISPOSITION.TRAINED
    elseif dfhack.units.isTamable(unit) then
        disposition = DISPOSITION.WILD_TRAINABLE
    elseif dfhack.units.isInvader(unit) or dfhack.units.isOpposedToLife(unit) then
        disposition = DISPOSITION.HOSTILE
    else
        disposition = DISPOSITION.WILD_UNTRAINABLE
    end
    return disposition.value
end

local function is_pasturable_unit(unit)
    return dfhack.units.isActive(unit) and
        ((dfhack.units.isAnimal(unit) and dfhack.units.isOwnCiv(unit)) or get_cage_ref(unit)) and
        not dfhack.units.isDead(unit) and
        not dfhack.units.isMerchant(unit) and
        not dfhack.units.isForest(unit)
end

function Pasture:cache_choices()
    if self.choices then return self.choices end

    local choices = {}
    for _, unit in ipairs(df.global.world.units.active) do
        if not is_pasturable_unit(unit) then goto continue end
        local raw = df.creature_raw.find(unit.race)
        local data = {
            unit=unit,
            desc=get_unit_description(unit, raw),
            gender=unit.sex,
            race=raw.creature_id,
            status=get_status(unit),
            disposition=get_disposition(unit),
            egg=dfhack.units.isEggLayerRace(unit),
            graze=dfhack.units.isGrazer(unit),
        }
        local choice = {
            search_key=make_search_key(data),
            data=data,
            text=make_choice_text(data),
        }
        table.insert(choices, choice)
        ::continue::
    end

    self.choices = choices
    return choices
end

function Pasture:get_choices()
    local raw_choices = self:cache_choices()
    local min_status = self.subviews.min_status:getOptionValue()
    local max_status = self.subviews.max_status:getOptionValue()
    local min_disposition = self.subviews.min_disposition:getOptionValue()
    local max_disposition = self.subviews.max_disposition:getOptionValue()
    local egg = self.subviews.egg:getOptionValue()
    local graze = self.subviews.graze:getOptionValue()
    local choices = {}
    for _,choice in ipairs(raw_choices) do
        local data = choice.data
        if min_status > data.status then goto continue end
        if max_status < data.status then goto continue end
        if min_disposition > data.disposition then goto continue end
        if max_disposition < data.disposition then goto continue end
        if egg == 'only' and not data.egg then goto continue end
        if egg == 'exclude' and data.egg then goto continue end
        if graze == 'only' and not data.graze then goto continue end
        if graze == 'exclude' and data.graze then goto continue end
        table.insert(choices, choice)
        ::continue::
    end
    table.sort(choices, self.subviews.sort:getOptionValue())
    return choices
end

local function unassign_unit(bld, unit)
    if not bld then return end
    for au_idx, au_id in ipairs(bld.assigned_units) do
        if au_id == unit.id then
            bld.assigned_units:erase(au_idx)
            return
        end
    end
end

local function detach_unit(unit)
    for idx = #unit.general_refs-1, 0, -1 do
        local ref = unit.general_refs[idx]
        if df.general_ref_building_civzone_assignedst:is_instance(ref) then
            unassign_unit(df.building.find(ref.building_id), unit)
            unit.general_refs:erase(idx)
            ref:delete()
        elseif df.general_ref_contained_in_itemst:is_instance(ref) then
            local built_cage = get_built_cage(df.item.find(ref.item_id))
            if built_cage and built_cage:getType() == df.building_type.Cage then
                unassign_unit(built_cage, unit)
                -- unit's general ref will be removed when the unit is released from the cage
            end
        elseif df.general_ref_building_chainst:is_instance(ref) then
            local chain = df.building.find(ref.building_id)
            if chain then
                chain.assigned = nil
            end
        end
    end
end

local function attach_unit(unit)
    local pasture = df.global.game.main_interface.civzone.cur_bld
    local ref = df.new(df.general_ref_building_civzone_assignedst)
    ref.building_id = pasture.id;
    unit.general_refs:insert('#', ref)
    pasture.assigned_units:insert('#', unit.id)
end

local function toggle_item_base(choice, target_value)
    if target_value == nil then
        target_value = choice.data.status ~= STATUS.PASTURED_HERE.value
    end

    if target_value and choice.data.status == STATUS.PASTURED_HERE.value then
        return
    end
    if not target_value and choice.data.status ~= STATUS.PASTURED_HERE.value then
        return
    end

    local unit = choice.data.unit
    detach_unit(unit)

    if target_value then
        attach_unit(unit)
    end

    choice.data.status = get_status(unit)
end

function Pasture:select_item(idx, choice)
    if not dfhack.internal.getModifiers().shift then
        self.prev_list_idx = self.subviews.list.list:getSelected()
    end
end

function Pasture:toggle_item(idx, choice)
    toggle_item_base(choice)
end

function Pasture:toggle_range(idx, choice)
    if not self.prev_list_idx then
        self:toggle_item(idx, choice)
        return
    end
    local choices = self.subviews.list:getVisibleChoices()
    local list_idx = self.subviews.list.list:getSelected()
    local target_value
    for i = list_idx, self.prev_list_idx, list_idx < self.prev_list_idx and 1 or -1 do
        target_value = toggle_item_base(choices[i], target_value)
    end
    self.prev_list_idx = list_idx
end

function Pasture:toggle_visible()
    local target_value
    for _, choice in ipairs(self.subviews.list:getVisibleChoices()) do
        target_value = toggle_item_base(choice, target_value)
    end
end

-- -------------------
-- PastureScreen
--

view = view or nil

PastureScreen = defclass(PastureScreen, gui.ZScreen)
PastureScreen.ATTRS {
    focus_path='zone/pasture',
}

function PastureScreen:init()
    self:addviews{Pasture{}}
end

function PastureScreen:onInput(keys)
    local handled = PastureScreen.super.onInput(self, keys)
    if keys._MOUSE_L_DOWN then
        -- if any click is made outside of our window, we need to recheck unit properites
        local window = self.subviews[1]
        if not window:getMouseFramePos() then
            for _, choice in ipairs(self.subviews.list:getChoices()) do
                choice.data.status = get_status(choice.data.unit)
            end
            window:refresh_list()
        end
    end
    return handled
end

function PastureScreen:onRenderFrame()
    if df.global.game.main_interface.bottom_mode_selected ~= df.main_bottom_mode_type.ZONE or
        not df.global.game.main_interface.civzone.cur_bld or
        df.global.game.main_interface.civzone.cur_bld.type ~= df.civzone_type.Pen
    then
        view:dismiss()
    end
end

function PastureScreen:onDismiss()
    view = nil
end

-- -------------------
-- PastureOverlay
--

PastureOverlay = defclass(PastureOverlay, overlay.OverlayWidget)
PastureOverlay.ATTRS{
    default_pos={x=7,y=13},
    default_enabled=true,
    viewscreens='dwarfmode/Zone/Some/Pen',
    frame={w=32, h=1},
    frame_background=gui.CLEAR_PEN,
}

function PastureOverlay:init()
    self:addviews{
        widgets.TextButton{
            frame={t=0, l=0},
            label='DFHack search and sort',
            key='CUSTOM_CTRL_T',
            on_activate=function() view = view and view:raise() or PastureScreen{}:show() end,
        },
    }
end

OVERLAY_WIDGETS = {
    pasture=PastureOverlay,
}

return _ENV
