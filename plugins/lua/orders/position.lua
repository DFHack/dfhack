local _ENV = mkmodule('plugins.orders.position')

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
        return nil, ('%s must be a positive integer; got %q.'):
            format(label, tostring(value))
    end

    return position
end

--- Validates and moves an existing order pointer within a zero-based vector.
---@param orders df.manager_order[]
---@param current_position string|number One-based current position.
---@param new_value string|number One-based destination position.
---@return df.manager_order|nil order
---@return string|nil error_message
local function move_in_vector(orders, current_position, new_value)
    local parsed_current_position, current_error =
        parse_position(current_position, 'Current position')
    if not parsed_current_position then return nil, current_error end

    local new_position, new_error = parse_position(new_value, 'New position')
    if not new_position then return nil, new_error end

    local order_count = #orders
    if parsed_current_position > order_count then
        return nil,
            ('Current position %d is outside the valid range 1..%d.'):
                format(parsed_current_position, order_count)
    end
    if new_position > order_count then
        return nil, ('New position %d is outside the valid range 1..%d.'):
            format(new_position, order_count)
    end

    local current_index = parsed_current_position - 1
    local order = orders[current_index]
    if not order then return nil, 'The selected manager order no longer exists.' end
    if parsed_current_position == new_position then return order end

    -- DF containers use zero-based indices. Erasing a pointer-vector cell does
    -- not delete its pointee, so retain and reinsert the existing order.
    orders:erase(current_index)
    orders:insert(new_position - 1, order)
    return order
end

--- Moves an existing manager-order pointer to a one-based vector position.
---@param current_position string|number
---@param new_value string|number
---@return df.manager_order|nil order
---@return string|nil error_message
function move(current_position, new_value)
    return move_in_vector(
        df.global.world.manager_orders.all, current_position, new_value)
end

unit_test_hooks = {
    move_in_vector = move_in_vector,
}

return _ENV
