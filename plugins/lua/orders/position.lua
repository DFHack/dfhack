local _ENV = mkmodule('plugins.orders.position')

---@alias MoveOrderStatus 'moved'|'unchanged'

---@class MoveOrderResult
---@field status MoveOrderStatus
---@field order df.manager_order Existing order pointer that was selected.
---@field current_position integer Original one-based position.
---@field new_position integer Final one-based position.
---@field previous_order? df.manager_order Order preceding the moved order.

---@class ManagerOrderPositionRow
---@field position string
---@field id? string
---@field frequency string
---@field progress string
---@field name string

local MOVE_USAGE =
    'Usage: orders move <current-position> <new-position> [--show-id]'
local POSITIONS_USAGE = 'Usage: orders positions [--show-id]'

---@param args string[]
---@return boolean|nil show_id
---@return string[]|nil positionals
---@return string|nil error_message
local function parse_show_id(args)
    local show_id = false
    local positionals = {}
    for _, arg in ipairs(args) do
        if arg == '--show-id' then
            show_id = true
        elseif arg:sub(1, 2) == '--' then
            return nil, nil, ('Unknown option: %s'):format(arg)
        else
            table.insert(positionals, arg)
        end
    end
    return show_id, positionals
end

---@param value string|number
---@param label string
---@return integer|nil position
---@return string|nil error_message
local function parse_position(value, label)
    local position
    if type(value) == 'number' then
        position = value
    elseif type(value) == 'string' and value:match('^%d+$') then
        position = tonumber(value)
    end

    position = position and math.tointeger(position)
    if not position or position < 1 then
        return nil, ('%s must be a positive integer; got %q.'):format(
            label, tostring(value))
    end

    return position
end

---@param order df.manager_order|table
---@param show_id boolean
---@param get_order_name fun(order: df.manager_order|table): string
---@return string description
local function describe_order(order, show_id, get_order_name)
    local order_name = get_order_name(order)
    if show_id then
        return ('order ID %d %q'):format(order.id, order_name)
    end
    return ('order %q'):format(order_name)
end

--- Validates and moves an existing order pointer within a zero-based vector.
---@param orders df.manager_order[]
---@param current_value string|number One-based current position.
---@param new_value string|number One-based destination position.
---@return MoveOrderResult|nil result
---@return string|nil error_message
local function move_in_vector(orders, current_value, new_value)
    local current_position, current_error =
        parse_position(current_value, 'Current position')
    if not current_position then return nil, current_error end

    local new_position, new_error = parse_position(new_value, 'New position')
    if not new_position then return nil, new_error end

    local order_count = #orders
    if order_count == 0 then
        return nil, 'There are no manager orders to move.'
    end

    if current_position > order_count then
        return nil, ('Current position %d is outside the valid range 1..%d.'):
            format(current_position, order_count)
    end
    if new_position > order_count then
        return nil, ('New position %d is outside the valid range 1..%d.'):
            format(new_position, order_count)
    end

    local current_index = current_position - 1
    local order = orders[current_index]
    if current_position == new_position then
        return {
            status = 'unchanged',
            order = order,
            current_position = current_position,
            new_position = new_position,
        }
    end

    -- DF containers use zero-based indices. Erasing a pointer-vector cell does
    -- not delete its pointee, so retain and reinsert the existing order.
    local new_index = new_position - 1
    orders:erase(current_index)
    orders:insert(new_index, order)

    return {
        status = 'moved',
        order = order,
        current_position = current_position,
        new_position = new_position,
        previous_order = new_position > 1 and orders[new_index - 1] or nil,
    }
end

--- Moves an existing manager-order pointer to a one-based vector position.
---@param current_value string|number
---@param new_value string|number
---@return MoveOrderResult|nil result
---@return string|nil error_message
function move(current_value, new_value)
    if not dfhack.isMapLoaded() or not dfhack.world.isFortressMode() then
        return nil, 'A fortress map must be loaded to move manager orders.'
    end

    return move_in_vector(
        df.global.world.manager_orders.all, current_value, new_value)
end

---@param result MoveOrderResult
---@param show_id boolean
---@param get_order_name fun(order: df.manager_order|table): string
---@return string
local function format_move_result(result, show_id, get_order_name)
    local order_description = describe_order(
        result.order, show_id, get_order_name)
    if result.status == 'unchanged' then
        return ('No changes made; %s is already at position %d.'):format(
            order_description, result.current_position)
    end
    if result.new_position == 1 then
        return ('Moved %s to the first position.'):format(order_description)
    end
    return ('Moved %s to the position after %s.'):format(
        order_description,
        describe_order(result.previous_order, show_id, get_order_name))
end

--- Prints a compact table of current fort-wide manager orders.
---@param show_id? boolean
---@return boolean success
---@return string? error_message
function print_positions(show_id)
    if not dfhack.isMapLoaded() or not dfhack.world.isFortressMode() then
        return false, 'A fortress map must be loaded to list manager orders.'
    end

    local orders = df.global.world.manager_orders.all
    if #orders == 0 then
        print('No manager orders.')
        return true
    end

    ---@type ManagerOrderPositionRow[]
    local rows = {}
    local position_width = #'POS'
    local id_width = #'ID'
    local frequency_width = #'FREQ'
    local progress_width = #'QTY'

    for order_index = 0, #orders - 1 do
        local order = orders[order_index]
        local frequency = df.workquota_frequency_type[order.frequency]
            or ('Unknown(%d)'):format(order.frequency)
        local row = {
            position = tostring(order_index + 1),
            frequency = frequency,
            progress = ('%d/%d'):format(order.amount_left, order.amount_total),
            name = dfhack.job.getManagerOrderName(order),
        }
        if show_id then row.id = tostring(order.id) end
        table.insert(rows, row)
        position_width = math.max(position_width, #row.position)
        if row.id then id_width = math.max(id_width, #row.id) end
        frequency_width = math.max(frequency_width, #row.frequency)
        progress_width = math.max(progress_width, #row.progress)
    end

    if show_id then
        local row_format = ('%%%ds  %%%ds  %%-%ds  %%%ds  %%s'):format(
            position_width, id_width, frequency_width, progress_width)
        print(row_format:format('POS', 'ID', 'FREQ', 'QTY', 'NAME'))
        for _, row in ipairs(rows) do
            print(row_format:format(
                row.position, row.id, row.frequency, row.progress, row.name))
        end
        return true
    end

    local row_format = ('%%%ds  %%-%ds  %%%ds  %%s'):format(
        position_width, frequency_width, progress_width)
    print(row_format:format('POS', 'FREQ', 'QTY', 'NAME'))
    for _, row in ipairs(rows) do
        print(row_format:format(
            row.position, row.frequency, row.progress, row.name))
    end
    return true
end

---@param args string[]
---@return boolean success
---@return string? error_message
local function run_positions(args)
    local show_id, positionals, option_error = parse_show_id(args)
    if option_error then
        return false, option_error .. '\n' .. POSITIONS_USAGE
    end
    positionals = positionals or {}
    if #positionals ~= 0 then
        return false,
            ('Expected no positional arguments for positions, but received '
                .. '%d.\n%s'):format(#positionals, POSITIONS_USAGE)
    end
    return print_positions(show_id)
end

---@param args string[]
---@return boolean success
---@return string? error_message
local function run_move(args)
    local show_id, positionals, option_error = parse_show_id(args)
    if option_error then return false, option_error .. '\n' .. MOVE_USAGE end
    positionals = positionals or {}
    if #positionals ~= 2 then
        return false,
            ('Expected exactly two positions, but received %d.\n%s'):
                format(#positionals, MOVE_USAGE)
    end

    local result, error_message = move(positionals[1], positionals[2])
    if not result then return false, error_message or 'Could not move order.' end
    print(format_move_result(
        result, show_id or false, dfhack.job.getManagerOrderName))
    return true
end

--- Handles the Lua-backed subcommands forwarded by the orders plugin.
---@param args string[]
---@return boolean success
---@return string? error_message
function parse_commandline(args)
    local command = table.remove(args, 1)
    if command == 'positions' then
        return run_positions(args)
    elseif command == 'move' then
        return run_move(args)
    end
    return false,
        ('Unknown Lua orders subcommand: %s'):format(tostring(command))
end

unit_test_hooks = {
    format_move_result = function(result, show_id)
        return format_move_result(
            result, show_id, function(order) return order.name end)
    end,
    move_in_vector = move_in_vector,
    parse_show_id = parse_show_id,
}

return _ENV
