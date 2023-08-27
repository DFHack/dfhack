-- Edit the build mode sidebar menus.
--
-- Based on my script for the Rubble addon "Libs/Change Build List".

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

local usage = [====[
modtools/change-build-menu
==========================

Change the build sidebar menus.

This script provides a flexible and comprehensive system for adding and removing
items from the build sidebar menus. You can add or remove workshops/furnaces by
text ID, or you can add/remove ANY building via a numeric building ID triplet.

Changes made with this script do not survive a save/load. You will need to redo
your changes each time the world loads.

Just to be clear: You CANNOT use this script AT ALL if there is no world
loaded!

**Usage:**

``modtools/change-build-menu start|enable``:

``enable modtools/change-build-menu``:

    Start the ticker. This needs to be done before any changes will take
    effect. Note that you can make changes before or after starting the
    ticker, both options should work equally well.

``modtools/change-build-menu stop|disable``:

``disable modtools/change-build-menu``:

    Stop the ticker. Does not clear stored changes. The ticker will
    automatically stop when the current world is unloaded.

``modtools/change-build-menu add <ID> <CATEGORY> [<KEY>]``:

    Add the workshop or furnace with the ID ``<ID>`` to ``<CATEGORY>``.
    ``<KEY>`` is an optional DF hotkey ID.

    ``<CATEGORY>`` may be one of:
        - MAIN_PAGE
        - SIEGE_ENGINES
        - TRAPS
        - WORKSHOPS
        - FURNACES
        - CONSTRUCTIONS
        - MACHINES
        - CONSTRUCTIONS_TRACK

    Valid ``<ID>`` values for hardcoded buildings are as follows:
        - CARPENTERS
        - FARMERS
        - MASONS
        - CRAFTSDWARFS
        - JEWELERS
        - METALSMITHSFORGE
        - MAGMAFORGE
        - BOWYERS
        - MECHANICS
        - SIEGE
        - BUTCHERS
        - LEATHERWORKS
        - TANNERS
        - CLOTHIERS
        - FISHERY
        - STILL
        - LOOM
        - QUERN
        - KENNELS
        - ASHERY
        - KITCHEN
        - DYERS
        - TOOL
        - MILLSTONE
        - WOOD_FURNACE
        - SMELTER
        - GLASS_FURNACE
        - MAGMA_SMELTER
        - MAGMA_GLASS_FURNACE
        - MAGMA_KILN
        - KILN

``modtools/change-build-menu remove <ID> <CATEGORY>``:

    Remove the workshop or furnace with the ID ``<ID>`` from ``<CATEGORY>``.

    ``<CATEGORY>`` and ``<ID>`` may have the same values as for the "add"
    option.

``modtools/change-build-menu revert <ID> <CATEGORY>``:

    Revert an earlier remove or add operation. It is NOT safe to "remove"
    an "add"ed building or vice versa, use this option to reverse any
    changes you no longer want/need.


**Module Usage:**

To use this script as a module put the following somewhere in your own script:

.. code-block:: lua

    local buildmenu = reqscript "change-build-menu"

Then you can call the functions documented here like so:

    - Example: Remove the carpenters workshop:

    .. code-block:: lua

        buildmenu.ChangeBuilding("CARPENTERS", "WORKSHOPS", false)

    - Example: Make it impossible to build walls (not recommended!):

    .. code-block:: lua

        local typ, styp = df.building_type.Construction, df.construction_type.Wall
        buildmenu.ChangeBuildingAdv(typ, styp, -1, "CONSTRUCTIONS", false)

Note that to allow any of your changes to take effect you need to start the
ticker. See the "Command Usage" section.


**Global Functions:**

``GetWShopID(btype, bsubtype, bcustom)``:
    GetWShopID returns a workshop's or furnace's string ID based on its
    numeric ID triplet. This string ID *should* match what is expected
    by eventful for hardcoded buildings.

``GetWShopType(id)``:
    GetWShopIDs returns a workshop or furnace's ID numbers as a table.
    The passed in ID should be the building's string identifier, it makes
    no difference if it is a custom building or a hardcoded one.
    The return table is structured like so: ``{type, subtype, custom}``

``IsEntityPermitted(id)``:
    IsEntityPermitted returns true if DF would normally allow you to build
    a workshop or furnace. Use this if you want to change a building, but
    only if it is permitted in the current entity. You do not need to
    specify an entity, the current fortress race is used.

``ChangeBuilding(id, category, [add, [key]])``:

``ChangeBuildingAdv(typ, subtyp, custom, category, [add, [key]]):``
    These two functions apply changes to the build sidebar menus. If "add"
    is true then the building is added to the specified category, else it
    is removed. When adding you may specify "key", a string DF hotkey ID.

    The first version of this function takes a workshop or furnace ID as a
    string, the second takes a numeric ID triplet (which can specify any
    building, not just workshops or furnaces).

``RevertBuildingChanges(id, category)``:

``RevertBuildingChangesAdv(typ, subtyp, custom, category)``:
    These two functions revert changes made by "ChangeBuilding" and
    "ChangeBuildingAdv". Like those two functions there are two versions,
    a simple one that takes a string ID and one that takes a numeric ID
    triplet.
]====]

-- From here until I say otherwise the APIs are generic building manipulation functions from "Libs/Buildings"

local utils = require "utils"

local wshop_id_to_type = {
    CARPENTERS = df.workshop_type.Carpenters,
    FARMERS = df.workshop_type.Farmers,
    MASONS = df.workshop_type.Masons,
    CRAFTSDWARFS = df.workshop_type.Craftsdwarfs,
    JEWELERS = df.workshop_type.Jewelers,
    METALSMITHSFORGE = df.workshop_type.MetalsmithsForge,
    MAGMAFORGE = df.workshop_type.MagmaForge,
    BOWYERS = df.workshop_type.Bowyers,
    MECHANICS = df.workshop_type.Mechanics,
    SIEGE = df.workshop_type.Siege,
    BUTCHERS = df.workshop_type.Butchers,
    LEATHERWORKS = df.workshop_type.Leatherworks,
    TANNERS = df.workshop_type.Tanners,
    CLOTHIERS = df.workshop_type.Clothiers,
    FISHERY = df.workshop_type.Fishery,
    STILL = df.workshop_type.Still,
    LOOM = df.workshop_type.Loom,
    QUERN = df.workshop_type.Quern,
    KENNELS = df.workshop_type.Kennels,
    ASHERY = df.workshop_type.Ashery,
    KITCHEN = df.workshop_type.Kitchen,
    DYERS = df.workshop_type.Dyers,
    TOOL = df.workshop_type.Tool,
    MILLSTONE = df.workshop_type.Millstone,
}
local wshop_type_to_id = utils.invert(wshop_id_to_type)

local furnace_id_to_type = {
    WOOD_FURNACE = df.furnace_type.WoodFurnace,
    SMELTER = df.furnace_type.Smelter,
    GLASS_FURNACE = df.furnace_type.GlassFurnace,
    MAGMA_SMELTER = df.furnace_type.MagmaSmelter,
    MAGMA_GLASS_FURNACE = df.furnace_type.MagmaGlassFurnace,
    MAGMA_KILN = df.furnace_type.MagmaKiln,
    KILN = df.furnace_type.Kiln,
}
local furnace_type_to_id = utils.invert(furnace_id_to_type)

-- GetWShopID returns a workshop or furnace's string ID based on it's numeric ID triplet.
-- This string ID *should* match what is expected by eventful for hardcoded buildings.
--luacheck: in=df.building_type,number,number
function GetWShopID(btype, bsubtype, bcustom)
    if btype == df.building_type.Workshop then
        if wshop_type_to_id[bsubtype] ~= nil then
            return wshop_type_to_id[bsubtype]
        else
            return df.building_def.find(bcustom).code
        end
    elseif btype == df.building_type.Furnace then
        if furnace_type_to_id[bsubtype] ~= nil then
            return furnace_type_to_id[bsubtype]
        else
            return df.building_def.find(bcustom).code
        end
    end
end

-- GetWShopIDs returns a workshop or furnace's ID numbers as a table.
-- The passed in ID should be the building's string identifier, it makes
-- no difference if it is a custom building or a hardcoded one.
-- The return table is structured like so: `{type, subtype, custom}`
--luacheck: in=string out={_type:table,type:df.building_type,subtype:number,custom:number}
function GetWShopType(id)
    if wshop_id_to_type[id] ~= nil then
        -- Hardcoded workshop
        return {
            type = df.building_type.Workshop,
            subtype = wshop_id_to_type[id],
            custom = -1,
        }
    elseif furnace_id_to_type[id] ~= nil then
        -- Hardcoded furnace
        return {
            type = df.building_type.Furnace,
            subtype = furnace_id_to_type[id],
            custom = -1,
        }
    else
        -- User defined workshop or furnace.
        for i, def in ipairs(df.global.world.raws.buildings.all) do
            if def.code == id then
                local typ = df.building_type.Furnace
                local styp = df.furnace_type.Custom
                if df.building_def_workshopst:is_instance(def) then --luacheck: skip
                    typ = df.building_type.Workshop
                    styp = df.workshop_type.Custom
                end

                return {
                    type = typ,
                    subtype = styp,
                    custom = i,
                }
            end
        end
    end
    return nil
end

-- OK, Now to "Libs/Change Build List" proper...

--[[
    -- Examples:

    -- Remove the carpenters workshop.
    ChangeBuilding("CARPENTERS", "WORKSHOPS", false)

    -- Make it impossible to build walls (not recommended!).
    ChangeBuildingAdv(df.building_type.Construction, df.construction_type.Wall, -1, "CONSTRUCTIONS", false)

    -- Add the mechanic's workshops to the machines category.
    ChangeBuilding("MECHANICS", "MACHINES", true, "CUSTOM_E")
]]

local category_name_to_id = {
    ["MAIN_PAGE"] = 0,
    ["SIEGE_ENGINES"] = 1,
    ["TRAPS"] = 2,
    ["WORKSHOPS"] = 3,
    ["FURNACES"] = 4,
    ["CONSTRUCTIONS"] = 5,
    ["MACHINES"] = 6,
    ["CONSTRUCTIONS_TRACK"] = 7,
}

--[[
    {
        category = 0, -- The menu category id (from category_name_to_id)
        add = true, -- Are we adding a workshop or removing one?
        id = {
            -- The building IDs.
            type = 0,
            subtype = 0,
            custom = 0,
        },
    }
]]
stuffToChange = stuffToChange or {} --as:{_type:table,category:number,add:bool,key:df.interface_key,id:{_type:table,type:df.building_type,subtype:number,custom:number}}[]

-- Returns true if DF would normally allow you to build a workshop or furnace.
-- Use this if you want to change a building, but only if it is permitted in the current entity.
--luacheck: in=string out=bool
function IsEntityPermitted(id)
    local wshop = GetWShopType(id)

    -- It's hard coded, so yes, of course it's permitted, why did you even ask?
    if wshop.custom == -1 then
        return true
    end

    local entsrc = df.historical_entity.find(df.global.plotinfo.civ_id)
    if entsrc == nil then
        return false
    end
    local entity = entsrc.entity_raw

    for _, bid in ipairs(entity.workshops.permitted_building_id) do
        if wshop.custom == bid then
            return true
        end
    end
    return false
end

function RevertBuildingChanges(id, category)
    local wshop = GetWShopType(id)
    if wshop == nil then
        qerror("RevertBuildingChanges: Invalid workshop ID: "..id)
    end

    RevertBuildingChangesAdv(wshop.type, wshop.subtype, wshop.custom, category)
end

function RevertBuildingChangesAdv(typ, subtyp, custom, category)
    local cat
    if tonumber(category) ~= nil then
        cat = tonumber(category)
    else
        cat = category_name_to_id[category]
        if cat == nil then
            qerror("ChangeBuilding: Invalid category ID: "..category)
        end
    end

    for i = #stuffToChange, 1, -1 do
        local change = stuffToChange[i]
        if change.category == cat then
            if typ == change.id.type and subtyp == change.id.subtype and custom == change.id.custom then
                table.remove(stuffToChange, i)
            end
        end
    end
end

function ChangeBuilding(id, category, add, key)
    local cat
    if tonumber(category) ~= nil then
        cat = tonumber(category)
    else
        cat = category_name_to_id[category]
        if cat == nil then
            qerror("ChangeBuilding: Invalid category ID: "..category)
        end
    end

    local wshop = GetWShopType(id)
    if wshop == nil then
        qerror("ChangeBuilding: Invalid workshop ID: "..id)
    end

    ChangeBuildingAdv(wshop.type, wshop.subtype, wshop.custom, category, add, key)
end

function ChangeBuildingAdv(typ, subtyp, custom, category, add, key)
    local cat
    if tonumber(category) ~= nil then
        cat = tonumber(category)
    else
        cat = category_name_to_id[category]
        if cat == nil then
            qerror("ChangeBuilding: Invalid category ID: "..category)
        end
    end

    if type(key) == 'string' then
        if tonumber(key) == nil then
            key = df.interface_key[key] --luacheck: retype
        else
            key = tonumber(key) --luacheck: retype
        end
    end
    key = key or 0

    table.insert(stuffToChange, {
        category = cat,
        add = add,
        key = key,
        id = {
            type = typ,
            subtype = subtyp,
            custom = custom,
        },
    })
end

-- Return early if module mode. Note that the ticker will not start in module mode!
-- To start the ticker you need to enable the script first ("enable modtools/change-build-menu").
if moduleMode then return end

if not dfhack.isWorldLoaded() then
    dfhack.printerr("change-build-menu: No World Loaded!")
    dfhack.print(usage)
    return
end

local args = {...}
if dfhack_flags and dfhack_flags.enable then
    table.insert(args, dfhack_flags.enable_state and 'enable' or 'disable')
end

tickerOn = tickerOn or false
local tickerStart = false

if #args >= 1 then
    if args[1] == 'start' or args[1] == 'enable' then
        if not tickerOn then tickerStart = true end
    elseif args[1] == 'stop' or args[1] == 'disable' then
        tickerOn = false
    elseif args[1] == 'add' then
        ChangeBuilding(args[2], args[3], true, args[4])
        return
    elseif args[1] == 'remove' then
        ChangeBuilding(args[2], args[3], false)
        return
    elseif args[1] == 'revert' then
        RevertBuildingChanges(args[2], args[3])
        return
    else
        dfhack.print(usage)
        return
    end
else
    dfhack.print(usage)
    return
end

-- These two store the values we *think* are in effect, they are used to detect changes.
sidebarLastCat = sidebarLastCat or -1
sidebarIsBuild = sidebarIsBuild or false

local function checkSidebar()
    -- Needs to be "frames" so it ticks over while paused.
    if tickerOn then
        dfhack.timeout(1, "frames", checkSidebar)
    end

    local sidebar = df.global.game.building

    if not sidebarIsBuild and df.global.plotinfo.main.mode ~= df.ui_sidebar_mode.Build then
        -- Not in build mode.
        return
    elseif sidebarIsBuild and df.global.plotinfo.main.mode ~= df.ui_sidebar_mode.Build then
        -- Just exited build mode
        sidebarIsBuild = false
        sidebarLastCat = -1
        return
    elseif sidebarIsBuild and sidebar.category_id == sidebarLastCat then
        -- In build mode, but category has not changed since last frame.
        return
    end
    -- Either we just entered build mode or the category has changed.
    sidebarIsBuild = true
    sidebarLastCat = sidebar.category_id

    -- Changes made here do not persist, they need to be made every time the side bar is shown.
    -- Will just deleting stuff cause a memory leak? (probably, but how can it be avoided?)

    local stufftoremove = {} --as:number[]
    local stufftoadd = {} --as:stuffToChange
    for i, btn in ipairs(sidebar.choices_all) do
        if df.interface_button_construction_building_selectorst:is_instance(btn) then
            for _, change in ipairs(stuffToChange) do
                if not change.add and sidebar.category_id == change.category and btn.building_type == change.id.type and btn.building_subtype == change.id.subtype and btn.custom_type == change.id.custom then --hint:df.interface_button_construction_building_selectorst
                    table.insert(stufftoremove, i)
                end
            end
        end
    end
    for _, change in ipairs(stuffToChange) do
        if sidebar.category_id == change.category and change.add then
            table.insert(stufftoadd, change)
        end
    end

    -- Do the actual adding and removing.
    -- We need to iterate the list backwards to keep from invalidating the stored indexes.
    for x = #stufftoremove, 1, -1 do
        -- AFAIK item indexes always match (except for one extra item at the end of "choices_all").
        local i = stufftoremove[x]
        sidebar.choices_visible:erase(i)
        sidebar.choices_all:erase(i)
    end
    for _, change in ipairs(stufftoadd) do
        local button = df.interface_button_construction_building_selectorst:new()
        button.hotkey_id = change.key
        button.building_type = change.id.type
        button.building_subtype = change.id.subtype
        button.custom_type = change.id.custom

        local last = #sidebar.choices_visible
        sidebar.choices_visible:insert(last, button)
        sidebar.choices_all:insert(last, button)
    end
end

-- The logic for the ticker is much more complicated here than the Rubble version.
-- The Rubble version can lean on the script loader, this has to do all the work itself.
-- Greatly complicating things is that Rubble will only evaluate a module once, whereas a
-- command script may be evaluated many times.

dfhack.onStateChange.ChangeBuildList = function(event)
    if event == SC_WORLD_LOADED and not tickerOn and tickerStart then
        -- Set ticker flags...
        tickerOn = true
        tickerStart = false

        -- Then start the ticker itself.
        checkSidebar()
    end
    if event == SC_WORLD_UNLOADED and tickerOn then
        -- This will kill the ticker next frame.
        tickerOn = false

        -- The Rubble version doesn't need to worry about this stuff, but we do here...
        sidebarLastCat = -1
        sidebarIsBuild = false
        stuffToChange = nil
    end
end
dfhack.onStateChange.ChangeBuildList(SC_WORLD_LOADED)
