config.mode = 'fortress'

local FILE_PATH_PATTERN = 'dfhack-config/orders/%s.json'

local BACKUP_FILE_NAME = 'tmp-backup'
local BACKUP_FILE_PATH = FILE_PATH_PATTERN:format(BACKUP_FILE_NAME)

local TMP_FILE_NAME = 'tmp-test'
local TMP_FILE_PATH = FILE_PATH_PATTERN:format(TMP_FILE_NAME)

local function test_wrapper(test_fn)
    -- save active orders
    dfhack.run_command_silent{'orders', 'export', BACKUP_FILE_NAME}
    return dfhack.with_finalize(
        function()
            -- clear test orders, restore original orders, remove temp file
            dfhack.run_command_silent{'orders', 'clear'}
            print('reimporting')
            dfhack.run_command_silent{'orders', 'import', BACKUP_FILE_NAME}
            os.remove(BACKUP_FILE_PATH)
        end,
        test_fn)
end
config.wrapper = test_wrapper

function run_orders_import(file_content)
    local f = io.open(TMP_FILE_PATH, 'w')
    f:write(file_content)
    f:close()

    return dfhack.with_finalize(
        function()
            os.remove(TMP_FILE_PATH)
        end,
        function()
            return dfhack.run_command_silent{'orders', 'import', TMP_FILE_NAME}
        end
    )
end

function check_import_success(file_content)
    local output, result = run_orders_import(file_content)
    expect.eq(result, CR_OK)
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
    check_import_success('[]')
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
