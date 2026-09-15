local _ENV = mkmodule('plugins.orders.position_overlay')

local dialogs = require('gui.dialogs')
local gui = require('gui')
local position = require('plugins.orders.position')
local overlay = require('plugins.overlay')
local work_order_list = require('plugins.orders.work_order_list')
local widgets = require('gui.widgets')

-- Keep the bracketed positions in the left-side gutter beside each order.
local POSITION_X = 6
local MIN_EDITOR_WIDTH = 4
local FIELD_BRACKETS_WIDTH = 2
local EDITOR_TEXT_PEN = COLOR_LIGHTCYAN
local INACTIVE_TEXT_PEN = COLOR_WHITE
local POSITION_HOVER_PEN = dfhack.pen.parse {
    fg = COLOR_BLACK,
    bg = COLOR_WHITE,
}

local TOGGLE_PANEL_X = 25
local TOGGLE_PANEL_BOTTOM = 6
local TOGGLE_PANEL_WIDTH = 15
local TOGGLE_PANEL_HEIGHT = 3

local ORDER_HEIGHT = work_order_list.ORDER_HEIGHT

---@param order_id integer
---@return integer|nil order_idx
local function find_order_index(order_id)
    local orders = df.global.world.manager_orders.all
    for order_idx = 0, #orders - 1 do
        if orders[order_idx].id == order_id then return order_idx end
    end
end

---@param first widgets.Widget.frame
---@param second widgets.Widget.frame
---@return boolean
local function frames_equal(first, second)
    return first.l == second.l
        and first.t == second.t
        and first.r == second.r
        and first.b == second.b
        and first.w == second.w
        and first.h == second.h
end

---@return boolean
local function are_order_details_open()
    return df.global.game.main_interface.job_details.open
end

---@type widgets.EditField|nil
local orders_search_field

---@type plugins.orders.position_overlay.PositionOverlay|nil
local active_position_overlay

--- Registers the search field that should lose focus when position editing begins.
---@param field widgets.EditField|nil
function bind_orders_search_field(field)
    orders_search_field = field
end

--- Cancels the active position edit when the Orders search field gains focus.
function clear_active_edit()
    if active_position_overlay then active_position_overlay:clear_selection() end
end

local function unfocus_orders_search()
    if orders_search_field and orders_search_field.focus then
        orders_search_field:setFocus(false)
    end
end

---@param modifiers table<string, boolean>
---@return boolean
local function has_modifier(modifiers)
    return not not (modifiers.ctrl
        or modifiers.shift
        or modifiers.alt
        or modifiers.super)
end

---@return boolean
local function is_modifier_active()
    return has_modifier(dfhack.internal.getModifiers())
end

---@param ch string
---@return boolean
local function accept_position_digit(ch)
    return ch:match('^%d$') ~= nil
end

---@param keys table
---@return boolean
local function is_mouse_or_scroll_key(keys)
    return keys._MOUSE_L
        or keys._MOUSE_L_DOWN
        or keys._MOUSE_R
        or keys._MOUSE_R_DOWN
        or keys._MOUSE_M
        or keys._MOUSE_M_DOWN
        or keys.CONTEXT_SCROLL_UP
        or keys.CONTEXT_SCROLL_DOWN
        or keys.CONTEXT_SCROLL_PAGEUP
        or keys.CONTEXT_SCROLL_PAGEDOWN
end

---@param field widgets.EditField
local function select_all_field_text(field)
    -- Use the EditField's native Ctrl+A behavior so typing replaces the
    -- displayed position while cursor and selection behavior remain standard.
    field:onInput { CUSTOM_CTRL_A = true }
end

---@param field widgets.EditField
local function clear_field_text_selection(field)
    -- EditField:setCursor() delegates to TextAreaContent:setCursor(), which
    -- clears its selection range without changing the field text.
    field:setCursor()
end

---@return integer
local function get_editor_width()
    local order_count = #df.global.world.manager_orders.all
    return math.max(MIN_EDITOR_WIDTH, #tostring(order_count))
end

---@class plugins.orders.position_overlay.PositionEdit
---@field order_id integer
---@field slot integer
---@field text string

---@class plugins.orders.position_overlay.PositionOverlay: dfhack.class, widgets.Panel
---@field super widgets.Panel
---@field edit plugins.orders.position_overlay.PositionEdit|nil
---@field clear_selection fun(self: plugins.orders.position_overlay.PositionOverlay)
---@overload fun(init_table: table): self
PositionOverlay = defclass(PositionOverlay, overlay.OverlayWidget)
PositionOverlay.ATTRS {
    desc = 'Displays and directly edits fort-wide work-order positions.',
    default_enabled = true,
    viewscreens = 'dwarfmode/Info/WORK_ORDERS/Default',
    -- Position fields occupy fixed row positions and are not repositionable.
    full_interface = true,
    frame = { w = 1, h = 1 },
    version = 1,
}

function PositionOverlay:init()
    active_position_overlay = self
    self.positions_visible = true
    self.panel_minimized = false
    self.position_rows = {}
    self.position_fields = {}
    self.slot_order_ids = {}
    self.edit = nil
    self.syncing_fields = true

    local interface_rect = gui.get_interface_rect()
    self.frame.w = interface_rect.width
    self.frame.h = interface_rect.height
    local viewport_size = work_order_list.get_viewport_size()
    self:addviews {
        widgets.Panel {
            view_id = 'positions_panel',
            frame = {
                l = TOGGLE_PANEL_X - 1,
                b = TOGGLE_PANEL_BOTTOM,
                w = TOGGLE_PANEL_WIDTH,
                h = TOGGLE_PANEL_HEIGHT,
            },
            frame_style = gui.MEDIUM_FRAME,
            frame_background = gui.CLEAR_PEN,
            visible = function() return not self.panel_minimized end,
            subviews = {
                widgets.Label {
                    frame = { l = 0, t = 0 },
                    text = 'positions',
                    text_pen = COLOR_WHITE,
                    text_hpen = POSITION_HOVER_PEN,
                    auto_width = true,
                    on_click = self:callback('toggle_positions'),
                },
                widgets.RadioButton {
                    view_id = 'positions_toggle',
                    frame = { l = 10, t = 0 },
                    initial_state = true,
                    on_change = self:callback('set_positions_visible'),
                },
            },
        },
        widgets.Panel {
            frame = {
                l = TOGGLE_PANEL_X + TOGGLE_PANEL_WIDTH - 4,
                b = TOGGLE_PANEL_BOTTOM + TOGGLE_PANEL_HEIGHT - 1,
                w = 3,
                h = 1,
            },
            subviews = {
                widgets.Label {
                    frame = { l = 0, w = 1, h = 1 },
                    text = '[',
                    text_pen = COLOR_RED,
                    visible = function() return self.panel_minimized end,
                },
                widgets.Label {
                    frame = { l = 1, w = 1, h = 1 },
                    text = { {
                        text = function()
                            return self.panel_minimized and string.char(31)
                                or string.char(30)
                        end,
                    } },
                    text_pen = dfhack.pen.parse {
                        fg = COLOR_BLACK,
                        bg = COLOR_GREY,
                    },
                    text_hpen = POSITION_HOVER_PEN,
                    on_click = self:callback('toggle_panel_minimized'),
                },
                widgets.Label {
                    frame = { r = 0, w = 1, h = 1 },
                    text = ']',
                    text_pen = COLOR_RED,
                    visible = function() return self.panel_minimized end,
                },
            },
        },
    }
    self:ensure_position_field_count(viewport_size)
    self.syncing_fields = false
end

--- Adds fields when a larger interface makes more order rows visible.
---@param count integer
function PositionOverlay:ensure_position_field_count(count)
    while #self.position_fields < count do
        local slot = #self.position_fields + 1
        local field
        field = widgets.EditField {
            frame = {
                l = 1,
                r = 1,
                h = 1,
            },
            visible = function()
                return self.edit ~= nil and self.edit.slot == slot
            end,
            text_pen = EDITOR_TEXT_PEN,
            on_char = accept_position_digit,
            on_change = function(text) self:on_field_change(slot, text) end,
            on_focus = function() self:on_field_focus(slot) end,
            on_unfocus = function()
                local position_field = self.position_fields[slot]
                if position_field then
                    clear_field_text_selection(position_field)
                end
            end,
            on_submit = function() self:on_field_submit(slot) end,
            on_submit2 = function() self:on_field_submit(slot) end,
        }
        local position_label = widgets.Label {
            frame = {
                l = 1,
                r = 1,
                h = 1,
            },
            visible = function()
                return self.edit == nil or self.edit.slot ~= slot
            end,
            text = { {
                text = function() return field.text end,
            } },
            text_pen = INACTIVE_TEXT_PEN,
            text_hpen = POSITION_HOVER_PEN,
            on_click = function() field:setFocus(true) end,
        }
        local row = widgets.Panel {
            frame = {
                l = POSITION_X - 1,
                t = work_order_list.get_list_start_y()
                    + (slot - 1) * ORDER_HEIGHT,
                w = get_editor_width() + FIELD_BRACKETS_WIDTH,
                h = 1,
            },
            visible = function()
                return self.positions_visible
                    and self.slot_order_ids[slot] ~= nil
            end,
            subviews = {
                widgets.Label {
                    frame = { l = 0, w = 1, h = 1 },
                    text = '[',
                    text_pen = COLOR_RED,
                },
                position_label,
                field,
                widgets.Label {
                    frame = { r = 0, w = 1, h = 1 },
                    text = ']',
                    text_pen = COLOR_RED,
                },
            },
        }
        self.position_rows[slot] = row
        self.position_fields[slot] = field
        self:addviews { row }

        -- A hotkey-less EditField requests focus when added. Row fields must
        -- all begin inactive and only acquire focus from a click.
        field:setFocus(false)
    end
end

--- Toggles whether the position rows are visible.
function PositionOverlay:toggle_positions()
    local toggle = self.subviews.positions_toggle
    toggle:setState(not toggle.toggle_state)
end

--- Shows or hides the position rows.
---@param visible boolean
function PositionOverlay:set_positions_visible(visible)
    if self.positions_visible == visible then return end
    if not visible then self:clear_selection() end
    self.positions_visible = visible
    self:sync_position_fields()
end

--- Shows or hides the bottom control panel.
function PositionOverlay:toggle_panel_minimized()
    self.panel_minimized = not self.panel_minimized
end

--- Starts editing the order currently assigned to a visible row field.
---@param slot integer
function PositionOverlay:on_field_focus(slot)
    if self.syncing_fields then return end

    local order_id = self.slot_order_ids[slot]
    if order_id == nil then return end

    unfocus_orders_search()
    self.edit = {
        order_id = order_id,
        slot = slot,
        text = self.position_fields[slot].text,
    }
end

--- Records text only from the field that owns the active edit.
---@param slot integer
---@param text string
function PositionOverlay:on_field_change(slot, text)
    local edit = self.edit
    if self.syncing_fields or not edit
        or self.slot_order_ids[slot] ~= edit.order_id then
        return
    end

    edit.text = text
end

--- Moves the selected order to the entered one-based position.
---@param slot integer
function PositionOverlay:on_field_submit(slot)
    local edit = self.edit
    if not edit or self.slot_order_ids[slot] ~= edit.order_id then return end

    local current_order_idx = find_order_index(edit.order_id)
    if current_order_idx == nil then
        self:clear_selection()
        dialogs.showMessage('Error',
            'orders: The selected manager order no longer exists.',
            COLOR_LIGHTRED)
        return
    end

    local field = self.position_fields[slot]
    local order, error_message = position.move(
        current_order_idx + 1, field.text)
    if not order then
        field:setFocus(true)
        dialogs.showMessage('Error',
            ('orders: %s'):format(error_message or 'Could not move order.'),
            COLOR_LIGHTRED)
        return
    end

    self:clear_selection()
end

---@return widgets.EditField|nil field
function PositionOverlay:get_selected_field()
    local edit = self.edit
    if not edit or self.slot_order_ids[edit.slot] ~= edit.order_id then
        return nil
    end
    return self.position_fields[edit.slot]
end

--- Cancels the current proposal and clears its field focus.
function PositionOverlay:clear_selection()
    local field = self.edit and self.position_fields[self.edit.slot] or nil
    self.edit = nil

    if field then
        if field.focus then field:setFocus(false) end
        clear_field_text_selection(field)
    end
end

function PositionOverlay:overlay_ondisable()
    self:clear_selection()
end

--- Synchronizes the fixed row fields with the current scroll position.
function PositionOverlay:sync_position_fields()
    local viewport_size = work_order_list.get_viewport_size()
    local viewport_start, viewport_end =
        work_order_list.get_visible_order_indices()
    local editor_width = get_editor_width()
    local old_selected_slot = self.edit and self.edit.slot or nil
    local old_selected_field = old_selected_slot
        and self.position_fields[old_selected_slot] or nil
    local selected_was_focused = old_selected_field
        and old_selected_field.focus or false

    self.syncing_fields = true
    self:ensure_position_field_count(viewport_size)

    local interface_rect = gui.get_interface_rect()
    local overlay_frame = {
        l = 0,
        t = 0,
        w = interface_rect.width,
        h = interface_rect.height,
    }
    local layout_changed = not frames_equal(self.frame, overlay_frame)
    if layout_changed then self.frame = overlay_frame end

    for slot, row in ipairs(self.position_rows) do
        local row_frame = {
            l = POSITION_X - 1,
            t = work_order_list.get_list_start_y()
                + (slot - 1) * ORDER_HEIGHT,
            w = editor_width + FIELD_BRACKETS_WIDTH,
            h = 1,
        }
        if not frames_equal(row.frame, row_frame) then
            row.frame = row_frame
            layout_changed = true
        end
    end

    if layout_changed then self:updateLayout() end

    local selected_order_idx = self.edit
        and find_order_index(self.edit.order_id) or nil
    local selected_is_visible = selected_order_idx ~= nil
        and selected_order_idx >= viewport_start
        and selected_order_idx <= viewport_end
    if self.edit and not selected_is_visible then
        self.edit = nil
    end

    if self.edit then
        self.edit.slot = selected_order_idx - viewport_start + 1
    end

    local orders = df.global.world.manager_orders.all
    for slot, field in ipairs(self.position_fields) do
        local order_idx = viewport_start + slot - 1
        local has_order = slot <= viewport_size and order_idx <= viewport_end
        local order_id = has_order and orders[order_idx].id or nil
        self.slot_order_ids[slot] = order_id

        local text = ''
        if has_order then
            if self.edit and order_id == self.edit.order_id then
                text = self.edit.text
            else
                text = tostring(order_idx + 1)
            end
        end
        if field.text ~= text then field:setText(text) end
    end

    local selected_field = self:get_selected_field()
    if selected_was_focused and selected_field then
        selected_field:setFocus(true)
    elseif selected_was_focused and old_selected_field then
        old_selected_field:setFocus(false)
    end

    self.syncing_fields = false
end

---@param keys table
---@return boolean
function PositionOverlay:onInput(keys)
    if are_order_details_open() then
        self:clear_selection()
        return false
    end

    self:sync_position_fields()
    local previous_order_id = self.edit and self.edit.order_id or nil

    if self.edit and (keys._MOUSE_R or keys.LEAVESCREEN) then
        self:clear_selection()
        return true
    end

    -- Position fields only need unmodified numeric editing keys. Cancel the
    -- proposal and let DFHack or vanilla DF handle any modified shortcut.
    if self.edit and is_modifier_active() then
        self:clear_selection()
        return false
    end

    -- Let an existing row field receive the complete click sequence. This is
    -- the same widget-first ordering used by OrdersSearchOverlay.
    if PositionOverlay.super.onInput(self, keys) then
        local selected_field = self:get_selected_field()
        if keys._MOUSE_L and self.edit
            and self.edit.order_id ~= previous_order_id
            and selected_field then
            select_all_field_text(selected_field)
        elseif keys._MOUSE_L_DOWN and selected_field
            and gui.View.getMousePos(selected_field) then
            -- Do not let the remainder of the activation click turn into a
            -- partial drag-selection.
            select_all_field_text(selected_field)
        end
        return true
    end

    -- Cancel an active proposal on an outside click, but let vanilla handle
    -- the click itself.
    if keys._MOUSE_L or keys._MOUSE_L_DOWN then
        if self.edit then self:clear_selection() end
        return false
    end

    -- Keep keyboard input in the focused field while allowing all mouse and
    -- Work Orders scrolling events through to vanilla.
    local selected_field = self:get_selected_field()
    if selected_field and selected_field.focus
        and not is_mouse_or_scroll_key(keys) then
        return true
    end

    return false
end

---@param dc gui.Painter
function PositionOverlay:render(dc)
    if are_order_details_open() then
        self:clear_selection()
        return
    end

    self:sync_position_fields()
    PositionOverlay.super.render(self, dc)
end

return _ENV
