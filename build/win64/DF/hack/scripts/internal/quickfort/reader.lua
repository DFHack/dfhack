-- encapsulates file I/O (well, really just file I)
--@ module = true

if not dfhack_flags.module then
    qerror('this script cannot be called directly')
end

local xlsxreader = require('plugins.xlsxreader')

local function chomp(line)
    return line and line:gsub('[\r\n]*$', '') or nil
end

-- abstract base class for the Reader class tree
Reader = defclass(Reader, nil)
Reader.ATTRS{
    -- relative or absolute filesystem path of target file
    filepath = DEFAULT_NIL,
    -- file open function to use; set in subclasses
    open_fn = DEFAULT_NIL,
}

function Reader:init()
    self.source = self.open_fn(self.filepath)
    if not self.source then
        qerror(string.format('failed to open "%s"', self.filepath))
    end
end

function Reader:cleanup()
    self.source:close()
end

-- causes the next call to get_next_row to re-return the value from the
-- previous time it was called
function Reader:redo()
    self.redo_requested = true
end

function Reader:get_next_row()
    if self.redo_requested then
        self.redo_requested = false
        return self.prev_row
    end
    self.prev_row = self:get_next_row_raw()
    return self.prev_row
end

TextReader = defclass(TextReader, Reader)
TextReader.ATTRS{
    -- file open function to use
    open_fn = io.open,
}

function TextReader:get_next_row_raw()
    return chomp(self.source:read())
end

CsvReader = defclass(CsvReader, TextReader)
CsvReader.ATTRS{
    -- Tokenizer function with the following signature:
    --  (get_next_line_fn, max_cols). Must return a list of strings. Required.
    row_tokenizer = DEFAULT_NIL,
    -- how many columns to read per row; 0 means all
    max_cols = 0,
}

function CsvReader:init()
    if not self.row_tokenizer then
        error('cannot initialize a CsvReader without a row_tokenizer.')
    end
end

function CsvReader:get_next_row_raw()
    return self.row_tokenizer(
        function() return CsvReader.super.get_next_row_raw(self) end,
        self.max_cols)
end

XlsxReader = defclass(XlsxReader, Reader)
XlsxReader.ATTRS{
    -- name of xlsx sheet to open, nil means first sheet
    sheet_name = DEFAULT_NIL,
    -- how many columns to read per row; 0 means all
    max_cols = 0,
    -- file open function to use
    open_fn = xlsxreader.open,
}

function XlsxReader:init()
    self.sheet_reader = self.source:open_sheet(self.sheet_name)
end

function XlsxReader:cleanup()
    self.sheet_reader:close()
    XlsxReader.super.cleanup(self)
end

function XlsxReader:get_next_row_raw()
    local tokens = self.sheet_reader:get_row(self.max_cols)
    if not tokens then return nil end
    -- raw numbers can get turned into floats. let's turn them back into ints
    for i,token in ipairs(tokens) do
        local token = tonumber(token)
        if token then tokens[i] = tostring(math.floor(token)) end
    end
    return tokens
end

if dfhack.internal.IN_TEST then
    unit_test_hooks = {
        chomp=chomp,
        TextReader=TextReader,
        CsvReader=CsvReader,
        XlsxReader=XlsxReader,
    }
end
