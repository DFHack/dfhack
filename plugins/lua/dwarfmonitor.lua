local _ENV = mkmodule('plugins.dwarfmonitor')

local gps = df.global.gps
local gui = require 'gui'

config = {}
widgets = {}

function dmerror(desc)
    qerror('dwarfmonitor: ' .. tostring(desc))
end

Widget = defclass(Widget)
function Widget:init(opts)
    self.opts = opts
end
function Widget:get_pos()
    local x = self.opts.x >= 0 and self.opts.x or gps.dimx + self.opts.x
    local y = self.opts.y >= 0 and self.opts.y or gps.dimy + self.opts.y
    if self.opts.anchor == 'right' then
        x = x - (self:get_width() or 0) + 1
    end
    return x, y
end
function Widget:render()
    if monitor_state(self.opts.type) == false then
        return
    end
    self:update()
    local x, y = self:get_pos()
    local p = gui.Painter.new_xy(x, y, gps.dimx - 1, y)
    self:render_body(p)
end
function Widget:update() end
function Widget:get_width() end
function Widget:render_body() end

Widget_weather = defclass(Widget_weather, Widget)

function Widget_weather:update()
    self.counts = get_weather_counts()
end

function Widget_weather:get_width()
    if self.counts.rain > 0 then
        if self.counts.snow > 0 then
            return 9
        end
        return 4
    elseif self.counts.snow > 0 then
        return 4
    end
    return 0
end

function Widget_weather:render_body(p)
    if self.counts.rain > 0 then
        p:string('Rain', COLOR_LIGHTBLUE):advance(1)
    end
    if self.counts.snow > 0 then
        p:string('Snow', COLOR_WHITE)
    end
end

Widget_date = defclass(Widget_date, Widget)
Widget_date.ATTRS = {
    output = ''
}

function Widget_date:update()
    if not self.opts.format then
        self.opts.format = 'Y-M-D'
    end
    local year = dfhack.world.ReadCurrentYear()
    local month = dfhack.world.ReadCurrentMonth() + 1
    local day = dfhack.world.ReadCurrentDay()
    self.output = 'Date:'
    for i = 1, #self.opts.format do
        local c = self.opts.format:sub(i, i)
        if c == 'y' or c == 'Y' then
            self.output = self.output .. year
        elseif c == 'm' or c == 'M' then
            if c == 'M' and month < 10 then
                self.output = self.output .. '0'
            end
            self.output = self.output .. month
        elseif c == 'd' or c == 'D' then
            if c == 'D' and day < 10 then
                self.output = self.output .. '0'
            end
            self.output = self.output .. day
        else
            self.output = self.output .. c
        end
    end
end

function Widget_date:get_width()
    return #self.output
end

function Widget_date:render_body(p)
    p:string(self.output, COLOR_GREY)
end

Widget_misery = defclass(Widget_misery, Widget)

function Widget_misery:update()
    self.data = get_misery_data()
end

function Widget_misery:get_width()
    local w = 2 + 6
    for k, v in pairs(self.data) do
        w = w + #tostring(v.value)
    end
    return w
end

function Widget_misery:render_body(p)
    p:string("H:", COLOR_WHITE)
    for i = 0, 6 do
        local v = self.data[i]
        p:string(tostring(v.value), v.color)
        if not v.last then
            p:string("/", COLOR_WHITE)
        end
    end
end

Widget_cursor = defclass(Widget_cursor, Widget)

function Widget_cursor:update()
    if gps.mouse_x == -1 and not self.opts.show_invalid then
        self.output = ''
        return
    end
    self.output = (self.opts.format or '(x,y)'):gsub('[xX]', gps.mouse_x):gsub('[yY]', gps.mouse_y)
end

function Widget_cursor:get_width()
    return #self.output
end

function Widget_cursor:render_body(p)
    p:string(self.output)
end

function render_all()
    for _, w in pairs(widgets) do
        w:render()
    end
end

function load_config()
    config = require('json').decode_file('dfhack-config/dwarfmonitor.json')
    if not config.widgets then
        dmerror('No widgets enabled')
    end
    if type(config.widgets) ~= 'table' then
        dmerror('"widgets" is not a table')
    end
    widgets = {}
    for _, opts in pairs(config.widgets) do
        if type(opts) ~= 'table' then qerror('"widgets" is not an array') end
        if not opts.type then dmerror('Widget missing type field') end
        local cls = _ENV['Widget_' .. opts.type]
        if not cls then
            dmerror('Invalid widget type: ' .. opts.type)
        end
        table.insert(widgets, cls(opts))
    end
end

return _ENV
