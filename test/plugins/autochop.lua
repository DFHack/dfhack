config.mode = 'fortress'
config.target = 'autochop'

local autochop = require('plugins.autochop')

local function with_targets(fn)
    local old_max, old_min = autochop.autochop_getTargets()
    return dfhack.with_finalize(
        function()
            autochop.autochop_setTargets(old_max, old_min)
            autochop.autochop_undesignate()
        end,
        fn)
end

local function with_burrow(fn)
    local burrows = df.global.plotinfo.burrows
    local b = df.burrow:new()
    b.id = burrows.next_id
    burrows.next_id = burrows.next_id + 1
    b.name = 'DFHACK_TEST_AUTOCHOP'
    burrows.list:insert('#', b)
    return dfhack.with_finalize(
        function()
            for i = #burrows.list - 1, 0, -1 do
                if burrows.list[i] == b then
                    burrows.list:erase(i)
                end
            end
        end,
        function() fn(b) end)
end

function test.target_command_sets_max_and_min()
    with_targets(function()
        expect.true_(autochop.parse_commandline('target', '50', '40'))
        local max, min = autochop.autochop_getTargets()
        expect.eq(50, max)
        expect.eq(40, min)
    end)
end

function test.target_defaults_min_to_80_percent_of_max()
    with_targets(function()
        expect.true_(autochop.parse_commandline('target', '200'))
        local max, min = autochop.autochop_getTargets()
        expect.eq(200, max)
        expect.eq(160, min)
    end)
end

function test.target_rejects_invalid_values()
    expect.error_match('non%-negative integer',
        function() autochop.setTargets('-1') end)
    expect.error_match('non%-negative integer',
        function() autochop.setTargets('notanumber') end)
    expect.error_match('between 0 and the maximum',
        function() autochop.setTargets('50', '60') end)
    expect.error_match('between 0 and the maximum',
        function() autochop.setTargets('50', '-1') end)
end

function test.chop_and_nochop_toggle_burrow()
    with_burrow(function(b)
        expect.true_(autochop.parse_commandline('chop', b.name))
        expect.eq(1, autochop.autochop_getBurrowConfig(b.id).chop)
        expect.true_(autochop.parse_commandline('nochop', b.name))
        expect.eq(0, autochop.autochop_getBurrowConfig(b.id).chop)
    end)
end

function test.clearcut_and_noclearcut_toggle_burrow()
    with_burrow(function(b)
        expect.true_(autochop.parse_commandline('clearcut', b.name))
        expect.eq(1, autochop.autochop_getBurrowConfig(b.id).clearcut)
        expect.true_(autochop.parse_commandline('noclear', b.name))
        expect.eq(0, autochop.autochop_getBurrowConfig(b.id).clearcut)
    end)
end

function test.protect_sets_each_listed_type()
    with_burrow(function(b)
        expect.true_(autochop.parse_commandline('protect', 'brewable,edible', b.name))
        local config = autochop.autochop_getBurrowConfig(b.id)
        expect.eq(1, config.protect_brewable)
        expect.eq(1, config.protect_edible)
        expect.eq(0, config.protect_cookable)
        expect.true_(autochop.parse_commandline('unprotect', 'edible', b.name))
        config = autochop.autochop_getBurrowConfig(b.id)
        expect.eq(1, config.protect_brewable)
        expect.eq(0, config.protect_edible)
    end)
end

function test.burrow_command_requires_a_burrow()
    expect.error_match('no target burrows',
        function() autochop.parse_commandline('chop') end)
    expect.error_match('burrow not found',
        function() autochop.parse_commandline('chop', 'DFHACK_NONEXISTENT_BURROW') end)
end

function test.help_and_unknown_command_return_false()
    expect.false_(autochop.parse_commandline('help'))
    expect.false_(autochop.parse_commandline('--help'))
    expect.false_(autochop.parse_commandline('bogus'))
end

function test.status_and_no_command_return_true()
    expect.true_(autochop.parse_commandline())
    expect.true_(autochop.parse_commandline('status'))
end
