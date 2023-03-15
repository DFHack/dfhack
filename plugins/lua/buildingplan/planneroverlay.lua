local _ENV = mkmodule('plugins.buildingplan.planneroverlay')

local itemselection = require('plugins.buildingplan.itemselection')
local filterselection = require('plugins.buildingplan.filterselection')
local gui = require('gui')
local guidm = require('gui.dwarfmode')
local overlay = require('plugins.overlay')
local pens = require('plugins.buildingplan.pens')
local utils = require('utils')
local widgets = require('gui.widgets')
require('dfhack.buildings')

local uibs = df.global.buildreq

reset_counts_flag = false

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

-- adjusted from CycleHotkeyLabel on the planner panel
local weapon_quantity = 1

local function get_quantity(filter, hollow, placement_data)
    if is_pressure_plate() then
        local flags = uibs.plate_info.flags
        return (flags.units and 1 or 0) + (flags.water and 1 or 0) +
                    (flags.magma and 1 or 0) + (flags.track and 1 or 0)
    elseif (is_weapon_trap() and filter.vector_id == df.job_item_vector_id.ANY_WEAPON) or is_spike_trap() then
        return weapon_quantity
    end
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

local function cur_building_has_no_area()
    if uibs.building_type == df.building_type.Construction then return false end
    local filters = dfhack.buildings.getFiltersByType({},
            uibs.building_type, uibs.building_subtype, uibs.custom_type)
    -- this works because all variable-size buildings have either no item
    -- filters or a quantity of -1 for their first (and only) item
    return filters and filters[1] and (not filters[1].quantity or filters[1].quantity > 0)
end

local function is_construction()
    return uibs.building_type == df.building_type.Construction
end

local function is_plannable()
    return get_cur_filters() and
            not (is_construction() and
                uibs.building_subtype == df.construction_type.TrackNSEW)
end

local function is_slab()
    return uibs.building_type == df.building_type.Slab
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

--------------------------------
-- ItemLine
--

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
                {tile=pens.BUTTON_START_PEN},
                {gap=6, tile=pens.BUTTON_END_PEN},
            },
            auto_width=true,
            on_click=function() self.on_filter(self.idx) end,
        },
        widgets.Label{
            frame={t=0, l=33},
            text={
                {tile=pens.BUTTON_START_PEN},
                {gap=1, tile=pens.BUTTON_END_PEN},
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
    return require('plugins.buildingplan').hasFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.idx-1) and
            COLOR_GREEN or COLOR_GREY
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
        self.note = 'Available now'
    else
        self.note_pen = COLOR_YELLOW
        self.note = 'Will link later'
    end

    return ('%d %s%s'):format(quantity, self.desc, quantity == 1 and '' or 's')
end

function ItemLine:reduce_quantity(used_quantity)
    if not self.available then return end
    local filter = get_cur_filters()[self.idx]
    used_quantity = used_quantity or get_quantity(filter, self.is_hollow_fn())
    self.available = math.max(0, self.available - used_quantity)
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
    self.minimized = false

    local function is_minimized() return self.minimized end
    local function not_is_minimized() return not self.minimized end

    local main_panel = widgets.Panel{
        view_id='main',
        frame={t=0, l=0, r=0, h=14},
        frame_style=gui.MEDIUM_FRAME,
        frame_background=gui.CLEAR_PEN,
        visible=function() return not self.minimized end,
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
            label='Top Stair Type:   ',
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
            label='Bottom Stair Type:',
            visible=is_stairs,
            options={
                {label='Auto', value='auto'},
                {label='UpDown', value=df.construction_type.UpDownStair},
                {label='Up', value=df.construction_type.UpStair},
            },
        },
        widgets.CycleHotkeyLabel {
            view_id='weapons',
            frame={t=5, l=4},
            key='CUSTOM_T',
            key_back='CUSTOM_SHIFT_T',
            label='Num weapons:',
            visible=is_weapon_or_spike_trap,
            options={1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
            on_change=function(val) weapon_quantity = val end,
        },
        widgets.ToggleHotkeyLabel {
            view_id='engraved',
            frame={t=5, l=4},
            key='CUSTOM_T',
            label='Engraved only:',
            visible=is_slab,
            on_change=function(val)
                buildingplan.setSpecial(uibs.building_type, uibs.building_subtype, uibs.custom_type, 'engraved', val)
            end,
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
                        return buildingplan.hasFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.selected - 1)
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
                    on_change=function(choose)
                        buildingplan.setChooseItems(uibs.building_type, uibs.building_subtype, uibs.custom_type, choose)
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
                        buildingplan.setHeatSafetyFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, heat)
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
        visible=function() return not self.minimized end,
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
    filterselection.FilterSelectionScreen{index=idx, desc=require('plugins.buildingplan').get_desc(get_cur_filters()[idx])}:show()
end

function PlannerOverlay:clear_filter(idx)
    desc=require('plugins.buildingplan').clearFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, idx-1)
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
        self.minimized = false
        self.subviews.hollow:setOption(false)
        self:reset()
        reset_counts_flag = true
        return false
    end
    if keys.CUSTOM_ALT_M then
        self.minimized = not self.minimized
        return true
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
        local x, y = self.subviews.main:getMousePos(gui.ViewRect{rect=detect_rect})
        if x or self.subviews.errors:getMousePos() then
            if x and x == detect_rect.width-2 and y == 0 then
                self.minimized = not self.minimized
                return true
            end
            return not self.minimized
        end
        if self.minimized then return false end
        if not is_construction() and #uibs.errors > 0 then return true end
        if dfhack.gui.getMousePos() then
            if is_choosing_area() or cur_building_has_no_area() then
                local filters = get_cur_filters()
                local num_filters = #filters
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
                            local filter = filters[idx]
                            active_screens[idx] = itemselection.ItemSelectionScreen{
                                index=idx,
                                desc=require('plugins.buildingplan').get_desc(filter),
                                quantity=get_quantity(filter, is_hollow,
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
   return keys._MOUSE_L or keys.SELECT
end

function PlannerOverlay:render(dc)
    if not is_plannable() then return end
    self.subviews.errors:updateLayout()
    PlannerOverlay.super.render(self, dc)
    -- render "minimize" button
    dc:seek(self.frame_rect.x2-1, self.frame_rect.y1)
    dc:char(string.char(self.minimized and 31 or 30), COLOR_RED)
end

local ONE_BY_ONE = xy2pos(1, 1)

function PlannerOverlay:onRenderFrame(dc, rect)
    PlannerOverlay.super.onRenderFrame(self, dc, rect)

    if reset_counts_flag then
        self:reset()
        local buildingplan = require('plugins.buildingplan')
        self.subviews.engraved:setOption(buildingplan.getSpecials(
            uibs.building_type, uibs.building_subtype, uibs.custom_type).engraved or false)
        self.subviews.choose:setOption(buildingplan.getChooseItems(
            uibs.building_type, uibs.building_subtype, uibs.custom_type))
        self.subviews.safety:setOption(buildingplan.getHeatSafetyFilter(
            uibs.building_type, uibs.building_subtype, uibs.custom_type))
    end

    local selection_pos = self.saved_selection_pos or uibs.selection_pos
    if not selection_pos or selection_pos.x < 0 then return end

    local pos = self.saved_pos or uibs.pos
    local bounds = {
        x1 = math.max(0, math.min(selection_pos.x, pos.x)),
        x2 = math.min(df.global.world.map.x_count-1, math.max(selection_pos.x, pos.x)),
        y1 = math.max(0, math.min(selection_pos.y, pos.y)),
        y2 = math.min(df.global.world.map.y_count-1, math.max(selection_pos.y, pos.y)),
    }

    local hollow = self.subviews.hollow:getOptionValue()
    local default_pen = (self.saved_selection_pos or #uibs.errors == 0) and pens.GOOD_TILE_PEN or pens.BAD_TILE_PEN

    local get_pen_fn = is_construction() and function(pos)
        return dfhack.buildings.checkFreeTiles(pos, ONE_BY_ONE) and pens.GOOD_TILE_PEN or pens.BAD_TILE_PEN
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
    local filters = get_cur_filters()
    if is_pressure_plate() or is_spike_trap() then
        filters[1].quantity = get_quantity(filters[1])
    elseif is_weapon_trap() then
        filters[2].quantity = get_quantity(filters[2])
    end
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
            direction=uibs.direction, filters=filters}
        if err then
            -- it's ok if some buildings fail to build
            goto continue
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
