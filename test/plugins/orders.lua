config.mode = 'fortress'
config.target = 'orders'

local FILE_PATH_PATTERN = 'dfhack-config/orders/%s.json'

local BACKUP_FILE_NAME = 'tmp-backup'
local BACKUP_FILE_PATH = FILE_PATH_PATTERN:format(BACKUP_FILE_NAME)

local TMP_FILE_NAME = 'tmp-test'
local TMP_FILE_PATH = FILE_PATH_PATTERN:format(TMP_FILE_NAME)

local function test_wrapper(test_fn)
    -- backup and clear active orders
    dfhack.run_command_silent{'orders', 'export', BACKUP_FILE_NAME}
    dfhack.run_command_silent{'orders', 'clear'}
    df.global.world.manager_order_next_id = 0
    return dfhack.with_finalize(
        function()
            -- clear test orders, restore original orders, remove temp files
            dfhack.run_command_silent{'orders', 'clear'}
            df.global.world.manager_order_next_id = 0
            dfhack.run_command_silent{'orders', 'import', BACKUP_FILE_NAME}
            df.global.world.manager_order_next_id =
                    #df.global.world.manager_orders
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
    local prev_num_orders = #df.global.world.manager_orders
    local output, result = run_orders_import(file_content)
    expect.eq(result, CR_OK, comment)
    expect.eq(prev_num_orders + num_expected_orders,
              #df.global.world.manager_orders, comment)
end

function check_import_fail(file_content, comment, prefix)
    comment = comment or ''
    local prev_num_orders = #df.global.world.manager_orders
    local output, result = run_orders_import(file_content)
    expect.eq(result, CR_FAILURE, ('%s: was successful'):format(comment))
    if prefix then
        expect.true_(output:lower():startswith(prefix), ('%s: "%s" missing "%s"'):format(comment, output, prefix))
    end
    expect.eq(prev_num_orders, #df.global.world.manager_orders, ('%s: number of manager orders changed'):format(comment))
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
                "reaction" : "MAKE_SOAP_FROM_TALLOW"
            }
        ]
    ]]
    check_import_success(file_content, 'valid reaction condition', 1)
    check_export_success(file_content)
end

local function get_last_order()
    return df.global.world.manager_orders[#df.global.world.manager_orders-1]
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
