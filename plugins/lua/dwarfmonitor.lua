local _ENV = mkmodule('plugins.dwarfmonitor')

local json = require('json')
local guidm = require('gui.dwarfmode')
local overlay = require('plugins.overlay')

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
end

function MiseryWidget:overlay_onupdate()
    local counts, num_colors = {}, #self.colors
    for _,unit in ipairs(df.global.world.units.active) do
        local stress_category = math.min(num_colors,
                                         dfhack.units.getStressCategory(unit))
        counts[stress_category] = (counts[stress_category] or 0) + 1
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
}

function CursorWidget:onRenderBody(dc)
    local screenx, screeny = dfhack.screen.getMousePos()
    local mouse_map = dfhack.gui.getMousePos()
    local keyboard_map = guidm.getCursorPos()

    local text = {}
    table.insert(text, ('mouse UI grid (%d,%d)'):format(screenx, screeny))
    if mouse_map then
        table.insert(text, ('mouse map coord (%d,%d,%d)')
                :format(mouse_map.x, mouse_map.y, mouse_map.z))
    end
    if keyboard_map then
        table.insert(text, ('kbd cursor coord (%d,%d,%d)')
                :format(keyboard_map.x, keyboard_map.y, keyboard_map.z))
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
