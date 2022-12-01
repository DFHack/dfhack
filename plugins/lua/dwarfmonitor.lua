local _ENV = mkmodule('plugins.dwarfmonitor')

local json = require('json')
local guidm = require('gui.dwarfmode')
local overlay = require('plugins.overlay')
local utils = require('utils')

local DWARFMONITOR_CONFIG_FILE = 'dfhack-config/dwarfmonitor.json'

-- ------------- --
-- WeatherWidget --
-- ------------- --

WeatherWidget = defclass(WeatherWidget, overlay.OverlayWidget)
WeatherWidget.ATTRS{
    default_pos={x=15,y=-1},
    viewscreens={'dungeonmode', 'dwarfmode'},
}

function WeatherWidget:init()
    self.rain = false
    self.snow = false
end

function WeatherWidget:overlay_onupdate()
    local rain, snow = false, false
    local cw = df.global.current_weather
    for i=0,4 do
        for j=0,4 do
            weather = cw[i][j]
            if weather == df.weather_type.Rain then rain = true end
            if weather == df.weather_type.Snow then snow = true end
        end
    end
    self.frame.w = (rain and 4 or 0) + (snow and 4 or 0) +
            ((snow and rain) and 1 or 0)
    self.rain, self.snow = rain, snow
end

function WeatherWidget:onRenderBody(dc)
    if self.rain then dc:string('Rain', COLOR_LIGHTBLUE):advance(1) end
    if self.snow then dc:string('Snow', COLOR_WHITE) end
end

-- ---------- --
-- DateWidget --
-- ---------- --

local function get_date_format()
    local ok, config = pcall(json.decode_file, DWARFMONITOR_CONFIG_FILE)
    if not ok or not config.date_format then
        return 'Y-M-D'
    end
    return config.date_format
end

DateWidget = defclass(DateWidget, overlay.OverlayWidget)
DateWidget.ATTRS{
    default_pos={x=-16,y=1},
    viewscreens={'dungeonmode', 'dwarfmode'},
}

function DateWidget:init()
    self.datestr = ''
    self.fmt = get_date_format()
end

function DateWidget:overlay_onupdate()
    local year = dfhack.world.ReadCurrentYear()
    local month = dfhack.world.ReadCurrentMonth() + 1
    local day = dfhack.world.ReadCurrentDay()

    local fmt = self.fmt
    local datestr = 'Date:'
    for i=1,#fmt do
        local c = fmt:sub(i, i)
        if c == 'y' or c == 'Y' then
            datestr = datestr .. year
        elseif c == 'm' or c == 'M' then
            if c == 'M' and month < 10 then
                datestr = datestr .. '0'
            end
            datestr = datestr .. month
        elseif c == 'd' or c == 'D' then
            if c == 'D' and day < 10 then
                datestr = datestr .. '0'
            end
            datestr = datestr .. day
        else
            datestr = datestr .. c
        end
    end

    self.frame.w = #datestr
    self.datestr = datestr
end

function DateWidget:onRenderBody(dc)
    dc:string(self.datestr, COLOR_GREY)
end

-- ------------ --
-- MiseryWidget --
-- ------------ --

MiseryWidget = defclass(MiseryWidget, overlay.OverlayWidget)
MiseryWidget.ATTRS{
    default_pos={x=-2,y=-1},
    viewscreens={'dwarfmode'},
}

function MiseryWidget:init()
    self.colors = getStressCategoryColors()
    self.stress_category_counts = {}
    self.frame.w = 2*#self.colors + 1
end

function MiseryWidget:overlay_onupdate()
    local counts, num_colors = {}, #self.colors
    for _,unit in ipairs(df.global.world.units.active) do
        if not dfhack.units.isCitizen(unit, true) then goto continue end
        local stress_category = math.min(num_colors,
                                         dfhack.units.getStressCategory(unit))
        counts[stress_category] = (counts[stress_category] or 0) + 1
        ::continue::
    end

    local width = 2 + num_colors - 1 -- 'H:' plus the slashes
    for i=1,num_colors do
        width = width + #tostring(counts[i] or 0)
    end

    self.stress_category_counts = counts
    self.frame.w = width
end

function MiseryWidget:onRenderBody(dc)
    dc:string('H:', COLOR_WHITE)
    local counts = self.stress_category_counts
    for i,color in ipairs(self.colors) do
        dc:string(tostring(counts[i] or 0), color)
        if i < #self.colors then dc:string('/', COLOR_WHITE) end
    end
end

-- ------------ --
-- CursorWidget --
-- ------------ --

CursorWidget = defclass(CursorWidget, overlay.OverlayWidget)
CursorWidget.ATTRS{
    default_pos={x=2,y=-4},
    viewscreens={'dungeonmode', 'dwarfmode'},
    type='all',
    short=false,
}

CursorWidget.COORD_TYPES = utils.OrderedTable()
CursorWidget.COORD_TYPES.mouse_ui = {
    description = 'mouse UI grid',
    get_coords = function()
        return xyz2pos(dfhack.screen.getMousePos())
    end,
}
CursorWidget.COORD_TYPES.mouse_map = {
    description = 'mouse map coord',
    get_coords = dfhack.gui.getMousePos,
}
CursorWidget.COORD_TYPES.keyboard_map = {
    description = 'kbd cursor coord',
    get_coords = guidm.getCursorPos,
}

function CursorWidget:init()
    local ok, config = pcall(json.decode_file, DWARFMONITOR_CONFIG_FILE)
    if ok then
        self.type = tostring(config.coords_type or self.type)
        self.short = config.coords_short
    else
        dfhack.printerr(('Failed to load %s: %s'):format(DWARFMONITOR_CONFIG_FILE, config))
    end
end

function CursorWidget:onRenderBody(dc)
    local text = {}
    for k, v in pairs(CursorWidget.COORD_TYPES) do
        -- fall back to 'all' behavior if type isn't recognized
        if k == self.type or not CursorWidget.COORD_TYPES[self.type] then
            local coords = v.get_coords()
            if coords then
                local prefix = self.short and '' or (v.description .. ' ')
                local coord_str = table.concat({coords.x, coords.y, coords.z}, ',')
                table.insert(text, ('%s(%s)'):format(prefix, coord_str))
            end
        end
    end

    local width = 0
    for i,line in ipairs(text) do
        dc:seek(0, i-1):string(line)
        width = math.max(width, #line)
    end
    self.frame.w = width
    self.frame.h = #text
end

-- register our widgets with the overlay
OVERLAY_WIDGETS = {
    cursor=CursorWidget,
    date=DateWidget,
    misery=MiseryWidget,
    weather=WeatherWidget,
}

return _ENV
