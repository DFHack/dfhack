config = {
    mode = 'fortress',
}

local gui = require('gui')
local guidm = require('gui.dwarfmode')

function test.enterSidebarMode()
    expect.error_match('Invalid or unsupported sidebar mode',
                       function() guidm.enterSidebarMode('badmode') end)
    expect.error_match('Invalid or unsupported sidebar mode',
                       function() guidm.enterSidebarMode(
                                        df.ui_sidebar_mode.OrdersRefuse) end)

    expect.error_match('must be a positive number',
                       function() guidm.enterSidebarMode(0, 'gg') end)
    expect.error_match('must be a positive number',
                       function() guidm.enterSidebarMode(0, 0) end)
    expect.error_match('must be a positive number',
                       function() guidm.enterSidebarMode(0, '0') end)
    expect.error_match('must be a positive number',
                       function() guidm.enterSidebarMode(0, -1) end)
    expect.error_match('must be a positive number',
                       function() guidm.enterSidebarMode(0, '-1') end)

    -- Simulate not being able to get to default from a screen via mocks. This
    -- failure can actually happen in-game in some situations, such as when
    -- naming a building with ctrl-N (no way to cancel changes).
    mock.patch({{dfhack.gui, 'getFocusString', mock.func()},
                {gui, 'simulateInput', mock.func()}},
               function()
                   expect.error_match('Unable to get into target sidebar mode',
                    function()
                        guidm.enterSidebarMode(df.ui_sidebar_mode.Default)
                    end)
               end)

    -- verify expected starting state
    expect.eq(df.ui_sidebar_mode.Default, df.global.plotinfo.main.mode)
    expect.eq('dwarfmode/Default', dfhack.gui.getCurFocus(true))

    -- get into the orders screen
    gui.simulateInput(dfhack.gui.getCurViewscreen(true), 'D_JOBLIST')
    gui.simulateInput(dfhack.gui.getCurViewscreen(true), 'UNITJOB_MANAGER')
    expect.eq(df.ui_sidebar_mode.Default, df.global.plotinfo.main.mode)
    expect.eq('jobmanagement/Main', dfhack.gui.getCurFocus(true))

    -- get back into default from some deep screen
    guidm.enterSidebarMode(df.ui_sidebar_mode.Default)
    expect.eq(df.ui_sidebar_mode.Default, df.global.plotinfo.main.mode)
    expect.eq('dwarfmode/Default', dfhack.gui.getCurFocus(true))

    -- move from default to some other mode
    guidm.enterSidebarMode(df.ui_sidebar_mode.QueryBuilding)
    expect.eq(df.ui_sidebar_mode.QueryBuilding, df.global.plotinfo.main.mode)
    expect.str_find('^dwarfmode/QueryBuilding', dfhack.gui.getCurFocus(true))

    -- move between non-default modes
    guidm.enterSidebarMode(df.ui_sidebar_mode.LookAround)
    expect.eq(df.ui_sidebar_mode.LookAround, df.global.plotinfo.main.mode)
    expect.str_find('^dwarfmode/LookAround', dfhack.gui.getCurFocus(true))

    -- get back into default from a supported mode
    guidm.enterSidebarMode(df.ui_sidebar_mode.Default)
    expect.eq(df.ui_sidebar_mode.Default, df.global.plotinfo.main.mode)
    expect.eq('dwarfmode/Default', dfhack.gui.getCurFocus(true))

    -- verify that all supported modes lead where we say they'll go
    for k,v in pairs(guidm.SIDEBAR_MODE_KEYS) do
        guidm.enterSidebarMode(k)
        expect.eq(k, df.global.plotinfo.main.mode, df.ui_sidebar_mode[k])
    end
    -- end test back in default so the test harness doesn't have to autocorrect
    guidm.enterSidebarMode(df.ui_sidebar_mode.Default)
end
