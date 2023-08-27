-- config ui for autochop

local gui = require('gui')
local widgets = require('gui.widgets')
local plugin = require('plugins.autochop')

local PROPERTIES_HEADER = 'Chop  Clear  Trees  Marked  Prot  '
local REFRESH_MS = 10000

--
-- BurrowSettings
--

BurrowSettings = defclass(BurrowSettings, widgets.Window)
BurrowSettings.ATTRS{
    frame={l=0, t=5, w=56, h=13},
}

function BurrowSettings:init()
    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text='Burrow: ',
        },
        widgets.Label{
            view_id='name',
            frame={t=0, l=8},
            text_pen=COLOR_GREEN,
        },
        widgets.ToggleHotkeyLabel{
            view_id='chop',
            frame={t=2, l=0},
            key='CUSTOM_C',
            label='Allow autochop to chop trees in this burrow',
        },
        widgets.ToggleHotkeyLabel{
            view_id='clearcut',
            frame={t=3, l=0},
            key='CUSTOM_SHIFT_C',
            label='Cut down all trees in this burrow',
        },
        widgets.ToggleHotkeyLabel{
            view_id='brewable',
            frame={t=4, l=0},
            key='CUSTOM_B',
            label='Protect trees that produce brewable items',
        },
        widgets.ToggleHotkeyLabel{
            view_id='edible',
            frame={t=5, l=0},
            key='CUSTOM_E',
            label='Protect trees that produce edible items',
        },
        widgets.ToggleHotkeyLabel{
            view_id='cookable',
            frame={t=6, l=0},
            key='CUSTOM_Z',
            label='Protect trees that produce cookable items',
        },
        widgets.HotkeyLabel{
            frame={t=8, l=0},
            key='SELECT',
            label='Apply',
            on_activate=self:callback('commit'),
        },
    }
end

function BurrowSettings:show(choice, on_commit)
    self.data = choice.data
    self.on_commit = on_commit
    local data = self.data
    self.subviews.name:setText(data.name)
    self.subviews.chop:setOption(data.chop)
    self.subviews.clearcut:setOption(data.clearcut)
    self.subviews.brewable:setOption(data.protect_brewable)
    self.subviews.edible:setOption(data.protect_edible)
    self.subviews.cookable:setOption(data.protect_cookable)
    self.visible = true
    self:setFocus(true)
    self:updateLayout()
end

function BurrowSettings:hide()
    self:setFocus(false)
    self.visible = false
end

function BurrowSettings:commit()
    local data = {
        id=self.data.id,
        chop=self.subviews.chop:getOptionValue(),
        clearcut=self.subviews.clearcut:getOptionValue(),
        protect_brewable=self.subviews.brewable:getOptionValue(),
        protect_edible=self.subviews.edible:getOptionValue(),
        protect_cookable=self.subviews.cookable:getOptionValue(),
    }
    plugin.setBurrowConfig(data)
    self:hide()
    self.on_commit()
end

function BurrowSettings:onInput(keys)
    if keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        self:hide()
        return true
    end
    BurrowSettings.super.onInput(self, keys)
    return true -- we're a modal dialog
end

--
-- Autochop
--

Autochop = defclass(Autochop, widgets.Window)
Autochop.ATTRS {
    frame_title='Autochop',
    frame={w=60, h=28},
    resizable=true,
    resize_min={h=25},
}

function Autochop:init()
    local minimal = false
    local saved_frame = {w=45, h=6, r=2, t=18}
    local saved_resize_min = {w=saved_frame.w, h=saved_frame.h}
    local function toggle_minimal()
        minimal = not minimal
        local swap = self.frame
        self.frame = saved_frame
        saved_frame = swap
        swap = self.resize_min
        self.resize_min = saved_resize_min
        saved_resize_min = swap
        self:updateLayout()
        self:refresh_data()
    end
    local function is_minimal()
        return minimal
    end
    local function is_not_minimal()
        return not minimal
    end

    self:addviews{
        widgets.ToggleHotkeyLabel{
            view_id='enable_toggle',
            frame={t=0, l=0, w=31},
            label='Autochop is',
            key='CUSTOM_CTRL_E',
            options={{value=true, label='Enabled', pen=COLOR_GREEN},
                     {value=false, label='Disabled', pen=COLOR_RED}},
            on_change=function(val) plugin.setEnabled(val) end,
        },
        widgets.HotkeyLabel{
            frame={r=0, t=0, w=10},
            key='CUSTOM_ALT_M',
            label=string.char(31)..string.char(30),
            on_activate=toggle_minimal},
        widgets.Label{
            view_id='minimal_summary',
            frame={t=1, l=0, h=1},
            auto_height=false,
            visible=is_minimal,
        },
        widgets.EditField{
            view_id='target',
            frame={t=1, l=0},
            label_text='Target number of logs: ',
            key='CUSTOM_CTRL_N',
            on_char=function(ch) return ch:match('%d') end,
            on_submit=function(text)
                plugin.setTargets(text)
                self:refresh_data()
                self:update_choices()
            end,
            visible=is_not_minimal,
        },
        widgets.Label{
            frame={t=3, l=0},
            text='Burrow',
            auto_width=true,
            visible=is_not_minimal,
        },
        widgets.Label{
            frame={t=3, r=0},
            text=PROPERTIES_HEADER,
            auto_width=true,
            visible=is_not_minimal,
        },
        widgets.List{
            view_id='list',
            frame={t=5, l=0, r=0, b=15},
            on_submit=self:callback('configure_burrow'),
            visible=is_not_minimal,
        },
        widgets.Label{
            frame={b=13, l=0},
            view_id='chop_message',
            visible=is_not_minimal,
        },
        widgets.ToggleHotkeyLabel{
            view_id='hide',
            frame={b=11, l=0},
            label='Hide burrows with no trees: ',
            key='CUSTOM_CTRL_H',
            initial_option=false,
            on_change=function() self:update_choices() end,
            visible=is_not_minimal,
        },
        widgets.ToggleHotkeyLabel{
            view_id='hide_disabled',
            frame={b=10, l=0},
            label='Hide burrows with no chop/clear order set: ',
            key='CUSTOM_ALT_H',
            initial_option=self:getDefaultHide(),
            on_change=function() self:update_choices() end,
            visible=is_not_minimal,
        },
        widgets.HotkeyLabel{
            frame={b=9, l=0},
            label='Designate trees for chopping now',
            key='CUSTOM_CTRL_D',
            on_activate=function()
                plugin.autochop_designate()
                self:refresh_data()
                self:update_choices()
            end,
            visible=is_not_minimal,
        },
        widgets.HotkeyLabel{
            frame={b=8, l=0},
            label='Undesignate all trees now',
            key='CUSTOM_CTRL_U',
            on_activate=function()
                plugin.autochop_undesignate()
                self:refresh_data()
                self:update_choices()
            end,
            visible=is_not_minimal,
        },
        widgets.Label{
            view_id='summary',
            frame={b=0, l=0},
            visible=is_not_minimal,
        },
        BurrowSettings{
            view_id='burrow_settings',
            visible=false,
        },
    }

    self:refresh_data()
end

function Autochop:hasEnabledBurrows()
    self.data = plugin.getTreeCountsAndBurrowConfigs()
    --- check to see if we have any already chop/clear enabled burrows
    for _,c in ipairs(self.data.burrow_configs) do
        if c.chop or c.clearcut then
            return true
        end
    end

    return false
end

function Autochop:getDefaultHide()
    return self:hasEnabledBurrows()
end

function Autochop:configure_burrow(idx, choice)
    self.subviews.burrow_settings:show(choice, function()
                self:refresh_data()
                self:update_choices()
            end)
end

function Autochop:update_choices()
    local list = self.subviews.list
    local name_width = list.frame_body.width - #PROPERTIES_HEADER
    local fmt = '%-'..tostring(name_width)..'s [%s]   [%s]   %5d  %5d    %s'
    local hide_empty = self.subviews.hide:getOptionValue()
    local hide_disabled = self.subviews.hide_disabled:getOptionValue()
    local choices = {}
    local has_chop_burrow = false

    for _,c in ipairs(self.data.burrow_configs) do
        has_chop_burrow = has_chop_burrow or c.chop
        local num_trees = self.data.tree_counts[c.id] or 0
        if not hide_empty or num_trees > 0 then
            if not hide_disabled or (c.chop or c.clearcut) then
                local text = (fmt):format(
                        c.name:sub(1,name_width), c.chop and 'x' or ' ',
                        c.clearcut and 'x' or ' ',
                        num_trees, self.data.designated_tree_counts[c.id] or 0,
                        (c.protect_brewable and 'b' or '')..
                            (c.protect_edible and 'e' or '')..
                            (c.protect_cookable and 'z' or ''))
                table.insert(choices, {text=text, data=c})
            end
        end
    end
    self.subviews.list:setChoices(choices)
    self.subviews.list:updateLayout()
    self.subviews.chop_message:setText(has_chop_burrow and
            'Will chop in burrows tagged with "Chop"' or
            'Will chop any accessible trees on the map')
end

local function get_stat_pen(val, mid_val, low_val)
    if val < low_val then return COLOR_RED end
    if val < mid_val then return COLOR_YELLOW end
    return COLOR_GREEN
end

local function get_stat_text(val, mid_val, low_val)
    return {text=tostring(val), pen=get_stat_pen(val, mid_val, low_val)}
end

function Autochop:refresh_data()
    self.subviews.enable_toggle:setOption(plugin.isEnabled())

    local target_max, target_min = plugin.autochop_getTargets()
    self.subviews.target:setText(tostring(target_max))

    self.data = plugin.getTreeCountsAndBurrowConfigs()

    local log_count = plugin.autochop_getLogCounts()
    local summary = self.data.summary
    local summary_text = {
        '                 Usable logs in stock: ',
        get_stat_text(log_count, target_max, target_min),
        NEWLINE,
        'Approx. logs left in accessible trees: ',
        get_stat_text(summary.accessible_yield, 3*target_max, target_max),
        NEWLINE,
        '               Total trees designated: ', tostring(summary.designated_trees), NEWLINE,
        '   Approx. logs from designated trees: ',
        get_stat_text(summary.expected_yield, target_max-log_count, target_max-log_count),
        NEWLINE,
        NEWLINE,
        'Total trees harvested: ', tostring(df.global.plotinfo.trees_removed)
    }
    self.subviews.summary:setText(summary_text)

    local minimal_summary_text = {
        'Usable logs in stock: ',
        get_stat_text(log_count, target_max, target_min),
        '/',
        tostring(target_max),
    }
    self.subviews.minimal_summary:setText(minimal_summary_text)

    self.next_refresh_ms = dfhack.getTickCount() + REFRESH_MS
end

function Autochop:postUpdateLayout()
    self:update_choices()
end

-- refreshes data every 10 seconds or so
function Autochop:onRenderBody()
    if self.next_refresh_ms <= dfhack.getTickCount() then
        self:refresh_data()
        self:update_choices()
    end
end

--
-- AutochopScreen
--

AutochopScreen = defclass(AutochopScreen, gui.ZScreen)
AutochopScreen.ATTRS {
    focus_path='autochop',
}

function AutochopScreen:init()
    self:addviews{Autochop{}}
end

function AutochopScreen:onDismiss()
    view = nil
end

if not dfhack.isMapLoaded() then
    qerror('autochop requires a map to be loaded')
end

view = view and view:raise() or AutochopScreen{}:show()
