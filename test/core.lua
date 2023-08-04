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
