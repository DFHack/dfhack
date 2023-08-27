-- A configuration UI for the autogems plugin

--[====[

gui/autogems
============

A frontend for the `autogems` plugin that allows configuring the gem types to be cut.

The following controls apply to the gems currently listed:

- ``s``: Searches for matching gems
- ``Shift+Enter``: Toggles the status of all listed gems

The following controls apply to the gems currently listed, as well as gems listed
*before* the current search with ``s``, if applicable:

- ``r``: Displays only "rock crystal" gems
- ``c``: Displays only gems whose color matches the selected gem
- ``m``: Displays only gems where at least one rough (uncut) gem is available
  somewhere on the map

This behavior is intended to allow for things like a search for "lazuli"
followed by pressing ``c`` to select all gems with the same color as lapis
lazuli (5 blue gems in vanilla DF), rather than further restricting that to gems
with "lazuli" in their name (only 1).

``x`` clears all filters, which is currently the only way to undo filters
(besides searching), and is useful to verify the gems selected.

]====]

gui = require "gui"
dialogs = require "gui.dialogs"
widgets = require "gui.widgets"
json = require "json"

CONFIG_KEY = "autogems/config"
blacklist = {}

gems = {}
for id, raw in pairs(df.global.world.raws.inorganics) do
    if raw.material.flags.IS_GEM then
        if blacklist[id] == nil then
            blacklist[id] = false
        end
        local name = raw.material.gem_name1
        local color = raw.material.basic_color[0] + (8 * raw.material.basic_color[1])
        local entry = {
            id = id,
            text = {
                {
                    text = function() return blacklist[id] and 'X' or string.char(251) end,
                    pen = function() return blacklist[id] and COLOR_LIGHTRED or COLOR_LIGHTGREEN end,
                },
                ' ',
                {
                    text = string.char(raw.material.tile),
                    pen = color
                },
                ' ',
                name,
            },
            color = color,
            search_key = name,
            crystal = raw.material.flags.CRYSTAL_GLASSABLE,
        }
        table.insert(gems, entry)
    end
end

local on_map_cache
function getGemTypesOnMap()
    if not on_map_cache then
        on_map_cache = {}
        for _, item in pairs(df.global.world.items.other.ROUGH) do
            on_map_cache[item.mat_index] = true
        end
    end
    return on_map_cache
end

function configPath()
    return dfhack.getSavePath() .. '/autogems.json'
end

function load()
    if dfhack.filesystem.isfile(configPath()) then
        local data = json.decode_file(configPath())
        if data.blacklist then
            for _, v in pairs(data.blacklist) do
                blacklist[v] = true
            end
        end
    end
end

function save()
    local save_blacklist = {}
    for id in ipairs(df.global.world.raws.inorganics) do
        if blacklist[id] then
            table.insert(save_blacklist, id)
        end
    end
    json.encode_file(
        {blacklist = save_blacklist},
        configPath(),
        {pretty = false}
    )
    dfhack.run_command('autogems-reload')
end

Autogems = defclass(nil, gui.FramedScreen)

Autogems.ATTRS{
    frame_title = "autogems dashboard",
}

function Autogems:init()
    load()
    self:addviews{
        widgets.FilteredList{
            view_id = 'list',
            frame = {t=1, l=1, b=4, w=38},
            choices = gems,
            on_submit = self:callback('toggle'),
            on_submit2 = self:callback('toggleAll'),
            edit_key = 'CUSTOM_S',
        },
        widgets.Label{
            view_id = 'main_controls',
            frame = {b=1, l=1},
            text = {
                {key = 'CUSTOM_X', text = ": Clear filters, ",
                    on_activate = self:callback('clearFilters'),
                    enabled = function() return self.filtered end},
                {key = 'CUSTOM_R', text = ": Rock crystal, ",
                    on_activate = self:callback('showCrystal')},
                {key = 'CUSTOM_C', text = ": Same color, ",
                    on_activate = self:callback('filterByColor')},
                {key = 'CUSTOM_M', text = ": On map",
                    on_activate = self:callback('filterOnMap')},
                NEWLINE,
                {key = 'LEAVESCREEN', text = ": Back, "},
                {key = 'SELECT', text = ": Toggle, "},
                {key = 'SEC_SELECT', text = ": Toggle all"}
            }
        },
        widgets.Label{
            view_id = 'edit_controls',
            frame = {b=1, l=1},
            text = {
                {key = 'LEAVESCREEN', text = ": Clear, "},
                {key = 'SELECT', text = ": Done"},
            }
        },
    }
    self:updateControls()
end

function Autogems:updateControls()
    self.subviews.main_controls.visible = not self.subviews.list.edit.active
    self.subviews.edit_controls.visible = self.subviews.list.edit.active
    if self.subviews.list.edit.active then
        self.filtered = true
    end
end

function Autogems:toggle(_, item)
    blacklist[item.id] = not blacklist[item.id]
end

function Autogems:toggleAll(_, item)
    local new_state = not blacklist[item.id]
    for _, item in pairs(self.subviews.list:getVisibleChoices()) do
        blacklist[item.id] = new_state
    end
end

function Autogems:clearFilters()
    self.subviews.list:setFilter('')
    self.subviews.list:setChoices(gems)
    self.filtered = false
end

function Autogems:showCrystal()
    local choices = {}
    for _, c in pairs(self.subviews.list:getChoices()) do
        if c.crystal then
            table.insert(choices, c)
        end
    end
    self.subviews.list:setChoices(choices)
    self.filtered = true
end

function Autogems:filterByColor()
    if not self.subviews.list:getSelected() then return end
    local choices = {}
    local color = select(2, self.subviews.list:getSelected()).color
    for _, c in pairs(self.subviews.list:getChoices()) do
        if c.color == color then
            table.insert(choices, c)
        end
    end
    self.subviews.list:setChoices(choices)
    self.filtered = true
end

function Autogems:filterOnMap()
    local choices = {}
    local on_map = getGemTypesOnMap()
    for _, c in pairs(self.subviews.list:getChoices()) do
        if on_map[c.id] then
            table.insert(choices, c)
        end
    end
    self.subviews.list:setChoices(choices)
    self.filtered = true
end

function Autogems:onInput(keys)
    if self.subviews.list.edit.active and (keys.SELECT or keys.LEAVESCREEN) then
        self.subviews.list.edit.active = false
        if keys.LEAVESCREEN then
            self.subviews.list:setFilter('')
        end
    elseif keys.LEAVESCREEN then
        self:dismiss()
    else
        self:inputToSubviews(keys)
    end
    self:updateControls()
end

function Autogems:onDismiss()
    save()
end

if not dfhack.isWorldLoaded() then qerror('No world loaded') end
Autogems():show()
