config.mode = 'fortress'
config.target = 'pet-uncapper'

local function status_output()
    local output, status = dfhack.run_command_silent('pet-uncapper', 'status')
    expect.eq(CR_OK, status)
    return output
end

local function get_config()
    local output = status_output()
    local cap = tonumber(output:match('population cap per species: (%-?%d+)'))
    local freq = tonumber(output:match('updating pregnancies every (%-?%d+) ticks'))
    local pregtime = tonumber(output:match('pregancies last (%-?%d+) ticks'))
    return cap, freq, pregtime
end

function test.status_reports_config()
    local cap, freq, pregtime = get_config()
    expect.ne(nil, cap)
    expect.ne(nil, freq)
    expect.ne(nil, pregtime)
end

function test.set_cap()
    local cap = get_config()
    local _, status = dfhack.run_command_silent('pet-uncapper', 'cap', '42')
    expect.eq(CR_OK, status)
    expect.eq(42, get_config())
    dfhack.run_command_silent('pet-uncapper', 'cap', tostring(cap))
end

function test.set_every()
    local _, freq = get_config()
    local _, status = dfhack.run_command_silent('pet-uncapper', 'every', '77')
    expect.eq(CR_OK, status)
    local _, new_freq = get_config()
    expect.eq(77, new_freq)
    dfhack.run_command_silent('pet-uncapper', 'every', tostring(freq))
end

function test.set_pregtime()
    local _, _, pregtime = get_config()
    local _, status = dfhack.run_command_silent('pet-uncapper', 'pregtime', '999')
    expect.eq(CR_OK, status)
    local _, _, new_pregtime = get_config()
    expect.eq(999, new_pregtime)
    dfhack.run_command_silent('pet-uncapper', 'pregtime', tostring(pregtime))
end

function test.negative_values_clamp_to_zero()
    local cap = get_config()
    local _, status = dfhack.run_command_silent('pet-uncapper', 'cap', '-5')
    expect.eq(CR_OK, status)
    expect.eq(0, get_config())
    dfhack.run_command_silent('pet-uncapper', 'cap', tostring(cap))
end

function test.bad_command()
    local _, status = dfhack.run_command_silent('pet-uncapper', 'bogus', '1')
    expect.eq(CR_WRONG_USAGE, status)
end

function test.enable_disable()
    dfhack.run_command_silent('disable', 'pet-uncapper')
    local output, status = dfhack.run_command_silent('pet-uncapper')
    expect.eq(CR_OK, status)
    expect.str_find('pet%-uncapper is not enabled', output)

    dfhack.run_command_silent('enable', 'pet-uncapper')
    output = status_output()
    expect.str_find('population cap per species:', output)
    dfhack.run_command_silent('disable', 'pet-uncapper')
end
