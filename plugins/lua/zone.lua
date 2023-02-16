local _ENV = mkmodule('plugins.zone')
local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

ZoneOverlay = defclass(ZoneOverlay, overlay.OverlayWidget)
ZoneOverlay.ATTRS{
    default_pos={x=46,y=39},
    default_enabled=true,
    viewscreens='dwarfmode',
    frame={w=42, h=15},
    frame_style=gui.MEDIUM_FRAME,
    frame_background=gui.CLEAR_PEN,
}

local function zone_assign_is_active()
    return dfhack.gui.matchFocusString('dwarfmode/UnitSelector') and (df.global.game.main_interface.unit_selector.context == 0 or df.global.game.main_interface.unit_selector.context == 1)
end

local function canShear(unit)
    return #df.global.world.raws.creatures.all[unit.race].caste[unit.caste].shearable_tissue_layer > 0
end

local function copyTable(table_to_copy)
    local new_table = {}

    for _, v in ipairs(table_to_copy) do
        table.insert(new_table, v)
    end

    return new_table
end

local function createSearchKeyForUnit(unit)
    local race_name = df.global.world.raws.creatures.all[unit.race].caste[unit.caste].caste_name[0]
    local species = df.global.world.raws.creatures.all[unit.race].name[0]
    local unit_actual_name = string.lower(dfhack.toSearchNormalized(dfhack.TranslateName(dfhack.units.getVisibleName(unit), false)))
    local is_stray = not dfhack.units.isPet(unit)
    local unit_profession = unit.profession
    local is_child = dfhack.units.isChild(unit)
    local child_name = df.global.world.raws.creatures.all[unit.race].general_child_name
    local baby_name = df.global.world.raws.creatures.all[unit.race].general_baby_name

    if child_name then
        child_name = child_name[0]
    end

    if baby_name then
        baby_name = baby_name[0]
    end

    local search_name = race_name
    if is_baby then search_name = baby_name end
    if is_child then search_name = child_name end

    local trained_war = false
    local trained_hunter = false

    if unit_profession == 98 then
        trained_hunter = true
    elseif unit_profession == 99 then
        trained_war = true
    end

    return search_name .. ' ' .. unit_actual_name .. (is_stray and ' stray ' or '') .. ' (tame) ' ..
    (trained_war and ' war ' or '') .. (trained_hunter and ' hunting ' or '') .. ' species=' .. species
end

function ZoneOverlay:init()
    self.screen = nil
    self:setDirty(false)
    self.search_focused = false

    -- used to store 
    self.original_unit_table = nil
    self.original_selected_table = nil

    -- used to update original_unit_table when a filter is applied
    self.selected_table_before_click = nil
    self.check_selected_on_render = false
    self.has_rendered = false

    -- filters
    self.non_grazing = true
    self.non_egg_laying = true
    self.non_milkable = true
    self.non_shearable = true
    self.not_caged = true
    self.currently_pastured = true
    self.female = true
    self.male = true

    self:addviews{
        widgets.HotkeyLabel{
            view_id='toggle_all',
            frame={t=0,l=0},
            label='Assign/unassign all',
            key='CUSTOM_ALT_Q',
            on_activate=self:callback('toggleAllUnits'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='non_grazing',
            frame={t=2,l=0},
            label='Non-Grazing',
            key='CUSTOM_ALT_G',
            on_change=self:callback('toggleNonGrazing'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='non_egg_laying',
            frame={t=3,l=0},
            label='Non-Egg Laying',
            key='CUSTOM_ALT_E',
            on_change=self:callback('toggleNonEggLaying'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='non_milkable',
            frame={t=4,l=0},
            label='Non-Milkable',
            key='CUSTOM_ALT_L',
            on_change=self:callback('toggleNonMilkable'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='non_shearable',
            frame={t=5,l=0},
            label='Non-Shearable',
            key='CUSTOM_ALT_A',
            on_change=self:callback('toggleNonShearable'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='not_caged',
            frame={t=6,l=0},
            label='Not Caged',
            key='CUSTOM_ALT_C',
            on_change=self:callback('toggleNotCaged'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='currently_pastured',
            frame={t=7,l=0},
            label='Currently Pastured',
            key='CUSTOM_ALT_P',
            on_change=self:callback('toggleCurrentlyPastured'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='female',
            frame={t=8,l=0},
            label='Female',
            key='CUSTOM_ALT_F',
            on_change=self:callback('toggleFemale'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='male',
            frame={t=9,l=0},
            label='Male',
            key='CUSTOM_ALT_M',
            on_change=self:callback('toggleMale'),
        },
        widgets.HotkeyLabel{
            frame={t=11, l=0},
            label='Search',
            key='CUSTOM_ALT_S',
            on_activate=self:callback('startSearch'),
        },
        widgets.EditField{
            view_id='search',
            frame={t=12,l=0},
            on_submit=function() self:setSearchFocus(false) end,
            on_change=self:callback('doSearch'),
        },
    }

    self:setSearchFocus(false)
end

function ZoneOverlay:toggleAllUnits()
    local current_civzone = dfhack.gui.getSelectedCivZone()
    local current_civzone_id
    if current_civzone then current_civzone_id = current_civzone.id end
    if not current_civzone_id then return end

    self.selected_table_before_click = copyTable(df.global.game.main_interface.unit_selector.selected)

    self.dirty = true
    local switching_to = 1

    for k, unit_id in ipairs(df.global.game.main_interface.unit_selector.unid) do
        local unit_selected_val = df.global.game.main_interface.unit_selector.selected[k]
        -- switch everything to the opposite of whatever the first element is set to
        if k == 0 then
            if unit_selected_val == 1 then
                switching_to = 0
            end
        end

        if df.global.game.main_interface.unit_selector.selected[k] ~= switching_to then
            df.global.game.main_interface.unit_selector.selected[k] = switching_to

            local unit = df.unit.find(unit_id)
            if switching_to == 0 then
                -- switching to unpastured, remove pastured ref
                for k2, ref in ipairs(unit.general_refs) do
                    if df.general_ref_building_civzone_assignedst:is_instance(ref) and ref.building_id == current_civzone_id then
                        unit.general_refs:erase(k2)
                        break
                    end
                end

                for k2, assigned_unit in ipairs(current_civzone.assigned_units) do
                    if assigned_unit == unit_id then
                        current_civzone.assigned_units:erase(k2)
                        break
                    end
                end
            else
                -- switching to pastured here, create pastured ref, remove any not for this pasture
                for k2, ref in ipairs(unit.general_refs) do
                    local civzone_to_unassign_from

                    if df.general_ref_building_civzone_assignedst:is_instance(ref) and ref.building_id ~= current_civzone_id then
                        civzone_to_unassign_from = df.building.find(ref.building_id)

                        unit.general_refs:erase(k2)

                        if civzone_to_unassign_from then
                            for k3, assigned_unit in ipairs(civzone_to_unassign_from.assigned_units) do
                                if assigned_unit == unit_id then
                                    civzone_to_unassign_from.assigned_units:erase(k3)
                                    break
                                end
                            end
                        end

                        break
                    end
                end
                local new_ref = df.general_ref_building_civzone_assignedst:new()
                new_ref.building_id = current_civzone_id
                unit.general_refs:insert('#', new_ref)
                current_civzone.assigned_units:insert('#', unit_id)
            end
        end
    end

    self.check_selected_on_render = true
end

function ZoneOverlay:toggleNonGrazing()
    self.dirty = true
    self.non_grazing = not self.non_grazing
    self:doSearch()
end

function ZoneOverlay:toggleNonEggLaying()
    self.dirty = true
    self.non_egg_laying = not self.non_egg_laying
    self:doSearch()
end

function ZoneOverlay:toggleNonMilkable()
    self.dirty = true
    self.non_milkable = not self.non_milkable
    self:doSearch()
end

function ZoneOverlay:toggleNonShearable()
    self.dirty = true
    self.non_shearable = not self.non_shearable
    self:doSearch()
end

function ZoneOverlay:toggleNotCaged()
    self.dirty = true
    self.not_caged = not self.not_caged
    self:doSearch()
end

function ZoneOverlay:toggleCurrentlyPastured()
    self.dirty = true
    self.currently_pastured = not self.currently_pastured
    self:doSearch()
end

function ZoneOverlay:toggleFemale()
    self.dirty = true
    self.female = not self.female
    self:doSearch()
end

function ZoneOverlay:toggleMale()
    self.dirty = true
    self.male = not self.male
    self:doSearch()
end

function ZoneOverlay:onInput(keys)
    if not (isEnabled() and zone_assign_is_active()) then return false end

    if keys._MOUSE_L then
        if self.original_unit_table ~= nil and df.global.game.main_interface.unit_selector.selected ~= nil then
            self.selected_table_before_click = copyTable(df.global.game.main_interface.unit_selector.selected)
            self.check_selected_on_render = true
        end
    elseif keys.LEAVESCREEN or keys._MOUSE_R then
        if self.subviews.search.focus then
            self:setSearchFocus(false)
            return true
        end
    end

    local handled_by_super = ZoneOverlay.super.onInput(self, keys)

    if keys._MOUSE_L and handled_by_super then
        df.global.enabler.mouse_lbut = 0
    end

    return handled_by_super
end

function ZoneOverlay:checkAndHandleSelectedChanges()
    if not self.original_unit_table then return end
    if not self.selected_table_before_click then return end

    for i, selected in ipairs(df.global.game.main_interface.unit_selector.selected) do
        if selected ~= self.selected_table_before_click[i+1] then
            self:updateOriginalSelectedTable(i, selected)
        end
    end
    self.selected_table_before_click = nil
end

function ZoneOverlay:updateOriginalSelectedTable(selected_index, selected_state)
    local unit_id = df.global.game.main_interface.unit_selector.unid[selected_index]

    for i, original_unit_id in ipairs(self.original_unit_table) do
        if unit_id == original_unit_id then
            self.original_selected_table[i] = selected_state
            break
        end
    end
end

function ZoneOverlay:restoreUnitSelectorTables()
    if self.original_unit_table then
        df.global.game.main_interface.unit_selector.unid = copyTable(self.original_unit_table)
    end

    if self.original_selected_table then
        df.global.game.main_interface.unit_selector.selected = copyTable(self.original_selected_table)
    end
end

function ZoneOverlay:unitValidForFilters(unit)
    if not self.non_grazing then
        if not dfhack.units.isGrazer(unit) then
            return false
        end
    end

    if not self.non_egg_laying then
        if not dfhack.units.isEggLayer(unit) then
            return false
        end
    end

    if not self.non_milkable then
        if not dfhack.units.isMilkable(unit) then
            return false
        end
    end

    if not self.non_shearable then
        if not canShear(unit) then
            return false
        end
    end

    if not self.not_caged then
        -- TODO: make method
        if not dfhack.units.getGeneralRef(unit, df.general_ref_type.CONTAINED_IN_ITEM) then
            return false
        end
    end

    if not self.currently_pastured then
        -- TODO: make method
        if dfhack.units.getGeneralRef(unit, df.general_ref_type.BUILDING_CIVZONE_ASSIGNED) then
            return false
        end
    end

    if not self.male then
        if dfhack.units.isMale(unit) then
            return false
        end
    end

    if not self.female then
        if dfhack.units.isFemale(unit) then
            return false
        end
    end

    return true
end

function ZoneOverlay:anyFiltersActive()
    return not self.non_grazing or not self.non_egg_laying or not self.non_milkable or not self.not_caged or
    not self.currently_pastured or not self.female or not self.male or not self.non_shearable or false
end

function ZoneOverlay:disableAllFilters()
    self.non_grazing = true
    self.subviews.non_grazing:setOption(1)
    self.non_egg_laying = true
    self.subviews.non_egg_laying:setOption(1)
    self.non_milkable = true
    self.subviews.non_milkable:setOption(1)
    self.non_shearable = true
    self.subviews.non_shearable:setOption(1)
    self.not_caged = true
    self.subviews.not_caged:setOption(1)
    self.currently_pastured = true
    self.subviews.currently_pastured:setOption(1)
    self.female = true
    self.subviews.female:setOption(1)
    self.male = true
    self.subviews.male:setOption(1)
end

function ZoneOverlay:doSearch()
    self:setDirty(true)
    local filter = self.subviews.search.text

    if filter == '' and not self:anyFiltersActive() then
        self:restoreUnitSelectorTables()
        return
    end

    filter = string.lower(filter)
    local tokens = filter:split()

    local new_unid_table = {}
    local new_selected_table = {}

    for idx, unit_id in ipairs(self.original_unit_table) do
        local unit = df.unit.find(unit_id)

        if not self:unitValidForFilters(unit) then
            goto skip_unit
        end

        local search_key = createSearchKeyForUnit(unit)

        local ok = true
        for _,key in ipairs(tokens) do
            key = key:escape_pattern()
            local invert = key:sub(1, 1) == '!'
            if invert then
                key = key:sub(2)
            end

            -- key will be blank if the token was only "!"
            if key ~= '' then
                local match = search_key:match('%f[^%s\x00]'..key)
                if (not invert and not match) or (invert and match) then
                    ok = false
                    break
                end
            end
        end

        if ok then
            table.insert(new_unid_table, unit_id)
            table.insert(new_selected_table, self.original_selected_table[idx])
        end

        ::skip_unit::
    end

    df.global.game.main_interface.unit_selector.unid = new_unid_table
    df.global.game.main_interface.unit_selector.selected = new_selected_table
end

function ZoneOverlay:setSearchFocus(focus)
    self.subviews.search:setFocus(focus)
    self.search_focused = focus
end

function ZoneOverlay:startSearch()
    self:setDirty(true)
    self:setSearchFocus(true)
end

function ZoneOverlay:setSearch(search, do_search)
    self.subviews.search.text = search

    if do_search == nil or do_search then
        self:doSearch()
    end
end

function ZoneOverlay:setDirty(dirty)
    self.dirty = dirty
end

function ZoneOverlay:render(dc)
    if not (isEnabled() and zone_assign_is_active()) then
        if self.dirty then
            self:setDirty(false)
            self.original_unit_table = nil
            self.original_selected_table = nil
            self:setSearch('', false)
            self:setSearchFocus(false)
            self:disableAllFilters()
            self.has_rendered = false
        end

        return false
    end

    -- first render after becoming enabled
    if not self.has_rendered then
        self.has_rendered = true
        self.dirty = true
        -- keep track of the original tables so we can mutate those as we change things in the modified tables, and then restore
        -- them when no longer searching
        self.original_unit_table = copyTable(df.global.game.main_interface.unit_selector.unid)
        self.original_selected_table = copyTable(df.global.game.main_interface.unit_selector.selected)
    end

    if self.check_selected_on_render then
        self.check_selected_on_render = false
        self:checkAndHandleSelectedChanges()
    end

    ZoneOverlay.super.render(self, dc)
end

OVERLAY_WIDGETS = {
    overlay=ZoneOverlay,
}

return _ENV
