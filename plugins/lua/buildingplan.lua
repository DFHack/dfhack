local _ENV = mkmodule('plugins.buildingplan')

--[[

 Native functions:

 * void setSetting(string name, boolean value)
 * bool isPlanModeEnabled(df::building_type type, int16_t subtype, int32_t custom)
 * bool isPlannableBuilding(df::building_type type, int16_t subtype, int32_t custom)
 * bool isPlannedBuilding(df::building *bld)
 * void addPlannedBuilding(df::building *bld)
 * void doCycle()
 * void scheduleCycle()

--]]

local dialogs = require('gui.dialogs')
local guidm = require('gui.dwarfmode')
require('dfhack.buildings')

-- does not need the core suspended
function get_num_filters(btype, subtype, custom)
    local filters = dfhack.buildings.getFiltersByType(
        {}, btype, subtype, custom)
    if filters then return #filters end
    return 0
end

local function to_title_case(str)
    str = str:gsub('(%a)([%w_]*)',
        function (first, rest) return first:upper()..rest:lower() end)
    str = str:gsub('_', ' ')
    return str
end

local function get_filter(btype, subtype, custom, reverse_idx)
    local filters = dfhack.buildings.getFiltersByType(
        {}, btype, subtype, custom)
    if not filters or reverse_idx < 0 or reverse_idx >= #filters then
        error(string.format('invalid index: %d', reverse_idx))
    end
    return filters[#filters-reverse_idx]
end

-- returns a reasonable label for the item based on the qualities of the filter
-- does not need the core suspended
-- reverse_idx is 0-based and is expected to be counted from the *last* filter
function get_item_label(btype, subtype, custom, reverse_idx)
    local filter = get_filter(btype, subtype, custom, reverse_idx)
    if filter.has_tool_use then
        return to_title_case(df.tool_uses[filter.has_tool_use])
    end
    if filter.item_type then
        return to_title_case(df.item_type[filter.item_type])
    end
    if filter.flags2 and filter.flags2.building_material then
        if filter.flags2.fire_safe then
            return "Fire-safe building material";
        end
        if filter.flags2.magma_safe then
            return "Magma-safe building material";
        end
        return "Generic building material";
    end
    if filter.vector_id then
        return to_title_case(df.job_item_vector_id[filter.vector_id])
    end
    return "Unknown";
end

-- returns whether the items matched by the specified filter can have a quality
-- rating. This also conveniently indicates whether an item can be decorated.
-- does not need the core suspended
-- reverse_idx is 0-based and is expected to be counted from the *last* filter
function item_can_be_improved(btype, subtype, custom, reverse_idx)
    local filter = get_filter(btype, subtype, custom, reverse_idx)
    if filter.flags2 and filter.flags2.building_material then
        return false;
    end
    return filter.item_type ~= df.item_type.WOOD and
            filter.item_type ~= df.item_type.BLOCKS and
            filter.item_type ~= df.item_type.BAR and
            filter.item_type ~= df.item_type.BOULDER
end

-- needs the core suspended
-- returns a vector of constructed buildings (usually of size 1, but potentially
-- more for constructions)
function construct_buildings_from_ui_state()
    local uibs = df.global.ui_build_selector
    local world = df.global.world
    local direction = world.selected_direction
    local _, width, height = dfhack.buildings.getCorrectSize(
        world.building_width, world.building_height, uibs.building_type,
        uibs.building_subtype, uibs.custom_type, direction)
    -- the cursor is at the center of the building; we need the upper-left
    -- corner of the building
    local pos = guidm.getCursorPos()
    pos.x = pos.x - math.floor(width/2)
    pos.y = pos.y - math.floor(height/2)
    local min_x, max_x = pos.x, pos.x
    local min_y, max_y = pos.y, pos.y
    if width == 1 and height == 1 and
            (world.building_width > 1 or world.building_height > 1) then
        min_x = math.ceil(pos.x - world.building_width/2)
        max_x = min_x + world.building_width - 1
        min_y = math.ceil(pos.y - world.building_height/2)
        max_y = min_y + world.building_height - 1
    end
    local blds = {}
    for y=min_y,max_y do for x=min_x,max_x do
        local bld, err = dfhack.buildings.constructBuilding{
            type=uibs.building_type, subtype=uibs.building_subtype,
            custom=uibs.custom_type, pos=xyz2pos(x, y, pos.z),
            width=width, height=height, direction=direction}
        if err then
            for _,b in ipairs(blds) do
                dfhack.buildings.deconstruct(b)
            end
            error(err)
        end
        -- assign fields for the types that need them. we can't pass them all in
        -- to the call to constructBuilding since attempting to assign unrelated
        -- fields to building types that don't support them causes errors.
        for k,v in pairs(bld) do
            if k == 'friction' then bld.friction = uibs.friction end
            if k == 'use_dump' then bld.use_dump = uibs.use_dump end
            if k == 'dump_x_shift' then bld.dump_x_shift = uibs.dump_x_shift end
            if k == 'dump_y_shift' then bld.dump_y_shift = uibs.dump_y_shift end
            if k == 'speed' then bld.speed = uibs.speed end
        end
        table.insert(blds, bld)
    end end
    return blds
end

--
-- GlobalSettings dialog
--

local GlobalSettings = defclass(GlobalSettings, dialogs.MessageBox)
GlobalSettings.focus_path = 'buildingplan_globalsettings'

GlobalSettings.ATTRS{
    settings = {}
}

function GlobalSettings:onDismiss()
    for k,v in pairs(self.settings) do
        -- call back into C++ to save changes
        setSetting(k, v)
    end
end

-- does not need the core suspended.
function show_global_settings_dialog(settings)
    GlobalSettings{
        frame_title="Buildingplan Global Settings",
        settings=settings,
    }:show()
end

function GlobalSettings:toggle_setting(name)
    self.settings[name] = not self.settings[name]
end

function GlobalSettings:get_setting_string(name)
    if self.settings[name] then return 'On' end
    return 'Off'
end

function GlobalSettings:get_setting_pen(name)
    if self.settings[name] then return COLOR_LIGHTGREEN end
    return COLOR_LIGHTRED
end

function GlobalSettings:is_setting_enabled(name)
    return self.settings[name]
end

function GlobalSettings:make_setting_label_token(text, key, name, width)
    return {text=text, key=key, key_sep=': ', key_pen=COLOR_LIGHTGREEN,
            on_activate=self:callback('toggle_setting', name), width=width}
end

function GlobalSettings:make_setting_value_token(name)
    return {text=self:callback('get_setting_string', name),
            enabled=self:callback('is_setting_enabled', name),
            pen=self:callback('get_setting_pen', name),
            dpen=COLOR_GRAY}
end

-- mockup:
--[[
                          Buildingplan Global Settings

  e: Enable all: Off
    Enables buildingplan for all building types. Use this to avoid having to
    manually enable buildingplan for each building type that you want to plan.
    Note that DFHack quickfort will use buildingplan to manage buildings
    regardless of whether buildingplan is "enabled" for the building type.

  Allowed types for generic, fire-safe, and magma-safe building material:
  b: Blocks:   On
  s: Boulders: On
  w: Wood:     On
  r: Bars:     Off
    Changes to these settings will be applied to newly-planned buildings.

  A: Apply building material filter settings to existing planned buildings
    Use this if your planned buildings can't be completed because the settings
    above were too restrictive when the buildings were originally planned.

  M: Edit list of materials to avoid
  potash
  pearlash
  ash
  coal
    Buildingplan will avoid using these material types when a planned building's
    material filter is set to 'any'. They can stil be matched when they are
    explicitly allowed by a planned building's material filter. Changes to this
    list take effect for existing buildings immediately.

  g: Allow bags: Off
    This allows bags to be placed where a 'coffer' is planned.

  f: Legacy Quickfort Mode: Off
    Compatibility mode for the legacy Python-based Quickfort application. This
    setting is not needed for DFHack quickfort.
--]]
function GlobalSettings:init()

    self.subviews.label:setText{
        self:make_setting_label_token('Enable all', 'CUSTOM_E', 'all_enabled', 12),
            self:make_setting_value_token('all_enabled'), '\n',
        '  Enables buildingplan for all building types. Use this to avoid having\n',
        '  to manually enable buildingplan for each building type that you want\n',
        '  to plan. Note that DFHack quickfort will use buildingplan to manage\n',
        '  buildings regardless of whether buildingplan is "enabled" for the\n',
        '  building type.\n',
        '\n',
        'Allowed types for generic, fire-safe, and magma-safe building material:\n',
        self:make_setting_label_token('Blocks', 'CUSTOM_B', 'blocks', 10),
            self:make_setting_value_token('blocks'), '\n',
        self:make_setting_label_token('Boulders', 'CUSTOM_S', 'boulders', 10),
            self:make_setting_value_token('boulders'), '\n',
        self:make_setting_label_token('Wood', 'CUSTOM_W', 'logs', 10),
            self:make_setting_value_token('logs'), '\n',
        self:make_setting_label_token('Bars', 'CUSTOM_R', 'bars', 10),
            self:make_setting_value_token('bars'), '\n',
        '  Changes to these settings will be applied to newly-planned buildings.\n',
        '  If no types are enabled above, then any building material is allowed.\n',
        '\n',
        self:make_setting_label_token('Legacy Quickfort Mode', 'CUSTOM_F',
                                      'quickfort_mode', 23),
            self:make_setting_value_token('quickfort_mode'), '\n',
        '  Compatibility mode for the legacy Python-based Quickfort application.\n',
        '  This setting is not needed for DFHack quickfort.'
    }
end

return _ENV
