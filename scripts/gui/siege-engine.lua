-- Front-end for the siege engine plugin.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local dlg = require 'gui.dialogs'

local plugin = require 'plugins.siege-engine'

SiegeEngine = defclass(SiegeEngine, guidm.MenuOverlay)

SiegeEngine.focus_path = 'siege-engine'

function SiegeEngine:init(building)
    self:init_fields{
        building = building,
        center = utils.getBuildingCenter(building),
        links = {}, selected = 1
    }
    guidm.MenuOverlay.init(self)
    return self
end

function SiegeEngine:onShow()
    guidm.MenuOverlay.onShow(self)

    self.old_cursor = guidm.getCursorPos()
    self.old_viewport = self:getViewport()
end

function SiegeEngine:onDestroy()
    guidm.setCursorPos(self.old_cursor)
    self:getViewport(self.old_viewport):set()
end

function SiegeEngine:onRenderBody(dc)
    dc:clear()
    dc:seek(1,1):string(utils.getBuildingName(self.building), COLOR_WHITE):newline()

    local view = self:getViewport()
    local map = self.df_layout.map

    plugin.paintAimScreen(
        self.building, view:getPos(),
        xy2pos(map.x1, map.y1), view:getSize()
    )

    dc:newline():newline(1):pen(COLOR_WHITE)
    dc:string("Esc", COLOR_LIGHTGREEN):string(": Back, ")
    dc:string("Enter", COLOR_LIGHTGREEN):string(": Switch")
end

function SiegeEngine:onInput(keys)
    if keys.LEAVESCREEN then
        self:dismiss()
    elseif self:simulateCursorMovement(keys, self.center) then
        return
    end
end

if not string.find(dfhack.gui.getCurFocus(), 'dwarfmode/QueryBuilding/Some') then
    qerror("This script requires the main dwarfmode view in 'q' mode")
end

local building = df.global.world.selected_building

if not df.building_siegeenginest:is_instance(building) then
    qerror("A siege engine must be selected")
end

local list = mkinstance(SiegeEngine):init(df.global.world.selected_building)
list:show()
