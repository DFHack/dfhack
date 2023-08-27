--rename items or units with the native interface
--[====[

names
=====

Rename units or items (including artifacts) with the native interface.
If a first name is desired, press ``f``. To clear the current first name,
leave this blank.

]====]

local gui = require 'gui'
local dlg = require 'gui.dialogs'
local widgets = require 'gui.widgets'
local utils = require 'utils'

local validArgs = utils.invert({
    'help',
})
local args = utils.processArgs({...}, validArgs)
if args.help then
    print(
[[names.lua
arguments:
    -help
        print this help message
    if a first name is desired press f, leave blank to clear current first name
    if viewing an artifact you can rename it
    if viewing a unit you can rename them
]])
    return
end

setup_screen = setup_screen or df.viewscreen_setupdwarfgamest:new()
setup_screen.show_play_now = 0

namescr = defclass(namescr, gui.Screen)
namescr.focus_path = 'names'

function namescr:init()
    local parent = dfhack.gui.getCurViewscreen()
    local nameType
    local trg = dfhack.gui.getAnyUnit(parent)
    if trg then
        nameType = df.language_name_type.Figure
    elseif df.viewscreen_itemst:is_instance(parent) then
        local fact = dfhack.items.getGeneralRef(parent.item, df.general_ref_type.IS_ARTIFACT) --hint:df.viewscreen_itemst
        if fact then
            local artifact = df.artifact_record.find(fact.artifact_id) --hint:df.general_ref_is_artifactst
            trg = artifact --luacheck: retype
            nameType = df.language_name_type.Artifact
        end
    elseif df.viewscreen_dungeon_monsterstatusst:is_instance(parent) then
        local uid = parent.unit.id --hint:df.viewscreen_dungeon_monsterstatusst
        trg = df.unit.find(uid) --luacheck: retype
        nameType = df.language_name_type.Figure
    elseif df.global.adventure.menu == df.ui_advmode_menu.Look then
        local t_look = df.global.ui_look_list.items[df.global.ui_look_cursor]
        if t_look.type == df.ui_look_list.T_items.T_type.Unit then
            trg = t_look.data.Unit --luacheck: retype
            nameType = df.language_name_type.Figure
        end
    else
        qerror('Could not find valid target')
    end
    if trg.name.language == -1 then
        qerror("Target's name does not have a language")
    end
    self.trg = trg
    setup_screen.fort_name:assign(trg.name)
    gui.simulateInput(setup_screen, 'SETUP_NAME_FORT')
    dfhack.gui.getCurViewscreen().type = nameType
end
function namescr:setName()
    local name = setup_screen.fort_name
    self.trg.name:assign(name)
    if self.trg._type == df.unit then
        local unit = self.trg
        if unit.status and unit.status.current_soul then
            unit.status.current_soul.name:assign(name)
        end
        local hf = df.historical_figure.find(unit.hist_figure_id)
        if hf then
            hf.name:assign(name)
        end
    end
end
function namescr:onRenderBody(dc)
    self._native.parent:render()
end
function namescr:onInput(keys)
    if keys.LEAVESCREEN then
        self:setName()
        self:dismiss()
        dfhack.screen.dismiss(self._native.parent)
        return
    end
    return self:sendInputToParent(keys)
end

if dfhack.gui.getViewscreenByType(df.viewscreen_layer_choose_language_namest, 0) then
    qerror('names screen already shown')
else
    namescr():show()
end
