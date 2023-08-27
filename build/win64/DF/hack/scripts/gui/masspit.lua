-- Interface to assign caged units in a stockpile to pond/pit.

local utils = require 'utils'
local gui = require('gui')
local widgets = require('gui.widgets')

local function getCagedUnits(stockpile)
    local cagedUnits = {}

    for _, item in pairs(dfhack.buildings.getStockpileContents(stockpile)) do
        if not dfhack.items.getGeneralRef(item, df.general_ref_type.CONTAINS_UNIT) then
            goto skipitem
        end

        for _, ref in pairs(item.general_refs) do
            if ref:getType() == df.general_ref_type.CONTAINS_UNIT then
                table.insert(cagedUnits, ref.unit_id)
            end
        end

        :: skipitem ::
    end

    return cagedUnits
end

Masspit = defclass(Masspit, widgets.Window)
Masspit.ATTRS {
    frame_title='Mass-pit caged creatures',
    frame={w=43, h=22, r = 2, t = 18},
    resizable=true,
    resize_min={h=12},
}

function Masspit:init()
    self:addviews{
        widgets.WrappedLabel{
            view_id = 'label',
            frame = { l = 0, t = 0 },
            text_to_wrap = "Select or click on a stockpile"
        },
        widgets.Pages{
            view_id ='pages',
            subviews = {
                widgets.List{
                    view_id = 'stockpiles',
                    frame = { l = 0, t = 4 },
                    on_select = self:callback("focusOnZone"),
                    on_submit = self:callback("setStockpile")
                },
                widgets.List{
                    view_id = 'pits',
                    frame = { l = 0, t = 3 },
                    on_select = self:callback("focusOnZone"),
                    on_submit = self:callback("setPit")
                },
                widgets.List{
                    view_id = 'pitted',
                    frame = { l = 0, t = 2 },
                },
            }
        },
    }

    self:showStockpiles()
end

function Masspit:showStockpiles()
    self.subviews.pages:setSelected('stockpiles')

    local choices = {}

    for _, zone in pairs(df.global.world.buildings.other.STOCKPILE) do
        if (#zone.settings.animals.enabled > 0) then
            if #getCagedUnits(zone) > 0 then
                local zone_name = zone.name.length and dfhack.TranslateName(zone.name) or "Animal stockpile #" .. zone.stockpile_number
                local zone_position = df.coord:new()
                zone_position.x = zone.centerx
                zone_position.y = zone.centery
                zone_position.z = zone.z

                table.insert(choices, {
                    text = ([[%s: %s x:%s y:%s]]):format(zone.id, zone_name, zone.centerx, zone.centery),
                    zone_position = zone_position,
                    zone_id = zone.id
                })
            end
        end
    end

    self.subviews.pages.subviews.stockpiles:setChoices(choices)

    if #choices == 0 then
        self.subviews.label.text_to_wrap = "No stockpiles with caged creatures found! Create one first."
    else
        self.subviews.label.text_to_wrap = "Select (or click on) an animal stockpile with caged creatures to pit."
    end
end

function Masspit:showPondPits()
    self.subviews.pages:setSelected('pits')

    local choices = {}

    for _, zone in pairs(df.global.world.buildings.other.ACTIVITY_ZONE) do
        if (zone.type == df.civzone_type.Pond) then
            local zone_name = zone.name.length and dfhack.TranslateName(zone.name) or "Unnamed pit/pond"
            local zone_position = df.coord:new()
            zone_position.x = zone.centerx
            zone_position.y = zone.centery
            zone_position.z = zone.z

            table.insert(choices, {
                text = ([[%s: %s x:%s y:%s]]):format(zone.id, zone_name, zone.centerx, zone.centery),
                zone_position = zone_position,
                zone_id = zone.id
            })
        end
    end

    self.subviews.pages.subviews.pits:setChoices(choices)

    if #choices == 0 then
        self.subviews.label.text_to_wrap = "No pond/pit zones found! Create one first."
    else
        self.subviews.label.text_to_wrap = ([[Select (or click on) a pond/pit zone to move %s caged creatures to.]]):format(#self.caged_units)
    end

    self:updateLayout()
end

function Masspit:focusOnZone(index, choice)
    if not index or not choice then
        return
    end

    if (choice.zone_position) then
        dfhack.gui.revealInDwarfmodeMap(choice.zone_position)
    end
end

function Masspit:setStockpile(_, choice)
    self.stockpile = utils.binsearch(df.global.world.buildings.other.STOCKPILE, choice.zone_id, 'id')
    self.caged_units = getCagedUnits(self.stockpile)

    if #self.caged_units == 0 then
        self.subviews.label.text_to_wrap = "This stockpile contains no caged units."
        self.subviews.label:updateLayout()
    else
        self:showPondPits()
    end
end

function Masspit:setPit(_, choice)
    self.pit = utils.binsearch(df.global.world.buildings.other.ACTIVITY_ZONE, choice.zone_id, 'id')

    self.subviews.label.text_to_wrap = ([[Pitting %s units.]]):format(#self.caged_units)
    self.subviews.label:updateLayout()

    local choices = {}

    for _, unit_id in pairs(self.caged_units) do
        local unit = utils.binsearch(df.global.world.units.all, unit_id, 'id')
        local unit_name = unit.name.has_name and dfhack.TranslateName(unit.name) or dfhack.units.getRaceNameById(unit.race)

        -- Prevents duplicate units in assignments, which can cause crashes.
        local duplicate = false
        for _, assigned_unit in pairs(self.pit.assigned_units) do
            if assigned_unit == unit.id then
                duplicate = true
            end
        end

        table.insert(choices, { text = ([[%s: %s %s]]):format(unit.id, unit_name, duplicate and "(ALREADY ASSIGNED)" or "") })

        if not duplicate then
            unit.general_refs:insert("#", { new = df.general_ref_building_civzone_assignedst, building_id=self.pit.id })
            self.pit.assigned_units:insert("#", unit.id)
        end
    end

    self.subviews.pages.subviews.pitted:setChoices(choices)
    self.subviews.pages:setSelected('pitted')
end

function Masspit:onInput(keys)
    if Masspit.super.onInput(self, keys) then return true end

    if keys._MOUSE_L_DOWN and not self:getMouseFramePos() then
        if self.subviews.pages:getSelected() == 1 then
            local building = dfhack.buildings.findAtTile(dfhack.gui.getMousePos())

            if building and df.building_stockpilest:is_instance(building) then
                self:setStockpile(nil, { zone_id = building.id })
            end
        elseif self.subviews.pages:getSelected() == 2 then
            local zones = dfhack.buildings.findCivzonesAt(dfhack.gui.getMousePos())

            for _, zone in pairs(zones) do
                if (zone.type == df.civzone_type.Pond) then
                    self:setPit(nil, { zone_id = zone.id })
                end
            end
        end
    end
end

-- Screen setup
MasspitScreen = defclass(MasspitScreen, gui.ZScreen)
MasspitScreen.ATTRS {
    focus_path='masspit',
    pass_movement_keys=true,
}

function MasspitScreen:init()
    self:addviews{Masspit{}}
end

function MasspitScreen:onDismiss()
    view = nil
end

if not dfhack.isMapLoaded() then
    qerror('This script requires a fortress map to be loaded')
end

view = view and view:raise() or MasspitScreen{}:show()
