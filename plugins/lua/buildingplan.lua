local _ENV = mkmodule('plugins.buildingplan')

--[[

 Native functions:

 * bool isPlannableBuilding(df::building_type type, int16_t subtype, int32_t custom)
 * bool isPlannedBuilding(df::building *bld)
 * void addPlannedBuilding(df::building *bld)
 * void doCycle()
 * void scheduleCycle()

--]]

local argparse = require('argparse')
local gui = require('gui')
local guidm = require('gui.dwarfmode')
local overlay = require('plugins.overlay')
local utils = require('utils')
local widgets = require('gui.widgets')
require('dfhack.buildings')

local uibs = df.global.buildreq

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    return argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
        })
end

function parse_commandline(...)
    local args, opts = {...}, {}
    local positionals = process_args(opts, args)

    if opts.help then
        return false
    end

    local command = table.remove(positionals, 1)
    if not command or command == 'status' then
        printStatus()
    elseif command == 'set' then
        setSetting(positionals[1], positionals[2] == 'true')
    else
        return false
    end

    return true
end

function get_num_filters(btype, subtype, custom)
    local filters = dfhack.buildings.getFiltersByType({}, btype, subtype, custom)
    return filters and #filters or 0
end

function get_job_item(btype, subtype, custom, index)
    local filters = dfhack.buildings.getFiltersByType({}, btype, subtype, custom)
    if not filters or not filters[index] then return nil end
    local obj = df.job_item:new()
    obj:assign(filters[index])
    return obj
end

local function get_cur_filters()
    return dfhack.buildings.getFiltersByType({}, uibs.building_type,
            uibs.building_subtype, uibs.custom_type)
end

local function is_choosing_area()
    return uibs.selection_pos.x >= 0
end

local function get_cur_area_dims(placement_data)
    if not placement_data and not is_choosing_area() then return 1, 1, 1 end
    local selection_pos = placement_data and placement_data.p1 or uibs.selection_pos
    local pos = placement_data and placement_data.p2 or uibs.pos
    return math.abs(selection_pos.x - pos.x) + 1,
            math.abs(selection_pos.y - pos.y) + 1,
            math.abs(selection_pos.z - pos.z) + 1
end

local function get_quantity(filter, hollow, placement_data)
    local quantity = filter.quantity or 1
    local dimx, dimy, dimz = get_cur_area_dims(placement_data)
    if quantity < 1 then
        return (((dimx * dimy) // 4) + 1) * dimz
    end
    if hollow and dimx > 2 and dimy > 2 then
        return quantity * (2*dimx + 2*dimy - 4) * dimz
    end
    return quantity * dimx * dimy * dimz
end

local BUTTON_START_PEN, BUTTON_END_PEN, SELECTED_ITEM_PEN = nil, nil, nil
local reset_counts_flag = false
local reset_inspector_flag = false
function signal_reset()
    BUTTON_START_PEN = nil
    BUTTON_END_PEN = nil
    SELECTED_ITEM_PEN = nil
    reset_counts_flag = true
    reset_inspector_flag = true
end

local to_pen = dfhack.pen.parse
local function get_button_start_pen()
    if not BUTTON_START_PEN then
        local texpos_base = dfhack.textures.getControlPanelTexposStart()
        BUTTON_START_PEN = to_pen{ch='[', fg=COLOR_YELLOW,
                tile=texpos_base > 0 and texpos_base + 13 or nil}
    end
    return BUTTON_START_PEN
end
local function get_button_end_pen()
    if not BUTTON_END_PEN then
        local texpos_base = dfhack.textures.getControlPanelTexposStart()
        BUTTON_END_PEN = to_pen{ch=']', fg=COLOR_YELLOW,
                tile=texpos_base > 0 and texpos_base + 15 or nil}
    end
    return BUTTON_END_PEN
end
local function get_selected_item_pen()
    if not SELECTED_ITEM_PEN then
        local texpos_base = dfhack.textures.getControlPanelTexposStart()
        SELECTED_ITEM_PEN = to_pen{ch='x', fg=COLOR_GREEN,
                tile=texpos_base > 0 and texpos_base + 9 or nil}
    end
    return SELECTED_ITEM_PEN
end

BuildingplanScreen = defclass(BuildingplanScreen, gui.ZScreen)
BuildingplanScreen.ATTRS {
    pass_movement_keys=true,
    pass_mouse_clicks=false,
    defocusable=false,
}

--------------------------------
-- ItemSelection
--

local BUILD_TEXT_PEN = to_pen{fg=COLOR_BLACK, bg=COLOR_GREEN, keep_lower=true}
local BUILD_TEXT_HPEN = to_pen{fg=COLOR_WHITE, bg=COLOR_GREEN, keep_lower=true}

-- map of building type -> {set=set of recently used, list=list of recently used}
-- most recent entries are at the *end* of the list
local recently_used = {}

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

ItemSelection = defclass(ItemSelection, widgets.Window)
ItemSelection.ATTRS{
    frame_title='Choose items',
    frame={w=56, h=20, l=4, t=8},
    resizable=true,
    index=DEFAULT_NIL,
    quantity=DEFAULT_NIL,
    on_submit=DEFAULT_NIL,
    on_cancel=DEFAULT_NIL,
}

function ItemSelection:init()
    local filter = get_cur_filters()[self.index]
    self.num_selected = 0
    self.selected_set = {}
    local plural = self.quantity == 1 and '' or 's'

    self:addviews{
        widgets.Label{
            frame={t=0, l=0, r=10},
            text={
                get_desc(filter),
                plural,
                NEWLINE,
                ('Select up to %d item%s ('):format(self.quantity, plural),
                {text=function() return self.num_selected end},
                ' selected)',
            },
        },
        widgets.Label{
            frame={r=0, w=9, t=0, h=3},
            text_pen=BUILD_TEXT_PEN,
            text_hpen=BUILD_TEXT_HPEN,
            text={
                '         ', NEWLINE,
                '  Build  ', NEWLINE,
                '         ',
            },
            on_click=self:callback('submit'),
        },
        widgets.FilteredList{
            view_id='flist',
            frame={t=3, l=0, r=0, b=4},
            case_sensitive=false,
            choices=self:get_choices(sort_by_recency),
            icon_width=2,
            on_submit=self:callback('toggle_group'),
        },
        widgets.CycleHotkeyLabel{
            frame={l=0, b=2},
            key='CUSTOM_CTRL_X',
            label='Sort by:',
            options={
                {label='Recently used', value=sort_by_recency},
                {label='Name', value=sort_by_name},
                {label='Amount', value=sort_by_quantity},
            },
            on_change=self:callback('on_sort'),
        },
        widgets.HotkeyLabel{
            frame={l=0, b=1},
            key='SELECT',
            label='Use all/none',
            auto_width=true,
            on_activate=function() self:toggle_group(self.subviews.flist.list:getSelected()) end,
        },
        widgets.HotkeyLabel{
            frame={l=22, b=1},
            key='CUSTOM_CTRL_D',
            label='Build',
            auto_width=true,
            on_activate=self:callback('submit'),
        },
        widgets.HotkeyLabel{
            frame={l=38, b=1},
            key='LEAVESCREEN',
            label='Go back',
            auto_width=true,
            on_activate=self:callback('on_cancel'),
        },
        widgets.HotkeyLabel{
            frame={l=0, b=0},
            key='KEYBOARD_CURSOR_RIGHT_FAST',
            key_sep='    : ',
            label='Use one',
            auto_width=true,
            on_activate=function() self:increment_group(self.subviews.flist.list:getSelected()) end,
        },
        widgets.Label{
            frame={l=6, b=0, w=5},
            text_pen=COLOR_LIGHTGREEN,
            text='Right',
        },
        widgets.HotkeyLabel{
            frame={l=23, b=0},
            key='KEYBOARD_CURSOR_LEFT_FAST',
            key_sep='   : ',
            label='Use one fewer',
            auto_width=true,
            on_activate=function() self:decrement_group(self.subviews.flist.list:getSelected()) end,
        },
        widgets.Label{
            frame={l=29, b=0, w=4},
            text_pen=COLOR_LIGHTGREEN,
            text='Left',
        },
    }
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
    local item_ids = getAvailableItems(uibs.building_type,
            uibs.building_subtype, uibs.custom_type, self.index - 1)
    local buckets = {}
    for _,item_id in ipairs(item_ids) do
        local item = df.item.find(item_id)
        if not item then goto continue end
        local desc = dfhack.items.getDescription(item, 0, true)
        if buckets[desc] then
            local bucket = buckets[desc]
            table.insert(bucket.data.item_ids, item_id)
            bucket.data.quantity = bucket.data.quantity + 1
        else
            local entry = {
                search_key=make_search_key(desc),
                icon=self:callback('get_entry_icon', item_id),
                data={
                    item_ids={item_id},
                    item_type=item:getType(),
                    item_subtype=item:getSubtype(),
                    quantity=1,
                    quality=item:getQuality(),
                    selected=0,
                },
            }
            buckets[desc] = entry
        end
        ::continue::
    end
    local choices = {}
    for desc,choice in pairs(buckets) do
        local data = choice.data
        choice.text = {
            {width=10, text=function() return ('[%d/%d]'):format(data.selected, data.quantity) end},
            {gap=2, text=desc},
        }
        table.insert(choices, choice)
    end
    table.sort(choices, sort_fn)
    return choices
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
    return self.selected_set[item_id] and get_selected_item_pen() or nil
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

function ItemSelection:submit()
    local selected_items = {}
    for item_id in pairs(self.selected_set) do
        table.insert(selected_items, item_id)
    end
    if #selected_items > 0 then
        track_recently_used(self.subviews.flist:getChoices())
    end
    self.on_submit(selected_items)
end

function ItemSelection:onInput(keys)
    if keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        self.on_cancel()
        return true
    elseif keys._MOUSE_L_DOWN then
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

ItemSelectionScreen = defclass(ItemSelectionScreen, BuildingplanScreen)
ItemSelectionScreen.ATTRS {
    focus_path='dwarfmode/Building/Placement/dfhack/lua/buildingplan/itemselection',
    force_pause=true,
    pass_pause=false,
    index=DEFAULT_NIL,
    quantity=DEFAULT_NIL,
    on_submit=DEFAULT_NIL,
    on_cancel=DEFAULT_NIL,
}

function ItemSelectionScreen:init()
    self:addviews{
        ItemSelection{
            index=self.index,
            quantity=self.quantity,
            on_submit=self.on_submit,
            on_cancel=self.on_cancel,
        }
    }
end

--------------------------------
-- FilterSelection
--

-- returns whether the items matched by the specified filter can have a quality
-- rating. This also conveniently indicates whether an item can be decorated.
local function can_be_improved(idx)
    local filter = get_cur_filters()[idx]
    if filter.flags2 and filter.flags2.building_material then
        return false;
    end
    return filter.item_type ~= df.item_type.WOOD and
            filter.item_type ~= df.item_type.BLOCKS and
            filter.item_type ~= df.item_type.BAR and
            filter.item_type ~= df.item_type.BOULDER
end

local OPTIONS_COL_WIDTH = 28
local TYPE_COL_WIDTH = 20
local HEADER_HEIGHT = 5
local FOOTER_HEIGHT = 4

FilterSelection = defclass(FilterSelection, widgets.Window)
FilterSelection.ATTRS{
    frame_title='Choose filters [MOCK -- NOT FUNCTIONAL]',
    frame={w=80, h=53, l=30, t=8},
    resizable=true,
    index=DEFAULT_NIL,
}

local STANDIN_PEN = to_pen{fg=COLOR_GREEN, bg=COLOR_GREEN, ch=' '}

function FilterSelection:init()
    self:addviews{
        widgets.Panel{
            view_id='options_panel',
            frame={l=0, t=0, b=FOOTER_HEIGHT, w=OPTIONS_COL_WIDTH},
            autoarrange_subviews=true,
            subviews={
                widgets.Panel{
                    view_id='quality_panel',
                    frame={l=0, r=0, h=24},
                    frame_inset={t=1},
                    frame_style=gui.INTERIOR_FRAME,
                    frame_title='Item quality',
                    subviews={
                        widgets.HotkeyLabel{
                            frame={l=0, t=0},
                            key='CUSTOM_SHIFT_Q',
                        },
                        widgets.HotkeyLabel{
                            frame={l=1, t=0},
                            key='CUSTOM_SHIFT_W',
                            label='Set max quality',
                        },
                        widgets.Panel{
                            view_id='quality_slider',
                            frame={l=0, t=2, w=3, h=15},
                            frame_background=STANDIN_PEN,
                        },
                        widgets.Label{
                            frame={l=3, t=3},
                            text='- Artifact (1)',
                        },
                        widgets.Label{
                            frame={l=3, t=5},
                            text='- Masterful (3)',
                        },
                        widgets.Label{
                            frame={l=3, t=7},
                            text='- Exceptional (34)',
                        },
                        widgets.Label{
                            frame={l=3, t=9},
                            text='- Superior (50)',
                        },
                        widgets.Label{
                            frame={l=3, t=11},
                            text='- FinelyCrafted (67)',
                        },
                        widgets.Label{
                            frame={l=3, t=13},
                            text='- WellCrafted (79)',
                        },
                        widgets.Label{
                            frame={l=3, t=15},
                            text='- Ordinary (206)',
                        },
                        widgets.HotkeyLabel{
                            frame={l=0, t=18},
                            key='CUSTOM_SHIFT_Z',
                        },
                        widgets.HotkeyLabel{
                            frame={l=1, t=18},
                            key='CUSTOM_SHIFT_X',
                            label='Set min quality',
                        },
                        widgets.CycleHotkeyLabel{
                            frame={l=0, t=20},
                            key='CUSTOM_SHIFT_D',
                            label='Decorated only:',
                            options={'No', 'Yes'},
                        },
                    },
                },
                widgets.ResizingPanel{
                    view_id='building_panel',
                    frame={l=0, r=0},
                    frame_inset={t=1},
                    frame_style=gui.INTERIOR_FRAME,
                    frame_title='Building options',
                    autoarrange_subviews=true,
                    autoarrange_gap=1,
                    subviews={
                        widgets.WrappedLabel{
                            frame={l=0},
                            text_to_wrap='These options will affect all items for the current building type.',
                        },
                        widgets.CycleHotkeyLabel{
                            frame={l=0},
                            key='CUSTOM_SHIFT_G',
                            label='Building safety:',
                            options={
                                {label='Any', value=0},
                                {label='Magma', value=2, pen=COLOR_RED},
                                {label='Fire', value=1, pen=COLOR_LIGHTRED},
                            },
                        },
                    },
                },
                widgets.Panel{
                    view_id='global_panel',
                    frame={l=0, r=0, b=0},
                    frame_inset={t=1},
                    frame_style=gui.INTERIOR_FRAME,
                    frame_title='Global options',
                    autoarrange_subviews=true,
                    subviews={
                        widgets.WrappedLabel{
                            frame={l=0},
                            text_to_wrap='These options will affect the selection of "Generic Materials" for future buildings.',
                        },
                        widgets.Panel{
                            frame={h=1},
                        },
                        widgets.ToggleHotkeyLabel{
                            frame={l=0},
                            key='CUSTOM_SHIFT_B',
                            label='Blocks',
                            label_width=8,
                        },
                        widgets.ToggleHotkeyLabel{
                            frame={l=0},
                            key='CUSTOM_SHIFT_L',
                            label='Logs',
                            label_width=8,
                        },
                        widgets.ToggleHotkeyLabel{
                            frame={l=0},
                            key='CUSTOM_SHIFT_O',
                            label='Boulders',
                            label_width=8,
                        },
                        widgets.ToggleHotkeyLabel{
                            frame={l=0},
                            key='CUSTOM_SHIFT_P',
                            label='Bars',
                            label_width=8,
                        },
                    },
                },
            },
        },
        widgets.Panel{
            view_id='materials_panel',
            frame={l=OPTIONS_COL_WIDTH, t=0, b=FOOTER_HEIGHT, r=0},
            subviews={
                widgets.Panel{
                    view_id='header',
                    frame={l=0, t=0, h=HEADER_HEIGHT, r=0},
                    subviews={
                        widgets.EditField{
                            frame={l=1, t=0},
                            label_text='Search: ',
                            on_char=function(ch) return ch:match('%l') end,
                        },
                        widgets.CycleHotkeyLabel{
                            frame={l=1, t=2, w=21},
                            label='Sort by:',
                            key='CUSTOM_SHIFT_R',
                            options={'name', 'available'},
                        },
                        widgets.ToggleHotkeyLabel{
                            frame={l=24, t=2, w=24},
                            label='Hide unavailable:',
                            key='CUSTOM_SHIFT_H',
                            initial_option=false,
                        },
                        widgets.Label{
                            frame={l=1, b=0},
                            text='Type',
                            text_pen=COLOR_LIGHTRED,
                        },
                        widgets.Label{
                            frame={l=TYPE_COL_WIDTH, b=0},
                            text='Material',
                            text_pen=COLOR_LIGHTRED,
                        },
                    },
                },
                widgets.Panel{
                    view_id='materials_lists',
                    frame={l=0, t=HEADER_HEIGHT, r=0, b=0},
                    frame_style=gui.INTERIOR_FRAME,
                    subviews={
                        widgets.List{
                            view_id='materials_categories',
                            frame={l=1, t=0, b=0, w=TYPE_COL_WIDTH-3},
                            scroll_keys={},
                            choices={
                                {text='Stone', key='CUSTOM_SHIFT_S'},
                                {text='Wood', key='CUSTOM_SHIFT_W'},
                                {text='Metal', key='CUSTOM_SHIFT_M'},
                                {text='Other', key='CUSTOM_SHIFT_O'},
                            },
                        },
                        widgets.List{
                            view_id='materials_mats',
                            frame={l=TYPE_COL_WIDTH, t=0, r=0, b=0},
                            choices={
                                {text='9    - granite'},
                                {text='0    - graphite'},
                            },
                        },
                    },
                },
                widgets.Panel{
                    view_id='divider',
                    frame={l=TYPE_COL_WIDTH-1, t=HEADER_HEIGHT, b=0, w=1},
                    on_render=self:callback('draw_divider'),
                }
            },
        },
        widgets.Panel{
            view_id='footer',
            frame={l=0, r=0, b=0, h=FOOTER_HEIGHT},
            frame_inset={l=20, t=1},
            subviews={
                widgets.HotkeyLabel{
                    frame={l=0, t=0},
                    label='Toggle',
                    auto_width=true,
                    key='SELECT',
                },
                widgets.HotkeyLabel{
                    frame={l=0, t=2},
                    label='Done',
                    auto_width=true,
                    key='LEAVESCREEN',
                },
                widgets.HotkeyLabel{
                    frame={l=30, t=0},
                    label='Select all',
                    auto_width=true,
                    key='CUSTOM_SHIFT_A',
                },
                widgets.HotkeyLabel{
                    frame={l=30, t=1},
                    label='Invert selection',
                    auto_width=true,
                    key='CUSTOM_SHIFT_I',
                },
                widgets.HotkeyLabel{
                    frame={l=30, t=2},
                    label='Clear selection',
                    auto_width=true,
                    key='CUSTOM_SHIFT_C',
                },
            },
        }
    }
end

local texpos = dfhack.textures.getThinBordersTexposStart()
local tp = function(offset)
    if texpos == -1 then return nil end
    return texpos + offset
end

local TOP_PEN = to_pen{tile=tp(10), ch=194, fg=COLOR_GREY, bg=COLOR_BLACK}
local MID_PEN = to_pen{tile=tp(4), ch=192, fg=COLOR_GREY, bg=COLOR_BLACK}
local BOT_PEN = to_pen{tile=tp(11), ch=179, fg=COLOR_GREY, bg=COLOR_BLACK}

function FilterSelection:draw_divider(dc)
    local y2 = dc.height - 1
    for y=0,y2 do
        dc:seek(0, y)
        if y == 0 then
            dc:char(nil, TOP_PEN)
        elseif y == y2 then
            dc:char(nil, BOT_PEN)
        else
            dc:char(nil, MID_PEN)
        end
    end
end

FilterSelectionScreen = defclass(FilterSelectionScreen, BuildingplanScreen)
FilterSelectionScreen.ATTRS {
    focus_path='dwarfmode/Building/Placement/dfhack/lua/buildingplan/filterselection',
    index=DEFAULT_NIL,
}

function FilterSelectionScreen:init()
    self:addviews{
        FilterSelection{index=self.index}
    }
end

function FilterSelectionScreen:onShow()
    df.global.game.main_interface.bottom_mode_selected = -1
end

function FilterSelectionScreen:onDismiss()
    df.global.game.main_interface.bottom_mode_selected = df.main_bottom_mode_type.BUILDING_PLACEMENT
end

--------------------------------
-- ItemLine
--

local function cur_building_has_no_area()
    if uibs.building_type == df.building_type.Construction then return false end
    local filters = dfhack.buildings.getFiltersByType({},
            uibs.building_type, uibs.building_subtype, uibs.custom_type)
    -- this works because all variable-size buildings have either no item
    -- filters or a quantity of -1 for their first (and only) item
    return filters and filters[1] and (not filters[1].quantity or filters[1].quantity > 0)
end

local function is_plannable()
    return get_cur_filters() and
            not (uibs.building_type == df.building_type.Construction
                 and uibs.building_subtype == df.construction_type.TrackNSEW)
end

local function is_construction()
    return uibs.building_type == df.building_type.Construction
end

local function is_stairs()
    return is_construction()
            and uibs.building_subtype == df.construction_type.UpDownStair
end

local direction_panel_frame = {t=4, h=13, w=46, r=28}

local direction_panel_types = utils.invert{
    df.building_type.Bridge,
    df.building_type.ScrewPump,
    df.building_type.WaterWheel,
    df.building_type.AxleHorizontal,
    df.building_type.Rollers,
}

local function has_direction_panel()
    return direction_panel_types[uibs.building_type]
        or (uibs.building_type == df.building_type.Trap
            and uibs.building_subtype == df.trap_type.TrackStop)
end

local pressure_plate_panel_frame = {t=4, h=37, w=46, r=28}

local function has_pressure_plate_panel()
    return uibs.building_type == df.building_type.Trap
            and uibs.building_subtype == df.trap_type.PressurePlate
end

local function is_over_options_panel()
    local frame = nil
    if has_direction_panel() then
        frame = direction_panel_frame
    elseif has_pressure_plate_panel() then
        frame = pressure_plate_panel_frame
    else
        return false
    end
    local v = widgets.Widget{frame=frame}
    local rect = gui.mkdims_wh(0, 0, dfhack.screen.getWindowSize())
    v:updateLayout(gui.ViewRect{rect=rect})
    return v:getMousePos()
end

local function to_title_case(str)
    str = str:gsub('(%a)([%w_]*)',
        function (first, rest) return first:upper()..rest:lower() end)
    str = str:gsub('_', ' ')
    return str
end

ItemLine = defclass(ItemLine, widgets.Panel)
ItemLine.ATTRS{
    idx=DEFAULT_NIL,
    is_selected_fn=DEFAULT_NIL,
    is_hollow_fn=DEFAULT_NIL,
    on_select=DEFAULT_NIL,
    on_filter=DEFAULT_NIL,
    on_clear_filter=DEFAULT_NIL,
}

function ItemLine:init()
    self.frame.h = 1
    self.visible = function() return #get_cur_filters() >= self.idx end
    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text='*',
            auto_width=true,
            visible=self.is_selected_fn,
        },
        widgets.Label{
            frame={t=0, l=25},
            text={
                {tile=get_button_start_pen},
                {gap=6, tile=get_button_end_pen},
            },
            auto_width=true,
            on_click=function() self.on_filter(self.idx) end,
        },
        widgets.Label{
            frame={t=0, l=33},
            text={
                {tile=get_button_start_pen},
                {gap=1, tile=get_button_end_pen},
            },
            auto_width=true,
            on_click=function() self.on_clear_filter(self.idx) end,
        },
        widgets.Label{
            frame={t=0, l=2},
            text={
                {width=21, text=self:callback('get_item_line_text')},
                {gap=3, text='filter', pen=COLOR_GREEN},
                {gap=2, text='x', pen=self:callback('get_x_pen')},
                {gap=3, text=function() return self.note end,
                 pen=function() return self.note_pen end},
            },
        },
    }
end

function ItemLine:reset()
    self.desc = nil
    self.available = nil
end

function ItemLine:onInput(keys)
    if keys._MOUSE_L_DOWN and self:getMousePos() then
        self.on_select(self.idx)
    end
    return ItemLine.super.onInput(self, keys)
end

function ItemLine:get_x_pen()
    return hasFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.idx - 1) and
            COLOR_GREEN or COLOR_GREY
end

function get_desc(filter)
    local desc = 'Unknown'
    if filter.has_tool_use and filter.has_tool_use > -1 then
        desc = to_title_case(df.tool_uses[filter.has_tool_use])
    elseif filter.flags2 and filter.flags2.screw then
        desc = 'Screw'
    elseif filter.item_type and filter.item_type > -1 then
        desc = to_title_case(df.item_type[filter.item_type])
    elseif filter.vector_id and filter.vector_id > -1 then
        desc = to_title_case(df.job_item_vector_id[filter.vector_id])
    elseif filter.flags2 and filter.flags2.building_material then
        desc = 'Building material';
        if filter.flags2.fire_safe then
            desc = 'Fire-safe material';
        end
        if filter.flags2.magma_safe then
            desc = 'Magma-safe material';
        end
    end

    if desc:endswith('s') then
        desc = desc:sub(1,-2)
    end
    if desc == 'Trappart' then
        desc = 'Mechanism'
    elseif desc == 'Wood' then
        desc = 'Log'
    end
    return desc
end

function ItemLine:get_item_line_text()
    local idx = self.idx
    local filter = get_cur_filters()[idx]
    local quantity = get_quantity(filter, self.is_hollow_fn())

    self.desc = self.desc or get_desc(filter)

    self.available = self.available or countAvailableItems(uibs.building_type,
            uibs.building_subtype, uibs.custom_type, idx - 1)
    if self.available >= quantity then
        self.note_pen = COLOR_GREEN
        self.note = 'Available now'
    else
        self.note_pen = COLOR_YELLOW
        self.note = 'Will link later'
    end

    return ('%d %s%s'):format(quantity, self.desc, quantity == 1 and '' or 's')
end

function ItemLine:reduce_quantity()
    if not self.available then return end
    local filter = get_cur_filters()[self.idx]
    self.available = math.max(0, self.available - get_quantity(filter, self.is_hollow_fn()))
end

local function get_placement_errors()
    local out = ''
    for _,str in ipairs(uibs.errors) do
        if #out > 0 then out = out .. NEWLINE end
        out = out .. str.value
    end
    return out
end

--------------------------------
-- PlannerOverlay
--

PlannerOverlay = defclass(PlannerOverlay, overlay.OverlayWidget)
PlannerOverlay.ATTRS{
    default_pos={x=5,y=9},
    default_enabled=true,
    viewscreens='dwarfmode/Building/Placement',
    frame={w=56, h=20},
}

function PlannerOverlay:init()
    self.selected = 1

    local main_panel = widgets.Panel{
        view_id='main',
        frame={t=0, l=0, r=0, h=14},
        frame_style=gui.MEDIUM_FRAME,
        frame_background=gui.CLEAR_PEN,
    }

    local function make_is_selected_fn(idx)
        return function() return self.selected == idx end
    end

    local function on_select_fn(idx)
        self.selected = idx
    end

    local function is_hollow_fn()
        return self.subviews.hollow:getOptionValue()
    end

    main_panel:addviews{
        widgets.Label{
            frame={},
            auto_width=true,
            text='No items required.',
            visible=function() return #get_cur_filters() == 0 end,
        },
        ItemLine{view_id='item1', frame={t=0, l=0, r=0}, idx=1,
                 is_selected_fn=make_is_selected_fn(1), is_hollow_fn=is_hollow_fn,
                 on_select=on_select_fn, on_filter=self:callback('set_filter'),
                 on_clear_filter=self:callback('clear_filter')},
        ItemLine{view_id='item2', frame={t=2, l=0, r=0}, idx=2,
                 is_selected_fn=make_is_selected_fn(2), is_hollow_fn=is_hollow_fn,
                 on_select=on_select_fn, on_filter=self:callback('set_filter'),
                 on_clear_filter=self:callback('clear_filter')},
        ItemLine{view_id='item3', frame={t=4, l=0, r=0}, idx=3,
                 is_selected_fn=make_is_selected_fn(3), is_hollow_fn=is_hollow_fn,
                 on_select=on_select_fn, on_filter=self:callback('set_filter'),
                 on_clear_filter=self:callback('clear_filter')},
        ItemLine{view_id='item4', frame={t=6, l=0, r=0}, idx=4,
                 is_selected_fn=make_is_selected_fn(4), is_hollow_fn=is_hollow_fn,
                 on_select=on_select_fn, on_filter=self:callback('set_filter'),
                 on_clear_filter=self:callback('clear_filter')},
        widgets.CycleHotkeyLabel{
            view_id='hollow',
            frame={t=3, l=4},
            key='CUSTOM_H',
            label='Hollow area:',
            visible=is_construction,
            options={
                {label='No', value=false},
                {label='Yes', value=true},
            },
        },
        widgets.CycleHotkeyLabel{
            view_id='stairs_top_subtype',
            frame={t=4, l=4},
            key='CUSTOM_R',
            label='Top Stair Type:    ',
            visible=is_stairs,
            options={
                {label='Auto', value='auto'},
                {label='UpDown', value=df.construction_type.UpDownStair},
                {label='Down', value=df.construction_type.DownStair},
            },
        },
        widgets.CycleHotkeyLabel {
            view_id='stairs_bottom_subtype',
            frame={t=5, l=4},
            key='CUSTOM_B',
            label='Bottom Stair Type: ',
            visible=is_stairs,
            options={
                {label='Auto', value='auto'},
                {label='UpDown', value=df.construction_type.UpDownStair},
                {label='Up', value=df.construction_type.UpStair},
            },
        },
        widgets.Label{
            frame={b=3, l=17},
            text={
                'Selected area: ',
                {text=function()
                     return ('%dx%dx%d'):format(get_cur_area_dims(self.saved_placement))
                 end
                },
            },
            visible=function()
                return not cur_building_has_no_area() and (self.saved_placement or is_choosing_area())
            end,
        },
        widgets.Panel{
            visible=function() return #get_cur_filters() > 0 end,
            subviews={
                widgets.HotkeyLabel{
                    frame={b=1, l=0},
                    key='STRING_A042',
                    auto_width=true,
                    enabled=function() return #get_cur_filters() > 1 end,
                    on_activate=function() self.selected = ((self.selected - 2) % #get_cur_filters()) + 1 end,
                },
                widgets.HotkeyLabel{
                    frame={b=1, l=1},
                    key='STRING_A047',
                    label='Prev/next item',
                    auto_width=true,
                    enabled=function() return #get_cur_filters() > 1 end,
                    on_activate=function() self.selected = (self.selected % #get_cur_filters()) + 1 end,
                },
                widgets.HotkeyLabel{
                    frame={b=1, l=21},
                    key='CUSTOM_F',
                    label='Set filter',
                    auto_width=true,
                    on_activate=function() self:set_filter(self.selected) end,
                },
                widgets.HotkeyLabel{
                    frame={b=1, l=37},
                    key='CUSTOM_X',
                    label='Clear filter',
                    auto_width=true,
                    on_activate=function() self:clear_filter(self.selected) end,
                    enabled=function()
                        return hasFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.selected - 1)
                    end
                },
                widgets.CycleHotkeyLabel{
                    view_id='choose',
                    frame={b=0, l=0, w=25},
                    key='CUSTOM_I',
                    label='Choose from items:',
                    options={{label='Yes', value=true},
                             {label='No', value=false}},
                    initial_option=false,
                    enabled=function()
                        for idx = 1,4 do
                            if (self.subviews['item'..idx].available or 0) > 0 then
                                return true
                            end
                        end
                    end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='safety',
                    frame={b=0, l=29, w=25},
                    key='CUSTOM_G',
                    label='Building safety:',
                    options={
                        {label='Any', value=0},
                        {label='Magma', value=2, pen=COLOR_RED},
                        {label='Fire', value=1, pen=COLOR_LIGHTRED},
                    },
                    initial_option=0,
                    on_change=function(heat)
                        setHeatSafetyFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, heat)
                    end,
                },
            },
        },
    }

    local error_panel = widgets.ResizingPanel{
        view_id='errors',
        frame={t=14, l=0, r=0},
        frame_style=gui.MEDIUM_FRAME,
        frame_background=gui.CLEAR_PEN,
    }

    error_panel:addviews{
        widgets.WrappedLabel{
            frame={t=0, l=0, r=0},
            text_pen=COLOR_LIGHTRED,
            text_to_wrap=get_placement_errors,
            visible=function() return #uibs.errors > 0 end,
        },
        widgets.Label{
            frame={t=0, l=0, r=0},
            text_pen=COLOR_GREEN,
            text='OK to build',
            visible=function() return #uibs.errors == 0 end,
        },
    }

    self:addviews{
        main_panel,
        error_panel,
    }
end

function PlannerOverlay:reset()
    self.subviews.item1:reset()
    self.subviews.item2:reset()
    self.subviews.item3:reset()
    self.subviews.item4:reset()
    reset_counts_flag = false
end

function PlannerOverlay:set_filter(idx)
    FilterSelectionScreen{index=idx}:show()
end

function PlannerOverlay:clear_filter(idx)
    setMaterialFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, idx - 1, "")
end

local function get_placement_data()
    local pos = uibs.pos
    local direction = uibs.direction
    local width, height, depth = get_cur_area_dims()
    local _, adjusted_width, adjusted_height = dfhack.buildings.getCorrectSize(
            width, height, uibs.building_type, uibs.building_subtype,
            uibs.custom_type, direction)
    -- get the upper-left corner of the building/area at min z-level
    local has_selection = is_choosing_area()
    local start_pos = xyz2pos(
        has_selection and math.min(uibs.selection_pos.x, pos.x) or pos.x - adjusted_width//2,
        has_selection and math.min(uibs.selection_pos.y, pos.y) or pos.y - adjusted_height//2,
        has_selection and math.min(uibs.selection_pos.z, pos.z) or pos.z
    )
    if uibs.building_type == df.building_type.ScrewPump then
        if direction == df.screw_pump_direction.FromSouth then
            start_pos.y = start_pos.y + 1
        elseif direction == df.screw_pump_direction.FromEast then
            start_pos.x = start_pos.x + 1
        end
    end
    local min_x, max_x = start_pos.x, start_pos.x
    local min_y, max_y = start_pos.y, start_pos.y
    local min_z, max_z = start_pos.z, start_pos.z
    if adjusted_width == 1 and adjusted_height == 1
            and (width > 1 or height > 1 or depth > 1) then
        max_x = min_x + width - 1
        max_y = min_y + height - 1
        max_z = math.max(uibs.selection_pos.z, pos.z)
    end
    return {
        p1=xyz2pos(min_x, min_y, min_z),
        p2=xyz2pos(max_x, max_y, max_z),
        width=adjusted_width,
        height=adjusted_height
    }
end

function PlannerOverlay:save_placement()
    self.saved_placement = get_placement_data()
    if (uibs.selection_pos:isValid()) then
        self.saved_selection_pos_valid = true
        self.saved_selection_pos = copyall(uibs.selection_pos)
        self.saved_pos = copyall(uibs.pos)
        uibs.selection_pos:clear()
    else
        self.saved_selection_pos = copyall(self.saved_placement.p1)
        self.saved_pos = copyall(self.saved_placement.p2)
        self.saved_pos.x = self.saved_pos.x + self.saved_placement.width - 1
        self.saved_pos.y = self.saved_pos.y + self.saved_placement.height - 1
    end
end

function PlannerOverlay:restore_placement()
    if self.saved_selection_pos_valid then
        uibs.selection_pos = self.saved_selection_pos
        self.saved_selection_pos_valid = nil
    else
        uibs.selection_pos:clear()
    end
    self.saved_selection_pos = nil
    self.saved_pos = nil
    local placement_data = self.saved_placement
    self.saved_placement = nil
    return placement_data
end

function PlannerOverlay:onInput(keys)
    if not is_plannable() then return false end
    if keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        if uibs.selection_pos:isValid() then
            uibs.selection_pos:clear()
            return true
        end
        self.selected = 1
        self.subviews.hollow:setOption(false)
        self.subviews.choose:setOption(false)
        self:reset()
        reset_counts_flag = true
        return false
    end
    if PlannerOverlay.super.onInput(self, keys) then
        return true
    end
    if keys._MOUSE_L_DOWN then
        if is_over_options_panel() then return false end
        local detect_rect = copyall(self.frame_rect)
        detect_rect.height = self.subviews.main.frame_rect.height +
                self.subviews.errors.frame_rect.height
        detect_rect.y2 = detect_rect.y1 + detect_rect.height - 1
        if self.subviews.main:getMousePos(gui.ViewRect{rect=detect_rect})
                or self.subviews.errors:getMousePos() then
            return true
        end
        if #uibs.errors > 0 then return true end
        if dfhack.gui.getMousePos() then
            if is_choosing_area() or cur_building_has_no_area() then
                local filters = get_cur_filters()
                local num_filters = #filters
                if num_filters == 0 then
                    return false -- we don't add value; let the game place it
                end
                local choose = self.subviews.choose
                if choose.enabled() and choose:getOptionValue() then
                    self:save_placement()
                    local is_hollow = self.subviews.hollow:getOptionValue()
                    local chosen_items, active_screens = {}, {}
                    local pending = num_filters
                    df.global.game.main_interface.bottom_mode_selected = -1
                    for idx = num_filters,1,-1 do
                        chosen_items[idx] = {}
                        if (self.subviews['item'..idx].available or 0) > 0 then
                            active_screens[idx] = ItemSelectionScreen{
                                index=idx,
                                quantity=get_quantity(filters[idx], is_hollow,
                                        self.saved_placement),
                                on_submit=function(items)
                                    chosen_items[idx] = items
                                    active_screens[idx]:dismiss()
                                    active_screens[idx] = nil
                                    pending = pending - 1
                                    if pending == 0 then
                                        df.global.game.main_interface.bottom_mode_selected = df.main_bottom_mode_type.BUILDING_PLACEMENT
                                        self:place_building(self:restore_placement(), chosen_items)
                                    end
                                end,
                                on_cancel=function()
                                    for i,scr in pairs(active_screens) do
                                        scr:dismiss()
                                    end
                                    df.global.game.main_interface.bottom_mode_selected = df.main_bottom_mode_type.BUILDING_PLACEMENT
                                    self:restore_placement()
                                end,
                            }:show()
                        else
                            pending = pending - 1
                        end
                    end
                else
                    self:place_building(get_placement_data())
                end
                return true
            elseif not is_choosing_area() then
                return false
            end
       end
   end
   return keys._MOUSE_L
end

function PlannerOverlay:render(dc)
    if not is_plannable() then return end
    self.subviews.errors:updateLayout()
    PlannerOverlay.super.render(self, dc)
end

local GOOD_PEN, BAD_PEN
function reload_cursors()
    GOOD_PEN = to_pen{ch='o', fg=COLOR_GREEN, tile=dfhack.screen.findGraphicsTile('CURSORS', 1, 2)}
    BAD_PEN = to_pen{ch='X', fg=COLOR_RED, tile=dfhack.screen.findGraphicsTile('CURSORS', 3, 0)}
end
reload_cursors()

function PlannerOverlay:onRenderFrame(dc, rect)
    PlannerOverlay.super.onRenderFrame(self, dc, rect)

    if reset_counts_flag then
        self:reset()
        self.subviews.safety:setOption(getHeatSafetyFilter(
                uibs.building_type, uibs.building_subtype, uibs.custom_type))
    end

    local selection_pos = self.saved_selection_pos or uibs.selection_pos
    if not selection_pos or selection_pos.x < 0 then return end

    local pos = self.saved_pos or uibs.pos
    local bounds = {
        x1 = math.min(selection_pos.x, pos.x),
        x2 = math.max(selection_pos.x, pos.x),
        y1 = math.min(selection_pos.y, pos.y),
        y2 = math.max(selection_pos.y, pos.y),
    }

    local hollow = self.subviews.hollow:getOptionValue()
    local pen = (self.saved_selection_pos or #uibs.errors == 0) and GOOD_PEN or BAD_PEN

    local function get_overlay_pen(pos)
        if not hollow then return pen end
        if pos.x == bounds.x1 or pos.x == bounds.x2 or
                pos.y == bounds.y1 or pos.y == bounds.y2 then
            return pen
        end
        return gui.TRANSPARENT_PEN
    end

    guidm.renderMapOverlay(get_overlay_pen, bounds)
end

function PlannerOverlay:get_stairs_subtype(pos, corner1, corner2)
    local subtype = uibs.building_subtype
    if pos.z == corner1.z then
        local opt = self.subviews.stairs_bottom_subtype:getOptionValue()
        if opt == 'auto' then
            local tt = dfhack.maps.getTileType(pos)
            local shape = df.tiletype.attrs[tt].shape
            if shape ~= df.tiletype_shape.STAIR_DOWN then
                subtype = df.construction_type.UpStair
            end
        else
            subtype = opt
        end
    elseif pos.z == corner2.z then
        local opt = self.subviews.stairs_top_subtype:getOptionValue()
        if opt == 'auto' then
            local tt = dfhack.maps.getTileType(pos)
            local shape = df.tiletype.attrs[tt].shape
            if shape ~= df.tiletype_shape.STAIR_UP then
                subtype = df.construction_type.DownStair
            end
        else
            subtype = opt
        end
    end
    return subtype
end

function PlannerOverlay:place_building(placement_data, chosen_items)
    local p1, p2 = placement_data.p1, placement_data.p2
    local blds = {}
    local hollow = self.subviews.hollow:getOptionValue()
    local subtype = uibs.building_subtype
    for z=p1.z,p2.z do for y=p1.y,p2.y do for x=p1.x,p2.x do
        if hollow and x ~= p1.x and x ~= p2.x and y ~= p1.y and y ~= p2.y then
            goto continue
        end
        local pos = xyz2pos(x, y, z)
        if is_stairs() then
            subtype = self:get_stairs_subtype(pos, p1, p2)
        end
        local bld, err = dfhack.buildings.constructBuilding{pos=pos,
            type=uibs.building_type, subtype=subtype, custom=uibs.custom_type,
            width=placement_data.width, height=placement_data.height,
            direction=uibs.direction}
        if err then
            for _,b in ipairs(blds) do
                dfhack.buildings.deconstruct(b)
            end
            dfhack.printerr(err .. (' (%d, %d, %d)'):format(pos.x, pos.y, pos.z))
            return
        end
        -- assign fields for the types that need them. we can't pass them all in
        -- to the call to constructBuilding since attempting to assign unrelated
        -- fields to building types that don't support them causes errors.
        for k,v in pairs(bld) do
            if k == 'friction' then bld.friction = uibs.friction end
            if k == 'use_dump' then bld.use_dump = uibs.use_dump end
            if k == 'dump_x_shift' then bld.dump_x_shift = uibs.dump_x_shift end
            if k == 'dump_y_shift' then bld.dump_y_shift = uibs.dump_y_shift end
            if k == 'speed' then bld.speed = uibs.speed end
        end
        table.insert(blds, bld)
        ::continue::
    end end end
    self.subviews.item1:reduce_quantity()
    self.subviews.item2:reduce_quantity()
    self.subviews.item3:reduce_quantity()
    self.subviews.item4:reduce_quantity()
    for _,bld in ipairs(blds) do
        -- attach chosen items and reduce job_item quantity
        if chosen_items then
            local job = bld.jobs[0]
            local jitems = job.job_items
            for idx=1,#get_cur_filters() do
                local item_ids = chosen_items[idx]
                while jitems[idx-1].quantity > 0 and #item_ids > 0 do
                    local item_id = item_ids[#item_ids]
                    local item = df.item.find(item_id)
                    if not item then
                        dfhack.printerr(('item no longer available: %d'):format(item_id))
                        break
                    end
                    if not dfhack.job.attachJobItem(job, item, df.job_item_ref.T_role.Hauled, idx-1, -1) then
                        dfhack.printerr(('cannot attach item: %d'):format(item_id))
                        break
                    end
                    jitems[idx-1].quantity = jitems[idx-1].quantity - 1
                    item_ids[#item_ids] = nil
                end
            end
        end
        addPlannedBuilding(bld)
    end
    scheduleCycle()
    uibs.selection_pos:clear()
end

--------------------------------
-- InspectorLine
--

local function get_building_filters()
    local bld = dfhack.gui.getSelectedBuilding()
    return dfhack.buildings.getFiltersByType({},
            bld:getType(), bld:getSubtype(), bld:getCustomType())
end

InspectorLine = defclass(InspectorLine, widgets.Panel)
InspectorLine.ATTRS{
    idx=DEFAULT_NIL,
}

function InspectorLine:init()
    self.frame.h = 2
    self.visible = function() return #get_building_filters() >= self.idx end
    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text={{text=self:callback('get_desc_string')}},
        },
        widgets.Label{
            frame={t=1, l=2},
            text={{text=self:callback('get_status_line')}},
        },
    }
end

function InspectorLine:get_desc_string()
    if self.desc then return self.desc end
    self.desc = getDescString(dfhack.gui.getSelectedBuilding(), self.idx-1)
    return self.desc
end

function InspectorLine:get_status_line()
    if self.status then return self.status end
    local queue_pos = getQueuePosition(dfhack.gui.getSelectedBuilding(), self.idx-1)
    if queue_pos <= 0 then
        return 'Item attached'
    end
    self.status = ('Position in line: %d'):format(queue_pos)
    return self.status
end

function InspectorLine:reset()
    self.desc = nil
    self.status = nil
end

--------------------------------
-- InspectorOverlay
--

InspectorOverlay = defclass(InspectorOverlay, overlay.OverlayWidget)
InspectorOverlay.ATTRS{
    default_pos={x=-41,y=14},
    default_enabled=true,
    viewscreens='dwarfmode/ViewSheets/BUILDING',
    frame={w=30, h=15},
    frame_style=gui.MEDIUM_FRAME,
    frame_background=gui.CLEAR_PEN,
}

function InspectorOverlay:init()
    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text='Waiting for items:',
        },
        InspectorLine{view_id='item1', frame={t=2, l=0}, idx=1},
        InspectorLine{view_id='item2', frame={t=4, l=0}, idx=2},
        InspectorLine{view_id='item3', frame={t=6, l=0}, idx=3},
        InspectorLine{view_id='item4', frame={t=8, l=0}, idx=4},
        widgets.HotkeyLabel{
            frame={t=11, l=0},
            label='adjust filters',
            key='CUSTOM_CTRL_F',
        },
        widgets.HotkeyLabel{
            frame={t=12, l=0},
            label='make top priority',
            key='CUSTOM_CTRL_T',
            on_activate=self:callback('make_top_priority'),
        },
    }
end

function InspectorOverlay:reset()
    self.subviews.item1:reset()
    self.subviews.item2:reset()
    self.subviews.item3:reset()
    self.subviews.item4:reset()
    reset_inspector_flag = false
end

function InspectorOverlay:make_top_priority()
    makeTopPriority(dfhack.gui.getSelectedBuilding())
    self:reset()
end

function InspectorOverlay:onInput(keys)
    if not isPlannedBuilding(dfhack.gui.getSelectedBuilding()) then
        return false
    end
    if keys._MOUSE_L_DOWN or keys._MOUSE_R_DOWN or keys.LEAVESCREEN then
        self:reset()
    end
    return InspectorOverlay.super.onInput(self, keys)
end

function InspectorOverlay:render(dc)
    if not isPlannedBuilding(dfhack.gui.getSelectedBuilding()) then
        return
    end
    if reset_inspector_flag then
        self:reset()
    end
    InspectorOverlay.super.render(self, dc)
end

OVERLAY_WIDGETS = {
    planner=PlannerOverlay,
    inspector=InspectorOverlay,
}

return _ENV
