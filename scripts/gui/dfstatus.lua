-- a quick access status screen
-- originally written by enjia2000@gmail.com (stolencatkarma)

--[[=begin

gui/dfstatus
============
Show a quick overview of critical stock quantities, including food, drinks, wood, and various bars.
Sections can be enabled/disabled/configured by editing ``dfhack-config/dfstatus.lua``.

=end]]
local gui = require 'gui'

function warn(msg)
    dfhack.color(COLOR_LIGHTRED)
    print(msg)
    dfhack.color(nil)
end

config = {
    flags = {
        drink = true,
        wood = true,
        fuel = true,
        prepared_meals = true,
        tanned_hides = true,
        cloth = true,
        metals = true,
    },
    metal_ids = {},
}

function parse_config()
    local metal_map = {}
    for id, raw in pairs(df.global.world.raws.inorganics) do
        if raw.material.flags.IS_METAL then
            metal_map[raw.id:upper()] = id
            metal_map[id] = raw.id:upper()
        end
    end

    local function add_metal(...)
        for _, m in pairs({...}) do
            id = metal_map[tostring(m):upper()]
            if id ~= nil then
                table.insert(config.metal_ids, id)
            elseif m == '-' then
                table.insert(config.metal_ids, '-')
            else
                warn('Invalid metal: ' .. tostring(m))
            end
        end
        return add_metal
    end

    local env = {}
    setmetatable(env, {
        __index = function(_, k)
            if k == 'metal' or k == 'metals' then
                return add_metal
            elseif k == 'flags' then
                return config.flags
            else
                error('unknown name: ' .. k, 2)
            end
        end,
        __newindex = function(_, k, v)
            if config.flags[k] ~= nil then
                if v ~= nil then
                    config.flags[k] = v
                else
                    config.flags[k] = false
                end
            else
                error('unknown flag: ' .. k, 2)
            end
        end,
    })
    local f, err = loadfile('dfhack-config/dfstatus.lua', 't', env)
    if not f then
        qerror('error loading config: ' .. err)
    end
    local ok, err = pcall(f)
    if not ok then
        qerror('error parsing config: ' .. err)
    end
end

function getInorganicName(id)
    return (df.inorganic_raw.find(id).material.state_name.Solid:gsub('^[a-z]', string.upper))
end

dfstatus = defclass(dfstatus, gui.FramedScreen)
dfstatus.ATTRS = {
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = 'dfstatus',
    frame_width = 16,
    frame_height = 17,
    frame_inset = 1,
    focus_path = 'dfstatus',
}

function dfstatus:init()
    self.text = {}
    self.start = 1
    local function write(line)
        table.insert(self.text, line)
        -- ensure that the window is wide enough for this line plus a scroll arrow
        if #line + 1 > self.frame_width then
            self.frame_width = #line + 1
        end
    end
    local function newline() write('') end
    local f = config.flags

    local drink = 0
    local wood = 0
    local fuel = 0

    local prepared_meals = 0
    local tanned_hides = 0
    local cloth = 0

    local metals = {}
    for _, id in pairs(config.metal_ids) do
        metals[id] = 0
    end

    for _, item in ipairs(df.global.world.items.all) do
        if not item.flags.rotten and not item.flags.dump and not item.flags.forbid then
            if item:getType() == df.item_type.WOOD then
                wood = wood + item:getStackSize()
            elseif item:getType() == df.item_type.DRINK then
                drink = drink + item:getStackSize()
            elseif item:getType() == df.item_type.SKIN_TANNED then
                tanned_hides = tanned_hides + item:getStackSize()
            elseif item:getType() == df.item_type.CLOTH then
                cloth = cloth + item:getStackSize()
            elseif item:getType() == df.item_type.FOOD then
                prepared_meals = prepared_meals + item:getStackSize()
            elseif item:getType() == df.item_type.BAR then
                if item:getMaterial() == df.builtin_mats.COAL then
                    fuel = fuel + item:getStackSize()
                elseif item:getMaterial() == df.builtin_mats.INORGANIC then
                    local mat_idx = item:getMaterialIndex()
                    if metals[mat_idx] ~= nil then
                        metals[mat_idx] = metals[mat_idx] + item:getStackSize()
                    end
                end
            end
        end
    end
    if f.drink then
        write("Drinks:  " .. drink)
    end
    if f.prepared_meals then
        write("Meals:   " .. prepared_meals)
    end
    if f.drink or f.prepared_meals then
        newline()
    end
    if f.wood then
        write("Wood: " .. wood)
    end
    if f.fuel then
        write("Fuel: " .. fuel)
    end
    if f.wood or f.fuel then
        newline()
    end
    if f.tanned_hides then
        write("Hides: " .. tanned_hides)
    end
    if f.cloth then
        write("Cloth: " .. cloth)
    end
    if f.tanned_hides or f.cloth then
        newline()
    end
    if f.metals then
        write("Metal bars:")
        for _, id in pairs(config.metal_ids) do
            if id == '-' then
                newline()
            else
                write(' ' .. ('%-10s'):format(getInorganicName(id) .. ': ') .. metals[id])
            end
        end
    end
    self.start_min = 1
    self.start_max = #self.text - self.frame_height + 1
end

function dfstatus:onRenderBody(dc)
    dc:pen(COLOR_LIGHTGREEN)
    for id, line in pairs(self.text) do
        if id >= self.start then
            dc:string(line):newline()
        end
    end
    dc:pen(COLOR_LIGHTCYAN)
    if self.start > self.start_min then
        dc:seek(self.frame_width - 1, 0):char(24)
    end
    if self.start < self.start_max then
        dc:seek(self.frame_width - 1, self.frame_height - 1):char(25)
    end
end

function dfstatus:onInput(keys)
    if keys.LEAVESCREEN or keys.SELECT then
        self:dismiss()
        scr = nil
    elseif keys.STANDARDSCROLL_UP then
        self.start = math.max(self.start - 1, self.start_min)
    elseif keys.STANDARDSCROLL_DOWN then
        self.start = math.min(self.start + 1, self.start_max)
    end
end

if not scr then
    parse_config()
    scr = dfstatus()
    scr:show()
else
    scr:dismiss()
    scr = nil
end

