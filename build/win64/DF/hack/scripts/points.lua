-- Set available points at the embark screen
-- http://www.bay12forums.com/smf/index.php?topic=135506.msg4925005#msg4925005
--[====[

points
======
Sets available points at the embark screen to the specified number.  Eg.
``points 1000000`` would allow you to buy everything, or ``points 0`` would
make life quite difficult.

]====]

if dfhack.isWorldLoaded() then
    df.global.world.worldgen.worldgen_parms.embark_points = tonumber(...)
    local scr = dfhack.gui.getCurViewscreen()
    if df.viewscreen_setupdwarfgamest:is_instance(scr) then
        local scr = scr --as:df.viewscreen_setupdwarfgamest
        scr.points_remaining = tonumber(...)
    end
else
    qerror('no world loaded - cannot modify embark points')
end
