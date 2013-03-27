-- Browses rooms owned by a unit.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'

local room_type_table = {
    [df.building_bedst] = { token = 'bed', qidx = 2, tile = 233 },
    [df.building_tablest] = { token = 'table', qidx = 3, tile = 209 },
    [df.building_chairst] = { token = 'chair', qidx = 4, tile = 210 },
    [df.building_coffinst] = { token = 'coffin', qidx = 5, tile = 48 },
}

local room_quality_table = {
    { 1, 'Meager Quarters', 'Meager Dining Room', 'Meager Office', 'Grave' },
    { 100, 'Modest Quarters', 'Modest Dining Room', 'Modest Office', "Servant's Burial Chamber" },
    { 250, 'Quarters', 'Dining Room', 'Office', 'Burial Chamber' },
    { 500, 'Decent Quarters', 'Decent Dining Room', 'Decent Office', 'Tomb' },
    { 1000, 'Fine Quarters', 'Fine Dining Room', 'Splendid Office', 'Fine Tomb' },
    { 1500, 'Great Bedroom', 'Great Dining Room', 'Throne Room', 'Mausoleum' },
    { 2500, 'Grand Bedroom', 'Grand Dining Room', 'Opulent Throne Room', 'Grand Mausoleum' },
    { 10000, 'Royal Bedroom', 'Royal Dining Room', 'Royal Throne Room', 'Royal Mausoleum' }
}

function getRoomName(building, unit)
    local info = room_type_table[building._type]
    if not info or not building.is_room then
        return utils.getBuildingName(building)
    end

    local quality = building:getRoomValue(unit)
    local row = room_quality_table[1]
    for _,v in ipairs(room_quality_table) do
        if v[1] <= quality then
            row = v
        else
            break
        end
    end
    return row[info.qidx]
end

function makeRoomEntry(bld, unit, is_spouse)
    local info = room_type_table[bld._type] or {}

    return {
        obj = bld,
        token = info.token or '?',
        tile = info.tile or '?',
        caption = getRoomName(bld, unit),
        can_use = (not is_spouse or bld:canUseSpouseRoom()),
        owner = unit
    }
end

function listRooms(unit, spouse)
    local rv = {}
    for _,v in pairs(unit.owned_buildings) do
        if v.owner == unit then
            rv[#rv+1] = makeRoomEntry(v, unit, spouse)
        end
    end
    return rv
end

function concat_lists(...)
    local rv = {}
    for i = 1,select('#',...) do
        local v = select(i,...)
        if v then
            for _,x in ipairs(v) do rv[#rv+1] = x end
        end
    end
    return rv
end

RoomList = defclass(RoomList, guidm.MenuOverlay)

RoomList.focus_path = 'room-list'

RoomList.ATTRS{ unit = DEFAULT_NIL }

function RoomList:init(info)
    local unit = info.unit
    local base_bld = df.global.world.selected_building

    self:assign{
        base_building = base_bld,
        items = {}, selected = 1,
        own_rooms = {}, spouse_rooms = {}
    }

    self.old_viewport = self:getViewport()
    self.old_cursor = guidm.getCursorPos()

    if unit then
        self.own_rooms = listRooms(unit)
        self.spouse = df.unit.find(unit.relations.spouse_id)
        if self.spouse then
            self.spouse_rooms = listRooms(self.spouse, unit)
        end
        self.items = concat_lists(self.own_rooms, self.spouse_rooms)
    end

    if base_bld then
        for i,v in ipairs(self.items) do
            if v.obj == base_bld then
                self.selected = i
                v.tile = 26
                goto found
            end
        end
        self.base_item = makeRoomEntry(base_bld, unit)
        self.base_item.owner = unit
        self.base_item.old_owner = base_bld.owner
        self.base_item.tile = 26
        self.items = concat_lists({self.base_item}, self.items)
    ::found::
    end
end

local sex_char = { [0] = 12, [1] = 11 }

function drawUnitName(dc, unit)
    dc:pen(COLOR_GREY)
    if unit then
        local color = dfhack.units.getProfessionColor(unit)
        dc:char(sex_char[unit.sex] or '?'):advance(1):pen(color)

        local vname = dfhack.units.getVisibleName(unit)
        if vname and vname.has_name then
            dc:string(dfhack.TranslateName(vname)..', ')
        end
        dc:string(dfhack.units.getProfessionName(unit))
    else
        dc:string("No Owner Assigned")
    end
end

function drawRoomEntry(dc, entry, selected)
    local color = COLOR_GREEN
    if not entry.can_use then
        color = COLOR_RED
    elseif entry.obj.owner ~= entry.owner or not entry.owner then
        color = COLOR_CYAN
    end
    dc:pen{fg = color, bold = (selected == entry)}
    dc:char(entry.tile):advance(1):string(entry.caption)
end

function can_modify(sel_item)
    return sel_item and sel_item.owner
       and sel_item.can_use and not sel_item.owner.flags1.dead
end

function RoomList:onRenderBody(dc)
    local sel_item = self.items[self.selected]

    dc:clear():seek(1,1)
    drawUnitName(dc, self.unit)

    if self.base_item then
        dc:newline():newline(2)
        drawRoomEntry(dc, self.base_item, sel_item)
    end
    if #self.own_rooms > 0 then
        dc:newline()
        for _,v in ipairs(self.own_rooms) do
            dc:newline(2)
            drawRoomEntry(dc, v, sel_item)
        end
    end
    if #self.spouse_rooms > 0 then
        dc:newline():newline(1)
        drawUnitName(dc, self.spouse)

        dc:newline()
        for _,v in ipairs(self.spouse_rooms) do
            dc:newline(2)
            drawRoomEntry(dc, v, sel_item)
        end
    end
    if self.unit and #self.own_rooms == 0 and #self.spouse_rooms == 0 then
        dc:newline():newline(2):string("No already assigned rooms.", COLOR_LIGHTRED)
    end

    dc:newline():newline(1):pen(COLOR_WHITE)
    dc:key('LEAVESCREEN'):string(": Back")

    if can_modify(sel_item) then
        dc:string(", "):key('SELECT')
        if sel_item.obj.owner == sel_item.owner then
            dc:string(": Unassign")
        else
            dc:string(": Assign")
        end
    end
end

function RoomList:changeSelected(delta)
    if #self.items <= 1 then return end
    self.selected = 1 + (self.selected + delta - 1) % #self.items
    self:selectBuilding(self.items[self.selected].obj)
end

function RoomList:onInput(keys)
    local sel_item = self.items[self.selected]

    if keys.SECONDSCROLL_UP then
        self:changeSelected(-1)
    elseif keys.SECONDSCROLL_DOWN then
        self:changeSelected(1)
    elseif keys.LEAVESCREEN then
        self:dismiss()

        if self.base_building then
            if not sel_item or self.base_building ~= sel_item.obj then
                self:selectBuilding(self.base_building, self.old_cursor, self.old_view)
            end
            if self.unit and self.base_building.owner == self.unit then
                df.global.ui_building_in_assign = false
            end
        end
    elseif keys.SELECT then
        if can_modify(sel_item) then
            local owner = sel_item.owner
            if sel_item.obj.owner == owner then
                owner = sel_item.old_owner
            end
            dfhack.buildings.setOwner(sel_item.obj, owner)
        end
    elseif self:simulateViewScroll(keys) then
        return
    end
end

local focus = dfhack.gui.getCurFocus()

if focus == 'dwarfmode/QueryBuilding/Some/Assign/Unit' then
    local unit = df.global.ui_building_assign_units[df.global.ui_building_item_cursor]
    RoomList{ unit = unit }:show()
elseif string.match(dfhack.gui.getCurFocus(), '^dwarfmode/QueryBuilding/Some') then
    local base = df.global.world.selected_building
    RoomList{ unit = base.owner }:show()
else
    qerror("This script requires the main dwarfmode view in 'q' mode")
end
