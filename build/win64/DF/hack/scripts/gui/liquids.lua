-- Interface for spawning liquids on tiles.

local liquids = reqscript('modtools/spawn-liquid')
local gui = require('gui')
local guidm = require('gui.dwarfmode')
local widgets = require('gui.widgets')

local SpawnLiquidMode = {
    SET = 1,
    ADD = 2,
    REMOVE = 3,
    CLEAN = 4,
}

local SpawnLiquidPaintMode = {
    DRAG = 1,
    CLICK = 2,
    AREA = 3,
}

local SpawnLiquidCursor = {
    [df.tile_liquid.Water] = dfhack.screen.findGraphicsTile('MINING_INDICATORS', 0, 0),
    [df.tile_liquid.Magma] = dfhack.screen.findGraphicsTile('MINING_INDICATORS', 1, 0),
    [df.tiletype.RiverSource] = dfhack.screen.findGraphicsTile('LIQUIDS', 0, 0),
}

SpawnLiquid = defclass(SpawnLiquid, widgets.Window)
SpawnLiquid.ATTRS {
    frame_title='Spawn liquid menu',
    frame={b = 4, r = 4, w = 50, h = 12},
}

function SpawnLiquid:init()
    self.type = df.tile_liquid.Water
    self.mode = SpawnLiquidMode.SET
    self.level = 3
    self.paint_mode = SpawnLiquidPaintMode.AREA
    self.tile = SpawnLiquidCursor[self.type]

    self:addviews{
        widgets.Label{
            frame = {l = 0, t = 0},
            text = {{ text = self:callback('getLabel') }}
        },
        widgets.HotkeyLabel{
            frame = {l = 0, b = 1},
            label = 'Decrease level',
            auto_width = true,
            key = 'KEYBOARD_CURSOR_LEFT',
            on_activate = self:callback('decreaseLiquidLevel'),
            disabled = function() return self.level == 1 end
        },
        widgets.HotkeyLabel{
            frame = { l = 19, b = 1},
            label = 'Increase level',
            auto_width = true,
            key = 'KEYBOARD_CURSOR_RIGHT',
            on_activate = self:callback('increaseLiquidLevel'),
            disabled = function() return self.level == 7 end
        },
        widgets.CycleHotkeyLabel{
            frame = {l = 0, b = 2},
            label = 'Brush:',
            auto_width = true,
            key = 'CUSTOM_Q',
            options = {
                { label = "Water", value = df.tile_liquid.Water, pen = COLOR_CYAN },
                { label = "Magma", value = df.tile_liquid.Magma, pen = COLOR_RED },
                { label = "River", value = df.tiletype.RiverSource, pen = COLOR_BLUE },
            },
            initial_option = 0,
            on_change = function(new, _)
                self.type = new
                self.tile = SpawnLiquidCursor[new]
            end,
        },
        widgets.CycleHotkeyLabel{
            frame = {l = 0, b = 0},
            label = 'Paint mode:',
            auto_width = true,
            key = 'CUSTOM_Z',
            options = {
                { label = "Area ", value = SpawnLiquidPaintMode.AREA, pen = COLOR_WHITE },
                { label = "Click", value = SpawnLiquidPaintMode.CLICK, pen = COLOR_WHITE },
                { label = "Drag ", value = SpawnLiquidPaintMode.DRAG, pen = COLOR_WHITE },
            },
            initial_option = 1,
            on_change = function(new, _) self.paint_mode = new end,
        },
        widgets.CycleHotkeyLabel{
            frame = {l = 18, b = 2},
            label = 'Mode:',
            auto_width = true,
            key = 'CUSTOM_X',
            options = {
                { label = "Set   ", value = SpawnLiquidMode.SET, pen = COLOR_WHITE },
                { label = "Add   ", value = SpawnLiquidMode.ADD, pen = COLOR_WHITE },
                { label = "Remove", value = SpawnLiquidMode.REMOVE, pen = COLOR_WHITE },
                { label = "Clean ", value = SpawnLiquidMode.CLEAN, pen = COLOR_WHITE },
            },
            initial_option = 1,
            on_change = function(new, _) self.mode = new end,
            disabled = function() return self.type == df.tiletype.RiverSource end
        },
    }
end

-- TODO: More reactive label dependant on options selected.
function SpawnLiquid:getLabel()
    return ([[Click on a tile to spawn a %s/7 level of %s]]):format(
        self.level,
        self.type == 0 and "Water" or self.type == 1 and "Magma" or "River"
    )
end

function SpawnLiquid:getLiquidLevel(position)
    local tile = dfhack.maps.getTileFlags(position)

    if self.mode == SpawnLiquidMode.ADD then
        return math.max(0, math.min(tile.flow_size + self.level, 7))
    elseif self.mode == SpawnLiquidMode.REMOVE then
        return math.max(0, math.min(tile.flow_size - self.level, 7))
    end

    return self.level
end

function SpawnLiquid:increaseLiquidLevel()
    self.level = math.min(self.level + 1, 7)
end

function SpawnLiquid:decreaseLiquidLevel()
    self.level = math.max(self.level - 1, 1)
end

function SpawnLiquid:spawn(pos)
    if dfhack.maps.isValidTilePos(pos) and dfhack.maps.isTileVisible(pos) then
        local map_block = dfhack.maps.getTileBlock(pos)

        if self.mode == SpawnLiquidMode.CLEAN then
            local tile = dfhack.maps.getTileFlags(pos)

            tile.water_salt = false
            tile.water_stagnant = false
        elseif self.type == df.tiletype.RiverSource then
            map_block.tiletype[pos.x % 16][pos.y % 16] = df.tiletype.RiverSource

            liquids.spawnLiquid(pos, 7, df.tile_liquid.Water)
        else
            liquids.spawnLiquid(pos, self:getLiquidLevel(pos), self.type)
        end

        -- Regardless of spawning or removing liquids, we need to reindex to
        -- ensure pathability is up to date.
        df.global.world.reindex_pathfinding = true
    end
end

function SpawnLiquid:getPen()
    return self.type == df.tile_liquid.Water and COLOR_BLUE or COLOR_RED, "X", self.tile
end

function SpawnLiquid:getBounds(start_position, end_position)
    return {
        x1=math.min(start_position.x, end_position.x),
        x2=math.max(start_position.x, end_position.x),
        y1=math.min(start_position.y, end_position.y),
        y2=math.max(start_position.y, end_position.y),
        z1=math.min(start_position.z, end_position.z),
        z2=math.max(start_position.z, end_position.z),
    }
end

function SpawnLiquid:onRenderFrame(dc, rect)
    SpawnLiquid.super.onRenderFrame(self, dc, rect)

    local mouse_pos = dfhack.gui.getMousePos()

    if self.is_dragging then
        if df.global.enabler.mouse_lbut == 0 then
            self.is_dragging = false
        elseif mouse_pos and not self:getMouseFramePos() then
            self:spawn(mouse_pos)
        end
    end

    if mouse_pos then
        guidm.renderMapOverlay(self:callback('getPen'), self:getBounds(
            self.is_dragging_area and self.area_first_pos or mouse_pos,
            mouse_pos
        ))
    end
end

function SpawnLiquid:onInput(keys)
    if SpawnLiquid.super.onInput(self, keys) then return true end

    if keys._MOUSE_L_DOWN and not self:getMouseFramePos() then
        local mouse_pos = dfhack.gui.getMousePos()

        if self.paint_mode == SpawnLiquidPaintMode.CLICK and mouse_pos then
            self:spawn(mouse_pos)
            return true
        elseif self.paint_mode == SpawnLiquidPaintMode.AREA and mouse_pos then
            if self.is_dragging_area then
                local bounds = self:getBounds(self.area_first_pos, mouse_pos)
                for y = bounds.y1, bounds.y2 do
                    for x = bounds.x1, bounds.x2 do
                        for z = bounds.z1, bounds.z2 do
                            self:spawn(xyz2pos(x, y, z))
                        end
                    end
                end
                self.is_dragging_area = false
                return true
            else
                self.is_dragging_area = true
                self.area_first_pos = mouse_pos
                return true
            end
        elseif self.paint_mode == SpawnLiquidPaintMode.DRAG then
            self.is_dragging = true
            return true
        end
    end

    -- TODO: Holding the mouse down causes event spam.
    if keys._MOUSE_L and not self:getMouseFramePos() then
        if self.paint_mode == SpawnLiquidPaintMode.DRAG then
            self.is_dragging = true
            return true
        end
    end
end

SpawnLiquidScreen = defclass(SpawnLiquidScreen, gui.ZScreen)
SpawnLiquidScreen.ATTRS {
    focus_path = 'spawnliquid',
    pass_movement_keys = true,
    pass_mouse_clicks = false,
}

function SpawnLiquidScreen:init()
    self:addviews{SpawnLiquid{}}
end

function SpawnLiquidScreen:onDismiss()
    view = nil
end

if not dfhack.isMapLoaded() then
    qerror('This script requires a fortress map to be loaded')
end

view = view and view:raise() or SpawnLiquidScreen{}:show()
