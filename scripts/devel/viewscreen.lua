-- Test lua viewscreens.

local gui = require 'gui'

local screen = mkinstance(gui.Screen, {
    onRender = function(self)
        local text = 'Woohoo, lua viewscreen :)'
        local dc = self:renderFrame(COLOR_GREY,'Hello World',#text+6,3)
        dc:seek(3,1):string(text, {fg=COLOR_LIGHTGREEN})
    end,
    onInput = function(self,keys)
        if keys and (keys.LEAVESCREEN or keys.SELECT) then
            self:dismiss()
        end
    end
})

screen:show()
