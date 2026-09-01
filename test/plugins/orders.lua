config.mode = 'fortress'
config.target = 'orders'

local orders_plugin = require('plugins.orders')
local position = require('plugins.orders.position')
local position_overlay = require('plugins.orders.position_overlay')
local work_order_list = require('plugins.orders.work_order_list')
local mock = require('test_util.mock')
local widgets = require('gui.widgets')

local FILE_PATH_PATTERN = dfhack.getConfigPath() .. '/orders/%s.json'

local BACKUP_FILE_NAME = 'tmp-backup'
local BACKUP_FILE_PATH = FILE_PATH_PATTERN:format(BACKUP_FILE_NAME)

local TMP_FILE_NAME = 'tmp-test'
local TMP_FILE_PATH = FILE_PATH_PATTERN:format(TMP_FILE_NAME)

local function test_wrapper(test_fn)
    -- backup and clear active orders
    dfhack.run_command_silent{'orders', 'export', BACKUP_FILE_NAME}
    dfhack.run_command_silent{'orders', 'clear'}
    df.global.world.manager_orders.manager_order_next_id = 0
    return dfhack.with_finalize(
        function()
            -- clear test orders, restore original orders, remove temp files
            dfhack.run_command_silent{'orders', 'clear'}
            df.global.world.manager_orders.manager_order_next_id = 0
            dfhack.run_command_silent{'orders', 'import', BACKUP_FILE_NAME}
            df.global.world.manager_orders.manager_order_next_id =
                    #df.global.world.manager_orders.all
            os.remove(BACKUP_FILE_PATH)
            os.remove(TMP_FILE_PATH)
        end,
        test_fn)
end
config.wrapper = test_wrapper

-- returns export command result and exported file content
function run_orders_export()
    local _, result = dfhack.run_command_silent{'orders', 'export',
                                                TMP_FILE_NAME}
    local f = io.open(TMP_FILE_PATH, 'r')
    return dfhack.with_finalize(
        function() f:close() end,
        function() return result, f:read('*all') end)
end

function run_orders_import(file_content)
    local f = io.open(TMP_FILE_PATH, 'w')
    f:write(file_content)
    f:close()
    return dfhack.run_command_silent{'orders', 'import', TMP_FILE_NAME}
end

local function normalize_whitespace(str)
    return str:gsub('%s+', ' '):trim()
end

function check_export_success(expected_file_content)
    local result, file_content = run_orders_export()
    expect.eq(result, CR_OK)

    -- ignore whitespace (otherwise the expected file content is impossible to
    -- format properly in this file)
    expect.eq(normalize_whitespace(expected_file_content),
              normalize_whitespace(file_content))
end

function check_import_success(file_content, comment, num_expected_orders)
    local prev_num_orders = #df.global.world.manager_orders.all
    local output, result = run_orders_import(file_content)
    expect.eq(result, CR_OK, comment)
    expect.eq(prev_num_orders + num_expected_orders,
              #df.global.world.manager_orders.all, comment)
end

function check_import_fail(file_content, comment, prefix)
    comment = comment or ''
    local prev_num_orders = #df.global.world.manager_orders.all
    local output, result = run_orders_import(file_content)
    expect.eq(result, CR_FAILURE, ('%s: was successful'):format(comment))
    if prefix then
        expect.true_(output:lower():startswith(prefix), ('%s: "%s" missing "%s"'):format(comment, output, prefix))
    end
    expect.eq(prev_num_orders, #df.global.world.manager_orders.all, ('%s: number of manager orders changed'):format(comment))
end

function test.import_empty()
    check_import_success('[]', 'empty input', 0)
end

function test.import_non_array()
    check_import_fail('{}', 'object', 'invalid')
    check_import_fail('null', 'null', 'invalid')
    check_import_fail('2', 'number', 'invalid')
end

function test.import_invalid_syntax()
    -- for https://github.com/DFHack/dfhack/pull/1770
    check_import_fail('', 'empty')
    check_import_fail(']', 'missing opening bracket')
    check_import_fail([[
        [
            {
                "amount_left" : 0,
                "amount_total" : 0,
                "frequency" : "OneTime",
                "id" : 0,
                "is_active" : false,
                "is_validated" : true,
                "job" : "CustomReaction",
                "reaction" : "BRASS_MAKING"
            }
    ]], 'missing closing bracket')
end

function test.import_missing_fields()
    check_import_fail('[{}]', 'empty order', 'invalid')
end

function test.import_invalid_id()
    -- for https://github.com/DFHack/dfhack/issues/1893
    check_import_fail([[
        [
            {
                "amount_left": 0,
                "amount_total": 0,
                "frequency": "OneTime",
                "id": "",
                "is_active": false,
                "is_validated": false,
                "item_category": [
                    "finished_goods"
                ],
                "job": "EncrustWithGems",
                "material": "INORGANIC:AMBER OPAL"
            }
        ]
    ]], 'string id instead of int', 'error')
end

function test.import_valid_and_invalid_orders()
    -- check_import_fail([[
    --     [
    --         {
    --             "amount_left" : 1,
    --             "amount_total" : 1,
    --             "frequency" : "OneTime",
    --             "id" : 0,
    --             "is_active" : false,
    --             "is_validated" : true,
    --             "job" : "ConstructTable",
    --             "material" : "INORGANIC:IRON"
    --         },
    --         {}
    --     ]
    -- ]], 'empty order after valid order')

    check_import_fail([[
        [
            {},
            {
                "amount_left" : 1,
                "amount_total" : 1,
                "frequency" : "OneTime",
                "id" : 0,
                "is_active" : false,
                "is_validated" : true,
                "job" : "ConstructTable",
                "material" : "INORGANIC:IRON"
            }
        ]
    ]], 'empty order before valid order')
end

function test.import_export_reaction_condition()
    local file_content = [[
        [
            {
                "amount_left" : 1,
                "amount_total" : 1,
                "frequency" : "Daily",
                "id" : 0,
                "is_active" : false,
                "is_validated" : false,
                "item_conditions" :
                [
                    {
                        "condition" : "AtLeast",
                        "contains" :
                        [
                            "lye"
                        ],
                        "reaction_id" : "MAKE_SOAP_FROM_TALLOW",
                        "value" : 5
                    }
                ],
                "job" : "CustomReaction",
                "name" : "Make soap from tallow",
                "reaction" : "MAKE_SOAP_FROM_TALLOW"
            }
        ]
    ]]
    check_import_success(file_content, 'valid reaction condition', 1)
    check_export_success(file_content)
end

local function get_last_order()
    return df.global.world.manager_orders.all[#df.global.world.manager_orders.all-1]
end

function test.import_invalid_reaction_conditions()
    check_import_success([[
        [
            {
                "amount_left" : 1,
                "amount_total" : 1,
                "frequency" : "OneTime",
                "id" : 0,
                "is_active" : false,
                "is_validated" : true,
                "item_conditions" :
                [
                    {
                        "condition" : "AtLeast",
                        "contains" :
                        [
                            "lye"
                        ],
                        "reaction_id" : "MAKE_SOAP_FROM_TALLOW_xxx",
                        "value" : 5
                    }
                ],
                "job" : "CustomReaction",
                "reaction" : "MAKE_SOAP_FROM_TALLOW"
            }
        ]
    ]], 'condition ignored for bad reaction id', 1)
    expect.eq(0, #get_last_order().item_conditions)

    check_import_success([[
        [
            {
                "amount_left" : 1,
                "amount_total" : 1,
                "frequency" : "OneTime",
                "id" : 0,
                "is_active" : false,
                "is_validated" : true,
                "item_conditions" :
                [
                    {
                        "condition" : "AtLeast",
                        "contains" :
                        [
                            "lye_xxx"
                        ],
                        "reaction_id" : "MAKE_SOAP_FROM_TALLOW",
                        "value" : 5
                    }
                ],
                "job" : "CustomReaction",
                "reaction" : "MAKE_SOAP_FROM_TALLOW"
            }
        ]
    ]], 'condition ignored for bad reagent name', 1)
    expect.eq(0, #get_last_order().item_conditions)
end

function test.list()
    local output, status = dfhack.run_command_silent('orders', 'list')
    expect.eq(CR_OK, status)
    expect.str_find(BACKUP_FILE_NAME:gsub('%-', '%%-'), output)
end

local TEST_ORDERS = [[
    [
        {
            "amount_left": 1,
            "amount_total": 1,
            "frequency": "OneTime",
            "id": 0,
            "is_active": false,
            "is_validated": true,
            "job": "ConstructTable",
            "material": "INORGANIC"
        },
        {
            "amount_left": 2,
            "amount_total": 2,
            "frequency": "Daily",
            "id": 1,
            "is_active": false,
            "is_validated": true,
            "job": "ConstructTable",
            "material": "INORGANIC"
        },
        {
            "amount_left": 3,
            "amount_total": 3,
            "frequency": "Monthly",
            "id": 2,
            "is_active": false,
            "is_validated": true,
            "job": "ConstructTable",
            "material": "INORGANIC"
        }
    ]
]]

local function import_test_orders()
    local output, status = run_orders_import(TEST_ORDERS)
    expect.eq(CR_OK, status, output)
    expect.eq(3, #df.global.world.manager_orders.all)
end

local FakeManagerOrderVector = {}
FakeManagerOrderVector.__index = function(self, key)
    if type(key) == 'number' then return self.values[key + 1] end
    return FakeManagerOrderVector[key]
end
FakeManagerOrderVector.__len = function(self) return #self.values end

function FakeManagerOrderVector:erase(index)
    table.remove(self.values, index + 1)
end

function FakeManagerOrderVector:insert(index, order)
    table.insert(self.values, index + 1, order)
end

local function make_order(id, name)
    return { id = id, name = name }
end

local function make_vector()
    return setmetatable({ values = {
        make_order(10, 'first'),
        make_order(20, 'second'),
        make_order(30, 'third'),
    } }, FakeManagerOrderVector)
end

local function get_vector_ids(vector)
    local ids = {}
    for _, order in ipairs(vector.values) do table.insert(ids, order.id) end
    return ids
end

function test.position_move_vector_preserves_pointer()
    local vector = make_vector()
    local first = vector[0]
    local third = vector[2]
    local result, error_message =
        position.unit_test_hooks.move_in_vector(vector, 1, 3)
    if not result then expect.fail(error_message) return end

    expect.eq('moved', result.status)
    expect.eq(first, result.order)
    expect.eq(third, result.previous_order)
    expect.eq(first, vector[2])
    expect.table_eq({ 20, 30, 10 }, get_vector_ids(vector))

    result, error_message =
        position.unit_test_hooks.move_in_vector(vector, 3, 1)
    if not result then expect.fail(error_message) return end
    expect.eq(first, vector[0])
    expect.table_eq({ 10, 20, 30 }, get_vector_ids(vector))
end

function test.position_move_vector_rejects_invalid_and_noop_is_atomic()
    local invalid_values = { 'one', '1.5', '0', '-1', 0, 1.5 }
    for _, value in ipairs(invalid_values) do
        local vector = make_vector()
        local result = position.unit_test_hooks.move_in_vector(vector, value, 1)
        expect.nil_(result, ('value %q should be rejected'):format(value))
        expect.table_eq({ 10, 20, 30 }, get_vector_ids(vector))
    end

    local vector = make_vector()
    local second = vector[1]
    local result, error_message =
        position.unit_test_hooks.move_in_vector(vector, 2, 2)
    if not result then expect.fail(error_message) return end
    expect.eq('unchanged', result.status)
    expect.eq(second, result.order)
    expect.table_eq({ 10, 20, 30 }, get_vector_ids(vector))

    result = position.unit_test_hooks.move_in_vector(vector, 1, 4)
    expect.nil_(result)
    expect.table_eq({ 10, 20, 30 }, get_vector_ids(vector))
end

function test.position_move_formatting_and_options()
    local result = {
        status = 'moved',
        order = make_order(10, 'first'),
        current_position = 1,
        new_position = 2,
        previous_order = make_order(20, 'second'),
    }
    expect.eq('Moved order "first" to the position after order "second".',
        position.unit_test_hooks.format_move_result(result, false))
    expect.eq(
        'Moved order ID 10 "first" to the position after order ID 20 "second".',
        position.unit_test_hooks.format_move_result(result, true))

    result = {
        status = 'moved',
        order = make_order(10, 'first'),
        current_position = 3,
        new_position = 1,
    }
    expect.eq('Moved order "first" to the first position.',
        position.unit_test_hooks.format_move_result(result, false))
    expect.eq('Moved order ID 10 "first" to the first position.',
        position.unit_test_hooks.format_move_result(result, true))

    result = {
        status = 'unchanged',
        order = make_order(20, 'second'),
        current_position = 2,
        new_position = 2,
    }
    expect.eq('No changes made; order "second" is already at position 2.',
        position.unit_test_hooks.format_move_result(result, false))
    expect.eq(
        'No changes made; order ID 20 "second" is already at position 2.',
        position.unit_test_hooks.format_move_result(result, true))

    local show_id, positionals, error_message =
        position.unit_test_hooks.parse_show_id { '5', '--show-id', '3' }
    expect.true_(show_id)
    expect.table_eq({ '5', '3' }, positionals)
    expect.nil_(error_message)

    show_id, positionals, error_message =
        position.unit_test_hooks.parse_show_id { '--unknown' }
    expect.nil_(show_id)
    expect.nil_(positionals)
    expect.eq('Unknown option: --unknown', error_message)
end

function test.position_list_validation_returns_errors()
    mock.patch(dfhack, 'isMapLoaded', mock.func(false), function()
        local success, error_message = position.print_positions()
        expect.false_(success)
        expect.eq(
            'A fortress map must be loaded to list manager orders.',
            error_message)
    end)

    mock.patch({
        {dfhack, 'isMapLoaded', mock.func(true)},
        {dfhack.world, 'isFortressMode', mock.func(false)},
    }, function()
        local success, error_message = position.print_positions()
        expect.false_(success)
        expect.eq(
            'A fortress map must be loaded to list manager orders.',
            error_message)
    end)
end

function test.work_order_list_geometry()
    local hooks = work_order_list.unit_test_hooks

    expect.eq(10, hooks.calculate_list_start_y(154))
    expect.eq(8, hooks.calculate_list_start_y(155))
    expect.eq(4, hooks.calculate_viewport_size(30, 8))
    expect.eq(0, hooks.calculate_viewport_size(16, 10))

    local viewport_start, viewport_end =
        hooks.calculate_visible_order_indices(0, 4, 0)
    expect.eq(0, viewport_start)
    expect.eq(-1, viewport_end)

    viewport_start, viewport_end =
        hooks.calculate_visible_order_indices(10, 0, 0)
    expect.eq(0, viewport_start)
    expect.eq(-1, viewport_end)

    viewport_start, viewport_end =
        hooks.calculate_visible_order_indices(10, 4, 8)
    expect.eq(6, viewport_start)
    expect.eq(9, viewport_end)

    viewport_start, viewport_end =
        hooks.calculate_visible_order_indices(10, 20, 8)
    expect.eq(0, viewport_start)
    expect.eq(9, viewport_end)
end

function test.position_commands_dispatch_to_lua()
    import_test_orders()
    local orders = df.global.world.manager_orders.all
    local first = orders[0]
    local second = orders[1]
    local third = orders[2]

    local names = {}
    for order_index = 0, #orders - 1 do
        names[order_index + 1] =
            dfhack.job.getManagerOrderName(orders[order_index])
    end

    local output, status = dfhack.run_command_silent { 'orders', 'positions' }
    expect.eq(CR_OK, status)
    expect.eq(normalize_whitespace(([[
        POS FREQ QTY NAME
        1 OneTime 1/1 %s
        2 Daily 2/2 %s
        3 Monthly 3/3 %s
    ]]):format(table.unpack(names))), normalize_whitespace(output))

    output, status =
        dfhack.run_command_silent { 'orders', 'positions', '--show-id' }
    expect.eq(CR_OK, status)
    expect.eq(normalize_whitespace(([[
        POS ID FREQ QTY NAME
        1 0 OneTime 1/1 %s
        2 1 Daily 2/2 %s
        3 2 Monthly 3/3 %s
    ]]):format(table.unpack(names))), normalize_whitespace(output))

    output, status = dfhack.run_command_silent { 'orders', 'move', '1', '3' }
    expect.eq(CR_OK, status)
    expect.eq(second, orders[0])
    expect.eq(third, orders[1])
    expect.eq(first, orders[2])
    expect.str_find('Moved order ', output)

    output, status = dfhack.run_command_silent { 'orders', 'move', '3', '3' }
    expect.eq(CR_OK, status)
    expect.eq(first, orders[2])
    expect.str_find('No changes made;', output)

    output, status = dfhack.run_command_silent { 'orders', 'move', '1', '4' }
    expect.eq(CR_FAILURE, status)
    expect.str_find('New position 4 is outside the valid range 1%.%.3%.', output)
    expect.nil_(output:find('Failed Lua call', 1, true))
    expect.eq(second, orders[0])
    expect.eq(third, orders[1])
    expect.eq(first, orders[2])
end

local function expect_position_command_failure(arguments, expected_message)
    local orders = df.global.world.manager_orders.all
    local original_orders = { orders[0], orders[1], orders[2] }
    local output, status = dfhack.run_command_silent(arguments)
    expect.eq(CR_FAILURE, status, output)
    expect.true_(output:find(expected_message, 1, true) ~= nil,
        ('expected %q in %q'):format(expected_message, output))
    expect.nil_(output:find('Failed Lua call', 1, true))
    expect.eq(#original_orders, #orders)
    for order_index = 0, #orders - 1 do
        expect.eq(original_orders[order_index + 1], orders[order_index])
    end
end

function test.position_commands_validate_arguments_atomically()
    import_test_orders()

    local cases = {
        {
            { 'orders', 'move' },
            'Expected exactly two positions, but received 0.',
        },
        {
            { 'orders', 'move', '1' },
            'Expected exactly two positions, but received 1.',
        },
        {
            { 'orders', 'move', '1', '2', '3' },
            'Expected exactly two positions, but received 3.',
        },
        {
            { 'orders', 'move', 'one', '2' },
            'Current position must be a positive integer; got "one".',
        },
        {
            { 'orders', 'move', '1.5', '2' },
            'Current position must be a positive integer; got "1.5".',
        },
        {
            { 'orders', 'move', '0', '1' },
            'Current position must be a positive integer; got "0".',
        },
        {
            { 'orders', 'move', '-1', '1' },
            'Current position must be a positive integer; got "-1".',
        },
        {
            { 'orders', 'move', '1', 'one' },
            'New position must be a positive integer; got "one".',
        },
        {
            { 'orders', 'move', '1', '0' },
            'New position must be a positive integer; got "0".',
        },
        {
            { 'orders', 'move', '4', '1' },
            'Current position 4 is outside the valid range 1..3.',
        },
        {
            { 'orders', 'move', '1', '4' },
            'New position 4 is outside the valid range 1..3.',
        },
        {
            { 'orders', 'move', '1', '2', '--unknown' },
            'Unknown option: --unknown',
        },
        {
            { 'orders', 'positions', '1' },
            'Expected no positional arguments for positions, but received 1.',
        },
        {
            { 'orders', 'positions', '--unknown' },
            'Unknown option: --unknown',
        },
    }

    for _, case in ipairs(cases) do
        expect_position_command_failure(table.unpack(case))
    end
end

function test.position_commands_reject_invalid_lua_responses()
    mock.patch(position, 'parse_commandline', mock.func(), function()
        local output, status =
            dfhack.run_command_silent { 'orders', 'positions' }
        expect.eq(CR_FAILURE, status)
        expect.str_find(
            'orders: Lua subcommand returned an invalid response%.', output)
        expect.nil_(output:find('Failed Lua call', 1, true))
    end)

    mock.patch(position, 'parse_commandline', mock.func(false), function()
        local output, status =
            dfhack.run_command_silent { 'orders', 'positions' }
        expect.eq(CR_FAILURE, status)
        expect.str_find(
            'orders: Lua subcommand failed without an error message%.', output)
        expect.nil_(output:find('Failed Lua call', 1, true))
    end)
end

function test.position_overlay_edit_helpers()
    local hooks = position_overlay.unit_test_hooks
    expect.false_(hooks.has_modifier {})
    expect.true_(hooks.has_modifier { ctrl = true })
    expect.true_(hooks.has_modifier { shift = true })
    expect.true_(hooks.has_modifier { alt = true })
    expect.true_(hooks.has_modifier { super = true })

    local field = widgets.EditField {
        text = '12',
        on_char = hooks.accept_position_digit,
    }
    field:setFocus(true)
    hooks.select_all_field_text(field)
    expect.true_(field:onInput { _STRING = string.byte('3') })
    expect.eq('3', field.text)

    hooks.select_all_field_text(field)
    expect.true_(field.text_area.text_area:hasSelection())
    hooks.clear_field_text_selection(field)
    expect.false_(field.text_area.text_area:hasSelection())
    expect.eq('3', field.text)

    local original_search_field = hooks.get_orders_search_field()
    dfhack.with_finalize(
        function()
            position_overlay.bind_orders_search_field(original_search_field)
        end,
        function()
            local search_field = widgets.EditField { text = 'query' }
            search_field:setFocus(true)
            position_overlay.bind_orders_search_field(search_field)
            hooks.unfocus_orders_search()
            expect.false_(search_field.focus)

            local clear_count = 0
            mock.patch(position_overlay, 'clear_active_edit', function()
                clear_count = clear_count + 1
            end, function()
                local search_overlay = orders_plugin.OrdersSearchOverlay {}
                search_overlay.subviews.filter:setFocus(true)
                expect.eq(1, clear_count)
            end)
        end)
end
