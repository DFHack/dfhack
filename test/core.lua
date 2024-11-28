config.target = 'core'

local function clean_path(p)
    -- todo: replace with dfhack.filesystem call?
    return p:gsub('\\', '/'):gsub('//', '/'):gsub('/$', '')
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
    local declared_arr_size = df.global.global_table:sizeof() // elem_size
    for i=0,declared_arr_size-1 do
        expect.true_(df._displace(df.global.global_table[0], i, elem_size).address,
            'nil address found in middle of global_table at idx ' .. i)
    end
    if df._displace(df.global.global_table[0], declared_arr_size, elem_size).address then
        local idx_of_nil
        for i=declared_arr_size+1,declared_arr_size*2 do
            if not df._displace(df.global.global_table[0], i, elem_size).address then
                idx_of_nil = i
                break
            end
        end
        expect.fail('nil *not* found at end of declared global-table (idx ' .. declared_arr_size .. '); table size should be: ' .. idx_of_nil)
    end
end
