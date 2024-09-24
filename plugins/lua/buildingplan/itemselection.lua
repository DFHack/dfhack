local _ENV = mkmodule('plugins.buildingplan.itemselection')

local gui = require('gui')
local pens = require('plugins.buildingplan.pens')
local utils = require('utils')
local widgets = require('gui.widgets')
local caravan = reqscript('internal/caravan/common')

local uibs = df.global.buildreq
local to_pen = dfhack.pen.parse

local BUILD_TEXT_PEN = to_pen{fg=COLOR_BLACK, bg=COLOR_GREEN, keep_lower=true}
local BUILD_TEXT_HPEN = to_pen{fg=COLOR_WHITE, bg=COLOR_GREEN, keep_lower=true}

-- map of building type -> {set=set of recently used, list=list of recently used}
-- most recent entries are at the *end* of the list
local recently_used = {}

function get_automaterial_selection(building_type)
    local tracker = recently_used[building_type]
    if not tracker or not tracker.list then return end
    return tracker.list[#tracker.list]
end

function get_item_description(item_id, item, safety_label)
    item = item or df.item.find(item_id)
    if not item then
        return ('No %s safe mechanisms available'):format(safety_label:lower())
    end
    return dfhack.items.getReadableDescription(item)
end

local function sort_by_type(a, b)
    local ad, bd = a.data, b.data
    return ad.item_type < bd.item_type or
            (ad.item_type == bd.item_type and ad.item_subtype < bd.item_subtype) or
            (ad.item_type == bd.item_type and ad.item_subtype == bd.item_subtype and a.search_key < b.search_key) or
            (ad.item_type == bd.item_type and ad.item_subtype == bd.item_subtype and a.search_key == b.search_key and ad.quality > bd.quality)
end

local function sort_by_recency(a, b)
    local tracker = recently_used[uibs.building_type]
    if not tracker then return sort_by_type(a, b) end
    local recent_a, recent_b = tracker.set[a.search_key], tracker.set[b.search_key]
    -- if they're both in the set, return the one with the greater index,
    -- indicating more recent
    if recent_a and recent_b then return recent_a > recent_b end
    if recent_a and not recent_b then return true end
    if not recent_a and recent_b then return false end
    return sort_by_type(a, b)
end

local function sort_by_name(a, b)
    return a.search_key < b.search_key or
            (a.search_key == b.search_key and sort_by_type(a, b))
end

local function sort_by_quantity(a, b)
    local ad, bd = a.data, b.data
    return ad.quantity > bd.quantity or
            (ad.quantity == bd.quantity and sort_by_type(a, b))
end

local function sort_by_value(a, b)
    local ad, bd = a.data, b.data
    return ad.value > bd.value or
            (ad.value == bd.value and sort_by_type(a, b))
end

ItemSelection = defclass(ItemSelection, widgets.Window)
ItemSelection.ATTRS{
    frame_title='Choose items',
    frame={w=56, h=24, l=4, t=7},
    resizable=true,
    get_available_items_fn=DEFAULT_NIL,
    desc=DEFAULT_NIL,
    quantity=DEFAULT_NIL,
    autoselect=DEFAULT_NIL,
    on_submit=DEFAULT_NIL,
    on_cancel=DEFAULT_NIL,
}

function ItemSelection:init()
    self.num_selected = 0
    self.selected_set = {}
    local plural = self.quantity == 1 and '' or 's'

    local choices = self:get_choices(sort_by_recency)

    if self.autoselect then
        self:do_autoselect(choices)
        if self.num_selected >= self.quantity then
            self:submit(choices)
            return
        end
    end

    self:addviews{
        widgets.Panel{
            view_id='header',
            frame={t=0, h=3},
            subviews={
                widgets.Label{
                    frame={t=0, l=0, r=16},
                    text={
                        self.desc, plural, NEWLINE,
                        ('Select up to %d item%s ('):format(self.quantity, plural),
                        {text=function() return self.num_selected end},
                        ' selected)',
                    },
                },
                widgets.Label{
                    frame={r=0, w=15, t=0, h=3},
                    text=widgets.makeButtonLabelText{
                        chars={
                            '               ',
                            '   Autoselect  ',
                            '               ',
                        },
                        pens=BUILD_TEXT_PEN,
                        pens_hover=BUILD_TEXT_HPEN,
                    },
                    on_click=self:callback('submit'),
                    visible=function() return self.num_selected < self.quantity end,
                },
                widgets.Label{
                    frame={r=0, w=15, t=0, h=3},
                    text=widgets.makeButtonLabelText{
                        chars={
                            '               ',
                            '    Continue   ',
                            '               ',
                        },
                        pens=BUILD_TEXT_PEN,
                        pens_hover=BUILD_TEXT_HPEN,
                    },
                    on_click=self:callback('submit'),
                    visible=function() return self.num_selected >= self.quantity end,
                },
            },
        },
    }

    self:addviews{
        widgets.Panel{
            view_id='body',
            frame={t=self.subviews.header.frame.h, b=4},
            subviews={
                widgets.EditField{
                    view_id='search',
                    frame={l=1, t=0},
                    label_text='Search: ',
                    on_char=function(ch) return ch:match('[%l -]') end,
                },
                widgets.CycleHotkeyLabel{
                    frame={l=1, t=2},
                    key='CUSTOM_SHIFT_R',
                    label='Sort by:',
                    options={
                        {label='Recently used', value=sort_by_recency},
                        {label='Amount', value=sort_by_quantity},
                        {label='Value', value=sort_by_value},
                        {label='Name', value=sort_by_name},
                    },
                    on_change=self:callback('on_sort'),
                },
                widgets.Panel{
                    frame={l=0, t=3, r=0, b=0},
                    frame_style=gui.FRAME_INTERIOR,
                    subviews={
                        widgets.FilteredList{
                            view_id='flist',
                            frame={t=0, b=0},
                            choices=choices,
                            icon_width=2,
                            on_submit=self:callback('toggle_group'),
                        },
                    },
                },
            },
        },
        widgets.Panel{
            view_id='footer',
            frame={l=1, r=1, b=0, h=3},
            subviews={
                widgets.HotkeyLabel{
                    frame={l=0, h=1, t=0},
                    key='KEYBOARD_CURSOR_RIGHT_FAST',
                    key_sep='----: ', -- these hypens function as "padding" to be overwritten by the next Label
                    label='Use one',
                    auto_width=true,
                    on_activate=function() self:increment_group(self.subviews.flist.list:getSelected()) end,
                },
                widgets.Label{
                    frame={l=6, w=5, t=0},
                    text_pen=COLOR_LIGHTGREEN,
                    text='Right', -- this overrides the "6----" characters from the previous HotkeyLabel
                },
                widgets.HotkeyLabel{
                    frame={l=1, h=1, t=1},
                    key='KEYBOARD_CURSOR_LEFT_FAST',
                    key_sep='---: ', -- these hypens function as "padding" to be overwritten by the next Label
                    label='Use one fewer',
                    auto_width=true,
                    on_activate=function() self:decrement_group(self.subviews.flist.list:getSelected()) end,
                },
                widgets.Label{
                    frame={l=7, w=4, t=1},
                    text_pen=COLOR_LIGHTGREEN,
                    text='Left', -- this overrides the "4---" characters from the previous HotkeyLabel
                },
                widgets.HotkeyLabel{
                    frame={l=6, t=2, h=2},
                    key='SELECT',
                    label='Use all/none',
                    auto_width=true,
                    on_activate=function() self:toggle_group(self.subviews.flist.list:getSelected()) end,
                },
                widgets.HotkeyLabel{
                    frame={r=5, t=0},
                    key='LEAVESCREEN',
                    label='Go back',
                    auto_width=true,
                    on_activate=self:callback('on_cancel'),
                },
                widgets.HotkeyLabel{
                    frame={r=4, t=2},
                    key='CUSTOM_SHIFT_C',
                    label='Continue',
                    auto_width=true,
                    on_activate=self:callback('submit'),
                },
            },
        },
    }

    self.subviews.flist.list.frame.t = 0
    self.subviews.flist.edit.visible = false
    self.subviews.flist.edit = self.subviews.search
    self.subviews.search.on_change = self.subviews.flist:callback('onFilterChange')
end

-- resort and restore selection
function ItemSelection:on_sort(sort_fn)
    local flist = self.subviews.flist
    local saved_filter = flist:getFilter()
    flist:setFilter('')
    flist:setChoices(self:get_choices(sort_fn), flist:getSelected())
    flist:setFilter(saved_filter)
end

local function make_search_key(str)
    local out = ''
    for c in str:gmatch("[%w%s]") do
        out = out .. c
    end
    return out
end

function ItemSelection:get_choices(sort_fn)
    local item_ids = self.get_available_items_fn()
    local buckets = {}
    for _,item_id in ipairs(item_ids) do
        local item = df.item.find(item_id)
        if not item then goto continue end
        local desc = get_item_description(item_id, item)
        local value = dfhack.items.getValue(item)
        local key = desc .. "_" .. tostring(value)
        if buckets[key] then
            local bucket = buckets[key]
            table.insert(bucket.data.item_ids, item_id)
            bucket.data.quantity = bucket.data.quantity + 1
        else
            local entry = {
                search_key=make_search_key(desc),
                icon=self:callback('get_entry_icon', item_id),
                data={
                    desc=desc,
                    item_ids={item_id},
                    item_type=item:getType(),
                    item_subtype=item:getSubtype(),
                    quantity=1,
                    quality=item:getQuality(),
                    value=value,
                    selected=0,
                },
            }
            buckets[key] = entry
        end
        ::continue::
    end
    local choices = {}
    for key,choice in pairs(buckets) do
        local data = choice.data
        local obfuscated_value = caravan.obfuscate_value(data.value)
        choice.text = {
            {width=8, text=function() return ('%d/%d'):format(data.selected, data.quantity) end},
            {width=9, gap=2, text=function() return ('%d%s'):format(obfuscated_value, caravan.CH_MONEY) end},
            {gap=2, text=data.desc},
        }
        table.insert(choices, choice)
    end
    table.sort(choices, sort_fn)
    return choices
end

function ItemSelection:do_autoselect(choices)
    if #choices == 0 then return end
    local desired = get_automaterial_selection(uibs.building_type)
    if choices[1].search_key ~= desired then return end
    self:toggle_group(1, choices[1])
end

function ItemSelection:increment_group(idx, choice)
    local data = choice.data
    if self.quantity <= self.num_selected then return false end
    if data.selected >= data.quantity then return false end
    data.selected = data.selected + 1
    self.num_selected = self.num_selected + 1
    local item_id = data.item_ids[data.selected]
    self.selected_set[item_id] = true
    return true
end

function ItemSelection:decrement_group(idx, choice)
    local data = choice.data
    if data.selected <= 0 then return false end
    local item_id = data.item_ids[data.selected]
    self.selected_set[item_id] = nil
    self.num_selected = self.num_selected - 1
    data.selected = data.selected - 1
    return true
end

function ItemSelection:toggle_group(idx, choice)
    local data = choice.data
    if data.selected > 0 then
        while self:decrement_group(idx, choice) do end
    else
        while self:increment_group(idx, choice) do end
    end
end

function ItemSelection:get_entry_icon(item_id)
    return self.selected_set[item_id] and pens.SELECTED_ITEM_PEN or nil
end

local function track_recently_used(choices)
    -- use same set for all subtypes
    local tracker = ensure_key(recently_used, uibs.building_type)
    for _,choice in ipairs(choices) do
        local data = choice.data
        if data.selected <= 0 then goto continue end
        local key = choice.search_key
        local recent_set = ensure_key(tracker, 'set')
        local recent_list = ensure_key(tracker, 'list')
        if recent_set[key] then
            if recent_list[#recent_list] ~= key then
                for i,v in ipairs(recent_list) do
                    if v == key then
                        table.remove(recent_list, i)
                        table.insert(recent_list, key)
                        break
                    end
                end
                tracker.set = utils.invert(recent_list)
            end
        else
            -- only keep most recent 10
            if #recent_list >= 10 then
                -- remove least recently used from list and set
                recent_set[table.remove(recent_list, 1)] = nil
            end
            table.insert(recent_list, key)
            recent_set[key] = #recent_list
        end
        ::continue::
    end
end

function ItemSelection:submit(choices)
    local selected_items = {}
    for item_id in pairs(self.selected_set) do
        table.insert(selected_items, item_id)
    end
    if #selected_items > 0 then
        track_recently_used(choices or self.subviews.flist:getChoices())
    end
    self.on_submit(selected_items)
end

function ItemSelection:onInput(keys)
    if keys.LEAVESCREEN or keys._MOUSE_R then
        self.on_cancel()
        return true
    elseif keys._MOUSE_L then
        local list = self.subviews.flist.list
        local idx = list:getIdxUnderMouse()
        if idx then
            list:setSelected(idx)
            local modstate = dfhack.internal.getModstate()
            if modstate & 2 > 0 then -- ctrl
                local choice = list:getChoices()[idx]
                if modstate & 1 > 0 then -- shift
                    self:decrement_group(idx, choice)
                else
                    self:increment_group(idx, choice)
                end
                return true
            end
        end
    end
    return ItemSelection.super.onInput(self, keys)
end

ItemSelectionScreen = defclass(ItemSelectionScreen, gui.ZScreen)
ItemSelectionScreen.ATTRS {
    focus_path='dwarfmode/Building/Placement/dfhack/lua/buildingplan/itemselection',
    force_pause=true,
    pass_movement_keys=true,
    pass_mouse_clicks=false,
    defocusable=false,
    get_available_items_fn=DEFAULT_NIL,
    desc=DEFAULT_NIL,
    quantity=DEFAULT_NIL,
    autoselect=DEFAULT_NIL,
    on_submit=DEFAULT_NIL,
    on_cancel=DEFAULT_NIL,
}

function ItemSelectionScreen:init()
    self:addviews{
        ItemSelection{
            get_available_items_fn=self.get_available_items_fn,
            desc=self.desc,
            quantity=self.quantity,
            autoselect=self.autoselect,
            on_submit=self.on_submit,
            on_cancel=self.on_cancel,
        }
    }
end

return _ENV
