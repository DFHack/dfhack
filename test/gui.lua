function test.getCurViewscreen()
    local scr = dfhack.gui.getCurViewscreen()
    local scr2 = df.global.gview.view
    for i = 1, 100 do
        if scr2.child then
            scr2 = scr2.child
        else
            break
        end
    end
    expect.eq(scr, scr2)
end

function test.getViewscreenByType()
    local scr = dfhack.gui.getCurViewscreen()
    local scr2 = dfhack.gui.getViewscreenByType(scr._type)
    expect.eq(scr, scr2)

    local bad_type = df.viewscreen_titlest
    if scr._type == bad_type then
        bad_type = df.viewscreen_optionst
    end
    local scr_bad = dfhack.gui.getViewscreenByType(bad_type)
    expect.eq(scr_bad, nil)
end
