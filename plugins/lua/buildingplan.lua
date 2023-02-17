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

local BUTTON_START_PEN, BUTTON_END_PEN = nil, nil
local reset_counts_flag = false
local reset_inspector_flag = false
function signal_reset()
    BUTTON_START_PEN = nil
    BUTTON_END_PEN = nil
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

--------------------------------
-- PlannerOverlay
--

local uibs = df.global.buildreq

local function cur_building_has_no_area()
    if uibs.building_type == df.building_type.Construction then return false end
    local filters = dfhack.buildings.getFiltersByType({},
            uibs.building_type, uibs.building_subtype, uibs.custom_type)
    -- this works because all variable-size buildings have either no item
    -- filters or a quantity of -1 for their first (and only) item
    return filters and filters[1] and (not filters[1].quantity or filters[1].quantity > 0)
end

local function is_choosing_area()
    return uibs.selection_pos.x >= 0
end

local function get_cur_area_dims()
    if not is_choosing_area() then return 1, 1, 1 end
    return math.abs(uibs.selection_pos.x - uibs.pos.x) + 1,
            math.abs(uibs.selection_pos.y - uibs.pos.y) + 1,
            math.abs(uibs.selection_pos.z - uibs.pos.z) + 1
end

local function get_cur_filters()
    return dfhack.buildings.getFiltersByType({}, uibs.building_type,
            uibs.building_subtype, uibs.custom_type)
end

local function is_plannable()
    return get_cur_filters() and
            not (uibs.building_type == df.building_type.Construction
                 and uibs.building_subtype == df.construction_type.TrackNSEW)
end

local function is_stairs()
    return uibs.building_type == df.building_type.Construction
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
}

function ItemLine:init()
    self.frame.h = 1
    self.visible = function() return #get_cur_filters() >= self.idx end
    self:addviews{
        widgets.Label{
            frame={t=0, l=23},
            text={
                {tile=get_button_start_pen},
                {gap=6, tile=get_button_end_pen},
                {tile=get_button_start_pen},
                {gap=1, tile=get_button_end_pen},
            },
        },
        widgets.Label{
            frame={t=0, l=0},
            text={
                {width=21, text=self:callback('get_item_line_text')},
                {gap=3, text='filter', pen=COLOR_GREEN},
                {gap=2, text='x', pen=COLOR_GREEN},
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

local function get_quantity(filter)
    local quantity = filter.quantity or 1
    local dimx, dimy, dimz = get_cur_area_dims()
    if quantity < 1 then
        quantity = (((dimx * dimy) // 4) + 1) * dimz
    else
        quantity = quantity * dimx * dimy * dimz
    end
    return quantity
end

function ItemLine:get_item_line_text()
    local idx = self.idx
    local filter = get_cur_filters()[idx]
    local quantity = get_quantity(filter)

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
    self.available = math.max(0, self.available - get_quantity(filter))
end

local function get_placement_errors()
    local out = ''
    for _,str in ipairs(uibs.errors) do
        if #out > 0 then out = out .. NEWLINE end
        out = out .. str.value
    end
    return out
end

PlannerOverlay = defclass(PlannerOverlay, overlay.OverlayWidget)
PlannerOverlay.ATTRS{
    default_pos={x=5,y=9},
    default_enabled=true,
    viewscreens='dwarfmode/Building/Placement',
    frame={w=56, h=20},
}

function PlannerOverlay:init()
    local main_panel = widgets.Panel{
        view_id='main',
        frame={t=0, l=0, r=0, h=14},
        frame_style=gui.MEDIUM_FRAME,
        frame_background=gui.CLEAR_PEN,
    }

    main_panel:addviews{
        widgets.Label{
            frame={},
            auto_width=true,
            text='No items required.',
            visible=function() return #get_cur_filters() == 0 end,
        },
        ItemLine{view_id='item1', frame={t=0, l=0, r=0}, idx=1},
        ItemLine{view_id='item2', frame={t=2, l=0, r=0}, idx=2},
        ItemLine{view_id='item3', frame={t=4, l=0, r=0}, idx=3},
        ItemLine{view_id='item4', frame={t=6, l=0, r=0}, idx=4},
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
                     return ('%dx%dx%d'):format(get_cur_area_dims())
                 end
                },
            },
            visible=is_choosing_area,
        },
        widgets.CycleHotkeyLabel{
            view_id='safety',
            frame={b=0, l=2},
            key='CUSTOM_G',
            label='Safety: ',
            options={
                {label='None', value='none'},
                {label='Magma', value='magma'},
                {label='Fire', value='fire'},
            },
        },
        widgets.HotkeyLabel{
            frame={b=1, l=0},
            key='SELECT',
            label='Choose item',
        },
        widgets.HotkeyLabel{
            frame={b=1, l=21},
            key='CUSTOM_F',
            label='Filter',
        },
        widgets.HotkeyLabel{
            frame={b=1, l=33},
            key='CUSTOM_X',
            label='Clear filter',
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

function PlannerOverlay:onInput(keys)
    if not is_plannable() then return false end
    if keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        if uibs.selection_pos:isValid() then
            uibs.selection_pos:clear()
            return true
        end
        self:reset()
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
        local pos = dfhack.gui.getMousePos()
        if pos then
            if is_choosing_area() or cur_building_has_no_area() then
                if #get_cur_filters() == 0 then
                    return false -- we don't add value; let the game place it
                end
                self.subviews.item1:reduce_quantity()
                self.subviews.item2:reduce_quantity()
                self.subviews.item3:reduce_quantity()
                self.subviews.item4:reduce_quantity()
                self:place_building()
                uibs.selection_pos:clear()
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

local GOOD_PEN = to_pen{ch='o', fg=COLOR_GREEN,
                        tile=dfhack.screen.findGraphicsTile('CURSORS', 1, 2)}
local BAD_PEN = to_pen{ch='X', fg=COLOR_RED,
                       tile=dfhack.screen.findGraphicsTile('CURSORS', 3, 0)}

function PlannerOverlay:onRenderFrame(dc, rect)
    PlannerOverlay.super.onRenderFrame(self, dc, rect)

    if reset_counts_flag then
        self:reset()
    end

    if not is_choosing_area() then return end

    local bounds = {
        x1 = math.min(uibs.selection_pos.x, uibs.pos.x),
        x2 = math.max(uibs.selection_pos.x, uibs.pos.x),
        y1 = math.min(uibs.selection_pos.y, uibs.pos.y),
        y2 = math.max(uibs.selection_pos.y, uibs.pos.y),
    }

    local pen = #uibs.errors > 0 and BAD_PEN or GOOD_PEN

    local function get_overlay_pen(pos)
        return pen
    end

    guidm.renderMapOverlay(get_overlay_pen, bounds)
end

function PlannerOverlay:place_building()
    local direction = uibs.direction
    local width, height, depth = get_cur_area_dims()
    local _, adjusted_width, adjusted_height = dfhack.buildings.getCorrectSize(
            width, height, uibs.building_type, uibs.building_subtype,
            uibs.custom_type, direction)
    -- get the upper-left corner of the building/area at min z-level
    local has_selection = is_choosing_area()
    local start_pos = xyz2pos(
        has_selection and math.min(uibs.selection_pos.x, uibs.pos.x) or uibs.pos.x - adjusted_width//2,
        has_selection and math.min(uibs.selection_pos.y, uibs.pos.y) or uibs.pos.y - adjusted_height//2,
        has_selection and math.min(uibs.selection_pos.z, uibs.pos.z) or uibs.pos.z
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
        max_z = math.max(uibs.selection_pos.z, uibs.pos.z)
    end
    local blds = {}
    local subtype = uibs.building_subtype
    for z=min_z,max_z do for y=min_y,max_y do for x=min_x,max_x do
        local pos = xyz2pos(x, y, z)
        if is_stairs() then
            if z == min_z then
                subtype = self.subviews.stairs_bottom_subtype:getOptionValue()
                if subtype == 'auto' then
                    local tt = dfhack.maps.getTileType(pos)
                    local shape = df.tiletype.attrs[tt].shape
                    if shape == df.tiletype_shape.STAIR_DOWN then
                        subtype = uibs.building_subtype
                    else
                        subtype = df.construction_type.UpStair
                    end
                end
            elseif z == max_z then
                subtype = self.subviews.stairs_top_subtype:getOptionValue()
                if subtype == 'auto' then
                    local tt = dfhack.maps.getTileType(pos)
                    local shape = df.tiletype.attrs[tt].shape
                    if shape == df.tiletype_shape.STAIR_UP then
                        subtype = uibs.building_subtype
                    else
                        subtype = df.construction_type.DownStair
                    end
                end
            else
                subtype = uibs.building_subtype
            end
        end
        local bld, err = dfhack.buildings.constructBuilding{pos=pos,
            type=uibs.building_type, subtype=subtype, custom=uibs.custom_type,
            width=adjusted_width, height=adjusted_height, direction=direction}
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
    end end end
    for _,bld in ipairs(blds) do
        addPlannedBuilding(bld)
    end
    scheduleCycle()
end

--------------------------------
-- InspectorOverlay
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
    self.status = nil
end

InspectorOverlay = defclass(InspectorOverlay, overlay.OverlayWidget)
InspectorOverlay.ATTRS{
    default_pos={x=-41,y=14},
    default_enabled=true,
    viewscreens='dwarfmode/ViewSheets/BUILDING',
    frame={w=30, h=14},
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
            frame={t=10, l=0},
            label='adjust filters',
            key='CUSTOM_CTRL_F',
        },
        widgets.HotkeyLabel{
            frame={t=11, l=0},
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

-- returns whether the items matched by the specified filter can have a quality
-- rating. This also conveniently indicates whether an item can be decorated.
-- does not need the core suspended
-- reverse_idx is 0-based and is expected to be counted from the *last* filter
function item_can_be_improved(btype, subtype, custom, reverse_idx)
    local filter = get_filter(btype, subtype, custom, reverse_idx)
    if filter.flags2 and filter.flags2.building_material then
        return false;
    end
    return filter.item_type ~= df.item_type.WOOD and
            filter.item_type ~= df.item_type.BLOCKS and
            filter.item_type ~= df.item_type.BAR and
            filter.item_type ~= df.item_type.BOULDER
end

return _ENV
