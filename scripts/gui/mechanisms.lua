-- Shows mechanisms linked to the current building.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'

function getBuildingName(building)
    return utils.call_with_string(building, 'getName')
end

function listMechanismLinks(building)
    local lst = {}
    local function push(item, mode)
        if item then
            lst[#lst+1] = { obj = item, name = getBuildingName(item), mode = mode }
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

function MechanismList.new(building)
    local self = {
        links = {},
        selected = 1
    }
    return mkinstance(MechanismList, self):init(building)
end

function MechanismList:init(building)
    local links = listMechanismLinks(building)

    links[1].viewport = guidm.getViewportPos()
    links[1].cursor = guidm.getCursorPos()
    if #links <= 1 then
        links[1].mode = 'none'
    end

    self.links = links
    self.selected = 1
    return self
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

function MechanismList:zoomToLink(link)
    self:updateLayout()

    df.global.world.selected_building = link.obj

    local cursor = link.cursor
    if not cursor then
        cursor = xyz2pos(link.obj.centerx, link.obj.centery, link.obj.z)
    end
    guidm.setCursorPos(cursor)

    guidm.revealInViewport(cursor, 5, link.viewport, self.df_layout)
end

function MechanismList:changeSelected(delta)
    if #self.links <= 1 then return end
    self.selected = 1 + (self.selected + delta - 1) % #self.links
    self:zoomToLink(self.links[self.selected])
end

function MechanismList:onInput(keys)
    if keys.STANDARDSCROLL_UP or keys.SECONDSCROLL_UP then
        self:changeSelected(-1)
    elseif keys.STANDARDSCROLL_DOWN or keys.SECONDSCROLL_DOWN then
        self:changeSelected(1)
    elseif keys.LEAVESCREEN then
        self:dismiss()
        if self.selected ~= 1 then
            self:zoomToLink(self.links[1])
        end
    elseif keys.SELECT_ALL then
        if self.selected > 1 then
            self:init(self.links[self.selected].obj)
            self.invalidate()
        end
    elseif keys.SELECT then
        self:dismiss()
    end
end

if not df.viewscreen_dwarfmodest:is_instance(dfhack.gui.getCurViewscreen()) then
    qerror("This script requires the main dwarfmode view")
end
if df.global.ui.main.mode ~= df.ui_sidebar_mode.QueryBuilding then
    qerror("This script requires the 'q' interface mode")
end

local list = MechanismList.new(df.global.world.selected_building)
list:show()
list:changeSelected(1)
