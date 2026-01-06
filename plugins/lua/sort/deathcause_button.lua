local _ENV = mkmodule('plugins.sort.deathcause_button')

local dialogs = require('gui.dialogs')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

DeathCauseOverlay = defclass(DeathCauseOverlay, overlay.OverlayWidget)
DeathCauseOverlay.ATTRS{
    desc='Adds a button to view death cause on the dead/missing tab.',
    default_pos={x=50, y=-7},
    default_enabled=true,
    viewscreens='dwarfmode/Info/CREATURES/DECEASED',
    frame={w=21, h=1},
}

function DeathCauseOverlay:init()
    local deathcause = reqscript('deathcause')

    local function get_selected_unit()
        -- Navigate to the creatures/deceased widget hierarchy:
        -- list_widget - the main deceased list
        --   ├─ children[0]: scrollbar widget
        --   └─ children[1]: container widget (list_container)
        --       ├─ grandchildren[0]: header
        --       ├─ grandchildren[1]: header or other UI
        --       └─ grandchildren[2]: scrollable rows container (scrollable_list)
        --           └─ rows: row widgets (each row = one unit in the list)
        --               └─ row.children[x]: unit widget
        --                   └─ unit_widget.u: pointer to the df.unit object

        local creatures = df.global.game.main_interface.info.creatures
        local list_widget = dfhack.gui.getWidget(creatures, 'Tabs', 'Dead/Missing')
        if not list_widget then return nil end
        
        local children = dfhack.gui.getWidgetChildren(list_widget)
        local list_container = children[1]
        local grandchildren = dfhack.gui.getWidgetChildren(list_container)
        
        local scrollable_list = grandchildren[2]
        if not scrollable_list then
            return nil
        end
        
        local rows = dfhack.gui.getWidgetChildren(scrollable_list)
        
        local cursor_idx = list_widget.cursor_idx or 0
        
        if cursor_idx >= 0 and cursor_idx < #rows then
            local row = rows[cursor_idx + 1]

            local ok, unit = pcall(function() return dfhack.gui.getWidget(row, 0).u end)
            if ok and unit then
                return unit
            end
        end

        return nil
    end

    self:addviews{
        widgets.TextButton{
            frame={t=0, l=0},
            label='Show death cause',
            key='CUSTOM_D',
            on_activate=function()
                local unit = get_selected_unit()
                if not unit then
                    dialogs.showMessage('Death Cause', 'No unit selected.')
                    return
                end
                local cause = deathcause.getDeathCause(unit)
                dialogs.showMessage('Death Cause', dfhack.df2console(cause))
            end,
        },
    }
end

return _ENV
