-- Adds more info to the Kitchen screen.
--@ enable = true
--[====[

gui/kitchen-info
================
Adds more info to the Kitchen screen.

Usage:
    gui/kitchen-info enable|disable|help
    enable|disable gui/kitchen-info

]====]

--[[
Possible future improvements:
* add option on Vegetables/fruit/leaves tab to show all processable plants and growths, not just cookable or brewable ones
* add column settings sub-window for selecting which columns are visible
--]]

gui = require 'gui'
utils = require 'utils'
gps = df.global.gps

kitchen_overlay = defclass(kitchen_overlay, gui.Screen)
kitchen_overlay.ATTRS {
    focus_path = 'kitchen_overlay'
}

local show_processing = true

local COLUMNS = {
    {
        label = "Extract",
        maxValueLength = 6,
        minValueLeftPadding = 0,
        emptyValueLeftPadding = 1,
        activeOnPages = { 'VegetablesFruitLeaves' }
    },
    {
        label = "ToBag",
        maxValueLength = 5,  -- actually 6 but rarely appears
        minValueLeftPadding = 0,
        emptyValueLeftPadding = 0,
        activeOnPages = { 'VegetablesFruitLeaves' },
        hidden = true  -- This column is hidden because the only plant with BAG_ITEM reaction is Quarry Bush, which is neither cookable nor brewable so it never appears in the Kitchen screen
    },
    {
        label = "Thread",
        maxValueLength = 6,
        minValueLeftPadding = 0,
        emptyValueLeftPadding = 1,
        activeOnPages = { 'VegetablesFruitLeaves' }
    },
    {
        label = "Mill",
        maxValueLength = 5,
        minValueLeftPadding = 0,
        activeOnPages = { 'VegetablesFruitLeaves', 'Seeds' }
    },
    --[[
    -- Unimplemented
    {
        label = "Press",
        maxValueLength = 5,
        minValueLeftPadding = 0,
        activeOnPages = { 'VegetablesFruitLeaves', 'Seeds' }
    },
    --]]
    {
        label = "EatRaw",
        maxValueLength = 4,
        minValueLeftPadding = 1,
        emptyValueLeftPadding = 1,
        activeOnPages = { 'VegetablesFruitLeaves', 'Seeds' }
    }
}

local COLUMNS_BY_LABEL = {}

for _, column in ipairs(COLUMNS) do
    column.activeOnPages = utils.invert(column.activeOnPages)
    column.minLength = math.max(#column.label, column.minValueLeftPadding + column.maxValueLength)

    COLUMNS_BY_LABEL[column.label] = column
end

function COLUMNS_BY_LABEL.Extract.getColumnValue(item_type, material, plant_raw)
    if (item_type == df.item_type.PLANT) then
        if (plant_raw.flags.EXTRACT_BARREL) then
            return "Barrel"
        elseif (plant_raw.flags.EXTRACT_VIAL) then
            return " Vial"
        elseif (plant_raw.flags.EXTRACT_STILL_VIAL) then
            return " Still"
        end
    end
    return false
end

-- This column is hidden because the only plant with BAG_ITEM reaction is Quarry Bush, which is neither cookable nor brewable so it never appears in the Kitchen screen
function COLUMNS_BY_LABEL.ToBag.getColumnValue(item_type, material, plant_raw)
    if (item_type == df.item_type.PLANT) then
        local reaction_product = material.reaction_product
        for i, reaction_product_id in ipairs(reaction_product.id) do
            if (reaction_product_id.value == "BAG_ITEM") then
                local bag_item_type = reaction_product.item_type[i]

                if (bag_item_type == df.item_type.PLANT_GROWTH) then
                    local bag_item_subtype = reaction_product.item_subtype[i]
                    local bag_mat_type  = reaction_product.material.mat_type[i]
                    local bag_mat_index = reaction_product.material.mat_index[i]
                    local bag_matinfo = dfhack.matinfo.decode(bag_mat_type, bag_mat_index)
                    local bag_mat_plant_raw = bag_matinfo.plant
                    local bag_mat_plant_growth = bag_mat_plant_raw.growths[bag_item_subtype]

                    if (bag_mat_plant_growth.name_plural:sub(-6) == "leaves") then
                        return "Leaves"
                    end
                end

                return "Other"
            end
        end
    end
    return false
end

function COLUMNS_BY_LABEL.Thread.getColumnValue(item_type, material, plant_raw)
    if (item_type == df.item_type.PLANT and plant_raw.flags.THREAD) then
        return "Thread"
    end
    return false
end

function COLUMNS_BY_LABEL.Mill.getColumnValue(item_type, material, plant_raw)
    if (item_type == df.item_type.PLANT and plant_raw.flags.MILL) then
        local mill_mat_type = plant_raw.material_defs.type.mill
        local mill_mat_index = plant_raw.material_defs.idx.mill
        local mill_matinfo = dfhack.matinfo.decode(mill_mat_type, mill_mat_index)
        local mill_material = mill_matinfo.material

        if (mill_material.flags.IS_DYE) then
            return "Dye"
        elseif (mill_material.state_name.Powder:sub(-5) == "flour") then
            return "Flour"
        else
            return "Other"
        end
    elseif (item_type == df.item_type.SEEDS) then
        for _, reaction_product_id in ipairs(material.reaction_product.id) do
            if (reaction_product_id.value == "PRESS_LIQUID_MAT") then
                return "Paste"
            end
        end
    end
    return false
end

--[[
-- TODO?
function COLUMNS_BY_LABEL.Press.getColumnValue(item_type, material, plant_raw)
    return false
end
--]]

function COLUMNS_BY_LABEL.EatRaw.getColumnValue(item_type, material, plant_raw)
    if (material.flags.EDIBLE_RAW) then
        return "Eat"
    end
    return false
end

-- In TrueType mode the string can be found as a series of TrueType tiles of the same length as the expected string
-- Column labels "Ingredient Type", "Number" and "Permissions" all have different lengths, which allows this method to find distinct positions of them
local function findTrueTypeSeriesInLine(x1, x2, y, length)
    local seriesStartX = x1
    local seriesLength = 1  -- assumes first tile is TrueType
    for x = x1 + 1, x2 do
        if (dfhack.screen.readTile(x, y) == nil) then  -- series started or continues
            if (seriesLength == 0) then  -- series started
                seriesStartX = x
            end
            seriesLength = seriesLength + 1
        elseif (seriesLength == length) then  -- series of expected length ended
            break
        else  -- series of other length ended, reset counter for next series
            seriesLength = 0
        end
    end
    if (seriesLength == length) then  -- recheck the length of the series at the end, in case the loop ended normally, not because of break
        return seriesStartX, seriesStartX + length - 1
    end
    return -1, -1
end

local function findStringInLine(x1, x2, y, str)
    local length = #str
    local firstByte = str:byte(1)
    local xEnd = x2 - (length - 1)
    for x = x1, xEnd do
        local tile = dfhack.screen.readTile(x, y)

        if (tile == nil) then  -- TrueType text detected
            return findTrueTypeSeriesInLine(x, x2, y, length)
        elseif (tile.ch == firstByte) then
            local matches = true
            for i = 2, length do
                tile = dfhack.screen.readTile(x + i - 1, y)

                if (not tile) or (tile.ch ~= str:byte(i)) then
                    matches = false
                    break
                end
            end
            if (matches) then
                return x, x + length - 1
            end
        end
    end
    return -1, -1
end

local function pluginCompatibility_search_isInputActive()
    return true  -- FIXME: Implement when API is available
end

local function pluginCompatibility_tweak_kitchen_prefs_color_isEnabled()
    return false  -- FIXME: Implement when API is available
end

local function drawNumberColumn(x, kitchen, firstVisibleIndex, numDisplayedItems)
    dfhack.screen.paintString({ fg = COLOR_WHITE }, x, 4, "Number")

    local p = gui.Painter.new_xy(x, 6, x + 10 - 1, 6 + numDisplayedItems - 1)
    local count = kitchen.count[kitchen.page]

    for i = 0, numDisplayedItems - 1 do
        local index = firstVisibleIndex + i

        p:seek(0, i)
        local color = (index == kitchen.cursor) and COLOR_YELLOW or COLOR_BROWN
        p:pen(color):string(tostring(count[index]))
    end
end

local function drawPermissionsColumn(x, kitchen, firstVisibleIndex, numDisplayedItems)
    dfhack.screen.paintString({ fg = COLOR_WHITE }, x, 4, "Permissions")

    local p = gui.Painter.new_xy(x, 6, x + string.len("Permissions") - 1, 6 + numDisplayedItems - 1)
    local possible  = kitchen.possible[kitchen.page]
    local forbidden = kitchen.forbidden[kitchen.page]

    local unforbiddenColor = pluginCompatibility_tweak_kitchen_prefs_color_isEnabled() and COLOR_LIGHTGREEN or COLOR_LIGHTBLUE

    for i = 0, numDisplayedItems - 1 do
        local index = firstVisibleIndex + i

        p:seek(1, i)
        if (possible[index].Cook) then
            local color = forbidden[index].Cook and COLOR_LIGHTRED or unforbiddenColor
            p:pen(color):string("Cook")
        else
            p:pen(COLOR_DARKGREY):string("----")
        end

        p:seek(6, i)
        if (possible[index].Brew) then
            local color = forbidden[index].Brew and COLOR_LIGHTRED or unforbiddenColor
            p:pen(color):string("Brew")
        else
            p:pen(COLOR_DARKGREY):string("----")
        end
    end
end

function kitchen_overlay:onRender()
    local kitchen = self._native.parent
    kitchen:render()

    if (not enabled) then
        self:dismiss()
        return
    end

    local origNumberStart, origNumberEnd = findStringInLine(2, gps.dimx - 3, 4, "Number")
    local origPermissionsStart, origPermissionsEnd = findStringInLine(math.max(2, origNumberEnd + 1), gps.dimx - 3, 4, "Permissions")
    local numberStart = origNumberStart
    local permissionsStart, permissionsEnd = origPermissionsStart, origPermissionsEnd

    local numItems = #kitchen.item_str[kitchen.page]
    local maxItemsOnPage = 1 + (gps.dimy - 5) - 6
    local cursorPage = math.floor(kitchen.cursor / maxItemsOnPage)
    local firstVisibleIndex = cursorPage * maxItemsOnPage
    local numDisplayedItems = math.min(numItems - firstVisibleIndex, maxItemsOnPage)

    if show_processing then
        local pageName = df.viewscreen_kitchenpref_page[kitchen.page]

        local visibleColumns = {}
        for _, column in ipairs(COLUMNS) do
            if (not column.hidden) and column.activeOnPages[pageName] then
                table.insert(visibleColumns, column)
            end
        end

        if (#visibleColumns >= 1 and origPermissionsEnd ~= -1) then
            local columnsPos = origPermissionsEnd + 3
            local currentOffset = 0
            local columnOffset = {}
            local prevColumn = nil

            for i, column in ipairs(visibleColumns) do
                if (prevColumn) then
                    currentOffset = currentOffset + math.max(#prevColumn.label, prevColumn.minValueLeftPadding + prevColumn.maxValueLength - column.minValueLeftPadding) + 2
                end

                columnOffset[i] = currentOffset
                prevColumn = column
            end

            local allColumnsWidth = currentOffset + prevColumn.minLength

            -- Check whether to redraw Permissions and Number columns
            if (columnsPos + allColumnsWidth > gps.dimx - 2) then
                columnsPos = gps.dimx - 2 - allColumnsWidth

                permissionsEnd = columnsPos - 3
                permissionsStart = permissionsEnd - (origPermissionsEnd - origPermissionsStart)

                dfhack.screen.fillRect(gui.CLEAR_PEN, origPermissionsStart, 4, origPermissionsEnd, 6 + numDisplayedItems - 1)

                if (numberStart + 9 >= permissionsStart) then
                    numberStart = permissionsStart - 9

                    dfhack.screen.fillRect(gui.CLEAR_PEN, numberStart, 4, origNumberStart + 10 - 1, 6 + numDisplayedItems - 1)
                    drawNumberColumn(numberStart, kitchen, firstVisibleIndex, numDisplayedItems)
                end

                drawPermissionsColumn(permissionsStart, kitchen, firstVisibleIndex, numDisplayedItems)
            end

            -- Column labels
            local labels_p = gui.Painter.new_xy(columnsPos, 4, gps.dimx - 2, 4)
            labels_p:pen(COLOR_WHITE)

            for i, column in ipairs(visibleColumns) do
                labels_p:seek(columnOffset[i], 0):string(column.label)
            end

            -- Columns data
            local p = gui.Painter.new_xy(columnsPos, 6, gps.dimx - 3, 6 + numDisplayedItems - 1)

            local item_type    = kitchen.item_type[kitchen.page]
            local item_subtype = kitchen.item_subtype[kitchen.page]
            local mat_type     = kitchen.mat_type[kitchen.page]
            local mat_index    = kitchen.mat_index[kitchen.page]

            for i = 0, numDisplayedItems - 1 do
                local index = firstVisibleIndex + i
                local matinfo = dfhack.matinfo.decode(mat_type[index], mat_index[index])
                local material = matinfo.material
                local plant_raw = df.plant_raw.find( mat_index[index] )

                for c, column in ipairs(visibleColumns) do
                    local columnValue = column.getColumnValue(item_type[index], material, plant_raw)

                    p:seek(columnOffset[c], i)
                    if (columnValue) then
                        p:advance(column.minValueLeftPadding)
                        p:pen(COLOR_LIGHTGREEN):string(columnValue)
                    else
                        p:advance(column.emptyValueLeftPadding)
                        p:pen(COLOR_DARKGREY):string("----")
                    end
                end
            end
        end
    end

    -- Cursor
    local cursorY = kitchen.cursor - firstVisibleIndex

    if (numDisplayedItems >= 1 and cursorY < numDisplayedItems) then
        local cursor_p = gui.Painter.new_xy(0, 6 + cursorY, gps.dimx - 1, 6 + cursorY)

        cursor_p:pen(COLOR_LIGHTGREEN)
        --cursor_p:seek(1, 0):char('>')
        cursor_p:seek(permissionsStart, 0):char('[')
        cursor_p:seek(permissionsEnd, 0):char(']')
        if show_processing then
            cursor_p:seek(gps.dimx - 2, 0):char('<')
        end
    end

    -- Hotkey
    --local keys_p = gui.Painter.new_xy(1, gps.dimy - 3, gps.dimx - 2, gps.dimy - 2)
    --local keys_x = math.min(gps.dimx - 3 - string.len("p: Hide processing info"), 62)
    --keys_p:key_pen(COLOR_LIGHTRED):pen(COLOR_WHITE)
    --keys_p:seek(keys_x, 1):key('CUSTOM_P'):string(show_processing and ": Hide processing info" or ": Show processing info")
end

function kitchen_overlay:onInput(keys)
    if not pluginCompatibility_search_isInputActive() then
        if keys.CUSTOM_P then
            show_processing = not show_processing
        end
    end

    self:sendInputToParent(keys)

    local parent = self._native.parent
    if dfhack.screen.isDismissed(parent) then
        self:dismiss()
    end
end

local stateChangeHandler = function(eventCode)
    if (eventCode == SC_VIEWSCREEN_CHANGED) then
        local curFocus  = dfhack.gui.getCurFocus()
        local curScreen = dfhack.gui.getCurViewscreen()

        if curFocus == 'kitchenpref' and (not dfhack.screen.isDismissed(curScreen)) then
            kitchen_overlay():show()
        end
    end
end

args = {...}
if dfhack_flags and dfhack_flags.enable then
    args = {dfhack_flags.enable_state and 'enable' or 'disable'}
end

if args[1] == 'enable' then
    enabled = true
    dfhack.onStateChange['gui/kitchen-info'] = stateChangeHandler

    if dfhack.gui.getCurFocus() == 'kitchenpref' then
        kitchen_overlay():show()
    end
elseif args[1] == 'disable' then
    enabled = false  -- dismissed in next onRender()
    dfhack.onStateChange['gui/kitchen-info'] = nil
else
    print(dfhack.script_help() .. '\n')
end
