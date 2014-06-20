-- Front-end for the siege engine plugin.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local dlg = require 'gui.dialogs'

local plugin = require 'plugins.siege-engine'
local wmap = df.global.world.map

local LEGENDARY = df.skill_rating.Legendary

-- Globals kept between script calls
last_target_min = last_target_min or nil
last_target_max = last_target_max or nil

local item_choices = {
    { caption = 'boulders (default)', item_type = df.item_type.BOULDER },
    { caption = 'blocks', item_type = df.item_type.BLOCKS },
    { caption = 'weapons', item_type = df.item_type.WEAPON },
    { caption = 'trap components', item_type = df.item_type.TRAPCOMP },
    { caption = 'bins', item_type = df.item_type.BIN },
    { caption = 'barrels', item_type = df.item_type.BARREL },
    { caption = 'cages', item_type = df.item_type.CAGE },
    { caption = 'anything', item_type = -1 },
}

local item_choice_idx = {}
for i,v in ipairs(item_choices) do
    item_choice_idx[v.item_type] = i
end

SiegeEngine = defclass(SiegeEngine, guidm.MenuOverlay)

SiegeEngine.focus_path = 'siege-engine'

SiegeEngine.ATTRS{ building = DEFAULT_NIL }

function SiegeEngine:init()
    self:assign{
        center = utils.getBuildingCenter(self.building),
        selected_pile = 1,
        mode_main = {
            render = self:callback 'onRenderBody_main',
            input = self:callback 'onInput_main',
        },
        mode_aim = {
            render = self:callback 'onRenderBody_aim',
            input = self:callback 'onInput_aim',
        },
        mode_pile = {
            render = self:callback 'onRenderBody_pile',
            input = self:callback 'onInput_pile',
        }
    }
end

function SiegeEngine:onShow()
    SiegeEngine.super.onShow(self)

    self.old_cursor = guidm.getCursorPos()
    self.old_viewport = self:getViewport()

    self.mode = self.mode_main
    self:showCursor(false)
end

function SiegeEngine:onDestroy()
    if self.save_profile then
        plugin.saveWorkshopProfile(self.building)
    end
    if not self.no_select_building then
        self:selectBuilding(self.building, self.old_cursor, self.old_viewport, 10)
    end
end

function SiegeEngine:onGetSelectedBuilding()
    return df.global.world.selected_building
end

function SiegeEngine:showCursor(enable)
    local cursor = guidm.getCursorPos()
    if cursor and not enable then
        self.cursor = cursor
        self.target_select_first = nil
        guidm.clearCursorPos()
    elseif not cursor and enable then
        local view = self:getViewport()
        cursor = self.cursor
        if not cursor or not view:isVisible(cursor) then
            cursor = view:getCenter()
        end
        self.cursor = nil
        guidm.setCursorPos(cursor)
    end
end

function SiegeEngine:centerViewOn(pos)
    local cursor = guidm.getCursorPos()
    if cursor then
        guidm.setCursorPos(pos)
    else
        self.cursor = pos
    end
    self:getViewport():centerOn(pos):set()
end

function SiegeEngine:zoomToTarget()
    local target_min, target_max = plugin.getTargetArea(self.building)
    if target_min then
        local cx = math.floor((target_min.x + target_max.x)/2)
        local cy = math.floor((target_min.y + target_max.y)/2)
        local cz = math.floor((target_min.z + target_max.z)/2)
        local pos = plugin.adjustToTarget(self.building, xyz2pos(cx,cy,cz))
        self:centerViewOn(pos)
    end
end

function paint_target_grid(dc, view, origin, p1, p2)
    local r1, sz, r2 = guidm.getSelectionRange(p1, p2)

    if view.z < r1.z or view.z > r2.z then
        return
    end

    local p1 = view:tileToScreen(r1)
    local p2 = view:tileToScreen(r2)
    local org = view:tileToScreen(origin)
    dc:pen{ fg = COLOR_CYAN, bg = COLOR_CYAN, ch = '+', bold = true }

    -- Frame
    dc:fill(p1.x,p1.y,p1.x,p2.y)
    dc:fill(p1.x,p1.y,p2.x,p1.y)
    dc:fill(p2.x,p1.y,p2.x,p2.y)
    dc:fill(p1.x,p2.y,p2.x,p2.y)

    -- Grid
    local gxmin = org.x+10*math.ceil((p1.x-org.x)/10)
    local gxmax = org.x+10*math.floor((p2.x-org.x)/10)
    local gymin = org.y+10*math.ceil((p1.y-org.y)/10)
    local gymax = org.y+10*math.floor((p2.y-org.y)/10)
    for x = gxmin,gxmax,10 do
        for y = gymin,gymax,10 do
            dc:fill(p1.x,y,p2.x,y)
            dc:fill(x,p1.y,x,p2.y)
        end
    end
end

function SiegeEngine:renderTargetView(target_min, target_max)
    local view = self:getViewport()
    local map = self.df_layout.map
    local map_dc = gui.Painter.new(map)

    plugin.paintAimScreen(
        self.building, view:getPos(),
        xy2pos(map.x1, map.y1), view:getSize()
    )

    if target_min and math.floor(dfhack.getTickCount()/500) % 2 == 0 then
        paint_target_grid(map_dc, view, self.center, target_min, target_max)
    end

    local cursor = guidm.getCursorPos()
    if cursor then
        local cx, cy, cz = pos2xyz(view:tileToScreen(cursor))
        if cz == 0 then
            map_dc:seek(cx,cy):char('X', COLOR_YELLOW)
        end
    end
end

function SiegeEngine:scrollPiles(delta)
    local links = plugin.getStockpileLinks(self.building)
    if links then
        self.selected_pile = 1+(self.selected_pile+delta-1) % #links
        return links[self.selected_pile]
    end
end

function SiegeEngine:renderStockpiles(dc, links, nlines)
    local idx = (self.selected_pile-1) % #links
    local page = math.floor(idx/nlines)
    for i = page*nlines,math.min(#links,(page+1)*nlines)-1 do
        local color = COLOR_BROWN
        if i == idx then
            color = COLOR_YELLOW
        end
        dc:newline(2):string(utils.getBuildingName(links[i+1]), color)
    end
end

function SiegeEngine:onRenderBody_main(dc)
    dc:newline(1):pen(COLOR_WHITE):string("Target: ")

    local target_min, target_max = plugin.getTargetArea(self.building)
    if target_min then
        dc:string(
            (target_max.x-target_min.x+1).."x"..
            (target_max.y-target_min.y+1).."x"..
            (target_max.z-target_min.z+1).." Rect"
        )
    else
        dc:string("None (default)")
    end

    dc:newline(3):key('CUSTOM_R'):string(": Rectangle")
    if last_target_min then
        dc:string(", "):key('CUSTOM_P'):string(": Paste")
    end
    dc:newline(3)
    if target_min then
        dc:key('CUSTOM_X'):string(": Clear, ")
        dc:key('CUSTOM_Z'):string(": Zoom")
    end

    dc:newline():newline(1)
    if self.building.type == df.siegeengine_type.Ballista then
        dc:string("Uses ballista arrows")
    else
        local item = plugin.getAmmoItem(self.building)
        dc:key('CUSTOM_U'):string(": Use ")
        if item_choice_idx[item] then
            dc:string(item_choices[item_choice_idx[item]].caption)
        else
            dc:string(df.item_type[item])
        end
    end

    dc:newline():newline(1)
    dc:key('CUSTOM_T'):string(": Take from stockpile"):newline(3)
    local links = plugin.getStockpileLinks(self.building)
    local bottom = dc.height - 5
    if links then
        dc:key('CUSTOM_D'):string(": Delete, ")
        dc:key('CUSTOM_O'):string(": Zoom"):newline()
        self:renderStockpiles(dc, links, bottom-2-dc:cursorY())
        dc:newline():newline()
    end

    local prof = self.building:getWorkshopProfile() or {}
    dc:seek(1,math.max(dc:cursorY(),19))
    dc:key('CUSTOM_G'):key('CUSTOM_H'):key('CUSTOM_J'):key('CUSTOM_K')
    dc:string(': ')
    dc:string(df.skill_rating.attrs[prof.min_level or 0].caption):string('-')
    dc:string(df.skill_rating.attrs[math.min(LEGENDARY,prof.max_level or 3000)].caption)
    dc:newline():newline()

    if self.target_select_first then
        self:renderTargetView(self.target_select_first, guidm.getCursorPos())
    else
        self:renderTargetView(target_min, target_max)
    end
end

function SiegeEngine:setTargetArea(p1, p2)
    self.target_select_first = nil

    if not plugin.setTargetArea(self.building, p1, p2) then
        dlg.showMessage(
            'Set Target Area',
            'Could not set the target area', COLOR_LIGHTRED
        )
    else
        last_target_min = p1
        last_target_max = p2
    end
end

function SiegeEngine:setAmmoItem(choice)
    if self.building.type == df.siegeengine_type.Ballista then
        return
    end

    if not plugin.setAmmoItem(self.building, choice.item_type) then
        dlg.showMessage(
            'Set Ammo Item',
            'Could not set the ammo item', COLOR_LIGHTRED
        )
    end
end

function SiegeEngine:onInput_main(keys)
    if keys.CUSTOM_R then
        self:showCursor(true)
        self.target_select_first = nil
        self.mode = self.mode_aim
    elseif keys.CUSTOM_P and last_target_min then
        self:setTargetArea(last_target_min, last_target_max)
    elseif keys.CUSTOM_U then
        local item = plugin.getAmmoItem(self.building)
        local idx = 1 + (item_choice_idx[item] or 0) % #item_choices
        self:setAmmoItem(item_choices[idx])
    elseif keys.CUSTOM_Z then
        self:zoomToTarget()
    elseif keys.CUSTOM_X then
        plugin.clearTargetArea(self.building)
    elseif keys.SECONDSCROLL_UP then
        self:scrollPiles(-1)
    elseif keys.SECONDSCROLL_DOWN then
        self:scrollPiles(1)
    elseif keys.CUSTOM_D then
        local pile = self:scrollPiles(0)
        if pile then
            plugin.removeStockpileLink(self.building, pile)
        end
    elseif keys.CUSTOM_O then
        local pile = self:scrollPiles(0)
        if pile then
            self:centerViewOn(utils.getBuildingCenter(pile))
        end
    elseif keys.CUSTOM_T then
        self:showCursor(true)
        self.mode = self.mode_pile
        self:sendInputToParent('CURSOR_DOWN_Z')
        self:sendInputToParent('CURSOR_UP_Z')
    elseif keys.CUSTOM_G then
        local prof = plugin.saveWorkshopProfile(self.building)
        prof.min_level = math.max(0, prof.min_level-1)
        plugin.saveWorkshopProfile(self.building)
    elseif keys.CUSTOM_H then
        local prof = plugin.saveWorkshopProfile(self.building)
        prof.min_level = math.min(LEGENDARY, prof.min_level+1)
        plugin.saveWorkshopProfile(self.building)
    elseif keys.CUSTOM_J then
        local prof = plugin.saveWorkshopProfile(self.building)
        prof.max_level = math.max(0, math.min(LEGENDARY,prof.max_level)-1)
        plugin.saveWorkshopProfile(self.building)
    elseif keys.CUSTOM_K then
        local prof = plugin.saveWorkshopProfile(self.building)
        prof.max_level = math.min(LEGENDARY, prof.max_level+1)
        if prof.max_level >= LEGENDARY then prof.max_level = 3000 end
        plugin.saveWorkshopProfile(self.building)
    elseif self:simulateViewScroll(keys) then
        self.cursor = nil
    else
        return false
    end
    return true
end

local status_table = {
    ok = { pen = COLOR_GREEN, msg = "Target accessible" },
    out_of_range = { pen = COLOR_CYAN, msg = "Target out of range" },
    blocked = { pen = COLOR_RED, msg = "Target obstructed" },
    semi_blocked = { pen = COLOR_BROWN, msg = "Partially obstructed" },
}

function SiegeEngine:onRenderBody_aim(dc)
    local cursor = guidm.getCursorPos()
    local first = self.target_select_first

    dc:newline(1):string('Select target rectangle'):newline()

    local info = status_table[plugin.getTileStatus(self.building, cursor)]
    if info then
        dc:newline(2):string(info.msg, info.pen)
    else
        dc:newline(2):string('ERROR', COLOR_RED)
    end

    dc:newline():newline(1):key('SELECT')
    if first then
        dc:string(": Finish rectangle")
    else
        dc:string(": Start rectangle")
    end
    dc:newline()

    local target_min, target_max = plugin.getTargetArea(self.building)
    if target_min then
        dc:newline(1):key('CUSTOM_Z'):string(": Zoom to current target")
    end

    if first then
        self:renderTargetView(first, cursor)
    else
        local target_min, target_max = plugin.getTargetArea(self.building)
        self:renderTargetView(target_min, target_max)
    end
end

function SiegeEngine:onInput_aim(keys)
    if keys.SELECT then
        local cursor = guidm.getCursorPos()
        if self.target_select_first then
            self:setTargetArea(self.target_select_first, cursor)

            self.mode = self.mode_main
            self:showCursor(false)
        else
            self.target_select_first = cursor
        end
    elseif keys.CUSTOM_Z then
        self:zoomToTarget()
    elseif keys.LEAVESCREEN then
        self.mode = self.mode_main
        self:showCursor(false)
    elseif self:simulateCursorMovement(keys) then
        self.cursor = nil
    else
        return false
    end
    return true
end

function SiegeEngine:onRenderBody_pile(dc)
    dc:newline(1):string('Select pile to take from'):newline():newline(2)

    local sel = df.global.world.selected_building

    if df.building_stockpilest:is_instance(sel) then
        dc:string(utils.getBuildingName(sel), COLOR_GREEN):newline():newline(1)

        if plugin.isLinkedToPile(self.building, sel) then
            dc:string("Already taking from here"):newline():newline(2)
            dc:key('CUSTOM_D'):string(": Delete link")
        else
            dc:key('SELECT'):string(": Take from this pile")
        end
    elseif sel then
        dc:string(utils.getBuildingName(sel), COLOR_DARKGREY)
        dc:newline():newline(1)
        dc:string("Not a stockpile",COLOR_LIGHTRED)
    else
        dc:string("No building selected", COLOR_DARKGREY)
    end
end

function SiegeEngine:onInput_pile(keys)
    if keys.SELECT then
        local sel = df.global.world.selected_building
        if df.building_stockpilest:is_instance(sel)
        and not plugin.isLinkedToPile(self.building, sel) then
            plugin.addStockpileLink(self.building, sel)

            df.global.world.selected_building = self.building
            self.mode = self.mode_main
            self:showCursor(false)
        end
    elseif keys.CUSTOM_D then
        local sel = df.global.world.selected_building
        if df.building_stockpilest:is_instance(sel) then
            plugin.removeStockpileLink(self.building, sel)
        end
    elseif keys.LEAVESCREEN then
        df.global.world.selected_building = self.building
        self.mode = self.mode_main
        self:showCursor(false)
    elseif self:propagateMoveKeys(keys) then
        --
    else
        return false
    end
    return true
end

function SiegeEngine:onRenderBody(dc)
    dc:clear()
    dc:seek(1,1):pen(COLOR_WHITE):string(utils.getBuildingName(self.building)):newline()

    self.mode.render(dc)

    dc:seek(1, math.max(dc:cursorY(), 21)):pen(COLOR_WHITE)
    dc:key('LEAVESCREEN'):string(": Back, ")
    dc:key('CUSTOM_C'):string(": Recenter")
end

function SiegeEngine:onInput(keys)
    if self.mode.input(keys) then
        --
    elseif keys.CUSTOM_C then
        self:centerViewOn(self.center)
    elseif keys.LEAVESCREEN then
        self:dismiss()
    elseif keys.LEAVESCREEN_ALL then
        self:dismiss()
        self.no_select_building = true
        guidm.clearCursorPos()
        df.global.ui.main.mode = df.ui_sidebar_mode.Default
        df.global.world.selected_building = nil
    end
end

if not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/QueryBuilding/Some/SiegeEngine') then
    qerror("This script requires a siege engine selected in 'q' mode")
end

local building = df.global.world.selected_building

if not df.building_siegeenginest:is_instance(building) then
    qerror("A siege engine must be selected")
end
if building:getBuildStage() < building:getMaxBuildStage() then
    qerror("This engine is not completely built yet")
end

local list = SiegeEngine{ building = building }
list:show()
