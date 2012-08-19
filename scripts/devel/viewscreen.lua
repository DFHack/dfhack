-- Test lua viewscreens.

local gui = require 'gui'

local screen = gui.Screen.new({
    onRender = function(self)
        local text = 'Woohoo, lua viewscreen :)'
        local x,y,w,h = self:renderFrame(COLOR_GREY,'Hello World',#text+6,3)
        self.paintString({fg=COLOR_LIGHTGREEN},x+3,y+1,text)
    end,
    onInput = function(self,keys)
        if keys and (keys.LEAVESCREEN or keys.SELECT) then
            self:dismiss()
        end
    end
})

screen:show()
