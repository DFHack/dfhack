-- View whether tiles on the map can be pathed to
--@module=true

local gui = require('gui')
local plugin = require('plugins.pathable')
local widgets = require('gui.widgets')

Pathable = defclass(Pathable, gui.ZScreen)
Pathable.ATTRS{
    focus_path='pathable',
}

function Pathable:init()
    local window = widgets.Window{
        view_id='main',
        frame={t=20, r=3, w=32, h=11},
        frame_title='Pathability Viewer',
        drag_anchors={title=true, frame=true, body=true},
    }

    window:addviews{
        widgets.ToggleHotkeyLabel{
            view_id='lock',
            frame={t=0, l=0},
            key='CUSTOM_CTRL_T',
            label='Lock target',
            initial_option=false,
        },
        widgets.ToggleHotkeyLabel{
            view_id='draw',
            frame={t=1, l=0},
            key='CUSTOM_CTRL_D',
            label='Draw',
            initial_option=true,
        },
        widgets.ToggleHotkeyLabel{
            view_id='show',
            frame={t=2, l=0},
            key='CUSTOM_CTRL_U',
            label='Show hidden',
            initial_option=false,
        },
        widgets.EditField{
            view_id='group',
            frame={t=4, l=0},
            label_text='Pathability group: ',
            active=false,
        },
        widgets.HotkeyLabel{
            frame={t=6, l=0},
            key='LEAVESCREEN',
            label='Close',
            on_activate=self:callback('dismiss'),
        },
    }

    self:addviews{window}
end

function Pathable:onRenderBody()
    local target = self.subviews.lock:getOptionValue() and
            self.saved_target or dfhack.gui.getMousePos()
    self.saved_target = target

    local group = self.subviews.group
    local show = self.subviews.show:getOptionValue()

    if not target then
        group:setText('')
        return
    elseif not show and not dfhack.maps.isTileVisible(target) then
        group:setText('Hidden')
        return
    end

    local block = dfhack.maps.getTileBlock(target)
    local walk_group = block and block.walkable[target.x % 16][target.y % 16] or 0
    group:setText(walk_group == 0 and 'None' or tostring(walk_group))

    if self.subviews.draw:getOptionValue() then
        plugin.paintScreenPathable(target, show)
    end
end

function Pathable:onDismiss()
    view = nil
end

if dfhack_flags.module then
    return
end

if not dfhack.isMapLoaded() then
    qerror('gui/pathable requires a map to be loaded')
end

view = view and view:raise() or Pathable{}:show()
