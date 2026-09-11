config.mode = 'fortress'
config.target = 'orders'

local work_order_list = require('plugins.orders.work_order_list')
local position = require('plugins.orders.position')
local position_overlay = require('plugins.orders.position_overlay')
local gui = require('gui')
local json = require('json')

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

function test.work_order_list_geometry()
    local hooks = work_order_list.unit_test_hooks

    expect.eq(10, hooks.calculate_list_start_y(154))
    expect.eq(8, hooks.calculate_list_start_y(155))
    expect.eq(4, hooks.calculate_viewport_size(30, 8))

    local viewport_start, viewport_end =
        hooks.calculate_visible_order_indices(0, 4, 0)
    expect.eq(0, viewport_start)
    expect.eq(-1, viewport_end)

    viewport_start, viewport_end =
        hooks.calculate_visible_order_indices(10, 4, 2)
    expect.eq(2, viewport_start)
    expect.eq(5, viewport_end)

    viewport_start, viewport_end =
        hooks.calculate_visible_order_indices(10, 4, 8)
    expect.eq(6, viewport_start)
    expect.eq(9, viewport_end)

    viewport_start, viewport_end =
        hooks.calculate_visible_order_indices(10, 20, 8)
    expect.eq(0, viewport_start)
    expect.eq(9, viewport_end)
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

local function make_order(id)
    return { id = id }
end

local function make_vector()
    return setmetatable({ values = {
        make_order(10),
        make_order(20),
        make_order(30),
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
    local order, error_message =
        position.unit_test_hooks.move_in_vector(vector, 1, 3)
    if not order then expect.fail(error_message) return end

    expect.eq(first, order)
    expect.eq(first, vector[2])
    expect.table_eq({ 20, 30, 10 }, get_vector_ids(vector))

    order, error_message =
        position.unit_test_hooks.move_in_vector(vector, 3, 1)
    if not order then expect.fail(error_message) return end
    expect.eq(first, order)
    expect.eq(first, vector[0])
    expect.table_eq({ 10, 20, 30 }, get_vector_ids(vector))
end

function test.position_move_vector_rejects_invalid_and_noop_is_atomic()
    local invalid_values = { 'one', '1.5', 0 }
    for _, value in ipairs(invalid_values) do
        local vector = make_vector()
        local result = position.unit_test_hooks.move_in_vector(vector, 1, value)
        expect.nil_(result, ('value %q should be rejected'):format(value))
        expect.table_eq({ 10, 20, 30 }, get_vector_ids(vector))
    end

    local vector = make_vector()
    local second = vector[1]
    local order, error_message =
        position.unit_test_hooks.move_in_vector(vector, 2, 2)
    if not order then expect.fail(error_message) return end
    expect.eq(second, order)
    expect.table_eq({ 10, 20, 30 }, get_vector_ids(vector))

    order = position.unit_test_hooks.move_in_vector(vector, 1, 4)
    expect.nil_(order)
    expect.table_eq({ 10, 20, 30 }, get_vector_ids(vector))

    vector = make_vector()
    local ok, result = pcall(position.unit_test_hooks.move_in_vector,
        vector, 'one', 2)
    expect.true_(ok, 'a malformed source position should not throw')
    expect.nil_(result)
    expect.table_eq({ 10, 20, 30 }, get_vector_ids(vector))
end

function test.position_overlay_focus_lifecycle()
    local position_widget = position_overlay.PositionOverlay {}
    local screen_rect = gui.mkdims_wh(0, 0, dfhack.screen.getWindowSize())
    position_widget:updateLayout(gui.ViewRect { rect = screen_rect })

    local first_row = position_widget.position_rows[1]
    local first_field = position_widget.position_fields[1]
    position_widget.slot_order_ids[1] = 123
    first_field:setFocus(true)
    expect.eq(123, position_widget.edit.order_id)

    position_overlay.clear_active_edit()
    expect.nil_(position_widget.edit)
    expect.false_(first_field.focus)

    first_field:setFocus(true)
    position_widget:set_positions_visible(false)
    expect.nil_(position_widget.edit)
    expect.false_(first_field.focus)
    expect.false_(first_row.visible())
end

function test.position_overlay_edit_follows_reordered_order()
    local orders_to_import = {}
    for _, id in ipairs { 10, 20, 30 } do
        table.insert(orders_to_import, {
            amount_left = 1,
            amount_total = 1,
            frequency = 'OneTime',
            id = id,
            is_active = false,
            is_validated = true,
            job = 'ConstructTable',
        })
    end
    local output, status = run_orders_import(json.encode(orders_to_import))
    expect.eq(CR_OK, status, output)

    local work_orders =
        df.global.game.main_interface.info.work_orders
    local original_scroll_position =
        work_orders.scroll_position_work_orders
    dfhack.with_finalize(
        function()
            work_orders.scroll_position_work_orders = original_scroll_position
        end,
        function()
            work_orders.scroll_position_work_orders = 0
            local position_widget = position_overlay.PositionOverlay {}
            local screen_rect =
                gui.mkdims_wh(0, 0, dfhack.screen.getWindowSize())
            position_widget:updateLayout(gui.ViewRect { rect = screen_rect })
            position_widget:sync_position_fields()

            local orders = df.global.world.manager_orders.all
            local edited_order = orders[0]
            local first_field = position_widget.position_fields[1]
            first_field:setFocus(true)
            first_field:setText('3')

            local moved_order, error_message = position.move(1, 2)
            if not moved_order then expect.fail(error_message) return end
            position_widget:sync_position_fields()

            local selected_field = position_widget:get_selected_field()
            expect.eq(edited_order, orders[1])
            expect.eq(position_widget.position_fields[2], selected_field)
            expect.eq('3', selected_field.text)
            expect.true_(selected_field.focus)
        end)
end
