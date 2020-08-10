local _ENV = mkmodule('plugins.xlsxreader')

--[[

 Native functions:

 * file_handle open_xlsx_file(filename)
 * close_xlsx_file(file_handle)
 * sheet_names list_sheets(file_handle)

 * sheet_handle open_sheet(file_handle, sheet_name)
 * close_sheet(sheet_handle)
 * cell_strings get_row(sheet_handle)

Sample usage:

    local xlsxreader = require('plugins.xlsxreader')

    local function dump_sheet(xlsx_file, sheet_name)
        print('reading sheet: '..sheet_name)
        local xlsx_sheet = xlsxreader.open_sheet(xlsx_file, sheet_name)
        dfhack.with_finalize(
            function () xlsxreader.close_sheet(xlsx_sheet) end,
            function ()
                local row_cells = xlsxreader.get_row(xlsx_sheet)
                while row_cells do
                    printall(row_cells)
                    row_cells = xlsxreader.get_row(xlsx_sheet)
                end
            end
        )
    end

    local filepath = "path/to/some_file.xlsx"
    local xlsx_file = xlsxreader.open_xlsx_file(filepath)
    dfhack.with_finalize(
        function () xlsxreader.close_xlsx_file(xlsx_file) end,
        function ()
            for _, sheet_name in ipairs(xlsxreader.list_sheets(xlsx_file)) do
                dump_sheet(xlsx_file, sheet_name)
            end
        end
    )

--]]

return _ENV
