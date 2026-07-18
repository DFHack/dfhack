config.target = 'core'

local function clean_path(p)
    return dfhack.filesystem.canonicalize(p)
end

local fs = dfhack.filesystem
local old_cwd = clean_path(fs.getcwd())
local function restore_cwd()
    fs.chdir(old_cwd)
end

function test.getDFPath()
    expect.eq(clean_path(dfhack.getDFPath()), old_cwd)
end

function test.get_initial_cwd()
    expect.eq(clean_path(dfhack.filesystem.get_initial_cwd()), clean_path(dfhack.getDFPath()))
end

function test.getDFPath_chdir()
    dfhack.with_finalize(restore_cwd, function()
        fs.chdir('data')
        expect.eq(clean_path(dfhack.getDFPath()), old_cwd)
        expect.ne(clean_path(dfhack.getDFPath()), clean_path(fs.getcwd()))
    end)
end

function test.getHackPath()
    expect.eq(clean_path(dfhack.getHackPath()), clean_path(dfhack.getDFPath() .. '/hack/'))
end

function test.getHackPath_chdir()
    dfhack.with_finalize(restore_cwd, function()
        fs.chdir('hack')
        expect.eq(clean_path(dfhack.getHackPath()), clean_path(old_cwd .. '/hack/'))
        expect.eq(clean_path(dfhack.getHackPath()), clean_path(fs.getcwd()))
    end)
end

-- validates that the size in df.globals.xml is correct
function test.global_table_size()
    local elem_size = df.global_table_entry:sizeof()
    local actual_arr_size = 0
    while df._displace(df.global.global_table[0], actual_arr_size, elem_size).address do
        actual_arr_size = actual_arr_size + 1
    end
    local declared_arr_size = df.global.global_table:sizeof() // elem_size
    expect.eq(declared_arr_size, actual_arr_size,
        ('global_table size mismatch: expected: %d, actual: %d'):format(declared_arr_size, actual_arr_size))
end
