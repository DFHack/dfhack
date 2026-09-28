local _ENV = mkmodule('plugins.sort.stressicons')

local dialogs = require('gui.dialogs')
local overlay = require('plugins.overlay')

-- indexed by stress category (0-6, where 0 is most stressed)
local STRESS_CATEGORY_NAMES = {
    [0]='Miserable', 'Unhappy', 'Displeased', 'Content', 'Pleased', 'Happy',
    'Ecstatic',
}

-- single-unit equivalent of the dfhack.units.getCitizens(true, true) filter
local function is_listable(unit)
    return dfhack.units.isCitizen(unit, true) and dfhack.units.isActive(unit)
end

local function show_stress_units(category)
    local choices = {}
    -- living, on-map citizens only (excluding residents), matching the game's
    -- own stress icon counts
    for _,unit in ipairs(dfhack.units.getCitizens(true, true)) do
        if dfhack.units.getStressCategory(unit) == category then
            table.insert(choices, {
                text=dfhack.units.getReadableName(unit),
                unit_id=unit.id,
            })
        end
    end
    local name = STRESS_CATEGORY_NAMES[category]
    if #choices == 0 then
        dialogs.showMessage('Citizens: ' .. name,
            ('No citizens are currently %s.'):format(name:lower()))
        return
    end
    dialogs.ListBox{
        frame_title='Citizens: ' .. name,
        with_filter=true,
        choices=choices,
        on_select=function(_, choice)
            -- re-resolve in case the unit died or left the map while the list
            -- was open; df.unit.find still resolves dead units
            local unit = df.unit.find(choice.unit_id)
            if not unit or not is_listable(unit) then return end
            dfhack.gui.revealInDwarfmodeMap(unit.pos.x, unit.pos.y, unit.pos.z, true, true)
        end,
    }:show()
end

-- invisible hotspot over the stress icons in the top bar; clicking an icon
-- opens a list of the citizens in that stress category
StressIconsOverlay = defclass(StressIconsOverlay, overlay.OverlayWidget)
StressIconsOverlay.ATTRS{
    desc='Click a stress icon in the top bar to list the citizens in that stress category.',
    default_pos={x=1, y=1},
    default_enabled=true,
    viewscreens={'dwarfmode/Default'},
    overlay_onupdate_max_freq_seconds=1,
    frame={w=1, h=1},
}

function StressIconsOverlay:overlay_onupdate()
    self.icon_rects = nil
    local dimx = df.global.gps.dimx
    -- the 'Pop' label immediately precedes the stress icons in the top bar;
    -- scan right to left so a fortress name containing 'Pop' doesn't win
    local pop_x
    for x = math.min(dimx-3, 200), 0, -1 do
        if dfhack.screen.readTile(x, 0).ch == 80 and         -- 'P'
                dfhack.screen.readTile(x+1, 0).ch == 111 and -- 'o'
                dfhack.screen.readTile(x+2, 0).ch == 112 then -- 'p'
            pop_x = x
            break
        end
    end
    if not pop_x then return end
    -- the icons are drawn in 3-column cells right of the 'Pop' block, with the
    -- happiest category first
    local rects = {}
    for i = 0, 6 do
        local x1 = pop_x + 4 + i * 3
        if x1 + 1 >= dimx then break end
        table.insert(rects, {x1=x1, y1=0, x2=x1+2, y2=2, category=6-i})
    end
    self.icon_rects = rects
    self.frame = {l=pop_x+4, t=0, w=#rects*3, h=3}
end

function StressIconsOverlay:onInput(keys)
    if not keys._MOUSE_L or not self.icon_rects then return false end
    local mx, my = dfhack.screen.getMousePos()
    if not mx then return false end
    for _,rect in ipairs(self.icon_rects) do
        if my >= rect.y1 and my <= rect.y2 and
                mx >= rect.x1 and mx <= rect.x2 then
            show_stress_units(rect.category)
            return true
        end
    end
    return false
end

return _ENV
