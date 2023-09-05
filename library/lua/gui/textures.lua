-- DFHack textures

local _ENV = mkmodule('gui.textures')

---@alias TexposHandle integer

-- Preloaded DFHack Assets.
-- Use this handles if you need to get dfhack standard textures.
---@type table<string, TexposHandle>
local texpos_handles = {
  green_pin = dfhack.textures.loadTileset('hack/data/art/green-pin.png', 8, 12),
  red_pin = dfhack.textures.loadTileset('hack/data/art/red-pin.png', 8, 12),
  icons = dfhack.textures.loadTileset('hack/data/art/icons.png', 8, 12),
  on_off = dfhack.textures.loadTileset('hack/data/art/on-off.png', 8, 12),
  control_panel = dfhack.textures.loadTileset('hack/data/art/control-panel.png', 8, 12),
  border_thin = dfhack.textures.loadTileset('hack/data/art/border-thin.png', 8, 12),
  border_medium = dfhack.textures.loadTileset('hack/data/art/border-medium.png', 8, 12),
  border_bold = dfhack.textures.loadTileset('hack/data/art/border-bold.png', 8, 12),
  border_panel = dfhack.textures.loadTileset('hack/data/art/border-panel.png', 8, 12),
  border_window = dfhack.textures.loadTileset('hack/data/art/border-window.png', 8, 12),
}

-- Get valid texpos for preloaded texture in tileset
---@param offset integer
---@return integer
function tp_green_pin(offset)
  return dfhack.textures.getTexposByHandle(texpos_handles.green_pin[offset])
end

-- Get valid texpos for preloaded texture in tileset
---@param offset integer
---@return integer
function tp_red_pin(offset)
  return dfhack.textures.getTexposByHandle(texpos_handles.red_pin[offset])
end

-- Get valid texpos for preloaded texture in tileset
---@param offset integer
---@return integer
function tp_icons(offset)
  return dfhack.textures.getTexposByHandle(texpos_handles.icons[offset])
end

-- Get valid texpos for preloaded texture in tileset
---@param offset integer
---@return integer
function tp_on_off(offset)
  return dfhack.textures.getTexposByHandle(texpos_handles.on_off[offset])
end

-- Get valid texpos for preloaded texture in tileset
---@param offset integer
---@return integer
function tp_control_panel(offset)
  return dfhack.textures.getTexposByHandle(texpos_handles.control_panel[offset])
end

-- Get valid texpos for preloaded texture in tileset
---@param offset integer
---@return integer
function tp_border_thin(offset)
  return dfhack.textures.getTexposByHandle(texpos_handles.border_thin[offset])
end

-- Get valid texpos for preloaded texture in tileset
---@param offset integer
---@return integer
function tp_border_medium(offset)
  return dfhack.textures.getTexposByHandle(texpos_handles.border_medium[offset])
end

-- Get valid texpos for preloaded texture in tileset
---@param offset integer
---@return TexposHandle
function tp_border_bold(offset)
  return dfhack.textures.getTexposByHandle(texpos_handles.border_bold[offset])
end

-- Get valid texpos for preloaded texture in tileset
---@param offset integer
---@return integer
function tp_border_panel(offset)
  return dfhack.textures.getTexposByHandle(texpos_handles.border_panel[offset])
end

-- Get valid texpos for preloaded texture in tileset
---@param offset integer
---@return integer
function tp_border_window(offset)
  return dfhack.textures.getTexposByHandle(texpos_handles.border_window[offset])
end

return _ENV
