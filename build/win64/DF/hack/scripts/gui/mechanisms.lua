-- Shows mechanisms linked to the current building.
--[====[

gui/mechanisms
==============
Lists mechanisms connected to the building, and their links. Navigating
the list centers the view on the relevant linked buildings.

.. image:: /docs/images/mechanisms.png

To exit, press :kbd:`Esc` or :kbd:`Enter`; :kbd:`Esc` recenters on
the original building, while :kbd:`Enter` leaves focus on the current
one. :kbd:`Shift`:kbd:`Enter` has an effect equivalent to pressing
:kbd:`Enter`, and then re-entering the mechanisms UI.

]====]
local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'

function listMechanismLinks(building)
    local lst = {}
    local function push(item, mode)
        if item then
            lst[#lst+1] = {
                obj = item, mode = mode,
                name = utils.getBuildingName(item)
            }
        end
    end

    push(building, 'self')

    if not df.building_actual:is_instance(building) then
        return lst
    end

    local item, tref, tgt
    for _,v in ipairs(building.contained_items) do
        item = v.item
        if df.item_trappartsst:is_instance(item) then
            tref = dfhack.items.getGeneralRef(item, df.general_ref_type.BUILDING_TRIGGER)
            if tref then
                push(tref:getBuilding(), 'trigger')
            end
            tref = dfhack.items.getGeneralRef(item, df.general_ref_type.BUILDING_TRIGGERTARGET)
            if tref then
                push(tref:getBuilding(), 'target')
            end
        end
    end

    return lst
end

MechanismList = defclass(MechanismList, guidm.MenuOverlay)

MechanismList.focus_path = 'mechanisms'

function MechanismList:init(info)
    self:assign{
        links = {}, selected = 1
    }
    self:fillList(info.building)
end

function MechanismList:fillList(building)
    local links = listMechanismLinks(building)

    self.old_viewport = self:getViewport()
    self.old_cursor = guidm.getCursorPos()

    if #links <= 1 then
        links[1].mode = 'none'
    end

    self.links = links
    self.selected = 1
end

local colors = {
    self = COLOR_CYAN, none = COLOR_CYAN,
    trigger = COLOR_GREEN, target = COLOR_GREEN
}
local icons = {
    self = 128, none = 63, trigger = 27, target = 26
}

function MechanismList:onRenderBody(dc)
    dc:clear()
    dc:seek(1,1):string("Mechanism Links", COLOR_WHITE):newline()

    for i,v in ipairs(self.links) do
        local pen = { fg=colors[v.mode], bold = (i == self.selected) }
        dc:newline(1):pen(pen):char(icons[v.mode])
        dc:advance(1):string(v.name)
    end

    local nlinks = #self.links

    if nlinks <= 1 then
        dc:newline():newline(1):string("This building has no links", COLOR_LIGHTRED)
    end

    dc:newline():newline(1):pen(COLOR_WHITE)
    dc:key('LEAVESCREEN'):string(": Back, ")
    dc:key('SELECT'):string(": Switch"):newline(1)
    dc:key_string('LEAVESCREEN_ALL', "Exit to map")
end

function MechanismList:changeSelected(delta)
    if #self.links <= 1 then return end
    self.selected = 1 + (self.selected + delta - 1) % #self.links
    self:selectBuilding(self.links[self.selected].obj)
end

function MechanismList:onInput(keys)
    if keys.SECONDSCROLL_UP then
        self:changeSelected(-1)
    elseif keys.SECONDSCROLL_DOWN then
        self:changeSelected(1)
    elseif keys.LEAVESCREEN or keys.LEAVESCREEN_ALL then
        self:dismiss()
        if self.selected ~= 1 and not keys.LEAVESCREEN_ALL then
            self:selectBuilding(self.links[1].obj, self.old_cursor, self.old_viewport)
        end
    elseif keys.SELECT_ALL then
        if self.selected > 1 then
            self:fillList(self.links[self.selected].obj)
        end
    elseif keys.SELECT then
        self:dismiss()
    elseif self:simulateViewScroll(keys) then
        return
    end
end

if not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/QueryBuilding/Some') then
    qerror("This script requires a mechanism-linked building to be selected in 'q' mode")
end

local list = MechanismList{ building = df.global.world.selected_building }
list:show()
list:changeSelected(1)
