config.mode = 'fortress'
config.target = 'getplants'

function test.lists_plant_ids()
    local output, status = dfhack.run_command_silent('getplants')
    expect.eq(CR_OK, status)
    expect.str_find('Valid plant IDs:', output)
    -- listings are tagged by growth form
    expect.true_(output:find('%(tree%)') ~= nil or output:find('%(shrub%)') ~= nil)
end

function test.trees_only_listing()
    local output, status = dfhack.run_command_silent('getplants', '-t')
    expect.eq(CR_OK, status)
    expect.str_find('%(tree%)', output)
    expect.nil_(output:find('%(shrub%)'))
end

function test.shrubs_only_listing()
    local output, status = dfhack.run_command_silent('getplants', '-s')
    expect.eq(CR_OK, status)
    expect.str_find('%(shrub%)', output)
    expect.nil_(output:find('%(tree%)'))
end

function test.conflicting_type_options()
    local _, status = dfhack.run_command_silent('getplants', '-t', '-s')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.conflicting_farm_options()
    local _, status = dfhack.run_command_silent('getplants', '-t', '-f')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.conflicting_select_options()
    local _, status = dfhack.run_command_silent('getplants', '-a', '-x')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.invalid_plant_id()
    local output, status = dfhack.run_command_silent('getplants', 'NOT_A_REAL_PLANT')
    expect.eq(CR_FAILURE, status)
    expect.str_find('Invalid plant ID%(s%):', output)
end

function test.all_with_ids_is_wrong_usage()
    local _, status = dfhack.run_command_silent('getplants', '-a', 'DWARF')
    expect.eq(CR_WRONG_USAGE, status)
end
