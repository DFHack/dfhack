local _ENV = mkmodule('plugins.buildingplan.planneroverlay')

local itemselection = require('plugins.buildingplan.itemselection')
local filterselection = require('plugins.buildingplan.filterselection')
local gui = require('gui')
local guidm = require('gui.dwarfmode')
local json = require('json')
local overlay = require('plugins.overlay')
local pens = require('plugins.buildingplan.pens')
local utils = require('utils')
local widgets = require('gui.widgets')
require('dfhack.buildings')

config = config or json.open('dfhack-config/buildingplan.json')

local uibs = df.global.buildreq

reset_counts_flag = false
editing_filters_flag = false

local function get_cur_filters()
    return dfhack.buildings.getFiltersByType({}, uibs.building_type,
            uibs.building_subtype, uibs.custom_type)
end

local function is_choosing_area()
    return uibs.selection_pos.x >= 0
end

-- TODO: reuse data in quickfort database
local function get_selection_size_limits()
    local btype = uibs.building_type
    if btype == df.building_type.Bridge
            or btype == df.building_type.FarmPlot
            or btype == df.building_type.RoadPaved
            or btype == df.building_type.RoadDirt then
        return {w=31, h=31}
    elseif btype == df.building_type.AxleHorizontal then
        return uibs.direction == 1 and {w=1, h=31} or {w=31, h=1}
    elseif btype == df.building_type.Rollers then
        return (uibs.direction == 1 or uibs.direction == 3) and {w=31, h=1} or {w=1, h=31}
    end
end

local function get_selected_bounds(selection_pos, pos)
    selection_pos = selection_pos or uibs.selection_pos
    if not is_choosing_area() then return end

    pos = pos or uibs.pos

    local bounds = {
        x1=math.min(selection_pos.x, pos.x),
        x2=math.max(selection_pos.x, pos.x),
        y1=math.min(selection_pos.y, pos.y),
        y2=math.max(selection_pos.y, pos.y),
        z1=math.min(selection_pos.z, pos.z),
        z2=math.max(selection_pos.z, pos.z),
    }

    -- clamp to map edges
    bounds = {
        x1=math.max(0, bounds.x1),
        x2=math.min(df.global.world.map.x_count-1, bounds.x2),
        y1=math.max(0, bounds.y1),
        y2=math.min(df.global.world.map.y_count-1, bounds.y2),
        z1=math.max(0, bounds.z1),
        z2=math.min(df.global.world.map.z_count-1, bounds.z2),
    }

    local limits = get_selection_size_limits()
    if limits then
        -- clamp to building type area limit
        bounds = {
            x1=math.max(selection_pos.x - (limits.w-1), bounds.x1),
            x2=math.min(selection_pos.x + (limits.w-1), bounds.x2),
            y1=math.max(selection_pos.y - (limits.h-1), bounds.y1),
            y2=math.min(selection_pos.y + (limits.h-1), bounds.y2),
            z1=bounds.z1,
            z2=bounds.z2,
        }
    end

    return bounds
end

local function get_cur_area_dims(bounds)
    if not bounds and not is_choosing_area() then return 1, 1, 1 end
    bounds = bounds or get_selected_bounds()
    if not bounds then return 1, 1, 1 end
    return bounds.x2 - bounds.x1 + 1,
            bounds.y2 - bounds.y1 + 1,
            bounds.z2 - bounds.z1 + 1
end

local function get_selected_volume(bounds)
    local w, h, depth = get_cur_area_dims(bounds)
    return w * h * depth
end

local function is_pressure_plate()
    return uibs.building_type == df.building_type.Trap
            and uibs.building_subtype == df.trap_type.PressurePlate
end

local function is_weapon_trap()
    return uibs.building_type == df.building_type.Trap
            and uibs.building_subtype == df.trap_type.WeaponTrap
end

local function is_spike_trap()
    return uibs.building_type == df.building_type.Weapon
end

local function is_weapon_or_spike_trap()
    return is_weapon_trap() or is_spike_trap()
end

local function is_construction()
    return uibs.building_type == df.building_type.Construction
end

local function tile_is_construction(pos)
    local tt = dfhack.maps.getTileType(pos)
    if not tt then return false end
    if df.tiletype.attrs[tt].material ~= df.tiletype_material.CONSTRUCTION then
        return false
    end
    local construction = df.construction.find(pos)
    return construction and not construction.flags.top_of_wall
end

local ONE_BY_ONE = xy2pos(1, 1)

local function can_reconstruct(bounds)
    return get_selected_volume(bounds) == 1 or require('plugins.buildingplan').getGlobalSettings().reconstruct
end

local function can_place_construction(reconstruct, pos)
    return dfhack.buildings.checkFreeTiles(pos, ONE_BY_ONE) and (reconstruct or not tile_is_construction(pos))
end

local function is_interior(bounds, x, y)
    return x ~= bounds.x1 and x ~= bounds.x2 and
        y ~= bounds.y1 and y ~= bounds.y2
end

-- adjusted from CycleHotkeyLabel on the planner panel
local weapon_quantity = 1

local function get_quantity(filter, hollow, bounds)
    if is_pressure_plate() then
        local flags = uibs.plate_info.flags
        return (flags.units and 1 or 0) + (flags.water and 1 or 0) +
                    (flags.magma and 1 or 0) + (flags.track and 1 or 0)
    elseif (is_weapon_trap() and filter.vector_id == df.job_item_vector_id.ANY_WEAPON) or is_spike_trap() then
        return weapon_quantity
    end
    local quantity = filter.quantity or 1
    bounds = bounds or get_selected_bounds()
    local dimx, dimy, dimz = get_cur_area_dims(bounds)
    if quantity < 1 then
        return (((dimx * dimy) // 4) + 1) * dimz
    end
    if bounds and is_construction() then
        local reconstruct = can_reconstruct(bounds)
        local count = 0
        for z = bounds.z1, bounds.z2 do
            for y = bounds.y1, bounds.y2 do
                for x = bounds.x1, bounds.x2 do
                    if hollow and is_interior(bounds, x, y) then goto continue end
                    if can_place_construction(reconstruct, xyz2pos(x, y, z)) then
                        count = count + 1
                    end
                    ::continue::
                end
            end
        end
        return quantity * count
    end
    return quantity * get_selected_volume(bounds)
end

local function cur_building_has_no_area()
    if uibs.building_type == df.building_type.Construction then return false end
    local filters = dfhack.buildings.getFiltersByType({},
            uibs.building_type, uibs.building_subtype, uibs.custom_type)
    -- this works because all variable-size buildings have either no item
    -- filters or a quantity of -1 for their first (and only) item
    return filters and filters[1] and (not filters[1].quantity or filters[1].quantity > 0)
end

local function is_tutorial_open()
    local help = df.global.game.main_interface.help
    return help.open and
            help.context == df.help_context_type.START_TUTORIAL_WORKSHOPS_AND_TASKS
end

local function is_plannable()
    return not is_tutorial_open() and
            get_cur_filters() and
            not (is_construction() and
                uibs.building_subtype == df.construction_type.TrackNSEW)
end

local function is_slab()
    return uibs.building_type == df.building_type.Slab
end

local function is_cage()
    return uibs.building_type == df.building_type.Cage
end

local function is_stairs()
    return is_construction()
            and uibs.building_subtype == df.construction_type.UpDownStair
end

local function is_single_level_stairs()
    if not is_stairs() then return false end
    local _, _, dimz = get_cur_area_dims()
    return dimz == 1
end

local function is_multi_level_stairs()
    if not is_stairs() then return false end
    local _, _, dimz = get_cur_area_dims()
    return dimz > 1
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
    return is_pressure_plate()
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

local function compress(str, len)
    if #str <= len then
        return str
    else
        local no_vowels = str:gsub('[aeiou]','')
        if #no_vowels <= len then
            return no_vowels
        else
            return no_vowels:sub(1,len-3)..'...'
        end
    end
end

local function filter_string(mats, cats, length)
    local enabled_mat_names = {}
    local enabled_cat_names = {}
    for name, props in pairs(mats) do
        local enabled = props.enabled == 'true' and cats[props.category]
        if enabled then table.insert(enabled_mat_names, name) end
    end
    if #enabled_mat_names == 1 then
        return '['..compress(enabled_mat_names[1], length)..']'
    elseif #enabled_mat_names > 1 then
        for cat, _ in pairs(cats) do
            if cat ~= 'unset' and cats[cat] then
                table.insert(enabled_cat_names, cat)
            end
        end
        if #enabled_cat_names == 1 then
            return '[' .. enabled_cat_names[1]:gsub("^%l", string.upper) .. ']'
        else
            return '['..#enabled_cat_names..' mat. categories]'
        end
    else
        -- can result from selecting wood and then toggling "fire safe" etc.
        return '[impossible filter]'
    end
end
--------------------------------
-- ItemLine
--

-- number of characters for item filter summary (excluding surrounding [ ])
local item_filter_chars = 17

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
    self.frame.h = 2
    self.visible = function() return #get_cur_filters() >= self.idx end
    self:addviews{
        widgets.Label{
            view_id='item_symbol',
            frame={t=0, l=0},
            text=string.char(16), -- this is the "►" character
            text_pen=COLOR_YELLOW,
            auto_width=true,
            visible=self.is_selected_fn,
        },
        widgets.Label{
            view_id='item_desc',
            frame={t=0, l=2},
            text={
                {text=self:callback('get_item_line_text'),
                 pen=function() return gui.invert_color(COLOR_WHITE, self.is_selected_fn()) end},
            },
        },
        widgets.Label{
            view_id='item_filter',
            frame={t=0, l=28},
            text={
                {text=self:callback('get_filter_text'),
                 width=item_filter_chars+2,
                 rjustify=true,
                 pen=function() return
                         self:is_impossible() and COLOR_RED or
                         gui.invert_color(COLOR_LIGHTCYAN, self.is_selected_fn()) end},
            },
            auto_width=true,
            on_click=function() self.on_filter(self.idx) end,
        },
        widgets.Label{
            frame={t=0, l=47},
            text='[clear]',
            text_pen=COLOR_LIGHTRED,
            auto_width=true,
            visible=self:callback('has_filter'),
            on_click=function() self.on_clear_filter(self.idx) end,
        },
        widgets.Label{
            frame={t=1, l=2},
            text={
                {gap=2, text=function() return self.note end,
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
    if keys._MOUSE_L and self:getMousePos() then
        self.on_select(self.idx)
    end
    return ItemLine.super.onInput(self, keys)
end

function ItemLine:get_item_line_text()
    local idx = self.idx
    local filter = get_cur_filters()[idx]
    local quantity = get_quantity(filter, self.is_hollow_fn())

    local buildingplan = require('plugins.buildingplan')
    self.desc = self.desc or buildingplan.get_desc(filter)

    self.available = self.available or buildingplan.countAvailableItems(
        uibs.building_type, uibs.building_subtype, uibs.custom_type, idx - 1)
    if self.available >= quantity then
        self.note_pen = COLOR_GREEN
        self.note = (' %d available now'):format(self.available)
    elseif self.available >= 0 then
        self.note_pen = COLOR_BROWN
        self.note = (' Will link next (need to make %d)'):format(quantity - self.available)
    else
        self.note_pen = COLOR_BROWN
        self.note = (' Will link later (need to make %d)'):format(-self.available + quantity)
    end
    self.note = string.char(192) .. self.note -- character 192 is "└"

    return ('%d %s%s'):format(quantity, self.desc, quantity == 1 and '' or 's')
end

function ItemLine:has_filter()
    return require('plugins.buildingplan').hasFilter(
        uibs.building_type, uibs.building_subtype, uibs.custom_type, self.idx-1)
end

function ItemLine:get_filter_text()
    local buildingplan = require('plugins.buildingplan')
    if not buildingplan.hasFilter(
            uibs.building_type, uibs.building_subtype, uibs.custom_type, self.idx - 1)
    then
        return '[any material]'
    end
    local mats = buildingplan.getMaterialFilter(
        uibs.building_type, uibs.building_subtype, uibs.custom_type, self.idx - 1)
    local cats = buildingplan.getMaterialMaskFilter(
        uibs.building_type, uibs.building_subtype, uibs.custom_type, self.idx - 1)
    return filter_string(mats, cats, item_filter_chars)
end

-- short circuit version of the '[impossible filter]' case above
function ItemLine:is_impossible()
    local buildingplan = require('plugins.buildingplan')
    local mats = buildingplan.getMaterialFilter(
        uibs.building_type, uibs.building_subtype, uibs.custom_type, self.idx-1)
    local cats = buildingplan.getMaterialMaskFilter(
        uibs.building_type, uibs.building_subtype, uibs.custom_type, self.idx - 1)
     for _,props in pairs(mats) do
        local enabled = props.enabled == 'true' and cats[props.category]
        if enabled then return false end
    end
    return true
end

function ItemLine:reduce_quantity(used_quantity)
    if not self.available then return end
    local filter = get_cur_filters()[self.idx]
    used_quantity = used_quantity or get_quantity(filter, self.is_hollow_fn())
    self.available = self.available - used_quantity
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
-- QuickFilter
--

-- Used to store a table of the following format:
-- table<integer, { label: string, mats: string[] }>
-- integer: quick filter slot
-- label: string representation of the filter
-- mats: list of material names allowed by the filter
BUILDINGPLAN_FILTERS_KEY = "buildingplan/quick-filters"

QuickFilter = defclass(QuickFilter, widgets.Panel)
QuickFilter.ATTRS{
    idx=DEFAULT_NIL,
    on_click_fn=DEFAULT_NIL,
    is_selected_fn=DEFAULT_NIL
}

function QuickFilter:init()
    local label = ('%d.'):format(self.idx)
    self.frame.w = 27
    self.renaming = false

    self:addviews {
        widgets.Label {
            frame = { t = 0, l = 0 },
            text = string.char(16), -- this is the "►" character
            text_pen = COLOR_YELLOW,
            auto_width = true,
            visible = self.is_selected_fn,
        },
        widgets.Label { frame = { t = 0, l = 2 }, text = label },
        widgets.Label {
            frame = { t = 0, l = 5, w = item_filter_chars + 2 },
            text = { { text = self:callback('get_label_text'), pen = function() return COLOR_CYAN end } },
            visible = function() return self.renaming == false end,
            on_click = self:callback("on_click"),
        },
        widgets.EditField {
            view_id = 'edit_field',
            frame = { t = 0, l = 5, w = item_filter_chars + 2 },
            text = "",
            visible = function() return self.renaming == true end,
            on_submit = function(text) self:submit_name(text) end,
        },
        widgets.Label {
            frame = { t = 0, r = 0, w = 3 },
            text = "[x]",
            text_pen = COLOR_LIGHTRED,
            visible = self:callback("slot_used"),
            on_click = self:callback("clear")
        }
    }
end

function QuickFilter:onInput(keys)
    if keys.LEAVESCREEN or keys._MOUSE_R then
        if self.renaming then
            self.subviews.edit_field:setFocus(false)
            self.renaming = false
            editing_filters_flag = false
            return true
        else
            return false
        end
    end
    return QuickFilter.super.onInput(self, keys)
end

function QuickFilter:on_click()
    if dfhack.internal.getModifiers().shift and
        self:slot_used() and not editing_filters_flag
    then
        self.subviews.edit_field:setText(self:get_label_text())
        self.renaming = true
        editing_filters_flag = true
        self.subviews.edit_field:setFocus(true)
    else
        self.on_click_fn(self.idx) -- save/apply filter based on selected ItemLine
    end
end

function QuickFilter:slot_used()
    local quick_filters = dfhack.persistent.getSiteData(BUILDINGPLAN_FILTERS_KEY, {})
    return quick_filters[self.idx] ~= nil
end

function QuickFilter:clear()
    local quick_filters = dfhack.persistent.getSiteData(BUILDINGPLAN_FILTERS_KEY, {})
    quick_filters[self.idx] = nil
    dfhack.persistent.saveSiteData(BUILDINGPLAN_FILTERS_KEY, quick_filters)
end

function QuickFilter:get_label_text()
    local quick_filters = dfhack.persistent.getSiteData(BUILDINGPLAN_FILTERS_KEY, {})
    local set = quick_filters[self.idx]
    if not set then
        return "empty"
    else
        return set.label
    end
end

function QuickFilter:submit_name(text)
    local quick_filters = dfhack.persistent.getSiteData(BUILDINGPLAN_FILTERS_KEY, {})
    quick_filters[self.idx].label = compress(text, item_filter_chars+2)
    dfhack.persistent.saveSiteData(BUILDINGPLAN_FILTERS_KEY, quick_filters)
    self.renaming = false
    editing_filters_flag = false
end
--------------------------------
-- PlannerOverlay
--

PlannerOverlay = defclass(PlannerOverlay, overlay.OverlayWidget)
PlannerOverlay.ATTRS{
    desc='Shows the building planner interface panel when building buildings.',
    default_pos={x=5,y=9},
    default_enabled=true,
    viewscreens='dwarfmode/Building/Placement',
    frame={w=56, h=32},
}

function PlannerOverlay:init()
    self.selected = 1
    self.state = ensure_key(config.data, 'planner')

    self.selected_favorite = 1

    local main_panel = widgets.Panel{
        view_id='main',
        frame={t=1, l=0, r=0, h=14},
        frame_style=gui.FRAME_INTERIOR_MEDIUM,
        frame_background=gui.CLEAR_PEN,
        visible=self:callback('is_not_minimized'),
    }

    local minimized_panel = widgets.Panel{
        frame={t=0, r=1, w=20, h=1},
        subviews={
            widgets.Label{
                frame={t=0, r=3, h=1},
                text={
                    {text=' show Planner ', pen=pens.MINI_TEXT_PEN, hpen=pens.MINI_TEXT_HPEN},
                    {text='['..string.char(31)..']', pen=pens.MINI_BUTT_PEN, hpen=pens.MINI_BUTT_HPEN},
                },
                visible=self:callback('is_minimized'),
                on_click=self:callback('toggle_minimized'),
            },
            widgets.Label{
                frame={t=0, r=3, h=1},
                text={
                    {text=' hide Planner ', pen=pens.MINI_TEXT_PEN, hpen=pens.MINI_TEXT_HPEN},
                    {text='['..string.char(30)..']', pen=pens.MINI_BUTT_PEN, hpen=pens.MINI_BUTT_HPEN},
                },
                visible=self:callback('is_not_minimized'),
                on_click=self:callback('toggle_minimized'),
            },
            widgets.HelpButton{
                frame={t=0, r=0},
                command='buildingplan',
            }
        },
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

    local buildingplan = require('plugins.buildingplan')

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
            frame={b=4, l=1, w=21},
            key='CUSTOM_H',
            label='Hollow area:',
            visible=is_construction,
            options={
                {label='No', value=false},
                {label='Yes', value=true, pen=COLOR_GREEN},
            },
        },
        widgets.CycleHotkeyLabel{
            view_id='stairs_top_subtype',
            frame={b=7, l=1, w=30},
            key='CUSTOM_R',
            label='Top stair type:   ',
            visible=is_multi_level_stairs,
            options={
                {label='Auto', value='auto'},
                {label='UpDown', value=df.construction_type.UpDownStair},
                {label='Down', value=df.construction_type.DownStair},
            },
        },
        widgets.CycleHotkeyLabel {
            view_id='stairs_bottom_subtype',
            frame={b=6, l=1, w=30},
            key='CUSTOM_B',
            label='Bottom Stair Type:',
            visible=is_multi_level_stairs,
            options={
                {label='Auto', value='auto'},
                {label='UpDown', value=df.construction_type.UpDownStair},
                {label='Up', value=df.construction_type.UpStair},
            },
        },
        widgets.CycleHotkeyLabel{
            view_id='stairs_only_subtype',
            frame={b=7, l=1, w=30},
            key='CUSTOM_R',
            label='Single level stair:',
            visible=is_single_level_stairs,
            options={
                {label='Up', value=df.construction_type.UpStair},
                {label='UpDown', value=df.construction_type.UpDownStair},
                {label='Down', value=df.construction_type.DownStair},
            },
        },
        widgets.CycleHotkeyLabel {  -- TODO: this thing also needs a slider
            view_id='weapons',
            frame={b=4, l=1, w=28},
            key='CUSTOM_T',
            key_back='CUSTOM_SHIFT_T',
            label='Number of weapons:',
            visible=is_weapon_or_spike_trap,
            options=utils.tabulate(function(i) return {label='('..i..')', value=i, pen=COLOR_YELLOW} end, 1, 10),
            on_change=function(val) weapon_quantity = val end,
        },
        widgets.ToggleHotkeyLabel {
            view_id='engraved',
            frame={b=4, l=1, w=22},
            key='CUSTOM_T',
            label='Engraved only:',
            visible=is_slab,
            on_change=function(val)
                buildingplan.setSpecial(uibs.building_type, uibs.building_subtype, uibs.custom_type, 'engraved', val)
            end,
        },
        widgets.ToggleHotkeyLabel {
            view_id='empty',
            frame={b=4, l=1, w=22},
            key='CUSTOM_T',
            label='Empty only:',
            visible=is_cage,
            on_change=function(val)
                buildingplan.setSpecial(uibs.building_type, uibs.building_subtype, uibs.custom_type, 'empty', val)
            end,
        },
        widgets.Label{
            frame={b=4, l=23},
            text_pen=COLOR_DARKGREY,
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
                    frame={b=2, l=1, w=22},
                    key='CUSTOM_F',
                    label=function()
                        return buildingplan.hasFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.selected - 1)
                        and 'Edit filter' or 'Set filter'
                     end,
                    on_activate=function() self:set_filter(self.selected) end,
                },
                widgets.HotkeyLabel{
                    frame={b=1, l=1, w=22},
                    key='CUSTOM_CTRL_D',
                    label='Delete filter',
                    on_activate=function() self:clear_filter(self.selected) end,
                    enabled=function()
                        return buildingplan.hasFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.selected - 1)
                    end
                },
                widgets.CycleHotkeyLabel{
                    view_id='show_favorites',
                    frame={b=0, l=1, w=22},
                    key='CUSTOM_CTRL_F',
                    label="",
                    option_gap=0,
                    options={
                        { label='Show favorites', value = false },
                        { label='Hide favorites', value = true },
                    },
                    initial_option=false,
                    on_change=function(new,_) self:show_hide_favorites(new) end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='choose',
                    frame={b=0, l=24},
                    key='CUSTOM_Z',
                    label='Choose items:',
                    label_below=true,
                    options={
                        {label='With filters', value=0},
                        {
                            label=function()
                                local automaterial = itemselection.get_automaterial_selection(uibs.building_type)
                                return ('Last used (%s)'):format(automaterial or 'pick manually')
                            end,
                            value=2,
                        },
                        {label='Manually', value=1},
                    },
                    initial_option=0,
                    on_change=function(choose)
                        buildingplan.setChooseItems(uibs.building_type, uibs.building_subtype, uibs.custom_type, choose)
                    end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='safety',
                    frame={b=2, l=24, w=25},
                    key='CUSTOM_G',
                    label='Building safety:',
                    options={
                        {label='Any', value=0},
                        {label='Magma', value=2, pen=COLOR_RED},
                        {label='Fire', value=1, pen=COLOR_LIGHTRED},
                    },
                    initial_option=0,
                    on_change=function(heat)
                        buildingplan.setHeatSafetyFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, heat)
                    end,
                },
            },
        },
    }

    local divider_widget = widgets.Divider{
        frame={t=10, l=0, r=0, h=1},
        frame_style=gui.FRAME_INTERIOR_MEDIUM,
        visible=self:callback('is_not_minimized'),
    }

    local error_panel = widgets.ResizingPanel{
        view_id='errors',
        frame={t=15, l=0, r=0},
        frame_style=gui.BOLD_FRAME,
        frame_background=gui.CLEAR_PEN,
        visible=self:callback('is_not_minimized'),
    }

    error_panel:addviews{
        widgets.WrappedLabel{
            frame={t=0, l=1, r=0},
            text_pen=COLOR_LIGHTRED,
            text_to_wrap=get_placement_errors,
            visible=function() return #uibs.errors > 0 end,
        },
        widgets.Label{
            frame={t=0, l=1, r=0},
            text_pen=COLOR_GREEN,
            text='OK to build',
            visible=function() return #uibs.errors == 0 end,
        },
    }

    local prev_next_selector = widgets.Panel{
        frame={h=1},
        auto_width=true,
        subviews={
                widgets.HotkeyLabel{
                    frame={t=0, l=1, w=9},
                    key='CUSTOM_SHIFT_Q',
                    key_sep='\0',
                    label=': Prev/',
                    on_activate=function() self.selected = ((self.selected - 2) % #get_cur_filters()) + 1 end,
                },
                widgets.HotkeyLabel{
                    frame={t=0, l=2, w=1},
                    key='CUSTOM_Q',
                    on_activate=function() self.selected = (self.selected % #get_cur_filters()) + 1 end,
                },
                widgets.Label{
                    frame={t=0,l=10},
                    text='next item',
                    on_click=function() self.selected = (self.selected % #get_cur_filters()) + 1 end,
                },
            },
        visible=function() return #get_cur_filters() > 1 end,
    }

    local black_bar = widgets.Panel{
        frame={t=0, l=1, w=37, h=1},
        frame_inset=0,
        frame_background=gui.CLEAR_PEN,
        visible=self:callback('is_not_minimized'),
        subviews={
            prev_next_selector,
        },
    }

    local function make_is_selected_filter(idx)
        return function () return self.selected_favorite == idx end
    end

    local favorites_panel = widgets.Panel{
        view_id='favorites',
        frame={t=15, l=0, r=0, h=9},
        frame_style=gui.FRAME_INTERIOR_MEDIUM,
        frame_background=gui.CLEAR_PEN,
        visible=self:callback('show_favorites'),
        subviews={
            QuickFilter{idx=1, frame={t=0,l=0},
                        on_click_fn=self:callback("save_restore_filter"),
                        is_selected_fn=make_is_selected_filter(1) },
            QuickFilter{idx=2, frame={t=1,l=0},
                        on_click_fn=self:callback("save_restore_filter"),
                        is_selected_fn=make_is_selected_filter(2) },
            QuickFilter{idx=3, frame={t=2,l=0},
                        on_click_fn=self:callback("save_restore_filter"),
                        is_selected_fn=make_is_selected_filter(3) },
            QuickFilter{idx=4, frame={t=3,l=0},
                        on_click_fn=self:callback("save_restore_filter"),
                        is_selected_fn=make_is_selected_filter(4) },
            QuickFilter{idx=5, frame={t=4,l=0},
                        on_click_fn=self:callback("save_restore_filter"),
                        is_selected_fn=make_is_selected_filter(5) },
            QuickFilter{idx=6, frame={t=0,l=27},
                        on_click_fn=self:callback("save_restore_filter"),
                        is_selected_fn=make_is_selected_filter(6) },
            QuickFilter{idx=7, frame={t=1,l=27},
                        on_click_fn=self:callback("save_restore_filter"),
                        is_selected_fn=make_is_selected_filter(7) },
            QuickFilter{idx=8, frame={t=2,l=27},
                        on_click_fn=self:callback("save_restore_filter"),
                        is_selected_fn=make_is_selected_filter(8) },
            QuickFilter{idx=9, frame={t=3,l=27},
                        on_click_fn=self:callback("save_restore_filter"),
                        is_selected_fn=make_is_selected_filter(9) },
            QuickFilter{idx=0, frame={t=4,l=27},
                        on_click_fn=self:callback("save_restore_filter"),
                        is_selected_fn=make_is_selected_filter(0) },
            widgets.CycleHotkeyLabel {
                view_id='slot_select',
                frame={b=0, l=2},
                key='CUSTOM_X',
                key_back='CUSTOM_SHIFT_X',
                label='next/previous slot',
                auto_width=true,
                options=utils.tabulate(function(i) return {label="", value=i} end, 0, 9),
                initial_option=1,
                on_change=function(val) print(val) self.selected_favorite = val end,
            },
            widgets.HotkeyLabel{
                frame={b=0, l=28},
                label="set/apply selected",
                key='CUSTOM_Y',
                on_activate=function () self:save_restore_filter(self.selected_favorite) end,
            },

        }
    }

    self:addviews{
        black_bar,
        minimized_panel,
        main_panel,
        divider_widget,
        error_panel,
        favorites_panel
    }
end

function PlannerOverlay:show_favorites()
    return not self.state.minimized and self.subviews.show_favorites:getOptionValue()
end

function PlannerOverlay:show_hide_favorites(new)
    local errors_frame = {t=15+(new and 9 or 0), l=0, r=0}
    self.subviews.errors.frame = errors_frame
    self:updateLayout()
end

function PlannerOverlay:save_restore_filter(slot)
    self.selected_favorite = slot
    local buildingplan = require('plugins.buildingplan')
    local quick_filters = dfhack.persistent.getSiteData(BUILDINGPLAN_FILTERS_KEY, {})
    if quick_filters[slot] then -- restore saved filter
        buildingplan.setMaterialFilter(
            uibs.building_type, uibs.building_subtype, uibs.custom_type, self.selected - 1,
            quick_filters[slot].mats
        )
    else -- save current filter

        if not buildingplan.hasFilter(
                uibs.building_type, uibs.building_subtype, uibs.custom_type, self.selected - 1)
        then return end

        local mats = buildingplan.getMaterialFilter(
            uibs.building_type, uibs.building_subtype, uibs.custom_type, self.selected - 1)
        local cats = buildingplan.getMaterialMaskFilter(
            uibs.building_type, uibs.building_subtype, uibs.custom_type, self.selected - 1)
        local label = filter_string(mats, cats, item_filter_chars)
        local enabled_mats = {}
        for mat, props in pairs(mats) do
            if props.enabled == "true" and cats[props.category] then
                table.insert(enabled_mats, mat)
            end
        end
        if #enabled_mats > 0 then
            quick_filters[slot] = { label = label, mats = enabled_mats }
            dfhack.persistent.saveSiteData(BUILDINGPLAN_FILTERS_KEY, quick_filters)
        end
    end
end


function PlannerOverlay:is_minimized()
    return self.state.minimized
end

function PlannerOverlay:is_not_minimized()
    return not self.state.minimized
end

function PlannerOverlay:toggle_minimized()
    self.state.minimized = not self.state.minimized
    config:write()
    self:reset()
end

function PlannerOverlay:reset()
    self.subviews.item1:reset()
    self.subviews.item2:reset()
    self.subviews.item3:reset()
    self.subviews.item4:reset()
    reset_counts_flag = false
end

function PlannerOverlay:set_filter(idx)
    filterselection.FilterSelectionScreen{index=idx, desc=require('plugins.buildingplan').get_desc(get_cur_filters()[idx])}:show()
end

function PlannerOverlay:clear_filter(idx)
    desc=require('plugins.buildingplan').clearFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, idx-1)
end

local function get_placement_data()
    local direction = uibs.direction
    local bounds = get_selected_bounds()
    local width, height, depth = get_cur_area_dims(bounds)
    local _, adjusted_width, adjusted_height = dfhack.buildings.getCorrectSize(
            width, height, uibs.building_type, uibs.building_subtype,
            uibs.custom_type, direction)
    -- get the upper-left corner of the building/area at min z-level
    local start_pos = bounds and xyz2pos(bounds.x1, bounds.y1, bounds.z1) or
            xyz2pos(
                uibs.pos.x - adjusted_width//2,
                uibs.pos.y - adjusted_height//2,
                uibs.pos.z)
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
        max_z = math.max(uibs.selection_pos.z, uibs.pos.z)
    end
    return {
        x1=min_x, y1=min_y, z1=min_z,
        x2=max_x, y2=max_y, z2=max_z,
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
        local sp = self.saved_placement
        self.saved_selection_pos = xyz2pos(sp.x1, sp.y1, sp.z1)
        self.saved_pos = xyz2pos(sp.x2, sp.y2, sp.z2)
        self.saved_pos.x = self.saved_pos.x + sp.width - 1
        self.saved_pos.y = self.saved_pos.y + sp.height - 1
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
    if PlannerOverlay.super.onInput(self, keys) then
        return true
    end
    if keys.LEAVESCREEN or keys._MOUSE_R then
        if uibs.selection_pos:isValid() then
            uibs.selection_pos:clear()
            return true
        end
        self.selected = 1
        self.subviews.hollow:setOption(false)
        self:reset()
        reset_counts_flag = true
        return false
    end
    if keys.CUSTOM_ALT_M then
        self:toggle_minimized()
        return true
    end
    if self:is_minimized() then return false end
    if keys._MOUSE_L then
        if is_over_options_panel() then return false end
        local detect_rect = copyall(self.frame_rect)
        detect_rect.height = self.subviews.main.frame_rect.height +
                self.subviews.errors.frame_rect.height
        detect_rect.y2 = detect_rect.y1 + detect_rect.height - 1
        if self.subviews.main:getMousePos(gui.ViewRect{rect=detect_rect})
                or self.subviews.errors:getMousePos() then
            return true
        end
        if not is_construction() and #uibs.errors > 0 then return true end
        if dfhack.gui.getMousePos() then
            if is_choosing_area() or cur_building_has_no_area() then
                local filters = get_cur_filters()
                local num_filters = #filters
                local choose = self.subviews.choose:getOptionValue()
                if choose == 0 then
                    self:place_building(get_placement_data())
                else
                    local bounds = get_selected_bounds()
                    self:save_placement()
                    local autoselect = choose == 2
                    local is_hollow = self.subviews.hollow:getOptionValue()
                    local chosen_items, active_screens = {}, {}
                    local pending = num_filters
                    df.global.game.main_interface.bottom_mode_selected = -1
                    for idx = num_filters,1,-1 do
                        chosen_items[idx] = {}
                        local filter = filters[idx]
                        local get_available_items_fn = function()
                            return require('plugins.buildingplan').getAvailableItems(
                                uibs.building_type, uibs.building_subtype, uibs.custom_type, idx-1)
                        end
                        local selection_screen = itemselection.ItemSelectionScreen{
                            get_available_items_fn=get_available_items_fn,
                            desc=require('plugins.buildingplan').get_desc(filter),
                            quantity=get_quantity(filter, is_hollow, bounds),
                            autoselect=autoselect,
                            on_submit=function(items)
                                chosen_items[idx] = items
                                if active_screens[idx] then
                                    active_screens[idx]:dismiss()
                                    active_screens[idx] = nil
                                else
                                    active_screens[idx] = true
                                end
                                pending = pending - 1
                                if pending == 0 then
                                    df.global.game.main_interface.bottom_mode_selected = df.main_bottom_mode_type.BUILDING_PLACEMENT
                                    self:place_building(self:restore_placement(), chosen_items)
                                end
                            end,
                            on_cancel=function()
                                for _,scr in pairs(active_screens) do
                                    scr:dismiss()
                                end
                                df.global.game.main_interface.bottom_mode_selected = df.main_bottom_mode_type.BUILDING_PLACEMENT
                                self:restore_placement()
                            end,
                        }
                        if active_screens[idx] then
                            -- we've already returned via autoselect
                            active_screens[idx] = nil
                        else
                            active_screens[idx] = selection_screen:show()
                        end
                    end
                end
                return true
            elseif not is_choosing_area() then
                return false
            end
       end
   end
   return keys._MOUSE_L or keys.SELECT
end

function PlannerOverlay:render(dc)
    if not is_plannable() then return end
    self.subviews.errors:updateLayout()
    PlannerOverlay.super.render(self, dc)
end

function PlannerOverlay:onRenderFrame(dc, rect)
    PlannerOverlay.super.onRenderFrame(self, dc, rect)

    if reset_counts_flag then
        self:reset()
        local buildingplan = require('plugins.buildingplan')
        self.subviews.engraved:setOption(buildingplan.getSpecials(
            uibs.building_type, uibs.building_subtype, uibs.custom_type).engraved or false)
        self.subviews.empty:setOption(buildingplan.getSpecials(
            uibs.building_type, uibs.building_subtype, uibs.custom_type).empty or false)
        self.subviews.choose:setOption(buildingplan.getChooseItems(
            uibs.building_type, uibs.building_subtype, uibs.custom_type))
        self.subviews.safety:setOption(buildingplan.getHeatSafetyFilter(
            uibs.building_type, uibs.building_subtype, uibs.custom_type))
    end

    if self:is_minimized() then return end

    local bounds = get_selected_bounds(self.saved_selection_pos, self.saved_pos)
    if not bounds then return end

    local hollow = self.subviews.hollow:getOptionValue()
    local default_pen = (self.saved_selection_pos or #uibs.errors == 0) and pens.GOOD_TILE_PEN or pens.BAD_TILE_PEN

    -- always allow reconstruction if it's a 1x1x1 selection (meaning the player selected that spot specifically)
    local reconstruct = can_reconstruct(bounds)

    local get_pen_fn = is_construction() and
        function(pos)
            return can_place_construction(reconstruct, pos) and pens.GOOD_TILE_PEN or pens.BAD_TILE_PEN
        end or function()
            return default_pen
        end

    local function get_overlay_pen(pos)
        if not hollow then return get_pen_fn(pos) end
        if pos.x == bounds.x1 or pos.x == bounds.x2 or
                pos.y == bounds.y1 or pos.y == bounds.y2 then
            return get_pen_fn(pos)
        end
        return gui.TRANSPARENT_PEN
    end

    guidm.renderMapOverlay(get_overlay_pen, bounds)
end

function PlannerOverlay:get_stairs_subtype(pos, bounds)
    local subtype = uibs.building_subtype
    if pos.z == bounds.z1 then
        local opt = bounds.z1 == bounds.z2 and self.subviews.stairs_only_subtype:getOptionValue() or
            self.subviews.stairs_bottom_subtype:getOptionValue()
        if opt == 'auto' then
            local tt = dfhack.maps.getTileType(pos)
            local shape = df.tiletype.attrs[tt].shape
            if shape ~= df.tiletype_shape.STAIR_DOWN and shape ~= df.tiletype_shape.STAIR_UPDOWN then
                subtype = df.construction_type.UpStair
            end
        else
            subtype = opt
        end
    elseif pos.z == bounds.z2 then
        local opt = self.subviews.stairs_top_subtype:getOptionValue()
        if opt == 'auto' then
            local tt = dfhack.maps.getTileType(pos)
            local shape = df.tiletype.attrs[tt].shape
            if shape ~= df.tiletype_shape.STAIR_UP and shape ~= df.tiletype_shape.STAIR_UPDOWN then
                subtype = df.construction_type.DownStair
            end
        else
            subtype = opt
        end
    end
    return subtype
end

function PlannerOverlay:place_building(placement_data, chosen_items)
    local pd = placement_data
    local blds = {}
    local hollow = self.subviews.hollow:getOptionValue()
    local subtype = uibs.building_subtype
    local filters = get_cur_filters()
    if is_pressure_plate() or is_spike_trap() then
        filters[1].quantity = get_quantity(filters[1])
    elseif is_weapon_trap() then
        filters[2].quantity = get_quantity(filters[2])
    end
    local reconstruct = can_reconstruct(pd)
    for z=pd.z1,pd.z2 do for y=pd.y1,pd.y2 do for x=pd.x1,pd.x2 do
        if hollow and is_interior(pd, x, y) then
            goto continue
        end
        local pos = xyz2pos(x, y, z)
        if is_construction() and not can_place_construction(reconstruct, pos) then
            goto continue
        end
        if is_stairs() then
            subtype = self:get_stairs_subtype(pos, pd)
        end
        local bld, err = dfhack.buildings.constructBuilding{pos=pos,
            type=uibs.building_type, subtype=subtype, custom=uibs.custom_type,
            width=pd.width, height=pd.height,
            direction=uibs.direction, filters=filters}
        if err then
            -- it's ok if some buildings fail to build
            goto continue
        end
        -- assign fields for the types that need them. we can't pass them all in
        -- to the call to constructBuilding since attempting to assign unrelated
        -- fields to building types that don't support them causes errors.
        for k in pairs(bld) do
            if k == 'friction' then bld.friction = uibs.friction end
            if k == 'use_dump' then bld.use_dump = uibs.use_dump end
            if k == 'dump_x_shift' then bld.dump_x_shift = uibs.dump_x_shift end
            if k == 'dump_y_shift' then bld.dump_y_shift = uibs.dump_y_shift end
            if k == 'speed' then bld.speed = uibs.speed end
            if k == 'plate_info' then utils.assign(bld.plate_info, uibs.plate_info) end
        end
        table.insert(blds, bld)
        ::continue::
    end end end
    local used_quantity = is_construction() and #blds or false
    self.subviews.item1:reduce_quantity(used_quantity)
    self.subviews.item2:reduce_quantity(used_quantity)
    self.subviews.item3:reduce_quantity(used_quantity)
    self.subviews.item4:reduce_quantity(used_quantity)
    local buildingplan = require('plugins.buildingplan')
    for _,bld in ipairs(blds) do
        -- attach chosen items and reduce job_item quantity
        if chosen_items then
            local job = bld.jobs[0]
            local jitems = job.job_items
            local num_filters = #get_cur_filters()
            for idx=1,num_filters do
                local item_ids = chosen_items[idx]
                local jitem = jitems[num_filters-idx]
                while jitem.quantity > 0 and #item_ids > 0 do
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
                    jitem.quantity = jitem.quantity - 1
                    item_ids[#item_ids] = nil
                end
            end
        end
        buildingplan.addPlannedBuilding(bld)
    end
    buildingplan.scheduleCycle()
    uibs.selection_pos:clear()
end


return _ENV
