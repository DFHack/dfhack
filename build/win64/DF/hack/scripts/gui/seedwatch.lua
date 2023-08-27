-- config ui for seedwatch

local gui = require('gui')
local widgets = require('gui.widgets')
local plugin = require('plugins.seedwatch')

local PROPERTIES_HEADER = '   Quantity     Target   '
local REFRESH_MS = 10000
local MAX_TARGET = 2147483647
--
-- SeedSettings
--
SeedSettings = defclass(SeedSettings, widgets.Window)
SeedSettings.ATTRS{
    frame={l=5, t=5, w=35, h=9},
}

function SeedSettings:init()
    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text='Seed: ',
        },
        widgets.Label{
            view_id='name',
            frame={t=0, l=6},
            text_pen=COLOR_GREEN,
        },
        widgets.Label{
            frame={t=1, l=0},
            text='Quantity: ',
        },
        widgets.Label{
            view_id='quantity',
            frame={t=1, l=10},
            text_pen=COLOR_GREEN,
        },
        widgets.EditField{
            view_id='target',
            frame={t=2, l=0},
            label_text='Target: ',
            key='CUSTOM_CTRL_T',
            on_char=function(ch) return ch:match('%d') end,
            on_submit=self:callback('commit'),
        },
        widgets.HotkeyLabel{
            frame={t=4, l=0},
            key='SELECT',
            label='Apply',
            on_activate=self:callback('commit'),
        },
    }
end

function SeedSettings:show(choice, on_commit)
    self.data = choice.data
    self.on_commit = on_commit
    self.subviews.name:setText(self.data.name)
    self.subviews.quantity:setText(tostring(self.data.quantity))
    self.subviews.target:setText(tostring(self.data.target))
    self.visible = true
    self:setFocus(true)
    self:updateLayout()
end

function SeedSettings:hide()
    self:setFocus(false)
    self.visible = false
end

function SeedSettings:commit()
    local target = math.tointeger(self.subviews.target.text) or 0
    target = math.min(MAX_TARGET, math.max(0, target))

    plugin.seedwatch_setTarget(self.data.id, target)
    self:hide()
    self.on_commit()
end

function SeedSettings:onInput(keys)
    if keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        self:hide()
        return true
    end
    SeedSettings.super.onInput(self, keys)
    return true
end

--
-- Seedwatch
--
Seedwatch = defclass(Seedwatch, widgets.Window)
Seedwatch.ATTRS {
    frame_title='Seedwatch',
    frame={w=60, h=27},
    resizable=true,
    resize_min={h=25},
}

function Seedwatch:init()
    local minimal = false
    local saved_frame = {w=50, h=6, r=2, t=18}
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
            label='Seedwatch is',
            key='CUSTOM_CTRL_E',
            options={{value=true, label='Enabled', pen=COLOR_GREEN},
                     {value=false, label='Disabled', pen=COLOR_RED}},
            on_change=function(val) plugin.setEnabled(val) end,
        },
        widgets.EditField{
            view_id='all',
            frame={t=1, l=0},
            label_text='Target for all: ',
            key='CUSTOM_CTRL_A',
            on_char=function(ch) return ch:match('%d') end,
            on_submit=function(text)
                local target = math.tointeger(text)
                if not target or target == '' then
                    target = 0
                elseif target > MAX_TARGET then
                    target = MAX_TARGET
                end
                plugin.seedwatch_setTarget('all', target)
                self.subviews.list:setFilter('')
                self:refresh_data()
                self:update_choices()
            end,
            visible=is_not_minimal,
            text='30',
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
        widgets.Label{
            frame={t=3, l=0},
            text='Seed',
            auto_width=true,
            visible=is_not_minimal,
        },
        widgets.Label{
            frame={t=3, r=0},
            text=PROPERTIES_HEADER,
            auto_width=true,
            visible=is_not_minimal,
        },
        widgets.FilteredList{
            view_id='list',
            frame={t=5, l=0, r=0, b=3},
            on_submit=self:callback('configure_seed'),
            visible=is_not_minimal,
            edit_key = 'CUSTOM_S',
        },
        widgets.Label{
            view_id='summary',
            frame={b=0, l=0},
            visible=is_not_minimal,
        },
        SeedSettings{
            view_id='seed_settings',
            visible=false,
        },

    }

    self:refresh_data()
end

function Seedwatch:configure_seed(idx, choice)
    self.subviews.seed_settings:show(choice, function()
                self:refresh_data()
                self:update_choices()
            end)
end

function Seedwatch:update_choices()
    local list = self.subviews.list
    local name_width = list.frame_body.width - #PROPERTIES_HEADER
    local fmt = '%-'..tostring(name_width)..'s %10d %10d  '
    local choices = {}
    local prior_search=self.subviews.list.edit.text
    for k, v in pairs(self.data.seeds) do
        local text = (fmt):format(v.name:sub(1,name_width), v.quantity or 0, v.target or 0)
        table.insert(choices, {text=text, data=v})
    end

    self.subviews.list:setChoices(choices)
    if prior_search then self.subviews.list:setFilter(prior_search) end
    self.subviews.list:updateLayout()
end

function Seedwatch:refresh_data()
    self.subviews.enable_toggle:setOption(plugin.isEnabled())
    local watch_map, seed_counts = plugin.seedwatch_getData()
    self.data = {}
    self.data.sum = 0
    self.data.seeds_qty = 0
    self.data.seeds_watched = 0
    self.data.seeds = {}
    for k,v in pairs(seed_counts) do
        local seed = {}
        seed.id = df.global.world.raws.plants.all[k].id
        seed.name = df.global.world.raws.plants.all[k].seed_singular
        seed.quantity = v
        seed.target = watch_map[k] or 0
        self.data.seeds[k] = seed
        if self.data.seeds[k].target > 0 then
            self.data.seeds_watched = self.data.seeds_watched + 1
        end
        self.data.seeds_qty = self.data.seeds_qty + v
    end
    if self.subviews.all.text == '' then
        self.subviews.all:setText('0')
    end
    local summary_text = ('Seeds quantity: %d  watched: %d\n'):format(tostring(self.data.seeds_qty),tostring(self.data.seeds_watched))
    self.subviews.summary:setText(summary_text)
    local minimal_summary_text = summary_text
    self.subviews.minimal_summary:setText(minimal_summary_text)

    self.next_refresh_ms = dfhack.getTickCount() + REFRESH_MS

end


function Seedwatch:postUpdateLayout()
    self:update_choices()
end

-- refreshes data every 10 seconds or so
function Seedwatch:onRenderBody()
    if self.next_refresh_ms <= dfhack.getTickCount()
    and self.subviews.seed_settings.visible == false
    and not self.subviews.all.focus
    and not self.subviews.list.edit.focus then
        self:refresh_data()
        self:update_choices()
    end
end

--
-- SeedwatchScreen
--

SeedwatchScreen = defclass(SeedwatchScreen, gui.ZScreen)
SeedwatchScreen.ATTRS {
    focus_path='seedwatch',
}

function SeedwatchScreen:init()
    self:addviews{Seedwatch{}}
end

function SeedwatchScreen:onDismiss()
    view = nil
end

if not dfhack.isMapLoaded() then
    qerror('seedwatch requires a map to be loaded')
end

view = view and view:raise() or SeedwatchScreen{}:show()
