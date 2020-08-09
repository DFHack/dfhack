local _ENV = mkmodule('plugins.xlsxreader')

--[[

 Native functions:

 * file_handle open_xlsx_file(filename)
 * close_xlsx_file(file_handle)
 * sheet_names list_sheets(file_handle)

 * sheet_handle open_sheet(file_handle, sheet_name)
 * close_sheet(sheet_handle)
 * cell_strings get_row(sheet_handle)

--]]

return _ENV
