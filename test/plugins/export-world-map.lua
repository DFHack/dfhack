config.mode = 'fortress'
config.target = 'export-world-map'

local export_dir = dfhack.getConfigPath() .. '/map-export'

local function sites_csv()
    local f = io.open(export_dir .. '/sites.csv')
    if not f then return nil end
    local content = f:read('*a')
    f:close()
    return content
end

function test.sites_export_writes_csv()
    return dfhack.with_finalize(function()
        os.remove(export_dir .. '/sites.csv')
    end, function()
        local _, status = dfhack.run_command_silent('export-world-map', 'sites')
        expect.eq(CR_OK, status)
        local content = sites_csv()
        expect.ne(nil, content, 'sites.csv was not written')
        expect.true_(#content > 0, 'sites.csv is empty')
    end)
end

function test.unknown_topic_is_wrong_usage()
    local _, status = dfhack.run_command_silent('export-world-map', 'bogus-topic')
    expect.eq(CR_WRONG_USAGE, status)
end
