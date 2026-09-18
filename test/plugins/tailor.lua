config.mode = 'fortress'
config.target = 'tailor'

local tailor = require('plugins.tailor')

local saved_confiscate, saved_dye, saved_mats

config.wrapper = function(test_fn)
    saved_confiscate = tailor.tailor_getConfiscate()
    saved_dye = tailor.tailor_getAutomateDye()
    saved_mats = tailor.tailor_getMaterialPreferences()
    return dfhack.with_finalize(function()
        tailor.tailor_setConfiscate(saved_confiscate)
        tailor.tailor_setAutomateDye(saved_dye)
        tailor.parse_commandline('materials', table.unpack(saved_mats))
    end, test_fn)
end

function test.status_and_unknown_command()
    expect.true_(tailor.parse_commandline())
    expect.true_(tailor.parse_commandline('status'))
    expect.true_(tailor.parse_commandline('now'))
    expect.false_(tailor.parse_commandline('help'))
    expect.false_(tailor.parse_commandline('--help'))
    expect.false_(tailor.parse_commandline('bogus'))
end

function test.confiscate_on_off()
    expect.true_(tailor.parse_commandline('confiscate', 'on'))
    expect.true_(tailor.tailor_getConfiscate())
    expect.true_(tailor.parse_commandline('confiscate', 'off'))
    expect.false_(tailor.tailor_getConfiscate())
end

function test.confiscate_rejects_bad_arg()
    expect.error(function()
        tailor.parse_commandline('confiscate', 'bogus')
    end)
end

function test.dye_on_off()
    expect.true_(tailor.parse_commandline('dye', 'on'))
    expect.true_(tailor.tailor_getAutomateDye())
    expect.true_(tailor.parse_commandline('dye', 'off'))
    expect.false_(tailor.tailor_getAutomateDye())
end

function test.dye_rejects_bad_arg()
    expect.error(function()
        tailor.parse_commandline('dye', 'bogus')
    end)
end

function test.materials_ordering()
    expect.true_(tailor.parse_commandline('materials', 'silk', 'cloth'))
    expect.table_eq({'silk', 'cloth'}, tailor.tailor_getMaterialPreferences())
    expect.true_(tailor.parse_commandline('materials', 'adamantine'))
    expect.table_eq({'adamantine'}, tailor.tailor_getMaterialPreferences())
end

function test.materials_no_args_restores_default()
    -- bare 'materials' zeroes the order, which the C++ side treats as
    -- "restore defaults" (adamantine is not a default material)
    tailor.parse_commandline('materials', 'adamantine')
    expect.true_(tailor.parse_commandline('materials'))
    expect.table_eq({'silk', 'cloth', 'yarn', 'leather'},
                    tailor.tailor_getMaterialPreferences())
end
