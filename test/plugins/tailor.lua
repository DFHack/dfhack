config.mode = 'fortress'
config.target = 'tailor'

function test.status()
    local output = dfhack.run_command_silent('tailor')
    expect.str_find('tailor is', output)
    expect.str_find('confiscating tattered clothing', output)
end

function test.wrong_command_shows_usage()
    local output, status = dfhack.run_command_silent('tailor', 'bogus')
    expect.true_(status ~= 0)
end

local CLOTHING_JOBS = {
    [df.job_type.MakeArmor] = true,
    [df.job_type.MakeHelm] = true,
    [df.job_type.MakeGloves] = true,
    [df.job_type.MakeShoes] = true,
    [df.job_type.MakePants] = true,
}

local function pending_clothing_items()
    local total = 0
    for _, o in ipairs(df.global.world.manager_orders.all) do
        if CLOTHING_JOBS[o.job_type] then
            total = total + o.amount_left
        end
    end
    return total
end

function test.repeated_run_does_not_duplicate_orders()
    dfhack.run_command_silent('tailor', 'now')
    local after_first = pending_clothing_items()
    dfhack.run_command_silent('tailor', 'now')
    local after_second = pending_clothing_items()
    -- pending orders must count toward needs, so an immediate second run
    -- cannot commit more items than the first run did
    expect.eq(after_first, after_second,
              ('second tailor run added %d duplicate pending items'):format(
                  after_second - after_first))
end
