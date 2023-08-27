local _ENV = mkmodule('plugins.xlsxreader')

XlsxioSheetReader = defclass(XlsxioSheetReader, nil)
XlsxioSheetReader.ATTRS{
    -- handle to the open sheet. required.
    sheet_handle = DEFAULT_NIL,
}

function XlsxioSheetReader:init()
    if not self.sheet_handle then
        error('XlsxSheetReader: sheet_handle is required')
    end
end

function XlsxioSheetReader:close()
    close_sheet(self.sheet_handle)
    self.sheet_handle = nil
end

function XlsxioSheetReader:get_row(max_tokens)
    return get_row(self.sheet_handle, max_tokens)
end

XlsxioReader = defclass(XlsxioReader, nil)
XlsxioReader.ATTRS{
    -- full or relative path to the target .xlsx file. required.
    filepath = DEFAULT_NIL,
}

function open(filepath)
    return XlsxioReader{filepath=filepath}
end

function XlsxioReader:init()
    if not self.filepath then
        error('XlsxReader: filepath is required')
    end
    self.xlsx_handle = open_xlsx_file(self.filepath)
    if not self.xlsx_handle then
        qerror(('failed to open "%s"'):format(self.filepath))
    end
end

function XlsxioReader:close()
    close_xlsx_file(self.xlsx_handle)
    self.xlsx_handle = nil
end

function XlsxioReader:list_sheets()
    return list_sheets(self.xlsx_handle)
end

-- if sheet_name is empty or nil, opens the first sheet
function XlsxioReader:open_sheet(sheet_name)
    local sheet_handle = open_sheet(self.xlsx_handle, sheet_name)
    return XlsxioSheetReader{sheet_handle=sheet_handle}
end

return _ENV
