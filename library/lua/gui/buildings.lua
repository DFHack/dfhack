-- Stock dialog for selecting buildings

local _ENV = mkmodule('gui.buildings')

local gui = require('gui')
local widgets = require('gui.widgets')
local dlg = require('gui.dialogs')
local utils = require('utils')

ARROW = string.char(26)

WORKSHOP_ABSTRACT={
    [df.building_type.Civzone]=true,[df.building_type.Stockpile]=true,
}
WORKSHOP_SPECIAL={
    [df.building_type.Workshop]=true,[df.building_type.Furnace]=true,[df.building_type.Trap]=true,
    [df.building_type.Construction]=true,[df.building_type.SiegeEngine]=true
}
BuildingDialog = defclass(BuildingDialog, gui.FramedScreen)

BuildingDialog.focus_path = 'BuildingDialog'

BuildingDialog.ATTRS{
    prompt = 'Type or select a building from this list',
    frame_style = gui.GREY_LINE_FRAME,
    frame_inset = 1,
    frame_title = 'Select Building',
    -- new attrs
    none_caption = 'none',
    hide_none = false,
    use_abstract = true,
    use_workshops = true,
    use_tool_workshop=true,
    use_furnace = true,
    use_construction = true,
    use_siege = true,
    use_trap = true,
    use_custom = true,
    building_filter = DEFAULT_NIL,
    on_select = DEFAULT_NIL,
    on_cancel = DEFAULT_NIL,
    on_close = DEFAULT_NIL,
}

function BuildingDialog:init(info)
    self:addviews{
        widgets.Label{
            text = {
                self.prompt, '\n\n',
                'Category: ', { text = self:cb_getfield('context_str'), pen = COLOR_CYAN }
            },
            text_pen = COLOR_WHITE,
            frame = { l = 0, t = 0 },
        },
        widgets.Label{
            view_id = 'back',
            visible = false,
            text = { { key = 'LEAVESCREEN', text = ': Back' } },
            frame = { r = 0, b = 0 },
            auto_width = true,
        },
        widgets.FilteredList{
            view_id = 'list',
            not_found_label = 'No matching buildings',
            frame = { l = 0, r = 0, t = 4, b = 2 },
            icon_width = 2,
            on_submit = self:callback('onSubmitItem'),
        },
        widgets.Label{
            text = { {
                key = 'SELECT', text = ': Select',
                disabled = function() return not self.subviews.list:canSubmit() end
            } },
            frame = { l = 0, b = 0 },
        }
    }
    self:initBuiltinMode()
end

function BuildingDialog:getWantedFrameSize(rect)
    return math.max(self.frame_width or 40, #self.prompt), math.min(28, rect.height-8)
end

function BuildingDialog:onDestroy()
    if self.on_close then
        self.on_close()
    end
end

function BuildingDialog:initBuiltinMode()
    local choices = {}
    if not self.hide_none then
        table.insert(choices, { text = self.none_caption, type_id = -1, subtype_id = -1, custom_id=-1})
    end

    if self.use_workshops then
        table.insert(choices, {
            icon = ARROW, text = 'workshop', key = 'CUSTOM_SHIFT_W',
            cb = self:callback('initWorkshopMode')
        })
    end
    if self.use_furnace then
        table.insert(choices, {
            icon = ARROW, text = 'furnaces', key = 'CUSTOM_SHIFT_F',
            cb = self:callback('initFurnaceMode')
        })
    end
    if self.use_trap then
        table.insert(choices, {
            icon = ARROW, text = 'traps', key = 'CUSTOM_SHIFT_T',
            cb = self:callback('initTrapMode')
        })
    end
    if self.use_construction then
        table.insert(choices, {
            icon = ARROW, text = 'constructions', key = 'CUSTOM_SHIFT_C',
            cb = self:callback('initConstructionMode')
        })
    end
    if self.use_siege then
        table.insert(choices, {
            icon = ARROW, text = 'siege engine', key = 'CUSTOM_SHIFT_S',
            cb = self:callback('initSiegeMode')
        })
    end
    if self.use_custom then
        table.insert(choices, {
            icon = ARROW, text = 'custom workshop', key = 'CUSTOM_SHIFT_U',
            cb = self:callback('initCustomMode')
        })
    end
    
    
   
    for i=0,df.building_type._last_item do
        if (not WORKSHOP_ABSTRACT[i] or self.use_abstract)and not WORKSHOP_SPECIAL[i]  then
            self:addBuilding(choices, df.building_type[i], i, -1,-1,nil)
        end
    end

    self:pushContext('Any building', choices)
end

function BuildingDialog:initWorkshopMode()
    local choices = {}

    for i=0,df.workshop_type._last_item do
        if i~=df.workshop_type.Custom and (i~=df.workshop_type.Tool or self.use_tool_workshop) then
            self:addBuilding(choices, df.workshop_type[i], df.building_type.Workshop, i,-1,nil)
        end
    end

    self:pushContext('Workshops', choices)
end
function BuildingDialog:initTrapMode()
    local choices = {}

    for i=0,df.trap_type._last_item do
        self:addBuilding(choices, df.trap_type[i], df.building_type.Trap, i,-1,nil)
    end

    self:pushContext('Traps', choices)
end

function BuildingDialog:initConstructionMode()
    local choices = {}

    for i=0,df.construction_type._last_item do
        self:addBuilding(choices, df.construction_type[i], df.building_type.Construction, i,-1,nil)
    end

    self:pushContext('Constructions', choices)
end

function BuildingDialog:initFurnaceMode()
    local choices = {}

    for i=0,df.furnace_type._last_item do
        self:addBuilding(choices, df.furnace_type[i], df.building_type.Furnace, i,-1,nil)
    end

    self:pushContext('Furnaces', choices)
end

function BuildingDialog:initSiegeMode()
    local choices = {}

    for i=0,df.siegeengine_type._last_item do
        self:addBuilding(choices, df.siegeengine_type[i], df.building_type.SiegeEngine, i,-1,nil)
    end

    self:pushContext('Siege weapons', choices)
end
function BuildingDialog:initCustomMode()
    local choices = {}
    local raws=df.global.world.raws.buildings.all
    for k,v in pairs(raws) do
        self:addBuilding(choices, v.name, df.building_type.Workshop,df.workshop_type.Custom,v.id,v)
    end

    self:pushContext('Custom workshops', choices)
end

function BuildingDialog:addBuilding(choices, name,type_id, subtype_id, custom_id, parent)
    -- Check the filter
    if self.building_filter and not self.building_filter(name,type_id,subtype_id,custom_id, parent) then
        return
    end

    table.insert(choices, {
        text = name:lower(),
        customshop = parent,
        type_id = type_id, subtype_id = subtype_id, custom_id=custom_id
    })
end

function BuildingDialog:pushContext(name, choices)
    if not self.back_stack then
        self.back_stack = {}
        self.subviews.back.visible = false
    else
        table.insert(self.back_stack, {
            context_str = self.context_str,
            all_choices = self.subviews.list:getChoices(),
            edit_text = self.subviews.list:getFilter(),
            selected = self.subviews.list:getSelected(),
        })
        self.subviews.back.visible = true
    end

    self.context_str = name
    self.subviews.list:setChoices(choices, 1)
end

function BuildingDialog:onGoBack()
    local save = table.remove(self.back_stack)
    self.subviews.back.visible = (#self.back_stack > 0)

    self.context_str = save.context_str
    self.subviews.list:setChoices(save.all_choices)
    self.subviews.list:setFilter(save.edit_text, save.selected)
end

function BuildingDialog:submitBuilding(type_id,subtype_id,custom_id,choice,index)
    self:dismiss()

    if self.on_select then
        self.on_select(type_id,subtype_id,custom_id,choice,index)
    end
end

function BuildingDialog:onSubmitItem(idx, item)
    if item.cb then
        item:cb(idx)
    else
        self:submitBuilding(item.type_id, item.subtype_id,item.custom_id,item,idx)
    end
end

function BuildingDialog:onInput(keys)
    if keys.LEAVESCREEN or keys.LEAVESCREEN_ALL then
        if self.subviews.back.visible and not keys.LEAVESCREEN_ALL then
            self:onGoBack()
        else
            self:dismiss()
            if self.on_cancel then
                self.on_cancel()
            end
        end
    else
        self:inputToSubviews(keys)
    end
end

function showBuildingPrompt(title, prompt, on_select, on_cancel, build_filter)
    BuildingDialog{
        frame_title = title,
        prompt = prompt,
        building_filter = build_filter,
        on_select = on_select,
        on_cancel = on_cancel,
    }:show()
end

return _ENV
