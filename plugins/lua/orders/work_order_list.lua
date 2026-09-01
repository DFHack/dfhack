local _ENV = mkmodule('plugins.orders.work_order_list')

local gui = require('gui')

ORDER_HEIGHT = 3

local TABS_WIDTH_THRESHOLD = 155
local LIST_START_Y_ONE_TABS_ROW = 8
local LIST_START_Y_TWO_TABS_ROWS = 10
local BOTTOM_MARGIN = 9

---@param interface_width integer
---@return integer
local function calculate_list_start_y(interface_width)
    if interface_width >= TABS_WIDTH_THRESHOLD then
        return LIST_START_Y_ONE_TABS_ROW
    end
    return LIST_START_Y_TWO_TABS_ROWS
end

---@param interface_height integer
---@param list_start_y integer
---@return integer
local function calculate_viewport_size(interface_height, list_start_y)
    local available_height = interface_height - list_start_y - BOTTOM_MARGIN
    return math.max(0, math.floor(available_height / ORDER_HEIGHT))
end

---@param order_count integer
---@param viewport_size integer
---@param requested_start integer
---@return integer viewport_start
---@return integer viewport_end
local function calculate_visible_order_indices(
        order_count, viewport_size, requested_start)
    if order_count <= 0 or viewport_size <= 0 then return 0, -1 end

    local final_page_start = math.max(0, order_count - viewport_size)
    local viewport_start = math.max(
        0, math.min(requested_start, final_page_start))
    local viewport_end = math.min(
        order_count - 1, viewport_start + viewport_size - 1)
    return viewport_start, viewport_end
end

---@return integer
function get_list_start_y()
    return calculate_list_start_y(gui.get_interface_rect().width)
end

---@return integer
function get_viewport_size()
    return calculate_viewport_size(
        gui.get_interface_rect().height, get_list_start_y())
end

---@return integer viewport_start
---@return integer viewport_end
function get_visible_order_indices()
    local order_count = #df.global.world.manager_orders.all
    local requested_start = df.global.game.main_interface.info.work_orders
        .scroll_position_work_orders
    return calculate_visible_order_indices(
        order_count, get_viewport_size(), requested_start)
end

---@param order_idx integer
---@return integer|nil y
function get_order_y(order_idx)
    local orders = df.global.world.manager_orders.all
    if order_idx < 0 or order_idx >= #orders then return nil end

    local viewport_start, viewport_end = get_visible_order_indices()
    if order_idx < viewport_start or order_idx > viewport_end then return nil end

    return get_list_start_y() + (order_idx - viewport_start) * ORDER_HEIGHT
end

unit_test_hooks = {
    calculate_list_start_y = calculate_list_start_y,
    calculate_viewport_size = calculate_viewport_size,
    calculate_visible_order_indices = calculate_visible_order_indices,
}

return _ENV
