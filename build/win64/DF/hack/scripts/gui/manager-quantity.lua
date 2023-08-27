-- Sets the quantity of the selected manager job
--[====[

gui/manager-quantity
====================

Sets the quantity of the selected manager job (in the j-m or u-m screens).

]====]

local dialog = require 'gui.dialogs'
local args = {...}

function show_error(text)
    dialog.showMessage("Error", text, COLOR_LIGHTRED)
end

local scr = dfhack.gui.getCurViewscreen()
if df.viewscreen_jobmanagementst:is_instance(scr) then
    local orders = df.global.world.manager_orders
    function set_quantity(value)
        if tonumber(value) then
            value = tonumber(value)
            local i = scr.sel_idx
            local old_total = orders[i].amount_total
            orders[i].amount_total = math.max(0, value)
            orders[i].amount_left = math.max(0, orders[i].amount_left + (value - old_total))
        else
            show_error(value .. " is not a number!")
        end
    end
    if scr.sel_idx < #orders then
        if #args >= 1 then
            set_quantity(args[1])
        else
            dialog.showInputPrompt(
                "Quantity",
                "Quantity:",
                COLOR_WHITE,
                '',
                set_quantity
            )
        end
    else
        show_error("Invalid order selected")
    end
else
    dfhack.printerr('Must be called on the manager screen (j-m or u-m)')
end
