local _ENV = mkmodule('plugins.zone')

local dialogs = require('gui.dialogs')
local gui = require('gui')
local overlay = require('plugins.overlay')
local utils = require('utils')
local widgets = require('gui.widgets')

local CH_UP = string.char(30)
local CH_DN = string.char(31)
local CH_MALE = string.char(11)
local CH_FEMALE = string.char(12)
local CH_NEUTER = '?'

local DISPOSITION = {
    NONE={label='Unknown', value=0},
    PET={label='Pet', value=1},
    TAME={label='Domesticated', value=2},
    TRAINED={label='Partially trained', value=3},
    WILD_TRAINABLE={label='Wild (trainable)', value=4},
    WILD_UNTRAINABLE={label='Untrainable', value=5},
    HOSTILE={label='Hostile', value=6},
}
local DISPOSITION_REVMAP = {}
for k, v in pairs(DISPOSITION) do
    DISPOSITION_REVMAP[v.value] = k
end

-- -------------------
-- AssignAnimal
--

local STATUS_COL_WIDTH = 18
local DIST_COL_WIDTH = 5
local DISPOSITION_COL_WIDTH = 18
local GENDER_COL_WIDTH = 6
local SLIDER_LABEL_WIDTH = math.max(STATUS_COL_WIDTH, DISPOSITION_COL_WIDTH) + 4
local SLIDER_WIDTH = 48

AssignAnimal = defclass(AssignAnimal, widgets.Window)
AssignAnimal.ATTRS {
    frame_title='Animal and prisoner assignment',
    frame={w=6+SLIDER_WIDTH*2, h=47},
    resizable=true,
    resize_min={h=27},
    target_name=DEFAULT_NIL,
    status=DEFAULT_NIL,
    status_revmap=DEFAULT_NIL,
    get_status=DEFAULT_NIL,
    get_distance=DEFAULT_NIL,
    get_allow_vermin=DEFAULT_NIL,
    get_multi_select=DEFAULT_NIL,
    attach=DEFAULT_NIL,
    initial_min_disposition=DISPOSITION.PET.value,
}

local function sort_noop(a, b)
    -- this function is used as a marker and never actually gets called
    error('sort_noop should not be called')
end

local function sort_by_race_desc(a, b)
    if a.data.race == b.data.race then
        return a.data.gender < b.data.gender
    end
    return a.data.race < b.data.race
end

local function sort_by_race_asc(a, b)
    if a.data.race == b.data.race then
        return a.data.gender < b.data.gender
    end
    return a.data.race > b.data.race
end

local function sort_by_dist_desc(a, b)
    local adist, bdist = a.data.dist_fn(), b.data.dist_fn()
    if adist == bdist then
        return sort_by_race_desc(a, b)
    end
    return adist < bdist
end

local function sort_by_dist_asc(a, b)
    local adist, bdist = a.data.dist_fn(), b.data.dist_fn()
    if adist == bdist then
        return sort_by_race_desc(a, b)
    end
    return adist > bdist
end

local function sort_by_name_desc(a, b)
    if a.search_key == b.search_key then
        return sort_by_race_desc(a, b)
    end
    return a.search_key < b.search_key
end

local function sort_by_name_asc(a, b)
    if a.search_key == b.search_key then
        return sort_by_race_desc(a, b)
    end
    return a.search_key > b.search_key
end

local function sort_by_gender_desc(a, b)
    if a.data.gender == b.data.gender then
        local ag = a.data.gelded or false
        local bg = b.data.gelded or false
        if ag == bg then
            return sort_by_race_desc(a, b)
        end
        return bg
    end
    return a.data.gender < b.data.gender
end

local function sort_by_gender_asc(a, b)
    if a.data.gender == b.data.gender then
        local ag = a.data.gelded or false
        local bg = b.data.gelded or false
        if ag == bg then
            return sort_by_race_desc(a, b)
        end
        return ag
    end
    return a.data.gender > b.data.gender
end

local function sort_by_disposition_desc(a, b)
    if a.data.disposition == b.data.disposition then
        return sort_by_race_desc(a, b)
    end
    return a.data.disposition < b.data.disposition
end

local function sort_by_disposition_asc(a, b)
    if a.data.disposition == b.data.disposition then
        return sort_by_race_desc(a, b)
    end
    return a.data.disposition > b.data.disposition
end

local function sort_by_status_desc(a, b)
    if a.data.status == b.data.status then
        return sort_by_race_desc(a, b)
    end
    return a.data.status < b.data.status
end

local function sort_by_status_asc(a, b)
    if a.data.status == b.data.status then
        return sort_by_race_desc(a, b)
    end
    return a.data.status > b.data.status
end

function AssignAnimal:init()
    local status_options = {}
    for k, v in ipairs(self.status_revmap) do
        status_options[k] = self.status[v]
    end

    local disposition_options = {}
    for k, v in ipairs(DISPOSITION_REVMAP) do
        disposition_options[k] = DISPOSITION[v]
    end

    local can_assign_pets = self.initial_min_disposition == DISPOSITION.PET.value

    self:addviews{
        widgets.CycleHotkeyLabel{
            view_id='sort',
            frame={l=0, t=0, w=26},
            label='Sort by:',
            key='CUSTOM_SHIFT_S',
            options={
                {label='status'..CH_DN, value=sort_by_status_desc},
                {label='status'..CH_UP, value=sort_by_status_asc},
                {label='dist'..CH_DN, value=sort_by_dist_desc},
                {label='dist'..CH_UP, value=sort_by_dist_asc},
                {label='disposition'..CH_DN, value=sort_by_disposition_desc},
                {label='disposition'..CH_UP, value=sort_by_disposition_asc},
                {label='gender'..CH_DN, value=sort_by_gender_desc},
                {label='gender'..CH_UP, value=sort_by_gender_asc},
                {label='name'..CH_DN, value=sort_by_name_desc},
                {label='name'..CH_UP, value=sort_by_name_asc},
                {label='race'..CH_DN, value=sort_by_race_desc},
                {label='race'..CH_UP, value=sort_by_race_asc},
            },
            initial_option=sort_by_status_desc,
            on_change=self:callback('refresh_list', 'sort'),
        },
        widgets.EditField{
            view_id='search',
            frame={l=35, t=0},
            label_text='Search: ',
            on_char=function(ch, text) return ch == ' ' and text:match('%S$') or ch:match('[%l-]') end,
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
                    options=status_options,
                    initial_option=1,
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
                    options=status_options,
                    initial_option=#self.status_revmap,
                    on_change=function(val)
                        if self.subviews.min_status:getOptionValue() > val then
                            self.subviews.min_status:setOption(val)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.RangeSlider{
                    frame={l=0, t=3},
                    num_stops=#self.status_revmap,
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
                    options=disposition_options,
                    initial_option=self.initial_min_disposition,
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
                    options=disposition_options,
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
            frame={t=3, l=SLIDER_WIDTH+2, r=0, h=5},
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
                    on_change=function() self:refresh_list() end,
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
                    on_change=function() self:refresh_list() end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='juvenile',
                    frame={l=0, t=4, w=22},
                    key_back='CUSTOM_SHIFT_U',
                    key='CUSTOM_SHIFT_I',
                    label='Juveniles:',
                    options={
                        {label='Include', value='include', pen=COLOR_GREEN},
                        {label='Only', value='only', pen=COLOR_YELLOW},
                        {label='Exclude', value='exclude', pen=COLOR_RED},
                    },
                    initial_option='include',
                    on_change=function() self:refresh_list() end,
                },
            },
        },
        widgets.Panel{
            view_id='list_panel',
            frame={t=12, l=0, r=0, b=4+(can_assign_pets and 0 or 1)},
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
                    view_id='sort_dist',
                    frame={t=0, l=STATUS_COL_WIDTH+2, w=5},
                    options={
                        {label='dist', value=sort_noop},
                        {label='dist'..CH_DN, value=sort_by_dist_desc},
                        {label='dist'..CH_UP, value=sort_by_dist_asc},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_dist'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_disposition',
                    frame={t=0, l=STATUS_COL_WIDTH+2+DIST_COL_WIDTH+2, w=12},
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
                    frame={t=0, l=STATUS_COL_WIDTH+2+DIST_COL_WIDTH+2+DISPOSITION_COL_WIDTH+2, w=7},
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
                    frame={t=0, l=STATUS_COL_WIDTH+2+DIST_COL_WIDTH+2+DISPOSITION_COL_WIDTH+2+GENDER_COL_WIDTH+2, w=5},
                    options={
                        {label='name', value=sort_noop},
                        {label='name'..CH_DN, value=sort_by_name_desc},
                        {label='name'..CH_UP, value=sort_by_name_asc},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_name'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_race',
                    frame={t=0, l=STATUS_COL_WIDTH+2+DIST_COL_WIDTH+2+DISPOSITION_COL_WIDTH+2+GENDER_COL_WIDTH+2+7, w=5},
                    options={
                        {label='race', value=sort_noop},
                        {label='race'..CH_DN, value=sort_by_race_desc},
                        {label='race'..CH_UP, value=sort_by_race_asc},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_race'),
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
            frame={l=0, b=2+(can_assign_pets and 0 or 1)},
            label='Assign all/none',
            key='CUSTOM_CTRL_A',
            on_activate=self:callback('toggle_visible'),
            visible=self.get_multi_select,
            auto_width=true,
        },
        widgets.Label{
            frame={l=30, b=2+(can_assign_pets and 0 or 1)},
            text={
                {text=self:callback('get_num_assigned_here'), pen=COLOR_YELLOW},
                ' creature(s) assigned here.'
            },
        },
        widgets.WrappedLabel{
            frame={b=0, l=0, r=0},
            text_to_wrap=function()
                return 'Click to assign/unassign to ' .. self.target_name .. '.' ..
                    (self.get_multi_select() and ' Shift click to assign/unassign a range.' or '') ..
                    (not can_assign_pets and '\nNote that pets cannot be assigned to cages or restraints.' or '')
            end,
        },
    }

    -- replace the FilteredList's built-in EditField with our own
    self.subviews.list.list.frame.t = 0
    self.subviews.list.edit.visible = false
    self.subviews.list.edit = self.subviews.search
    self.subviews.search.on_change = self.subviews.list:callback('onFilterChange')

    self.subviews.list:setChoices(self:get_choices())
end

function AssignAnimal:get_num_assigned_here()
    local count = 0
    for _,choice in ipairs(self.subviews.list:getChoices()) do
        if choice.data.status == 1 then
            count = count + 1
        end
    end
    return count
end

function AssignAnimal:refresh_list(sort_widget, sort_fn)
    sort_widget = sort_widget or 'sort'
    sort_fn = sort_fn or self.subviews.sort:getOptionValue()
    if sort_fn == sort_noop then
        self.subviews[sort_widget]:cycle()
        return
    end
    for _,widget_name in ipairs{'sort', 'sort_status', 'sort_dist', 'sort_disposition', 'sort_gender', 'sort_race', 'sort_name'} do
        self.subviews[widget_name]:setOption(sort_fn)
    end
    local list = self.subviews.list
    local saved_filter = list:getFilter()
    list:setFilter('')
    list:setChoices(self:get_choices(), list:getSelected())
    list:setFilter(saved_filter)
end

function add_words(words, str)
    for word in dfhack.toSearchNormalized(str):gmatch("[%w-]+") do
        table.insert(words, word:lower())
    end
end

function make_search_key(desc, race_raw)
    local words = {}
    add_words(words, desc)
    if race_raw then
        add_words(words, race_raw.name[0])
    end
    return table.concat(words, ' ')
end

function AssignAnimal:make_choice_text(data)
    local gender_ch = CH_NEUTER
    if data.gender == df.pronoun_type.she then
        gender_ch = CH_FEMALE
    elseif data.gender == df.pronoun_type.he then
        gender_ch = CH_MALE
    end
    gender_ch = string.format(data.gelded and 'x%sx' or ' %s ', gender_ch)
    return {
        {width=STATUS_COL_WIDTH, text=function() return self.status[self.status_revmap[data.status]].label end},
        {gap=2, width=DIST_COL_WIDTH, text=data.dist_fn},
        {gap=2, width=DISPOSITION_COL_WIDTH, text=function() return DISPOSITION[DISPOSITION_REVMAP[data.disposition]].label end},
        {gap=2, width=GENDER_COL_WIDTH, text=gender_ch},
        {gap=2, text=data.desc},
    }
end

local function get_general_ref(unit_or_vermin, ref_type)
    local is_unit = df.unit:is_instance(unit_or_vermin)
    return dfhack[is_unit and 'units' or 'items'].getGeneralRef(unit_or_vermin, ref_type)
end

local function get_cage_ref(unit_or_vermin)
    return get_general_ref(unit_or_vermin, df.general_ref_type.CONTAINED_IN_ITEM)
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

local function get_bld_assignments()
    local assignments = {}
    for _,cage in ipairs(df.global.world.buildings.other.CAGE) do
        for _,unit_id in ipairs(cage.assigned_units) do
            assignments[unit_id] = cage
        end
    end
    for _,chain in ipairs(df.global.world.buildings.other.CHAIN) do
        if chain.assigned then
            assignments[chain.assigned.id] = chain
        end
    end
    return assignments
end

local function get_unit_disposition(unit)
    local disposition = DISPOSITION.NONE
    if dfhack.units.isInvader(unit) or dfhack.units.isOpposedToLife(unit) then
        disposition = DISPOSITION.HOSTILE
    elseif dfhack.units.isPet(unit) then
        disposition = DISPOSITION.PET
    elseif dfhack.units.isDomesticated(unit) then
        disposition = DISPOSITION.TAME
    elseif dfhack.units.isTame(unit) then
        disposition = DISPOSITION.TRAINED
    elseif dfhack.units.isTamable(unit) then
        disposition = DISPOSITION.WILD_TRAINABLE
    else
        disposition = DISPOSITION.WILD_UNTRAINABLE
    end
    return disposition.value
end

local function get_item_disposition(item)
    if dfhack.units.casteFlagSet(item.race, item.caste, df.caste_raw_flags.OPPOSED_TO_LIFE) then
        return DISPOSITION.HOSTILE.value
    end

    if df.item_petst:is_instance(item) then
        if item.owner_id > -1 then
            return DISPOSITION.PET.value
        end
        return DISPOSITION.TAME.value
    end

    if dfhack.units.casteFlagSet(item.race, item.caste, df.caste_raw_flags.PET) or
        dfhack.units.casteFlagSet(item.race, item.caste, df.caste_raw_flags.PET_EXOTIC)
    then
        return DISPOSITION.WILD_TRAINABLE.value
    end
    return DISPOSITION.WILD_UNTRAINABLE.value
end

local function is_assignable_unit(unit)
    if not dfhack.units.isActive(unit) or dfhack.units.isDead(unit) then
        return false
    end
    local caged = get_cage_ref(unit)
    if dfhack.units.isAnimal(unit) then
        return (dfhack.units.isOwnCiv(unit) or caged) and
            not dfhack.units.isMerchant(unit) and
            not dfhack.units.isForest(unit)
    end
    return caged
end

local function is_assignable_item(item)
    -- all vermin/small pets are assignable
    return true
end

local function get_vermin_desc(vermin, raw)
    if not raw then return 'Unknown vermin' end
    if vermin.stack_size > 1 then
        return ('%s [%d]'):format(raw.name[1], vermin.stack_size)
    end
    return ('%s'):format(raw.name[0])
end

local function get_small_pet_desc(raw)
    if not raw then return 'Unknown small pet' end
    return ('tame %s'):format(raw.name[0])
end

function AssignAnimal:cache_choices()
    if self.choices then return self.choices end

    local bld_assignments = get_bld_assignments()
    local choices = {}
    for _, unit in ipairs(df.global.world.units.active) do
        if not get_general_ref(unit, df.general_ref_type.BUILDING_CIVZONE_ASSIGNED) then
            if not is_assignable_unit(unit) then goto continue end
        end
        local raw = df.creature_raw.find(unit.race)
        local desc = dfhack.units.getReadableName(unit)
        if #unit.custom_profession > 0 then
            desc = unit.custom_profession .. ', ' .. desc
        end
        local data = {
            unit=unit,
            desc=desc,
            dist_fn=curry(self.get_distance, xyz2pos(dfhack.units.getPosition(unit))),
            gender=unit.sex,
            race=raw and raw.creature_id or '',
            status=self.get_status(unit, bld_assignments),
            disposition=get_unit_disposition(unit),
            egg=dfhack.units.isEggLayerRace(unit),
            graze=dfhack.units.isGrazer(unit),
            juvenile=not dfhack.units.isAdult(unit),
            gelded=dfhack.units.isGelded(unit),
        }
        local choice = {
            search_key=make_search_key(data.desc, raw),
            data=data,
            text=self:make_choice_text(data),
        }
        table.insert(choices, choice)
        ::continue::
    end
    for _, vermin in ipairs(df.global.world.items.other.VERMIN) do
        if not is_assignable_item(vermin) then goto continue end
        local raw = df.creature_raw.find(vermin.race)
        local data = {
            vermin=vermin,
            desc=get_vermin_desc(vermin, raw),
            dist_fn=curry(self.get_distance, xyz2pos(dfhack.items.getPosition(vermin))),
            gender=df.pronoun_type.it,
            race=raw and raw.creature_id or '',
            status=self.get_status(vermin, bld_assignments),
            disposition=get_item_disposition(vermin),
        }
        local choice = {
            search_key=make_search_key(data.desc, raw),
            data=data,
            text=self:make_choice_text(data),
        }
        table.insert(choices, choice)
        ::continue::
    end
    for _, small_pet in ipairs(df.global.world.items.other.PET) do
        if not is_assignable_item(small_pet) then goto continue end
        local raw = df.creature_raw.find(small_pet.race)
        local data = {
            vermin=small_pet,
            desc=get_small_pet_desc(raw),
            dist_fn=curry(self.get_distance, xyz2pos(dfhack.items.getPosition(small_pet))),
            gender=df.pronoun_type.it,
            race=raw and raw.creature_id or '',
            status=self.get_status(small_pet, bld_assignments),
            disposition=get_item_disposition(small_pet),
        }
        local choice = {
            search_key=make_search_key(data.desc, raw),
            data=data,
            text=self:make_choice_text(data),
        }
        table.insert(choices, choice)
        ::continue::
    end

    self.choices = choices
    return choices
end

function matches_filter(filter, value)
    if filter == 'only' and not value then return false end
    if filter == 'exclude' and value then return false end
    return true
end

function AssignAnimal:get_choices()
    local raw_choices = self:cache_choices()
    local show_vermin = self.get_allow_vermin()
    local min_status = self.subviews.min_status:getOptionValue()
    local max_status = self.subviews.max_status:getOptionValue()
    local min_disposition = self.subviews.min_disposition:getOptionValue()
    local max_disposition = self.subviews.max_disposition:getOptionValue()
    local egg = self.subviews.egg:getOptionValue()
    local graze = self.subviews.graze:getOptionValue()
    local juvenile = self.subviews.juvenile:getOptionValue()
    local choices = {}
    for _,choice in ipairs(raw_choices) do
        local data = choice.data
        if not show_vermin and data.vermin then goto continue end
        if min_status > data.status then goto continue end
        if max_status < data.status then goto continue end
        if min_disposition > data.disposition then goto continue end
        if max_disposition < data.disposition then goto continue end
        if not matches_filter(egg, data.egg) then goto continue end
        if not matches_filter(graze, data.graze) then goto continue end
        if not matches_filter(juvenile, data.juvenile) then goto continue end
        table.insert(choices, choice)
        ::continue::
    end
    table.sort(choices, self.subviews.sort:getOptionValue())
    return choices
end

local function get_bld_assigned_vec(bld, unit_or_vermin)
    if not bld then return end
    return df.unit:is_instance(unit_or_vermin) and bld.assigned_units or bld.assigned_items
end

local function get_assigned_unit_or_vermin_idx(bld, unit_or_vermin, vec)
    vec = vec or get_bld_assigned_vec(bld, unit_or_vermin)
    if not vec then return end
    for assigned_idx, assigned_id in ipairs(vec) do
        if assigned_id == unit_or_vermin.id then
            return assigned_idx
        end
    end
end

local function unassign_unit_or_vermin(bld, unit_or_vermin)
    if not bld then return end
    if df.building_chainst:is_instance(bld) then
        if bld.assigned == unit_or_vermin then
            bld.assigned = nil
        end
        return
    end
    local vec = get_bld_assigned_vec(bld, unit_or_vermin)
    local idx = get_assigned_unit_or_vermin_idx(bld, unit_or_vermin, vec)
    if vec and idx then
        vec:erase(idx)
    end
end

local function detach_unit_or_vermin(unit_or_vermin, bld_assignments)
    for idx = #unit_or_vermin.general_refs-1, 0, -1 do
        local ref = unit_or_vermin.general_refs[idx]
        if df.general_ref_building_civzone_assignedst:is_instance(ref) then
            unassign_unit_or_vermin(df.building.find(ref.building_id), unit_or_vermin)
            unit_or_vermin.general_refs:erase(idx)
            ref:delete()
        elseif df.general_ref_contained_in_itemst:is_instance(ref) then
            local built_cage = get_built_cage(df.item.find(ref.item_id))
            if built_cage and built_cage:getType() == df.building_type.Cage then
                unassign_unit_or_vermin(built_cage, unit_or_vermin)
                -- unit's general ref will be removed when the unit is released from the cage
            end
        end
    end
    bld_assignments = bld_assignments or get_bld_assignments()
    if bld_assignments[unit_or_vermin.id] then
        unassign_unit_or_vermin(bld_assignments[unit_or_vermin.id], unit_or_vermin)
    end
end

function AssignAnimal:toggle_item_base(choice, target_value, bld_assignments)
    local true_value = self.status[self.status_revmap[1]].value

    if target_value == nil then
        target_value = choice.data.status ~= true_value
    end

    if target_value and choice.data.status == true_value then
        return target_value
    end
    if not target_value and choice.data.status ~= true_value then
        return target_value
    end

    if self.initial_min_disposition ~= DISPOSITION.PET.value and choice.data.disposition == DISPOSITION.PET.value then
        return target_value
    end

    local unit_or_vermin = choice.data.unit or choice.data.vermin
    detach_unit_or_vermin(unit_or_vermin, bld_assignments)

    if target_value then
        local displaced_unit = self.attach(unit_or_vermin)
        if displaced_unit then
            -- assigning a unit to a restraint can unassign a different unit
            for _, c in ipairs(self.subviews.list:getChoices()) do
                if c.data.unit == displaced_unit then
                    c.data.status = self.get_status(displaced_unit)
                end
            end
        end
    end

    -- don't pass bld_assignments since it may no longer be valid
    choice.data.status = self.get_status(unit_or_vermin)

    return target_value
end

function AssignAnimal:select_item(idx, choice)
    if not dfhack.internal.getModifiers().shift then
        self.prev_list_idx = self.subviews.list.list:getSelected()
    end
end

function AssignAnimal:toggle_item(idx, choice)
    self:toggle_item_base(choice)
end

function AssignAnimal:toggle_range(idx, choice)
    if not self.get_multi_select() then
        self:toggle_item(idx, choice)
        return
    end
    if not self.prev_list_idx then
        self:toggle_item(idx, choice)
        return
    end
    local choices = self.subviews.list:getVisibleChoices()
    local list_idx = self.subviews.list.list:getSelected()
    local bld_assignments = get_bld_assignments()
    local target_value
    for i = list_idx, self.prev_list_idx, list_idx < self.prev_list_idx and 1 or -1 do
        target_value = self:toggle_item_base(choices[i], target_value, bld_assignments)
    end
    self.prev_list_idx = list_idx
end

function AssignAnimal:toggle_visible()
    local bld_assignments = get_bld_assignments()
    local target_value
    for _, choice in ipairs(self.subviews.list:getVisibleChoices()) do
        target_value = self:toggle_item_base(choice, target_value, bld_assignments)
    end
end

-- -------------------
-- AssignAnimalScreen
--

view = view or nil

AssignAnimalScreen = defclass(AssignAnimalScreen, gui.ZScreen)
AssignAnimalScreen.ATTRS {
    focus_path='zone/assign',
    is_valid_ui_state=DEFAULT_NIL,
    status=DEFAULT_NIL,
    status_revmap=DEFAULT_NIL,
    get_status=DEFAULT_NIL,
    get_distance=DEFAULT_NIL,
    get_allow_vermin=DEFAULT_NIL,
    get_multi_select=DEFAULT_NIL,
    attach=DEFAULT_NIL,
    initial_min_disposition=DEFAULT_NIL,
    target_name=DEFAULT_NIL,
}

function AssignAnimalScreen:init()
    self:addviews{
        AssignAnimal{
            status=self.status,
            status_revmap=self.status_revmap,
            get_status=self.get_status,
            get_distance=self.get_distance,
            get_allow_vermin=self.get_allow_vermin,
            get_multi_select=self.get_multi_select,
            attach=self.attach,
            initial_min_disposition=self.initial_min_disposition,
            target_name=self.target_name,
        }
    }
end

function AssignAnimalScreen:onInput(keys)
    local handled = AssignAnimalScreen.super.onInput(self, keys)
    if not self.is_valid_ui_state() then
        if view then
            view:dismiss()
        end
        return
    end
    if keys._MOUSE_L then
        -- if any click is made outside of our window, we need to recheck unit properties
        local window = self.subviews[1]
        if not window:getMouseFramePos() then
            for _, choice in ipairs(self.subviews.list:getChoices()) do
                choice.data.status = self.get_status(choice.data.unit or choice.data.vermin)
            end
            window:refresh_list()
        end
    end
    return handled
end

function AssignAnimalScreen:onRenderFrame()
    if view and not self.is_valid_ui_state() then
        view:dismiss()
    end
end

function AssignAnimalScreen:onDismiss()
    view = nil
end

-- -------------------
-- PasturePondOverlay
--

PasturePondOverlay = defclass(PasturePondOverlay, overlay.OverlayWidget)
PasturePondOverlay.ATTRS{
    desc='Adds a link to launch the animal assignment UI to pastures and ponds.',
    default_pos={x=7,y=13},
    default_enabled=true,
    viewscreens={'dwarfmode/Zone/Some/Pen', 'dwarfmode/Zone/Some/Pond'},
    frame={w=31, h=4},
}

local function is_valid_zone()
    return df.global.game.main_interface.bottom_mode_selected == df.main_bottom_mode_type.ZONE and
        df.global.game.main_interface.civzone.cur_bld and
        (df.global.game.main_interface.civzone.cur_bld.type == df.civzone_type.Pen or
         df.global.game.main_interface.civzone.cur_bld.type == df.civzone_type.Pond)
end

local function is_pit_selected()
    return df.global.game.main_interface.bottom_mode_selected == df.main_bottom_mode_type.ZONE and
        df.global.game.main_interface.civzone.cur_bld and
        df.global.game.main_interface.civzone.cur_bld.type == df.civzone_type.Pond
end

local function attach_to_zone(unit_or_vermin)
    local zone = df.global.game.main_interface.civzone.cur_bld
    local ref = df.new(df.general_ref_building_civzone_assignedst)
    ref.building_id = zone.id;
    unit_or_vermin.general_refs:insert('#', ref)
    local is_unit = df.unit:is_instance(unit_or_vermin)
    local vec = is_unit and zone.assigned_units or zone.assigned_items
    utils.insert_sorted(vec, unit_or_vermin.id)
end

local PASTURE_STATUS = {
    NONE={label='Unknown', value=0},
    ASSIGNED_HERE={label='Assigned here', value=1},
    PASTURED={label='In other pasture', value=2},
    PITTED={label='In other pit/pond', value=3},
    RESTRAINED={label='On restraint', value=4},
    BUILT_CAGE={label='In built cage', value=5},
    ITEM_CAGE={label='In loose cage', value=6},
    ROAMING={label='Roaming', value=7},
}
local PASTURE_STATUS_REVMAP = {}
for k, v in pairs(PASTURE_STATUS) do
    PASTURE_STATUS_REVMAP[v.value] = k
end

local function get_zone_status(unit_or_vermin, bld_assignments)
    local assigned_zone_ref = get_general_ref(unit_or_vermin, df.general_ref_type.BUILDING_CIVZONE_ASSIGNED)
    if assigned_zone_ref then
        if df.global.game.main_interface.civzone.cur_bld.id == assigned_zone_ref.building_id then
            return PASTURE_STATUS.ASSIGNED_HERE.value
        else
            local civzone = df.building.find(assigned_zone_ref.building_id)
            if civzone.type == df.civzone_type.Pen then
                return PASTURE_STATUS.PASTURED.value
            elseif civzone.type == df.civzone_type.Pond then
                return PASTURE_STATUS.PITTED.value
            else
                return PASTURE_STATUS.NONE.value
            end
        end
    end
    if get_general_ref(unit_or_vermin, df.general_ref_type.BUILDING_CHAIN) then
        return PASTURE_STATUS.RESTRAINED.value
    end
    local cage_ref = get_cage_ref(unit_or_vermin)
    if cage_ref then
        if get_built_cage(df.item.find(cage_ref.item_id)) then
            return PASTURE_STATUS.BUILT_CAGE.value
        else
            return PASTURE_STATUS.ITEM_CAGE.value
        end
    end
    bld_assignments = bld_assignments or get_bld_assignments()
    if bld_assignments and bld_assignments[unit_or_vermin.id] then
        if df.building_chainst:is_instance(bld_assignments[unit_or_vermin.id]) then
            return PASTURE_STATUS.RESTRAINED.value
        end
        return PASTURE_STATUS.BUILT_CAGE.value
    end
    return PASTURE_STATUS.ROAMING.value
end

local function get_zone_distance(pos)
    local zone = df.global.game.main_interface.civzone.cur_bld
    if not zone then return 0 end
    if zone.z == pos.z and dfhack.buildings.containsTile(zone, pos.x, pos.y) then return 0 end
    return math.max(math.abs(zone.centerx - pos.x), math.abs(zone.centery - pos.y)) + math.abs(zone.z - pos.z)
end

local function show_pasture_pond_screen()
    return AssignAnimalScreen{
        is_valid_ui_state=is_valid_zone,
        status=PASTURE_STATUS,
        status_revmap=PASTURE_STATUS_REVMAP,
        get_status=get_zone_status,
        get_distance=get_zone_distance,
        get_allow_vermin=is_pit_selected,
        get_multi_select=function() return true end,
        attach=attach_to_zone,
        target_name='pasture/pond/pit',
    }:show()
end

function PasturePondOverlay:init()
    self:addviews{
        widgets.TextButton{
            frame={t=0, l=0, w=23, h=1},
            label='DFHack assign',
            key='CUSTOM_CTRL_T',
            on_activate=function() view = view and view:raise() or show_pasture_pond_screen() end,
        },
        widgets.TextButton{
            frame={b=0, l=0, w=28, h=1},
            label='DFHack autobutcher',
            key='CUSTOM_CTRL_B',
            on_activate=function() dfhack.run_script('gui/autobutcher') end,
        },
    }
end

-- -------------------
-- CageChainOverlay
--

CageChainOverlay = defclass(CageChainOverlay, overlay.OverlayWidget)
CageChainOverlay.ATTRS{
    desc='Adds a link to launch the animal assignment UI to cages and chains.',
    default_pos={x=-64,y=34},
    version=2,
    default_enabled=true,
    viewscreens={'dwarfmode/ViewSheets/BUILDING/Cage', 'dwarfmode/ViewSheets/BUILDING/Chain'},
    frame={w=23, h=1},
    frame_background=gui.CLEAR_PEN,
}

local function is_valid_building()
    local bld = dfhack.gui.getSelectedBuilding(true)
    if not bld or bld:getBuildStage() ~= bld:getMaxBuildStage() then return false end
    local bt = bld:getType()
    if bt ~= df.building_type.Cage and bt ~= df.building_type.Chain then return false end
    for _,zone in ipairs(bld.relations) do
        if zone.type == df.civzone_type.Dungeon then return false end
    end
    return true
end

local function is_cage_selected()
    local bld = dfhack.gui.getSelectedBuilding(true)
    return bld and bld:getType() == df.building_type.Cage
end

local function attach_to_building(unit_or_vermin)
    local bld = dfhack.gui.getSelectedBuilding(true)
    if not bld then return end
    local is_unit = df.unit:is_instance(unit_or_vermin)
    if is_unit and unit_or_vermin.relationship_ids[df.unit_relationship_type.PetOwner] ~= -1 then
        -- pet owners would just release them
        return
    end
    if bld:getType() == df.building_type.Cage then
        local vec = is_unit and bld.assigned_units or bld.assigned_items
        vec:insert('#', unit_or_vermin.id)
    elseif is_unit and bld:getType() == df.building_type.Chain then
        local prev_unit = bld.assigned
        bld.assigned = unit_or_vermin
        return prev_unit
    end
end

local function get_built_chain(unit_or_vermin)
    local built_chain_ref = get_general_ref(unit_or_vermin, df.general_ref_type.BUILDING_CHAIN)
    if not built_chain_ref then return end
    return df.building.find(built_chain_ref.building_id)
end

local CAGE_STATUS = {
    NONE={label='Unknown', value=0},
    ASSIGNED_HERE={label='Assigned here', value=1},
    PASTURED={label='In pasture', value=2},
    PITTED={label='In pit/pond', value=3},
    RESTRAINED={label='On other chain', value=4},
    BUILT_CAGE={label='In built cage', value=5},
    ITEM_CAGE={label='In stockpiled cage', value=6},
    ROAMING={label='Roaming', value=7},
}
local CAGE_STATUS_REVMAP = {}
for k, v in pairs(CAGE_STATUS) do
    CAGE_STATUS_REVMAP[v.value] = k
end

local function get_cage_status(unit_or_vermin, bld_assignments)
    local bld = dfhack.gui.getSelectedBuilding(true)
    local is_chain = bld and df.building_chainst:is_instance(bld)

    if is_chain and bld.assigned == unit_or_vermin then
        return CAGE_STATUS.ASSIGNED_HERE.value
    elseif not is_chain and get_assigned_unit_or_vermin_idx(bld, unit_or_vermin) then
        return CAGE_STATUS.ASSIGNED_HERE.value
    end
    local cage_ref = get_cage_ref(unit_or_vermin)
    if cage_ref then
        if get_built_cage(df.item.find(cage_ref.item_id)) then
            return CAGE_STATUS.BUILT_CAGE.value
        end
        return CAGE_STATUS.ITEM_CAGE.value
    end
    local built_chain = get_built_chain(unit_or_vermin)
    if built_chain then
        if bld and bld == built_chain then
            return CAGE_STATUS.ASSIGNED_HERE.value
        end
        return CAGE_STATUS.RESTRAINED.value
    end
    local assigned_zone_ref = get_general_ref(unit_or_vermin, df.general_ref_type.BUILDING_CIVZONE_ASSIGNED)
    if assigned_zone_ref then
        local civzone = df.building.find(assigned_zone_ref.building_id)
        if civzone.type == df.civzone_type.Pen then
            return CAGE_STATUS.PASTURED.value
        elseif civzone.type == df.civzone_type.Pond then
            return CAGE_STATUS.PITTED.value
        else
            return CAGE_STATUS.NONE.value
        end
    end
    bld_assignments = bld_assignments or get_bld_assignments()
    if bld_assignments and bld_assignments[unit_or_vermin.id] then
        if df.building_chainst:is_instance(bld_assignments[unit_or_vermin.id]) then
            return CAGE_STATUS.RESTRAINED.value
        end
        return CAGE_STATUS.BUILT_CAGE.value
    end
    return CAGE_STATUS.ROAMING.value
end

local function get_cage_distance(pos)
    local bld = dfhack.gui.getSelectedBuilding(true)
    if not bld then return 0 end
    return math.max(math.abs(bld.centerx - pos.x), math.abs(bld.centery - pos.y)) + math.abs(bld.z - pos.z)
end

local function show_cage_chain_screen()
    return AssignAnimalScreen{
        is_valid_ui_state=is_valid_building,
        status=CAGE_STATUS,
        status_revmap=CAGE_STATUS_REVMAP,
        get_status=get_cage_status,
        get_distance=get_cage_distance,
        get_allow_vermin=is_cage_selected,
        get_multi_select=is_cage_selected,
        attach=attach_to_building,
        initial_min_disposition=DISPOSITION.TAME.value,
        target_name='cage/restraint',
    }:show()
end

function CageChainOverlay:init()
    self:addviews{
        widgets.TextButton{
            frame={t=0, l=0, r=0, h=1},
            label='DFHack assign',
            key='CUSTOM_CTRL_T',
            visible=is_valid_building,
            on_activate=function() view = view and view:raise() or show_cage_chain_screen() end,
        },
    }
end

-- ---------------------
-- RetireLocationOverlay
--

local mi = df.global.game.main_interface

local function location_details_is_on_top()
    return not dfhack.gui.matchFocusString('dwarfmode/NameCreator') and
        not dfhack.gui.matchFocusString('dwarfmode/UnitSelector')
end

RetireLocationOverlay = defclass(RetireLocationOverlay, overlay.OverlayWidget)
RetireLocationOverlay.ATTRS{
    desc='Adds a button to retire unneeded locations to the location details screen.',
    default_pos={x=-39,y=6},
    default_enabled=true,
    viewscreens='dwarfmode/LocationDetails',
    frame={w=25, h=3},
}

function RetireLocationOverlay:init()
    self:addviews{
        widgets.Panel{
            frame_background=gui.CLEAR_PEN,
            frame_style=gui.FRAME_MEDIUM,
            visible=location_details_is_on_top,
            subviews={
                widgets.HotkeyLabel{
                    frame={t=0, l=0},
                    label='Retire location',
                    key='CUSTOM_CTRL_D',
                    on_activate=self:callback('confirm_retire'),
                },
            },
        },
    }
end

local function retire(location)
    location.flags.DOES_NOT_EXIST = true
    for idx, loc in ipairs(mi.location_selector.valid_ab) do
        if loc.id == location.id then
            mi.location_selector.valid_ab:erase(idx)
            break
        end
    end
    local occupations = df.global.world.occupations.all
    for idx = #occupations-1,0,-1 do
        local occupation = occupations[idx]
        if occupation.site_id == location.site_id and
            occupation.location_id == location.id
        then
            occupations:erase(idx)
        end
    end
    mi.location_details.open = false
end

function RetireLocationOverlay:confirm_retire()
    local details = mi.location_details
    local location = details.selected_ab
    local num_occupations, num_zones = 0, 0
    for _, occupation in ipairs(location.occupations) do
        if occupation.histfig_id ~= -1 then
            num_occupations = num_occupations + 1
        end
    end
    for _, zone_id in ipairs(location.contents.building_ids) do
        -- there can be dangling references in this list; only count
        -- "attached" zones that actually exist
        if df.building.find(zone_id) then
            num_zones = num_zones + 1
        end
    end
    if num_occupations + num_zones > 0 then
        local messages = {'Cannot retire location! Please:', ''}
        if num_occupations > 0 then
            table.insert(messages, ('- unassign %d location occupation(s)'):format(num_occupations))
        end
        if num_zones > 0 then
            table.insert(messages, ('- detach this location from %d zone(s)'):format(num_zones))
        end
        table.insert(messages, '')
        table.insert(messages, 'and try again')
        dialogs.showMessage('Location in use', table.concat(messages, NEWLINE))
        return
    end
    dialogs.showYesNoPrompt('Confirm retire location',
        'Are you sure you want to retire this location?'..NEWLINE..'You won\'t be able to use it again.',
        COLOR_WHITE,
        curry(retire, location))
end

-- -------------------
-- AnimalActionsWidget
--

local function isFortAnimal(unit)
    return dfhack.units.isFortControlled(unit)
        and dfhack.units.isAlive(unit)
        and dfhack.units.isAnimal(unit)
end

local function isCagedWildAnimal(unit)
    return dfhack.units.isTamable(unit)
        and unit.flags1.caged
        and not unit.flags1.tame
end

-- Make sure an animal unit of your civ is selected
local function check_valid_unit()
    local unit = dfhack.gui.getSelectedUnit(true)

    return unit and (isFortAnimal(unit) or isCagedWildAnimal(unit))
end

local function is_geldable()
    local unit = dfhack.gui.getSelectedUnit(true)

    return unit and dfhack.units.isGeldable(unit) and dfhack.units.isFortControlled(unit) and not dfhack.units.isGelded(unit)
end

local function is_not_pet()
    local unit = dfhack.gui.getSelectedUnit(true)

    return not unit or not dfhack.units.isPet(unit) and dfhack.units.isFortControlled(unit)
end

local function is_avail_adoption()
    local unit = dfhack.gui.getSelectedUnit(true)
    if not unit then return false end

    if dfhack.units.isPet(unit) then return false end

    local raw = df.creature_raw.find(unit.race)
    if not raw then return false end

    if not dfhack.units.isFortControlled(unit) then return false end

    -- cats adopt owners; owners can't adopt cats
    return raw.creature_id ~= 'CAT'
end

local function is_tamable()
    local unit = dfhack.gui.getSelectedUnit(true)

    return dfhack.units.isTamable(unit) and not dfhack.units.isDomesticated(unit)
end

AnimalActionsWidget=defclass(AnimalActionsWidget, overlay.OverlayWidget)
AnimalActionsWidget.ATTRS {
    desc="Add options to tamed animals view sheet.",
    default_pos={x=-41,y=37},
    default_enabled=true,
    viewscreens='dwarfmode/ViewSheets/UNIT/Overview',
    frame={w=25, h=6},
}

-- The above function already handles checking if valid unit
-- so just set slaughter flag
function AnimalActionsWidget:set_slaughter_flag(option)
    local unit = dfhack.gui.getSelectedUnit(true)

    if not unit then return end

    -- emulate vanilla game behavior by only being able to have
    -- 1 set at a time
    if option then
        self:set_geld_flag(false)
        self:set_adoption_flag(false)
    end

    unit.flags2.slaughter = option
end

-- set units marked for gelding flag
function AnimalActionsWidget:set_geld_flag(option)
    local unit = dfhack.gui.getSelectedUnit(true)

    if not unit then return end

    if option then
        self:set_slaughter_flag(false)
    end

    unit.flags3.marked_for_gelding = option
end

-- set available for adoption flag
function AnimalActionsWidget:set_adoption_flag(option)
    local unit = dfhack.gui.getSelectedUnit(true)

    if not unit then return end

    if option then
        self:set_slaughter_flag(false)
    end

    unit.flags3.available_for_adoption = option
end

-- Assign any trainer
function AnimalActionsWidget:set_tame(option)
    local unit = dfhack.gui.getSelectedUnit(true)

    if not unit then return end

    if option then
        dfhack.units.assignTrainer(unit)
    else
        dfhack.units.unassignTrainer(unit)
    end
end

-- Use render to set On/Off dynamically for each unit
function AnimalActionsWidget:render(dc)
    local unit = dfhack.gui.getSelectedUnit(true)

    if unit then
        self.subviews.butcher_animal:setOption(dfhack.units.isMarkedForSlaughter(unit))
        self.subviews.geld_animal:setOption(dfhack.units.isMarkedForGelding(unit))
        self.subviews.adopt_animal:setOption(dfhack.units.isAvailableForAdoption(unit))
        self.subviews.tame_animal:setOption(dfhack.units.isMarkedForTraining(unit))
    end

    AnimalActionsWidget.super.render(self, dc)
end

function AnimalActionsWidget:init()
    self:addviews{
        widgets.Panel{
            frame_background=gui.CLEAR_PEN,
            frame_style=gui.FRAME_MEDIUM,
            visible=check_valid_unit,
            subviews={
                widgets.ToggleHotkeyLabel{
                    frame={t=0,l=0},
                    label='Butcher    ',
                    key='CUSTOM_CTRL_B',
                    options={
                        {label='No', value=false, pen=COLOR_WHITE},
                        {label='Yes', value=true, pen=COLOR_YELLOW},
                    },
                    view_id='butcher_animal',
                    enabled=is_not_pet,
                    on_change=self:callback('set_slaughter_flag'),
                },
                widgets.ToggleHotkeyLabel{
                    frame={t=1,l=0},
                    label='Geld       ',
                    key='CUSTOM_CTRL_G',
                    options={
                        {label='No', value=false, pen=COLOR_WHITE},
                        {label='Yes', value=true, pen=COLOR_YELLOW},
                    },
                    view_id='geld_animal',
                    enabled=is_geldable,
                    on_change=self:callback('set_geld_flag'),
                },
                widgets.ToggleHotkeyLabel{
                    frame={t=2,l=0},
                    label='Adopt      ',
                    key='CUSTOM_CTRL_A',
                    options={
                        {label='No', value=false, pen=COLOR_WHITE},
                        {label='Yes', value=true, pen=COLOR_YELLOW},
                    },
                    view_id='adopt_animal',
                    enabled=is_avail_adoption,
                    on_change=self:callback('set_adoption_flag'),
                },
                widgets.ToggleHotkeyLabel{
                    frame={t=3,l=0},
                    label='Has Trainer',
                    key='CUSTOM_CTRL_T',
                    options={
                        {label='No', value=false, pen=COLOR_WHITE},
                        {label='Yes', value=true, pen=COLOR_YELLOW},
                    },
                    view_id='tame_animal',
                    enabled=is_tamable,
                    on_change=self:callback('set_tame'),
                },
            },
        },
    }
end

OVERLAY_WIDGETS = {
    pasturepond=PasturePondOverlay,
    cagechain=CageChainOverlay,
    retirelocation=RetireLocationOverlay,
    animal_actions=AnimalActionsWidget,
}

return _ENV
