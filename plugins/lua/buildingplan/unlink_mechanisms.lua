local _ENV = mkmodule("plugins.buildingplan.unlink_mechanisms")

local gui = require("gui")
local overlay = require("plugins.overlay")
local utils = require("utils")
local widgets = require("gui.widgets")

saved_mode = saved_mode or 0

local function mech_iter(b) --iterate mechanisms backwards
    local t = b.contained_items
    local i_cur = #t - 1
    return function()
        for i = i_cur, 1, -1 do --index 0 is always building component
            local item = t[i].item
            if item._type == df.item_trappartsst and
                item.flags.in_building and
                not item.flags.in_job then
                    i_cur = i - 1 --resume here
                    return item
            end
        end
    end
end

local function get_trigger_index(m) --return gref index or nil
    for k, gref in ipairs(m.general_refs) do
        if gref._type == df.general_ref_building_triggerst or
            gref._type == df.general_ref_building_triggertargetst then
                return k
        end
    end
end

local function get_mech_target(m) --mechanism target building if exists
    local i = get_trigger_index(m)
    return i and df.building.find(m.general_refs[i].building_id) or nil
end

local function has_link_tab(b) --linked building tab exists
    if not b then
        return
    elseif b._type == df.building_trapst and
        (b.trap_type == df.trap_type.Lever or
        b.trap_type == df.trap_type.PressurePlate) then
            return true --always active
    end

    for item in mech_iter(b) do
        if get_trigger_index(item) then
            return true
        end
    end
end

local function unlink_building(b, m) --clean up extra stuff
    if b._type == df.building_trapst then
        for k, v in ipairs(b.linked_mechanisms) do
            if v == m then
                b.linked_mechanisms:erase(k)
                break
            end
        end
    elseif b._type == df.building_doorst or b._type == df.building_hatchst then
        local p = utils.getBuildingCenter(b)
        local mb = dfhack.maps.getTileBlock(p)
        if mb then
            b.door_flags.forbidden = false
            b.door_flags.operated_by_mechanisms = false
            mb.occupancy[p.x % 16][p.y % 16].building = df.tile_building_occ.Dynamic
            df.global.world.reindex_pathfinding = true
        end
    end
end

local function unlink_mech(b_src, m_src, b_dst, free_src, free_dst) --unlink two buildings
    local m_dst
    for item in mech_iter(b_dst) do
        local i = get_trigger_index(item)
        if i and item.general_refs[i].building_id == b_src.id then
            m_dst = item
            item.general_refs:erase(i) --unlink from b_src
            break
        end
    end

    unlink_building(b_dst, m_src)

    if m_dst then
        unlink_building(b_src, m_dst)

        if free_dst then
            m_dst.flags.in_building = false
            dfhack.items.moveToGround(m_dst, xyz2pos(dfhack.items.getPosition(m_dst)))
        end
    else
        dfhack.printerr(("MechLinkOverlay: Corresponding mechanism not found in %s!"):format(b_dst))
    end

    local i = get_trigger_index(m_src)
    if i then
        m_src.general_refs:erase(i) --unlink from b_dst
    else
        dfhack.printerr(("MechLinkOverlay: General ref went missing in %s!"):format(m_src))
    end

    if free_src then
        m_src.flags.in_building = false
        dfhack.items.moveToGround(m_src, xyz2pos(dfhack.items.getPosition(m_src)))
    end
end

local sheet = df.global.game.main_interface.view_sheets

local valid_build = {
    "Trap/Lever",
    "Trap/PressurePlate",
    "Trap/TrackStop",
    "BarsFloor",
    "BarsVertical",
    "Bridge",
    "Cage",
    "Chain",
    "Door",
    "Floodgate",
    "GearAssembly",
    "GrateFloor",
    "GrateWall",
    "Hatch",
    "Support",
    "Weapon",
}

------------------------
-- ConfirmWindow

ConfirmWindow = defclass(ConfirmWindow, widgets.Window)
ConfirmWindow.ATTRS
{
    frame = {w=56, h=8},
    message = DEFAULT_NIL,
    desired_lines = 1,
    propagate_fn = DEFAULT_NIL,
}

function ConfirmWindow:init()
    self.frame.h = 8 + self.desired_lines

    self:addviews
    {
        widgets.Label
        {
            frame = {t=0, l=0, r=0},
            text = self.message,
        },
        widgets.HotkeyLabel
        {
            frame = {b=2, l=0},
            label = "Yes, proceed",
            key = "SELECT",
            auto_width = true,
            on_activate = self:callback("proceed"),
        },
        widgets.HotkeyLabel
        {
            frame = {b=0, l=0},
            label = "Suppress this warning until DF restarts",
            key = "CUSTOM_SHIFT_S",
            auto_width = true,
            on_activate = self:callback("suppress"),
        },
    }
end

function ConfirmWindow:proceed()
    self.parent_view:dismiss()
    self.propagate_fn()
end

function ConfirmWindow:suppress()
    self.parent_view:dismiss()
    self.propagate_fn(true)
end

ConfirmScreen = defclass(ConfirmScreen, gui.ZScreenModal)
ConfirmScreen.ATTRS
{
    focus_path = "mech/confirm",
    title = DEFAULT_NIL,
    message = DEFAULT_NIL,
    desired_lines = 1,
    propagate_fn = DEFAULT_NIL,
}

function ConfirmScreen:init()
    self:addviews
    {
        ConfirmWindow
        {
            frame_title = self.title,
            message = self.message,
            desired_lines = self.desired_lines,
            propagate_fn = self.propagate_fn,
        }
    }
end

-- ----------------------
-- MechLinkOverlay
--

MechLinkOverlay = defclass(MechLinkOverlay, overlay.OverlayWidget)
MechLinkOverlay.ATTRS
{
    desc = "Allows unlinking mechanisms from buildings.",
    default_enabled = true,
    overlay_only = true,
    default_pos = {x=-41, y=-4},
    frame = {w=56, h=27},
    viewscreens = {},
}

for _,v in ipairs(valid_build) do
    table.insert(MechLinkOverlay.ATTRS.viewscreens, "dwarfmode/ViewSheets/BUILDING/"..v.."/LinkedBuildings")
end

function MechLinkOverlay:init()
    self.num_buttons = 0
    self.links = {}

    self:addviews
    {
        widgets.CycleHotkeyLabel
        {
            view_id = "unlink_mode",
            frame = {b=1, l=0, w=36},
            label = "Unlink mode",
            key = "CUSTOM_M",
            options =
            {
                {value=0, label="Don't auto free", pen=COLOR_DARKGRAY},
                {value=1, label="Free local mechanism", pen=COLOR_GREEN},
                {value=2, label="Free target mechanism", pen=COLOR_GREEN},
                {value=3, label="Free local and target", pen=COLOR_GREEN},
            },
            initial_option = saved_mode,
            on_change = function(val) saved_mode = val end,
        },
        widgets.HotkeyLabel
        {
            view_id = "unlink_all",
            frame = {b=1, r=3},
            key = "CUSTOM_SHIFT_U",
            label = "Unlink all",
            auto_width = true,
            on_activate = self:callback("ask_unlink_all"),
            enabled = function() return next(self.links) end,
        },
        widgets.Scrollbar --Work around for https://dwarffortressbugtracker.com/view.php?id=12721
        {
            view_id = "scroll",
            frame = {t=0, r=0, h=24},
            on_scroll = self:callback("do_scroll"),
        },
    }
end

function MechLinkOverlay:do_scroll(spec)
    if type(spec) == "number" then
        self:scroll(spec - sheet.scroll_position_linked_buildings-1)
    elseif spec == "up_small" then
        self:scroll(-1)
    elseif spec == "down_small" then
        self:scroll(1)
    elseif spec == "up_large" then
        self:scroll(-self.num_buttons*3)
    elseif spec == "down_large" then
        self:scroll(self.num_buttons*3)
    end
end

function MechLinkOverlay:scroll(delta)
    local val = sheet.scroll_position_linked_buildings + delta
    sheet.scroll_position_linked_buildings = math.min(val, (#self.links - self.num_buttons)*3)
    sheet.scroll_position_linked_buildings = math.max(sheet.scroll_position_linked_buildings, 0)
    self.subviews.scroll:update(sheet.scroll_position_linked_buildings + 1)
end

function MechLinkOverlay:build_links_table()
    self.links = {}
    for idx = 1, #self.building.contained_items-1 do --index 0 is always building component
        local item = self.building.contained_items[idx].item
        if item._type == df.item_trappartsst and item.flags.in_building and
            not item.flags.in_job and get_trigger_index(item) then --item is linked mechanism
                table.insert(self.links, idx)
        end
    end

    if self.num_buttons >= #self.links then --no scrolling
        sheet.scroll_position_linked_buildings = 0
    end
end

function MechLinkOverlay:idx_from_offset(offset)
    if offset <= 0 or offset >= self.num_buttons*3 - 1 then --linked icons disappear early
        return 0 --outside of list
    else
        return self.links[(offset + sheet.scroll_position_linked_buildings) // 3 + 1] or 0
    end
end

function MechLinkOverlay:get_button(n, ensure)
    local button = self.subviews["unlink_"..n]
    if not button and ensure then
        self:addviews
        {
            widgets.TextButton
            {
                view_id = "unlink_"..n,
                frame = {t=0, r=8, w=8, h=1},
                label = "Unlink",
                on_activate = function() self:activate_button(n) end,
                visible = false,
            },
        }
        button = self.subviews["unlink_"..n]
        button:updateLayout(self.frame_body)
    end

    return button
end

function MechLinkOverlay:activate_button(n)
    local button = self:get_button(n)

    local idx = self:idx_from_offset(button.frame.t)
    if idx > 0 and idx < #self.building.contained_items then
        local item = self.building.contained_items[idx].item
        local target = get_mech_target(item)

        if target then
            local free_src = (saved_mode % 2 == 1)
            local free_dst = (saved_mode // 2 > 0)

            if always_unlink_single then
                unlink_mech(self.building, item, target, free_src, free_dst)
                if not has_link_tab(self.building) then
                    sheet.show_linked_buildings = false --DF will swap viewscreen
                end
                return
            end

            ConfirmScreen
            {
                title = "Unlink Mechanism",
                message =
                {
                    "Really unlink mechanism? This cannot be undone.\n\nThis mechanism: ",
                    {text = dfhack.items.getDescription(item, 0, true), pen = COLOR_BLUE},
                    "\nTarget building: ",
                    {text = utils.getBuildingName(target), pen = COLOR_BLUE},
                    "\n\n",
                    free_src and {text = "This mechanism will be freed.", pen = COLOR_GREEN} or
                        {text = "This mechanism will not be freed.", pen = COLOR_DARKGRAY},
                    "\n",
                    free_dst and {text = "Target mechanism will be freed.", pen = COLOR_GREEN} or
                    {text = "Target mechanism will not be freed.", pen = COLOR_DARKGRAY},
                },
                desired_lines = 7,
                propagate_fn = function(suppress)
                    always_unlink_single = suppress
                    unlink_mech(self.building, item, target, free_src, free_dst)
                    if not has_link_tab(self.building) then
                        sheet.show_linked_buildings = false --DF will swap viewscreen
                    end
                end,
            }:show()
        else
            dfhack.printerr(("MechLinkOverlay: Mechanism %s had no target!"):format(item))
        end
    else
        dfhack.printerr(("MechLinkOverlay: Invalid button! Offset %d"):format(button.frame.t))
    end
end

function MechLinkOverlay:ask_unlink_all()
    if always_unlink_all then
        self:do_unlink_all()
        return
    end

    ConfirmScreen
    {
        title = "Unlink All Mechanisms",
        message =
        {
            "Really unlink all mechanisms? This cannot be undone.\n\n",
            (saved_mode % 2 == 1) and {text = "These mechanisms will be freed.", pen = COLOR_GREEN} or
                {text = "These mechanisms will not be freed.", pen = COLOR_DARKGRAY},
            "\n",
            (saved_mode // 2 > 0) and {text = "Target mechanisms will be freed.", pen = COLOR_GREEN} or
            {text = "Target mechanisms will not be freed.", pen = COLOR_DARKGRAY},
        },
        desired_lines = 4,
        propagate_fn = function(suppress)
            always_unlink_all = suppress
            self:do_unlink_all()
        end,
    }:show()
end

function MechLinkOverlay:do_unlink_all()
    local free_src = (saved_mode % 2 == 1)
    local free_dst = (saved_mode // 2 > 0)

    for _, idx in ipairs(self.links) do
        local item = self.building.contained_items[idx].item
        local target = get_mech_target(item)

        if target then
            unlink_mech(self.building, item, target, free_src, free_dst)
        else
            dfhack.printerr(("MechLinkOverlay: Mechanism %s had no target!"):format(item))
        end
    end

    if not has_link_tab(self.building) then
        sheet.show_linked_buildings = false --DF will swap viewscreen
    end
end

function MechLinkOverlay:update_buttons()
    self:build_links_table()
    local scroll_pos = sheet.scroll_position_linked_buildings
    self.subviews.scroll:update(scroll_pos + 1, self.num_buttons*3, #self.links*3)
    self:scroll(0)

    local bci_len = #self.building.contained_items
    local h_offset = #self.links > self.num_buttons and 8 or 6 --account for scrollbar

    for i=1, self.num_buttons do
        local button = self:get_button(i, true)
        local offset = i*3 - 1 - ((scroll_pos + 1) % 3)
        local idx = self:idx_from_offset(offset)

        button.visible = false
        if idx > 0 and idx < bci_len then
            button.frame.t = offset
            button.frame.r = h_offset
            button.visible = true
        end
        button:updateLayout()
    end

    local b = (self.frame.h % 3) == 1 and #self.links >= self.num_buttons and 0 or 1
    self.subviews.unlink_mode.frame.b = b --avoid overlapping list
    self.subviews.unlink_all.frame.b = b
    self.subviews.unlink_mode:updateLayout()
    self.subviews.unlink_all:updateLayout()
end

function MechLinkOverlay:preUpdateLayout(parent_rect)
    for i=1, self.num_buttons do --hide existing buttons
        local button = self:get_button(i)
        if button then
            button.visible = false
        end
    end

    local h = parent_rect.height - 49
    self.frame.h = h + 1 --includes lower border
    self.num_buttons = h // 3

    self.subviews.scroll.frame.h = self.num_buttons*3
end

function MechLinkOverlay:onRenderFrame(dc, rect)
    if self.bld_id ~= sheet.viewing_bldid then
        self.bld_id = sheet.viewing_bldid
        self.building = df.building.find(self.bld_id)
    end

    self:update_buttons()

    MechLinkOverlay.super.onRenderFrame(self, dc, rect)
end

-- ----------------------
-- MechItemOverlay
--

MechItemOverlay = defclass(MechItemOverlay, overlay.OverlayWidget)
MechItemOverlay.ATTRS
{
    desc = "Allows freeing unlinked mechanisms from buildings.",
    default_enabled = true,
    overlay_only = true,
    default_pos = {x=-41, y=-4},
    frame = {w=56, h=27},
    viewscreens = {},
}

for _,v in ipairs(valid_build) do
    table.insert(MechItemOverlay.ATTRS.viewscreens, "dwarfmode/ViewSheets/BUILDING/"..v.."/Items")
end
valid_build = nil

function MechItemOverlay:init()
    self.num_buttons = 0

    self:addviews
    {
        widgets.HotkeyLabel
        {
            view_id = "free_all",
            frame = {b=1, r=3},
            key = "CUSTOM_SHIFT_F",
            label = "Free all",
            auto_width = true,
            on_activate = self:callback("ask_free_all"),
        },
    }
end

function MechItemOverlay:idx_from_offset(offset)
    if offset < 0 or offset >= self.num_buttons*3 then
        return 0 --outside of list
    else
        return (offset + sheet.scroll_position_item) // 3
    end
end

function MechItemOverlay:get_button(n, ensure)
    local button = self.subviews["free_"..n]
    if not button and ensure then
        self:addviews
        {
            widgets.TextButton
            {
                view_id = "free_"..n,
                frame = {t=0, r=21, w=6, h=1},
                label = "Free",
                on_activate = function() self:activate_button(n) end,
                visible = false,
            },
        }
        button = self.subviews["free_"..n]
        button:updateLayout(self.frame_body)
    end

    return button
end

function MechItemOverlay:activate_button(n)
    local button = self:get_button(n)

    local idx = self:idx_from_offset(button.frame.t)
    if idx > 0 and idx < #self.building.contained_items then
        local item = self.building.contained_items[idx].item
        local target = get_mech_target(item)

        if target then
            dfhack.printerr(("MechItemOverlay: Mechanism %s still linked to %s!"):format(item, target))
        else
            if always_free_single then
                item.flags.in_building = false
                dfhack.items.moveToGround(item, xyz2pos(dfhack.items.getPosition(item)))
                return
            end

            ConfirmScreen
            {
                title = "Free Mechanism",
                message =
                {
                    "Really free mechanism? This cannot be undone.\n\nThis mechanism: ",
                    {text = dfhack.items.getDescription(item, 0, true), pen = COLOR_BLUE},
                },
                desired_lines = 3,
                propagate_fn = function(suppress)
                    always_free_single = suppress
                    item.flags.in_building = false
                    dfhack.items.moveToGround(item, xyz2pos(dfhack.items.getPosition(item)))
                end,
            }:show()
        end
    else
        dfhack.printerr(("MechItemOverlay: Invalid button! Offset %d"):format(button.frame.t))
    end
end

function MechItemOverlay:ask_free_all()
    if always_free_all then
        self:do_free_all()
        return
    end

    ConfirmScreen
    {
        title = "Free All Mechanisms",
        message = "Really free all mechanisms? This cannot be undone.",
        desired_lines = 1,
        propagate_fn = function(suppress)
            always_free_all = suppress
            self:do_free_all()
        end,
    }:show()
end

function MechItemOverlay:do_free_all()
    for item in mech_iter(self.building) do
        if not get_mech_target(item) then
            item.flags.in_building = false
            dfhack.items.moveToGround(item, xyz2pos(dfhack.items.getPosition(item)))
        end
    end
end

function MechItemOverlay:update_buttons()
    local scroll_pos = sheet.scroll_position_item
    local bci_len = #self.building.contained_items
    local h_offset = bci_len > self.num_buttons and 21 or 19 --account for scrollbar

    for i=1, self.num_buttons do
        local button = self:get_button(i, true)
        local offset = i*3 - 1 - ((scroll_pos + 1) % 3)
        local idx = self:idx_from_offset(offset)

        button.visible = false
        if idx > 0 and idx < bci_len then
            local item = self.building.contained_items[idx].item
            if item._type == df.item_trappartsst and item.flags.in_building and
                not item.flags.in_job and not get_trigger_index(item) then
                    button.frame.t = offset
                    button.frame.r = h_offset
                    button.visible = true
            end
        end
        button:updateLayout()
    end

    self.subviews.free_all.visible = false
    for item in mech_iter(self.building) do
        if not get_mech_target(item) then
            self.subviews.free_all.visible = true

            local b = (self.frame.h % 3) == 1 and bci_len >= self.num_buttons and 0 or 1
            self.subviews.free_all.frame.b = b --avoid overlapping list
            self.subviews.free_all:updateLayout()
            break
        end
    end
end

function MechItemOverlay:fix_layout()
    for i=1, self.num_buttons do --hide existing buttons
        local button = self:get_button(i)
        if button then
            button.visible = false
        end
    end

    local h = self.parent_height - 46 - (has_link_tab(self.building) and 3 or 0)
    self.frame.h = h + 1 --includes lower border
    self.num_buttons = h // 3
end

function MechItemOverlay:preUpdateLayout(parent_rect)
    self.parent_height = parent_rect.height
    self:fix_layout()
end

function MechItemOverlay:onRenderFrame(dc, rect)
    if self.bld_id ~= sheet.viewing_bldid then
        self.bld_id = sheet.viewing_bldid
        self.building = df.building.find(self.bld_id)
    end

    self:fix_layout()
    self:update_buttons()

    MechItemOverlay.super.onRenderFrame(self, dc, rect)
end

return _ENV
