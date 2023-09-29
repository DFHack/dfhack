local _ENV = mkmodule('plugins.buildingplan.inspectoroverlay')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

reset_inspector_flag = false

local function get_building_filters()
    local bld = dfhack.gui.getSelectedBuilding()
    return dfhack.buildings.getFiltersByType({},
            bld:getType(), bld:getSubtype(), bld:getCustomType())
end

--------------------------------
-- InspectorLine
--

InspectorLine = defclass(InspectorLine, widgets.Panel)
InspectorLine.ATTRS{
    idx=DEFAULT_NIL,
}

function InspectorLine:init()
    self.frame.h = 2
    self.visible = function() return #get_building_filters() >= self.idx end
    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text={{text=self:callback('get_desc_string')}},
        },
        widgets.Label{
            frame={t=1, l=2},
            text={{text=self:callback('get_status_line')}},
        },
    }
end

function InspectorLine:get_desc_string()
    if self.desc then return self.desc end
    self.desc = require('plugins.buildingplan').getDescString(dfhack.gui.getSelectedBuilding(), self.idx-1)
    return self.desc
end

function InspectorLine:get_status_line()
    if self.status then return self.status end
    local queue_pos = require('plugins.buildingplan').getQueuePosition(dfhack.gui.getSelectedBuilding(), self.idx-1)
    if queue_pos <= 0 then
        return 'Item attached'
    end
    self.status = ('Position in line: %d'):format(queue_pos)
    return self.status
end

function InspectorLine:reset()
    self.desc = nil
    self.status = nil
end

--------------------------------
-- InspectorOverlay
--

InspectorOverlay = defclass(InspectorOverlay, overlay.OverlayWidget)
InspectorOverlay.ATTRS{
    default_pos={x=-41,y=14},
    default_enabled=true,
    viewscreens='dwarfmode/ViewSheets/BUILDING',
    frame={w=30, h=15},
    frame_style=gui.MEDIUM_FRAME,
    frame_background=gui.CLEAR_PEN,
}

function InspectorOverlay:init()
    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text='Waiting for items:',
        },
        InspectorLine{view_id='item1', frame={t=2, l=0}, idx=1},
        InspectorLine{view_id='item2', frame={t=4, l=0}, idx=2},
        InspectorLine{view_id='item3', frame={t=6, l=0}, idx=3},
        InspectorLine{view_id='item4', frame={t=8, l=0}, idx=4},
        widgets.HotkeyLabel{
            frame={t=11, l=0},
            label='adjust filters',
            key='CUSTOM_CTRL_F',
            visible=false, -- until implemented
        },
        widgets.HotkeyLabel{
            frame={t=12, l=0},
            label='make top priority',
            key='CUSTOM_CTRL_T',
            on_activate=self:callback('make_top_priority'),
        },
    }
end

function InspectorOverlay:reset()
    self.subviews.item1:reset()
    self.subviews.item2:reset()
    self.subviews.item3:reset()
    self.subviews.item4:reset()
    reset_inspector_flag = false
end

function InspectorOverlay:make_top_priority()
    require('plugins.buildingplan').makeTopPriority(dfhack.gui.getSelectedBuilding())
    self:reset()
end

local RESUME_BUTTON_FRAME = {t=15, h=3, r=73, w=25}

local function mouse_is_over_resume_button(rect)
    local x,y = dfhack.screen.getMousePos()
    if not x then return false end
    if y < RESUME_BUTTON_FRAME.t or y > RESUME_BUTTON_FRAME.t + RESUME_BUTTON_FRAME.h - 1 then
        return false
    end
    if x > rect.x2 - RESUME_BUTTON_FRAME.r + 1 or x < rect.x2 - RESUME_BUTTON_FRAME.r - RESUME_BUTTON_FRAME.w + 2 then
        return false
    end
    return true
end

function InspectorOverlay:onInput(keys)
    if not require('plugins.buildingplan').isPlannedBuilding(dfhack.gui.getSelectedBuilding(true)) then
        return false
    end
    if keys._MOUSE_L and mouse_is_over_resume_button(self.frame_parent_rect) then
        return true
    elseif keys._MOUSE_L or keys._MOUSE_R or keys.LEAVESCREEN then
        self:reset()
    end
    return InspectorOverlay.super.onInput(self, keys)
end

function InspectorOverlay:render(dc)
    if not require('plugins.buildingplan').isPlannedBuilding(dfhack.gui.getSelectedBuilding(true)) then
        return
    end
    if reset_inspector_flag then
        self:reset()
    end
    InspectorOverlay.super.render(self, dc)
end

return _ENV
