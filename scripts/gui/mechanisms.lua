-- Shows mechanisms linked to the current building.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'

function getBuildingName(building)
    return utils.call_with_string(building, 'getName')
end

function getBuildingCenter(building)
    return xyz2pos(building.centerx, building.centery, building.z)
end

function listMechanismLinks(building)
    local lst = {}
    local function push(item, mode)
        if item then
            lst[#lst+1] = {
                obj = item, mode = mode,
                name = getBuildingName(item),
                center = getBuildingCenter(item)
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

function MechanismList:init(building)
    self:init_fields{
        links = {}, selected = 1
    }
    guidm.MenuOverlay.init(self)
    self:fillList(building)
    return self
end

function MechanismList:fillList(building)
    local links = listMechanismLinks(building)

    links[1].viewport = self:getViewport()
    links[1].cursor = guidm.getCursorPos()
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
    dc:string("Esc", COLOR_LIGHTGREEN):string(": Back, ")
    dc:string("Enter", COLOR_LIGHTGREEN):string(": Switch")
end

function MechanismList:zoomToLink(link,back)
    df.global.world.selected_building = link.obj

    if back then
        guidm.setCursorPos(link.cursor)
        self:getViewport(link.viewport):set()
    else
        guidm.setCursorPos(link.center)
        self:getViewport():reveal(link.center, 5, 0, 10):set()
    end
end

function MechanismList:changeSelected(delta)
    if #self.links <= 1 then return end
    self.selected = 1 + (self.selected + delta - 1) % #self.links
    self:zoomToLink(self.links[self.selected])
end

function MechanismList:onInput(keys)
    if keys.SECONDSCROLL_UP then
        self:changeSelected(-1)
    elseif keys.SECONDSCROLL_DOWN then
        self:changeSelected(1)
    elseif keys.LEAVESCREEN then
        self:dismiss()
        if self.selected ~= 1 then
            self:zoomToLink(self.links[1], true)
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

if dfhack.gui.getCurFocus() ~= 'dwarfmode/QueryBuilding/Some' then
    qerror("This script requires the main dwarfmode view in 'q' mode")
end

local list = mkinstance(MechanismList):init(df.global.world.selected_building)
list:show()
list:changeSelected(1)
