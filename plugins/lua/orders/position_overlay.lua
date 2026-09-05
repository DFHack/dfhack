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
local POSITION_TEXT_PEN = COLOR_LIGHTCYAN
local POSITION_HOVER_PEN = dfhack.pen.parse {
    fg = COLOR_BLACK,
    bg = COLOR_WHITE,
}

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

---@type PositionOverlay|nil
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
local function get_position_width()
    return math.max(3, #tostring(#df.global.world.manager_orders.all))
end

---@return integer
local function get_editor_width()
    return math.max(MIN_EDITOR_WIDTH, get_position_width())
end

PositionOverlay = defclass(PositionOverlay, overlay.OverlayWidget)
PositionOverlay.ATTRS {
    desc = 'Displays and directly edits fort-wide work-order positions.',
    default_enabled = true,
    viewscreens = 'dwarfmode/Info/WORK_ORDERS/Default',
    -- Position fields occupy a fixed gutter and are not player-repositionable.
    full_interface = true,
    frame = { w = MIN_EDITOR_WIDTH + FIELD_BRACKETS_WIDTH, h = 1 },
    version = 1,
}

function PositionOverlay:init()
    active_position_overlay = self
    self.position_rows = {}
    self.position_fields = {}
    self.slot_order_ids = {}
    self.selected_order_id = nil
    self.selected_slot = nil
    self.edit_text = nil
    self.syncing_fields = true

    local viewport_size = work_order_list.get_viewport_size()
    self.frame.w = get_editor_width() + FIELD_BRACKETS_WIDTH
    self.frame.h = math.max(1, viewport_size * ORDER_HEIGHT)
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
            visible = function() return self.selected_slot == slot end,
            text_pen = POSITION_TEXT_PEN,
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
            visible = function() return self.selected_slot ~= slot end,
            text = { {
                text = function() return field.text end,
            } },
            text_pen = POSITION_TEXT_PEN,
            text_hpen = POSITION_HOVER_PEN,
            on_click = function() field:setFocus(true) end,
        }
        local row = widgets.Panel {
            frame = {
                l = 0,
                t = (slot - 1) * ORDER_HEIGHT,
                w = get_editor_width() + FIELD_BRACKETS_WIDTH,
                h = 1,
            },
            visible = function() return self.slot_order_ids[slot] ~= nil end,
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

--- Starts editing the order currently assigned to a visible row field.
---@param slot integer
function PositionOverlay:on_field_focus(slot)
    if self.syncing_fields then return end

    local order_id = self.slot_order_ids[slot]
    if order_id == nil then return end

    unfocus_orders_search()
    self.selected_order_id = order_id
    self.selected_slot = slot
    self.edit_text = self.position_fields[slot].text
end

--- Records text only from the field that owns the active edit.
---@param slot integer
---@param text string
function PositionOverlay:on_field_change(slot, text)
    if self.syncing_fields
        or self.slot_order_ids[slot] ~= self.selected_order_id then
        return
    end

    self.edit_text = text
end

--- Moves the selected order to the entered one-based position.
---@param slot integer
function PositionOverlay:on_field_submit(slot)
    if self.slot_order_ids[slot] ~= self.selected_order_id then return end

    local current_order_idx = find_order_index(self.selected_order_id)
    if current_order_idx == nil then
        self:clear_selection()
        dialogs.showMessage('Error',
            'orders: The selected manager order no longer exists.',
            COLOR_LIGHTRED)
        return
    end

    local field = self.position_fields[slot]
    local result, error_message = position.move(
        current_order_idx + 1, field.text)
    if not result then
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
    local slot = self.selected_slot
    if slot == nil or self.slot_order_ids[slot] ~= self.selected_order_id then
        return nil
    end
    return self.position_fields[slot]
end

--- Cancels the current proposal and restores the displayed position.
function PositionOverlay:clear_selection()
    local order_id = self.selected_order_id
    local field = self:get_selected_field()

    self.selected_order_id = nil
    self.selected_slot = nil
    self.edit_text = nil

    if field then
        if field.focus then field:setFocus(false) end
        clear_field_text_selection(field)
    end

    local order_idx = order_id and find_order_index(order_id) or nil
    if field and order_idx ~= nil then
        local was_syncing = self.syncing_fields
        self.syncing_fields = true
        field:setText(tostring(order_idx + 1))
        self.syncing_fields = was_syncing
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
    local old_selected_slot = self.selected_slot
    local old_selected_field = old_selected_slot
        and self.position_fields[old_selected_slot] or nil
    local selected_was_focused = old_selected_field
        and old_selected_field.focus or false

    self.syncing_fields = true
    self:ensure_position_field_count(viewport_size)

    local overlay_frame = {
        l = POSITION_X - 1,
        t = work_order_list.get_list_start_y(),
        w = editor_width + FIELD_BRACKETS_WIDTH,
        h = math.max(1, viewport_size * ORDER_HEIGHT),
    }
    local layout_changed = not frames_equal(self.frame, overlay_frame)
    if layout_changed then self.frame = overlay_frame end

    for slot, row in ipairs(self.position_rows) do
        local row_frame = {
            l = 0,
            t = (slot - 1) * ORDER_HEIGHT,
            w = editor_width + FIELD_BRACKETS_WIDTH,
            h = 1,
        }
        if not frames_equal(row.frame, row_frame) then
            row.frame = row_frame
            layout_changed = true
        end
    end

    if layout_changed then self:updateLayout() end

    local selected_order_idx = self.selected_order_id
        and find_order_index(self.selected_order_id) or nil
    local selected_is_visible = selected_order_idx ~= nil
        and selected_order_idx >= viewport_start
        and selected_order_idx <= viewport_end
    if self.selected_order_id ~= nil and not selected_is_visible then
        self.selected_order_id = nil
        self.edit_text = nil
    end

    self.selected_slot = selected_is_visible
        and selected_order_idx - viewport_start + 1 or nil

    local orders = df.global.world.manager_orders.all
    for slot, field in ipairs(self.position_fields) do
        local order_idx = viewport_start + slot - 1
        local has_order = slot <= viewport_size and order_idx <= viewport_end
        local order_id = has_order and orders[order_idx].id or nil
        self.slot_order_ids[slot] = order_id

        local text = ''
        if has_order then
            if order_id == self.selected_order_id then
                text = self.edit_text or tostring(order_idx + 1)
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
    local previous_order_id = self.selected_order_id

    if self.selected_order_id ~= nil and (keys._MOUSE_R or keys.LEAVESCREEN) then
        self:clear_selection()
        return true
    end

    -- Position fields only need unmodified numeric editing keys. Cancel the
    -- proposal and let DFHack or vanilla DF handle any modified shortcut.
    if self.selected_order_id ~= nil and is_modifier_active() then
        self:clear_selection()
        return false
    end

    -- Let an existing row field receive the complete click sequence. This is
    -- the same widget-first ordering used by OrdersSearchOverlay.
    if PositionOverlay.super.onInput(self, keys) then
        local selected_field = self:get_selected_field()
        if keys._MOUSE_L and self.selected_order_id ~= previous_order_id
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
        if self.selected_order_id ~= nil then self:clear_selection() end
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

unit_test_hooks = {
    accept_position_digit = accept_position_digit,
    clear_field_text_selection = clear_field_text_selection,
    get_orders_search_field = function() return orders_search_field end,
    has_modifier = has_modifier,
    select_all_field_text = select_all_field_text,
    unfocus_orders_search = unfocus_orders_search,
}

return _ENV
