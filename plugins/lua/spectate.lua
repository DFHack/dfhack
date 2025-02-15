local _ENV = mkmodule('plugins.spectate')

local argparse = require('argparse')
local dlg = require('gui.dialogs')
local json = require('json')
local overlay = require('plugins.overlay')
local utils = require('utils')

local gui = require('gui')
local widgets = require('gui.widgets')

-- settings starting with 'tooltip-' are not passed to the C++ plugin
local lua_only_settings_prefix = 'tooltip-'

-- how many lines the text following unit is allowed to be moved down to avoid overlapping
local max_banner_y_offset = 4

local function get_default_state()
    return {
        ['auto-disengage']=true,
        ['auto-unpause']=false,
        ['cinematic-action']=true,
        ['follow-seconds']=10,
        ['include-animals']=false,
        ['include-hostiles']=false,
        ['include-visitors']=false,
        ['include-wildlife']=false,
        ['prefer-conflict']=true,
        ['prefer-new-arrivals']=true,
        ['tooltip-follow']=true,
        ['tooltip-follow-blink-milliseconds']=3000,
        ['tooltip-follow-job']=true,
        ['tooltip-follow-job-shortenings'] = {
            ["Store item in stockpile"] = "Store item",
        },
        ['tooltip-follow-name']=false,
        ['tooltip-follow-stress']=true,
        ['tooltip-follow-stress-levels']={
            ["0"] = true, -- Miserable
            ["1"] = true,
            ["2"] = false,
            ["3"] = false,
            ["4"] = false,
            ["5"] = true,
            ["6"] = true, -- Ecstatic
        },
        ['tooltip-hover']=true,
        ['tooltip-hover-job']=true,
        ['tooltip-hover-name']=true,
        ['tooltip-hover-stress']=true,
        ['tooltip-hover-stress-levels']={
            ["0"] = true, -- Miserable
            ["1"] = true,
            ["2"] = false,
            ["3"] = false,
            ["4"] = false,
            ["5"] = true,
            ["6"] = true, -- Ecstatic
        },
        ['tooltip-stress-levels']={
            -- keep in mind, the text will look differently with game's font
            -- colors are same as in ASCII mode, but for then middle (3), which is GREY instead of WHITE
            ["0"] = {text = "=C", pen = COLOR_RED,        name = "Miserable"},
            ["1"] = {text = ":C", pen = COLOR_LIGHTRED,   name = "Unhappy"},
            ["2"] = {text = ":(", pen = COLOR_YELLOW,     name = "Displeased"},
            ["3"] = {text = ":]", pen = COLOR_GREY,       name = "Content"},
            ["4"] = {text = ":)", pen = COLOR_GREEN,      name = "Pleased"},
            ["5"] = {text = ":D", pen = COLOR_LIGHTGREEN, name = "Happy"},
            ["6"] = {text = "=D", pen = COLOR_LIGHTCYAN,  name = "Ecstatic"},
        }
    }
end

local function load_state()
    local state = get_default_state()
    local config_file = json.open('dfhack-config/spectate.json')
    for key in pairs(config_file.data) do
        if state[key] == nil then
            config_file.data[key] = nil
        end
    end
    utils.assign(state, config_file.data)
    config_file.data = state
    return config_file.data,
           function() config_file:write() end
end

local config, save_state = load_state()

function refresh_cpp_config()
    for name,value in pairs(config) do
        if not name:startswith(lua_only_settings_prefix) then
            if type(value) == 'boolean' then
                value = value and 1 or 0
            end
            spectate_setSetting(name, value)
        end
    end
end

function show_squads_warning()
    local message = {
        'Cannot start spectate mode while auto-disengage is enabled and',
        'the squads panel is open. The auto-disengage feature automatically',
        'stops spectate mode when you open the squads panel.',
        '',
        'Please either close the squads panel or disable auto-disengage by',
        'running the following command:',
        '',
        'spectate set auto-disengage false',
    }
    dlg.showMessage("Spectate", table.concat(message, '\n'))
end

-----------------------------
-- commandline interface

local function pairsByKeys(t, f)
    local a = {}
    for n in pairs(t) do table.insert(a, n) end
    table.sort(a, f)
    local i = 0      -- iterator variable
    local iter = function ()   -- iterator function
        i = i + 1
        if a[i] == nil then return nil
        else return a[i], t[a[i]]
        end
    end
    return iter
end

-- no recursion protection, but it shouldn't be needed for a config...
local function print_table(t, indent)
    indent = indent or ''
    for key, value in pairsByKeys(t) do
        if type(value) == 'table' then
            print(indent .. key .. ':')
            print_table(value, indent .. '  ')
        else
            print(indent .. key .. ': ' .. tostring(value))
        end
    end
end

local function print_status()
    print('spectate is:', isEnabled() and 'enabled' or 'disabled')
    print()
    print('settings:')

    print_table(config, '  ')
end

local function do_toggle()
    if isEnabled() then
        dfhack.run_command('disable', 'spectate')
    else
        dfhack.run_command('enable', 'spectate')
    end
end

local function set_setting(args)
    local key = table.remove(args, 1)
    if config[key] == nil then
        qerror('unknown setting: ' .. key)
    end
    local n = #args
    if n == 0 then
        qerror('missing value')
    end

    if n == 1 then
        local value = args[1]
        if key == 'follow-seconds' then
            value = argparse.positiveInt(value, 'follow-seconds')
        elseif key == 'tooltip-follow-blink-milliseconds' then
            value = argparse.nonnegativeInt(value, 'tooltip-follow-blink-milliseconds')
        else
            value = argparse.boolean(value, key)
        end

        config[key] = value

        if not key:startswith(lua_only_settings_prefix) then
            if type(value) == 'boolean' then
                value = value and 1 or 0
            end
            spectate_setSetting(key, value)
        end
    else
        local t = config[key]
        for i = 1, n - 2 do
            t = t[args[i]]
        end
        local k = args[n-1]
        local v = args[n]
        if key ~= 'tooltip-follow-job-shortenings' then
            -- user should be able to add new shortenings, but not other things
            if t[k] == nil then
                table.remove(args)
                qerror('unknown setting: ' .. key .. '/' .. table.concat(args, '/'))
            elseif key:endswith('-stress-levels') then
                v = argparse.boolean(v, key .. '/' .. k)
            end
        end
        t[k] = v
    end

    save_state()
end

local function set_overlay(value)
    value = argparse.boolean(value, name)
    dfhack.run_command('overlay', value and 'enable' or 'disable', 'spectate.tooltip')
end

function parse_commandline(args)
    local command = table.remove(args, 1)
    if not command or command == 'status' then
        print_status()
    elseif command == 'toggle' then
        do_toggle()
    elseif command == 'set' then
        set_setting(args)
    elseif command == 'overlay' then
        set_overlay(args[1])
    else
        return false
    end

    return true
end

-----------------------------
-- info functions

local function GetUnitStress(unit, stress_levels)
    local stressCat = dfhack.units.getStressCategory(unit)
    if stressCat > 6 then stressCat = 6 end
    stressCat = tostring(stressCat)
    if not stress_levels[stressCat] then return end

    local level_cfg = config['tooltip-stress-levels'][stressCat]
    return {text=level_cfg.text, pen=level_cfg.pen}
end

local function GetUnitName(unit)
    return dfhack.units.getReadableName(unit)
end

local function GetUnitJob(unit)
    local job = unit.job.current_job
    return job and dfhack.job.getName(job)
end

local function GetRelevantSettings(key)
    return config['tooltip-' .. key .. '-name'],
           config['tooltip-' .. key .. '-job'],
           config['tooltip-' .. key .. '-stress'],
           config['tooltip-' .. key .. '-stress-levels'],
           config['tooltip-' .. key .. '-job-shortenings']
end

local function GetUnitInfoText(unit, settings_group_name)
    local show_name, show_job, show_stress, stress_levels, job_shortenings = GetRelevantSettings(settings_group_name)

    local stress = show_stress and GetUnitStress(unit, stress_levels) or nil
    local name = show_name and GetUnitName(unit) or nil
    local job = show_job and GetUnitJob(unit) or nil
    if job_shortenings then job = job_shortenings[job] or job end

    local txt = {}
    if stress then
        txt[#txt+1] = stress
        if name or job then txt[#txt+1] = ' ' end
    end
    if name then
        txt[#txt+1] = name
    end
    if job then
        if name then txt[#txt+1] = ": " end
        txt[#txt+1] = job
    end

    return txt
end

local function GetHoverText(pos)
    if not pos then return end

    local txt = {}
    local units = dfhack.units.getUnitsInBox(pos, pos) or {} -- todo: maybe (optionally) use filter parameter here?

    for _,unit in ipairs(units) do
        local info = GetUnitInfoText(unit, 'hover')
        if not next(info) then goto continue end

        for _,t in ipairs(info) do
            txt[#txt+1] = t
        end
        txt[#txt+1] = NEWLINE

        ::continue::
    end

    return txt
end

-----------------------------
-- overlays

TooltipOverlay = defclass(TooltipOverlay, overlay.OverlayWidget)
TooltipOverlay.ATTRS{
    desc='Adds info tooltips that follow units or appear when you hover the mouse.',
    default_pos={x=1,y=1},
    fullscreen=true,
    viewscreens='dwarfmode/Default',
}

function TooltipOverlay:init()
    self:addviews{MouseTooltip{view_id = 'tooltip'}}
end

function TooltipOverlay:preUpdateLayout(parent_rect)
    -- this is required, otherwise there is no room to draw child widgets in
    self.frame.w = parent_rect.width
    self.frame.h = parent_rect.height
end

function TooltipOverlay:render(dc)
    self:render_unit_banners(dc)
    TooltipOverlay.super.render(self, dc)
end

local function AnyFollowOptionOn()
    return config['tooltip-follow-job']
        or config['tooltip-follow-name']
        or config['tooltip-follow-stress']
end

-- map coordinates -> interface layer coordinates
local function GetScreenCoordinates(map_coord)
    -- -> map viewport offset
    local vp = df.global.world.viewport
    local vp_Coord = vp.corner
    local map_offset_by_vp = {
        x = map_coord.x - vp_Coord.x,
        y = map_coord.y - vp_Coord.y,
        z = map_coord.z - vp_Coord.z,
    }

    if not dfhack.screen.inGraphicsMode() then
        return map_offset_by_vp
    else
        -- -> pixel offset
        local gps = df.global.gps
        local map_tile_pixels = gps.viewport_zoom_factor // 4;
        local screen_coord_px = {
            x = map_tile_pixels * map_offset_by_vp.x,
            y = map_tile_pixels * map_offset_by_vp.y,
        }
        -- -> interface layer coordinates
        local screen_coord_text = {
            x = math.ceil( screen_coord_px.x / gps.tile_pixel_x ),
            y = math.ceil( screen_coord_px.y / gps.tile_pixel_y ),
        }

        return screen_coord_text
    end
end

local function GetString(tokens)
    local sb = {}
    for _, tok in ipairs(tokens) do
        if type(tok) == "string" then
            sb[#sb+1] = tok
        else -- must be a table token
            sb[#sb+1] = tok.text
        end
    end
    if not next(sb) then return nil end
    return table.concat(sb)
end

function TooltipOverlay:render_unit_banners(dc)
    if not (config['tooltip-follow'] and AnyFollowOptionOn()) then return end

    local blink_duration = config['tooltip-follow-blink-milliseconds']
    if blink_duration > 0 and not gui.blink_visible(blink_duration) then
        return
    end

    if not dfhack.screen.inGraphicsMode() and not gui.blink_visible(500) then
        return
    end

    local vp = df.global.world.viewport
    local topleft = vp.corner
    local width = vp.max_x
    local height = vp.max_y
    local bottomright = {x = topleft.x + width, y = topleft.y + height, z = topleft.z}

    local units = dfhack.units.getUnitsInBox(topleft, bottomright)
    if not units or #units == 0 then return end

    local oneTileOffset = GetScreenCoordinates({x = topleft.x + 1, y = topleft.y + 1, z = topleft.z + 0})
    local pen = COLOR_WHITE

    local used_tiles = {}
    -- reverse order yields better offsets for overlapping texts
    for i = #units, 1, -1 do
        local unit = units[i]

        local posX, posY, posZ = dfhack.units.getPosition(unit)
        if not posX then goto continue end
        local pos = xyz2pos(posX, posY, posZ)

        local info = GetUnitInfoText(unit, 'follow')
        if not info or not next(info) then goto continue end

        local str = GetString(info)
        if not str then goto continue end

        local scrPos = GetScreenCoordinates(pos)
        local y = scrPos.y - 1 -- subtract 1 to move the text over the heads
        local x = scrPos.x + oneTileOffset.x - 1 -- subtract 1 to move the text inside the map tile

        -- to resolve overlaps, we'll mark every coordinate we write anything in,
        -- and then check if the new tooltip will overwrite any used coordinate.
        -- if it will, try the next row, to a maximum offset of 4.
        local row
        local dy = 0
        -- todo: search for the "best" offset instead, f.e. max `usedAt` value, with `-1` the best
        local usedAt = -1
        for yOffset = 0, max_banner_y_offset do
            dy = yOffset

            row = used_tiles[y + dy]
            if not row then
                row = {}
                used_tiles[y + dy] = row
            end

            usedAt = -1
            for j = 0, #str - 1 do
                if row[x + j] then
                    usedAt = j
                    break
                end
            end

            if usedAt == -1 then break end
        end -- for dy
        -- if other text starts at the same position, or even 2 to the right,
        -- we can't place any useful information, and will ignore it instead.
        if 0 <= usedAt and usedAt <= 2 then goto continue end

        local writer = dc:seek(x, y + dy)
        local ix = 0
        for _, tok in ipairs(info) do
            local s
            if type(tok) == "string" then
                writer = writer:pen(pen)
                s = tok
            else
                writer = writer:pen(tok.pen)
                s = tok.text
            end

            -- in case there isn't enough space, cut the text off
            local len = #s
            if usedAt > 0 and ix + len + 1 >= usedAt then
                -- last position we can write is `usedAt - len - ix - 1`
                -- we want to replace it with an `_`, so we need another `- 1`
                s = s:sub(1, usedAt - len - ix - 1 - 1) .. '_'

                writer = writer:string(s)
                break -- nothing more will fit
            else
                writer = writer:string(s)
            end

            ix = ix + len
        end

        -- mark coordinates as used
        for j = 0, #str - 1 do
            row[x + j] = true
        end

        ::continue::
    end
end

-- MouseTooltip is an almost copy&paste of the DimensionsTooltip
MouseTooltip = defclass(MouseTooltip, widgets.ResizingPanel)

MouseTooltip.ATTRS{
    frame_style=gui.FRAME_THIN,
    frame_background=gui.CLEAR_PEN,
    no_force_pause_badge=true,
    auto_width=true,
    display_offset={x=3, y=3},
}

function MouseTooltip:init()
    ensure_key(self, 'frame').w = 17
    self.frame.h = 4

    self.label = widgets.Label{
        frame={t=0},
        auto_width=true,
    }

    self:addviews{
        widgets.Panel{
            -- set minimum size for tooltip frame so the DFHack frame badge fits
            frame={t=0, l=0, w=7, h=2},
        },
        self.label,
    }
end

local function AnyHoverOptionOn()
    return config['tooltip-hover-job']
        or config['tooltip-hover-name']
        or config['tooltip-hover-stress']
end

function MouseTooltip:render(dc)
    if not (config['tooltip-hover'] and AnyHoverOptionOn()) then return end

    local x, y = dfhack.screen.getMousePos()
    if not x then return end

    local pos = dfhack.gui.getMousePos()
    local text = GetHoverText(pos)
    if not text or not next(text) then return end
    self.label:setText(text)

    local sw, sh = dfhack.screen.getWindowSize()
    local frame_width = math.max(9, self.label:getTextWidth() + 2)
    self.frame.l = math.min(x + self.display_offset.x, sw - frame_width)
    self.frame.t = math.min(y + self.display_offset.y, sh - self.frame.h)
    self:updateLayout()
    MouseTooltip.super.render(self, dc)
end

OVERLAY_WIDGETS = {
    tooltip=TooltipOverlay,
}

return _ENV
