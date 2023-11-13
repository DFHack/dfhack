local _ENV = mkmodule('plugins.buildingplan.filterselection')

local gui = require('gui')
local pens = require('plugins.buildingplan.pens')
local widgets = require('gui.widgets')

local uibs = df.global.buildreq
local to_pen = dfhack.pen.parse

local function get_cur_filters()
    return dfhack.buildings.getFiltersByType({}, uibs.building_type,
            uibs.building_subtype, uibs.custom_type)
end

--------------------------------
-- QualityAndMaterialsPage
--

QualityAndMaterialsPage = defclass(QualityAndMaterialsPage, widgets.Panel)
QualityAndMaterialsPage.ATTRS{
    frame={t=0, l=0},
    index=DEFAULT_NIL,
    desc=DEFAULT_NIL,
}

local TYPE_COL_WIDTH = 20
local HEADER_HEIGHT = 7
local QUALITY_HEIGHT = 9
local FOOTER_HEIGHT = 4

-- returns whether the items matched by the specified filter can have a quality
-- rating. This also conveniently indicates whether an item can be decorated.
local function can_be_improved(idx)
    local filter = get_cur_filters()[idx]
    if filter.flags2 and filter.flags2.building_material then
        return false;
    end
    return filter.item_type ~= df.item_type.WOOD and
            filter.item_type ~= df.item_type.BLOCKS and
            filter.item_type ~= df.item_type.BAR and
            filter.item_type ~= df.item_type.BOULDER
end

local function mat_sort_by_name(a, b)
    return a.name < b.name
end

local function mat_sort_by_quantity(a, b)
    return a.quantity > b.quantity or
            (a.quantity == b.quantity and mat_sort_by_name(a, b))
end

function QualityAndMaterialsPage:init()
    self.dirty = true
    self.summary = ''

    local enable_item_quality = can_be_improved(self.index)

    self:addviews{
        widgets.Panel{
            view_id='header',
            frame={l=0, t=0, h=HEADER_HEIGHT, r=0},
            frame_inset={l=1},
            subviews={
                widgets.Label{
                    frame={l=0, t=0},
                    text='Current filter:',
                },
                widgets.WrappedLabel{
                    frame={l=16, t=0, h=2, r=0},
                    text_pen=COLOR_LIGHTCYAN,
                    text_to_wrap=function() return self.summary end,
                    auto_height=false,
                },
                widgets.CycleHotkeyLabel{
                    view_id='mat_sort',
                    frame={l=0, t=3, w=21},
                    label='Sort by:',
                    key='CUSTOM_SHIFT_R',
                    options={
                        {label='name', value=mat_sort_by_name},
                        {label='available', value=mat_sort_by_quantity}
                    },
                    on_change=function() self.dirty = true end,
                },
                widgets.ToggleHotkeyLabel{
                    view_id='hide_zero',
                    frame={l=0, t=4, w=24},
                    label='Hide unavailable:',
                    key='CUSTOM_SHIFT_H',
                    initial_option=false,
                    on_change=function() self.dirty = true end,
                },
                widgets.EditField{
                    view_id='search',
                    frame={l=26, t=3},
                    label_text='Search: ',
                    on_char=function(ch) return ch:match('[%l -]') end,
                },
                widgets.Label{
                    frame={l=1, b=0},
                    text='Type',
                    text_pen=COLOR_LIGHTRED,
                },
                widgets.Label{
                    frame={l=TYPE_COL_WIDTH, b=0},
                    text='Material',
                    text_pen=COLOR_LIGHTRED,
                },
            },
        },
        widgets.Panel{
            view_id='materials_lists',
            frame={l=0, t=HEADER_HEIGHT, r=0, b=FOOTER_HEIGHT+QUALITY_HEIGHT},
            frame_style=gui.INTERIOR_FRAME,
            subviews={
                widgets.List{
                    view_id='materials_categories',
                    frame={l=1, t=0, b=0, w=TYPE_COL_WIDTH-3},
                    scroll_keys={},
                    icon_width=2,
                    cursor_pen=COLOR_CYAN,
                    on_submit=self:callback('toggle_category'),
                },
                widgets.FilteredList{
                    view_id='materials_mats',
                    frame={l=TYPE_COL_WIDTH, t=0, r=0, b=0},
                    icon_width=2,
                    on_submit=self:callback('toggle_material'),
                },
            },
        },
        widgets.Panel{
            view_id='divider',
            frame={l=TYPE_COL_WIDTH-1, t=HEADER_HEIGHT, b=FOOTER_HEIGHT+QUALITY_HEIGHT, w=1},
            on_render=self:callback('draw_divider'),
        },
        widgets.Panel{
            view_id='quality_panel',
            frame={l=0, r=0, h=QUALITY_HEIGHT, b=FOOTER_HEIGHT},
            frame_style=gui.INTERIOR_FRAME,
            frame_title='Item quality',
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='decorated',
                    frame={l=0, t=1, w=23},
                    key='CUSTOM_SHIFT_D',
                    label='Decorated only:',
                    options={
                        {label='No', value=false},
                        {label='Yes', value=true},
                    },
                    enabled=enable_item_quality,
                    on_change=self:callback('set_decorated'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='min_quality',
                    frame={l=0, t=3, w=18},
                    label='Min quality:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_Z',
                    key='CUSTOM_SHIFT_X',
                    options={
                        {label='Ordinary', value=0},
                        {label='Well Crafted', value=1},
                        {label='Finely Crafted', value=2},
                        {label='Superior', value=3},
                        {label='Exceptional', value=4},
                        {label='Masterful', value=5},
                        {label='Artifact', value=6},
                    },
                    enabled=enable_item_quality,
                    on_change=function(val) self:set_min_quality(val+1) end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='max_quality',
                    frame={r=1, t=3, w=18},
                    label='Max quality:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_Q',
                    key='CUSTOM_SHIFT_W',
                    options={
                        {label='Ordinary', value=0},
                        {label='Well Crafted', value=1},
                        {label='Finely Crafted', value=2},
                        {label='Superior', value=3},
                        {label='Exceptional', value=4},
                        {label='Masterful', value=5},
                        {label='Artifact', value=6},
                    },
                    enabled=enable_item_quality,
                    on_change=function(val) self:set_max_quality(val+1) end,
                },
                widgets.RangeSlider{
                    frame={l=0, t=6},
                    num_stops=7,
                    get_left_idx_fn=function()
                        return self.subviews.min_quality:getOptionValue() + 1
                    end,
                    get_right_idx_fn=function()
                        return self.subviews.max_quality:getOptionValue() + 1
                    end,
                    on_left_change=self:callback('set_min_quality'),
                    on_right_change=self:callback('set_max_quality'),
                    active=enable_item_quality,
                },
            },
        },
        widgets.Panel{
            view_id='footer',
            frame={l=0, r=0, b=0, h=FOOTER_HEIGHT},
            frame_inset={t=1, l=1},
            subviews={
                widgets.HotkeyLabel{
                    frame={l=0, t=0},
                    label='Toggle',
                    auto_width=true,
                    key='SELECT',
                },
                widgets.HotkeyLabel{
                    frame={l=0, t=2},
                    label='Done',
                    auto_width=true,
                    key='LEAVESCREEN',
                },
                widgets.HotkeyLabel{
                    frame={l=30, t=0},
                    label='Invert selection',
                    auto_width=true,
                    key='CUSTOM_SHIFT_I',
                    on_activate=self:callback('invert_materials'),
                },
                widgets.HotkeyLabel{
                    frame={l=30, t=2},
                    label='Reset filter',
                    auto_width=true,
                    key='CUSTOM_CTRL_X',
                    on_activate=self:callback('clear_filter'),
                },
            },
        }
    }

    -- replace the FilteredList's built-in EditField with our own
    self.subviews.materials_mats.list.frame.t = 0
    self.subviews.materials_mats.edit.visible = false
    self.subviews.materials_mats.edit = self.subviews.search
    self.subviews.search.on_change = self.subviews.materials_mats:callback('onFilterChange')
end

local MAT_ENABLED_PEN = to_pen{ch=string.char(251), fg=COLOR_LIGHTGREEN}
local MAT_DISABLED_PEN = to_pen{ch='x', fg=COLOR_RED}

local function make_cat_choice(label, cat, key, cats)
    local enabled = cats[cat]
    local icon = nil
    if not cats.unset then
        icon = enabled and MAT_ENABLED_PEN or MAT_DISABLED_PEN
    end
    return {
        text=label,
        key=key,
        enabled=enabled,
        cat=cat,
        icon=icon,
    }
end

local function make_mat_choice(name, props, enabled, cats)
    local quantity = tonumber(props.count)
    local text = ('%5d - %s'):format(quantity, name)
    local icon = nil
    if not cats.unset then
        icon = enabled and MAT_ENABLED_PEN or MAT_DISABLED_PEN
    end
    return {
        text=text,
        enabled=enabled,
        icon=icon,
        name=name,
        cat=props.category,
        quantity=quantity,
    }
end

function QualityAndMaterialsPage:refresh()
    local summary = self.desc
    local subviews = self.subviews

    local buildingplan = require('plugins.buildingplan')

    local heat = buildingplan.getHeatSafetyFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type)
    if heat >= 2 then summary = 'Magma safe ' .. summary
    elseif heat == 1 then summary = 'Fire safe ' .. summary
    else summary = 'Any ' .. summary
    end

    local specials = buildingplan.getSpecials(uibs.building_type, uibs.building_subtype, uibs.custom_type)
    if next(specials) then
        local specials_list = {}
        for special in pairs(specials) do
            table.insert(specials_list, special)
        end
        summary = summary .. ' [' .. table.concat(specials_list, ', ') .. ']'
    end

    local quality = buildingplan.getQualityFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.index-1)
    subviews.decorated:setOption(quality.decorated ~= 0)
    subviews.min_quality:setOption(quality.min_quality)
    subviews.max_quality:setOption(quality.max_quality)

    local cats = buildingplan.getMaterialMaskFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.index-1)
    local category_choices={
        make_cat_choice('Stone', 'stone', 'CUSTOM_SHIFT_S', cats),
        make_cat_choice('Wood', 'wood', 'CUSTOM_SHIFT_O', cats),
        make_cat_choice('Metal', 'metal', 'CUSTOM_SHIFT_M', cats),
        make_cat_choice('Glass', 'glass', 'CUSTOM_SHIFT_G', cats),
        make_cat_choice('Gem', 'gem', 'CUSTOM_SHIFT_E', cats),
        make_cat_choice('Clay', 'clay', 'CUSTOM_SHIFT_C', cats),
        make_cat_choice('Cloth', 'cloth', 'CUSTOM_SHIFT_L', cats),
        make_cat_choice('Silk', 'silk', 'CUSTOM_SHIFT_K', cats),
        make_cat_choice('Yarn', 'yarn', 'CUSTOM_SHIFT_Y', cats),
    }
    self.subviews.materials_categories:setChoices(category_choices)

    local mats = buildingplan.getMaterialFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.index-1)
    local mat_choices = {}
    local hide_zero = self.subviews.hide_zero:getOptionValue()
    local enabled_mat_names = {}
    for name,props in pairs(mats) do
        local enabled = props.enabled == 'true' and cats[props.category]
        if not cats.unset and enabled then
            table.insert(enabled_mat_names, name)
        end
        if not hide_zero or tonumber(props.count) > 0 then
            table.insert(mat_choices, make_mat_choice(name, props, enabled, cats))
        end
    end
    table.sort(mat_choices, self.subviews.mat_sort:getOptionValue())

    local prev_filter = self.subviews.search.text
    self.subviews.materials_mats:setChoices(mat_choices)
    self.subviews.materials_mats:setFilter(prev_filter)

    if #enabled_mat_names > 0 then
        table.sort(enabled_mat_names)
        summary = summary .. (' of %s'):format(table.concat(enabled_mat_names, ', '))
    end

    self.summary = summary
    self.dirty = false
    self:updateLayout()
end

function QualityAndMaterialsPage:toggle_category(_, choice)
    local cats = {}
    if not choice.icon then
        -- toggling from unset to something is set
        table.insert(cats, choice.cat)
    else
        choice.enabled = not choice.enabled
        for _,c in ipairs(self.subviews.materials_categories:getChoices()) do
            if c.enabled then
                table.insert(cats, c.cat)
            end
        end
    end
    require('plugins.buildingplan').setMaterialMaskFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.index-1, cats)
    self.dirty = true
end

function QualityAndMaterialsPage:toggle_material(_, choice)
    local mats = {}
    if not choice.icon then
        -- toggling from unset to something is set
        table.insert(mats, choice.name)
    else
        for _,c in ipairs(self.subviews.materials_mats:getChoices()) do
            local enabled = c.enabled
            if choice.name == c.name then
                enabled = not c.enabled
            end
            if enabled then
                table.insert(mats, c.name)
            end
        end
    end
    require('plugins.buildingplan').setMaterialFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.index-1, mats)
    self.dirty = true
end

function QualityAndMaterialsPage:invert_materials()
    local mats = {}
    for _,c in ipairs(self.subviews.materials_mats:getChoices()) do
        if not c.icon then return end
        if not c.enabled then
            table.insert(mats, c.name)
        end
    end
    require('plugins.buildingplan').setMaterialFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.index-1, mats)
    self.dirty = true
end

function QualityAndMaterialsPage:clear_filter()
    require('plugins.buildingplan').clearFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.index-1)
    self.dirty = true
end

function QualityAndMaterialsPage:set_decorated(decorated)
    local subviews = self.subviews
    require('plugins.buildingplan').setQualityFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.index-1,
            decorated and 1 or 0, subviews.min_quality:getOptionValue(), subviews.max_quality:getOptionValue())
    self.dirty = true
end

function QualityAndMaterialsPage:set_min_quality(idx)
    idx = math.min(6, math.max(0, idx-1))
    local subviews = self.subviews
    subviews.min_quality:setOption(idx)
    if subviews.max_quality:getOptionValue() < idx then
        subviews.max_quality:setOption(idx)
    end
    require('plugins.buildingplan').setQualityFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.index-1,
            subviews.decorated:getOptionValue() and 1 or 0, idx, subviews.max_quality:getOptionValue())
    self.dirty = true
end

function QualityAndMaterialsPage:set_max_quality(idx)
    idx = math.min(6, math.max(0, idx-1))
    local subviews = self.subviews
    subviews.max_quality:setOption(idx)
    if subviews.min_quality:getOptionValue() > idx then
        subviews.min_quality:setOption(idx)
    end
    require('plugins.buildingplan').setQualityFilter(uibs.building_type, uibs.building_subtype, uibs.custom_type, self.index-1,
            subviews.decorated:getOptionValue() and 1 or 0, subviews.min_quality:getOptionValue(), idx)
    self.dirty = true
end

function QualityAndMaterialsPage:draw_divider(dc)
    local y2 = dc.height - 1
    for y=0,y2 do
        dc:seek(0, y)
        if y == 0 then
            dc:char(nil, pens.VERT_TOP_PEN)
        elseif y == y2 then
            dc:char(nil, pens.VERT_BOT_PEN)
        else
            dc:char(nil, pens.VERT_MID_PEN)
        end
    end
end

function QualityAndMaterialsPage:onRenderFrame(dc, rect)
    QualityAndMaterialsPage.super.onRenderFrame(self, dc, rect)
    if self.dirty then
        self:refresh()
    end
end

--------------------------------
-- GlobalSettingsPage
--

GlobalSettingsPage = defclass(GlobalSettingsPage, widgets.ResizingPanel)
GlobalSettingsPage.ATTRS{
    autoarrange_subviews=true,
    frame={t=0, l=0},
    frame_style=gui.INTERIOR_FRAME,
}

function GlobalSettingsPage:init()
    self:addviews{
        widgets.WrappedLabel{
            frame={l=0},
            text_to_wrap='These options will affect the selection of "Generic Materials" for all future buildings.',
        },
        widgets.Panel{
            frame={h=1},
        },
        widgets.ToggleHotkeyLabel{
            view_id='blocks',
            frame={l=0},
            key='CUSTOM_B',
            label='Blocks',
            label_width=8,
            on_change=self:callback('update_setting', 'blocks'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='logs',
            frame={l=0},
            key='CUSTOM_L',
            label='Logs',
            label_width=8,
            on_change=self:callback('update_setting', 'logs'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='boulders',
            frame={l=0},
            key='CUSTOM_O',
            label='Boulders',
            label_width=8,
            on_change=self:callback('update_setting', 'boulders'),
        },
        widgets.ToggleHotkeyLabel{
            view_id='bars',
            frame={l=0},
            key='CUSTOM_R',
            label='Bars',
            label_width=8,
            on_change=self:callback('update_setting', 'bars'),
        },
    }

    self:init_settings()
end

function GlobalSettingsPage:init_settings()
    local settings = require('plugins.buildingplan').getGlobalSettings()
    local subviews = self.subviews
    subviews.blocks:setOption(settings.blocks)
    subviews.logs:setOption(settings.logs)
    subviews.boulders:setOption(settings.boulders)
    subviews.bars:setOption(settings.bars)
end

function GlobalSettingsPage:update_setting(setting, val)
    dfhack.run_command('buildingplan', 'set', setting, tostring(val))
    self:init_settings()
end

--------------------------------
-- FilterSelection
--

FilterSelection = defclass(FilterSelection, widgets.Window)
FilterSelection.ATTRS{
    frame_title='Choose filters',
    frame={w=55, h=53, l=30, t=8},
    frame_inset={t=1},
    resizable=true,
    index=DEFAULT_NIL,
    desc=DEFAULT_NIL,
    autoarrange_subviews=true,
}

function FilterSelection:init()
    self:addviews{
        widgets.TabBar{
            frame={t=0},
            labels={
                'Quality and materials',
                'Global settings',
            },
            on_select=function(idx)
                self.subviews.pages:setSelected(idx)
                self:updateLayout()
            end,
            get_cur_page=function() return self.subviews.pages:getSelected() end,
            key='CUSTOM_CTRL_T',
        },
        widgets.Widget{
            frame={h=1},
        },
        widgets.Pages{
            view_id='pages',
            frame={t=5, l=0, b=0, r=0},
            subviews={
                QualityAndMaterialsPage{
                    index=self.index,
                    desc=self.desc
                },
                GlobalSettingsPage{},
            },
        },
    }
end

FilterSelectionScreen = defclass(FilterSelectionScreen, gui.ZScreen)
FilterSelectionScreen.ATTRS {
    focus_path='dwarfmode/Building/Placement/dfhack/lua/buildingplan/filterselection',
    pass_movement_keys=true,
    pass_mouse_clicks=false,
    defocusable=false,
    index=DEFAULT_NIL,
    desc=DEFAULT_NIL,
}

function FilterSelectionScreen:init()
    self:addviews{
        FilterSelection{
            index=self.index,
            desc=self.desc
        }
    }
end

function FilterSelectionScreen:onShow()
    -- don't let the building "shadow" follow the mouse cursor while this screen is open
    df.global.game.main_interface.bottom_mode_selected = -1
end

function FilterSelectionScreen:onDismiss()
    -- re-enable building shadow
    df.global.game.main_interface.bottom_mode_selected = df.main_bottom_mode_type.BUILDING_PLACEMENT
end

return _ENV
