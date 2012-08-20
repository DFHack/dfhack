-- Shows mechanisms linked to the current building.

local gui = require 'gui'
local guidm = require 'gui.dwarfmode'

function getBuildingName(building)
    return dfhack.with_temp_object(
        df.new "string",
        function(str)
            building:getName(str)
            return str.value
        end
    )
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

MechanismList = defclass(MechanismList, guidm.DwarfOverlay)

MechanismList.focus_path = 'mechanisms'

function MechanismList.new(building)
    local self = {
        old_cursor = guidm.getCursorPos(),
        links = listMechanismLinks(building),
        selected = 1
    }
    self.links[1].viewport = guidm.getViewportPos()
    self.links[1].cursor = guidm.getCursorPos()
    return mkinstance(MechanismList, self)
end

local colors = {
    self = COLOR_CYAN, trigger = COLOR_MAGENTA, target = COLOR_GREEN
}
local icons = {
    self = 128, trigger = 17, target = 16
}

function MechanismList:onRender()
    self:renderParent()
    self:updateLayout()
    local x,y,w,h = self:clearMenu()

    self.paintString({fg=COLOR_WHITE},x+1,y+1,"Mechanism Links")

    for i,v in ipairs(self.links) do
        local pen = { fg=colors[v.mode], bold = (i == self.selected) }
        self.paintTile(pen, x+2, y+2+i, icons[v.mode])
        self.paintString(pen, x+4, y+2+i, v.name)
    end

    local nlinks = #self.links
    local line = y+4+nlinks

    if nlinks <= 1 then
        self.paintString({fg=COLOR_LIGHTRED},x+1,line,"This building has no links")
        line = line+2
    end

    self.paintString({fg=COLOR_LIGHTGREEN},x+1,line,"Esc")
    self.paintString({fg=COLOR_WHITE},x+4,line,": Back,")
    self.paintString({fg=COLOR_LIGHTGREEN},x+12,line,"Enter")
    self.paintString({fg=COLOR_WHITE},x+17,line,": Switch")
end

function MechanismList:zoomToLink(link)
    df.global.world.selected_building = link.obj

    local cursor = link.cursor
    if not cursor then
        cursor = xyz2pos(link.obj.centerx, link.obj.centery, link.obj.z)
    end
    guidm.setCursorPos(cursor)

    if not guidm.isInViewport(self.df_layout, self.df_viewport, cursor, 5) then
        local vp = link.viewport
        if vp then
            guidm.setViewportPos(self.df_layout,vp)
        else
            guidm.centerViewportOn(self.df_layout,cursor)
        end
    end
end

function MechanismList:changeSelected(delta)
    self.selected = 1 + (self.selected + delta - 1) % #self.links
    self:zoomToLink(self.links[self.selected])
end

function MechanismList:onInput(keys)
    if keys.STANDARDSCROLL_UP or keys.SECONDSCROLL_UP then
        self:changeSelected(-1)
    elseif keys.STANDARDSCROLL_DOWN or keys.SECONDSCROLL_DOWN then
        self:changeSelected(1)
    elseif keys.LEAVESCREEN then
        if self.selected ~= 1 then
            self:zoomToLink(self.links[1])
        end
        self:dismiss()
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

MechanismList.new(df.global.world.selected_building):show()
