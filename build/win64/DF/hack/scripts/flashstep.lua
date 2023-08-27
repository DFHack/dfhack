--Teleports adventurer to cursor
--[====[

flashstep
=========
A hotkey-friendly teleport that places your adventurer where your cursor is.

]====]

function flashstep()
    local unit = df.global.world.units.active[0]
    if df.global.adventure.menu ~= df.ui_advmode_menu.Look then
        qerror("No [l] cursor located! You kinda need it for this script.")
    end
    dfhack.units.teleport(unit, xyz2pos(pos2xyz(df.global.cursor)))
    dfhack.maps.getTileBlock(unit.pos).designation[unit.pos.x%16][unit.pos.y%16].hidden = false
end

flashstep()
