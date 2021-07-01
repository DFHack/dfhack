TMP_FILE_NAME = 'tmp-test'
TMP_FILE_PATH = ('dfhack-config/orders/%s.json'):format(TMP_FILE_NAME)

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
    prefix = prefix or 'error'
    local output, result = run_orders_import(file_content)
    expect.eq(result, CR_FAILURE)
    expect.true_(output:lower():startswith(prefix), ('%s: output missing "%s"'):format(comment, prefix))
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
    ]], 'string id instead of int')
end
