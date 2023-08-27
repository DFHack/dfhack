-- Swap color palettes with the changes of the seasons.
--
-- Based on my script for the Rubble addon "Libs/Colors/Swap Palette", modified at
-- Meph's request to support automatic seasonal switching.

--[[
Copyright 2016 Milo Christiansen

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim
that you wrote the original software. If you use this software in a product, an
acknowledgment in the product documentation would be appreciated but is not
required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
]]

--@ module = true
--@ enable = true

local help = [====[

season-palette
==============

Swap color palettes when the seasons change.

For this script to work you need to add *at least* one color palette file to
your save raw directory.

Palette file names:
    "colors.txt": The world "default" (worldgen and replacement) palette.
    "colors_spring.txt": The palette displayed during spring.
    "colors_summer.txt": The palette displayed during summer.
    "colors_autumn.txt": The palette displayed during autumn.
    "colors_winter.txt": The palette displayed during winter.

If you do not provide a world default palette, palette switching will be
disabled for the current world. The seasonal palettes are optional, the default
palette is not! The default palette will be used to replace any missing
seasonal palettes and during worldgen.

When the world is unloaded or this script is disabled, the system default color
palette ("/data/init/colors.txt") will be loaded. The system default palette
will always be used in the main menu, but your custom palettes should be used
everywhere else.

Usage:

    ``season-palette start|enable`` or ``enable season-palette``:
        Begin swapping seasonal color palettes.

    ``season-palette stop|disable`` or ``disable season-palette``:
        Stop swapping seasonal color palettes and load the default color
        palette.

If loaded as a module this script will export a single Lua function:

    ``LoadPalette(path)``:
        Load a color palette from the text file at "path". This file must be in
        the same format as "/data/init/colors.txt". If there is an error any
        changes will be reverted and this function will return false (returns
        true normally).
]====]

local mappingsA = {
    ["BLACK"] = 0,
    ["BLUE"] = 1,
    ["GREEN"] = 2,
    ["CYAN"] = 3,
    ["RED"] = 4,
    ["MAGENTA"] = 5,
    ["BROWN"] = 6,
    ["LGRAY"] = 7,
    ["DGRAY"] = 8,
    ["LBLUE"] = 9,
    ["LGREEN"] = 10,
    ["LCYAN"] = 11,
    ["LRED"] = 12,
    ["LMAGENTA"] = 13,
    ["YELLOW"] = 14,
    ["WHITE"] = 15,
}

local mappingsB = {
    ["R"] = 0,
    ["G"] = 1,
    ["B"] = 2,
}

local function trimPrefix(pre, str)
    if (string.find(str, pre, 1, true)) ~= 1 then return str end
    return string.sub(str, #pre + 1)
end

function LoadPalette(path)
    -- Strip off the DF path (if it is included) so the log message is shorter.
    print("Attempting to load color palette file: \""..trimPrefix(dfhack.getDFPath(), path).."\"")

    local file, err = io.open(path, "rb")
    if file == nil then
        -- Use a less verbose error message to keep the console cleaner.
        --print("  Load failed: "..err)
        print("  Could not open file.")
        return false
    end

    local contents = file:read("*a")
    file:close()

    -- Keep track of the old colors so we can revert to them if we have trouble parsing the new color file.
    local revcolors = {}
    for a = 0, 15, 1 do
        revcolors[a] = {}
        for b = 0, 2, 1 do
            revcolors[a][b] = df.global.enabler.ccolor[a][b]
        end
    end

    -- If only I could use the Rubble raw parser here... Oh well, I suppose regular expressions will do almost as well.
    for a, b, v in string.gmatch(contents, "%[([A-Z]+)_([RGB]):([0-9]+)%]") do
        local ka, kb, v = mappingsA[a], mappingsB[b], tonumber(v)
        if ka == nil or kb == nil or v == nil then
            -- Parse error, revert changes.
            for x = 0, 15, 1 do
                for y = 0, 2, 1 do
                    df.global.enabler.ccolor[x][y] = revcolors[x][y]
                end
            end
            print("  Color file parse error (all changes reverted).")
            return false
        end

        if v == 0 then
            df.global.enabler.ccolor[ka][kb] = 0
        else
            v = v / 255
            if v > 1 then
                print("  Warning: The "..b.." component for color "..a.." is out of range! Adjusting value.")
                v = 1
            end
            df.global.enabler.ccolor[ka][kb] = v
        end
    end
    df.global.gps.force_full_display_count = 1
    return true
end

if moduleMode then return end

function usage()
    print(help)
end

local args = {...}
if dfhack_flags.enable then
    table.insert(args, dfhack_flags.enable_state and 'enable' or 'disable')
end

--luacheck: global
enabled = false
if #args >= 1 then
    if args[1] == 'start' or args[1] == 'enable' then
        enabled = true
    elseif args[1] == 'stop' or args[1] == 'disable' then
        enabled = false
    else
        usage()
        return
    end
else
    usage()
    return
end

local seasons = {
    [-1] = '', -- worldgen
    [0] = '_spring',
    [1] = '_summer',
    [2] = '_autumn',
    [3] = '_winter',
}
lastSeason = lastSeason or nil --as:number

local function seasonSwapLoop()
    if not enabled then
        LoadPalette(dfhack.getDFPath().."/data/init/colors.txt")
        return
    end

    if lastSeason == nil or lastSeason ~= df.global.cur_season then
        lastSeason = df.global.cur_season

        -- Try to load the seasonal palette, if that fails fall back on the world default palette.
        if not LoadPalette(dfhack.getSavePath().."/raw/colors"..seasons[lastSeason]..".txt") then
            if seasons[lastSeason] == "" or not LoadPalette(dfhack.getSavePath().."/raw/colors.txt") then
                print("The current world do not provide a valid default palette, disabling palette swapping for this world.")
                return
            end
        end
    end

    dfhack.timeout(50, 'ticks', seasonSwapLoop)
end

dfhack.onStateChange.SeasonPalette = function(event)
    if event == SC_WORLD_LOADED and enabled then
        seasonSwapLoop()
    end
    if event == SC_WORLD_UNLOADED and enabled then
        LoadPalette(dfhack.getDFPath().."/data/init/colors.txt")
        -- Ticker is auto-canceled by DFHack.
    end
end

if dfhack.isWorldLoaded() then
    dfhack.onStateChange.SeasonPalette(SC_WORLD_LOADED)
end
