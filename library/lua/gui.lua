-- Viewscreen implementation utility collection.

local _ENV = mkmodule('gui')

local dscreen = dfhack.screen

Screen = defclass(Screen, dfhack.screen)

function Screen.new(attrs)
    return mkinstance(Screen, attrs)
end

function Screen:isShown()
    return self._native ~= nil
end

function Screen:isActive()
    return self:isShown() and not self:isDismissed()
end

function Screen:renderParent()
    if self._native and self._native.parent then
        self._native.parent:render()
    else
        dscreen.clear()
    end
end

function paint_border(x1,y1,x2,y2,color,title)
    local pen = { ch = ' ', fg = COLOR_BLACK, bg = color }
    dscreen.fillRect(pen,x1,y1,x2,y1)
    dscreen.fillRect(pen,x1,y2,x2,y2)
    dscreen.fillRect(pen,x1,y1+1,x1,y2-1)
    dscreen.fillRect(pen,x2,y1+1,x2,y2-1)

    if title then
        local x = math.floor((x2-x1-3-#title)/2 + x1)
        pen.bg = bit32.bxor(pen.bg, 8)
        dscreen.paintString(pen, x, y1, '  '..title..'  ')
    end
end

function Screen:renderFrame(color,title,width,height)
    local sw, sh = dscreen.getWindowSize()
    local iw, ih = sw-2, sh-2
    width = math.min(width or iw, iw)
    height = math.min(height or ih, ih)
    local gw, gh = iw-width, ih-height
    local x1, y1 = math.floor(gw/2), math.floor(gh/2)
    local x2, y2 = x1+width+1, y1+height+1

    if gw == 0 and gh == 0 then
        dscreen.clear()
    else
        self:renderParent()
        dscreen.fillRect({ch=' ',fg=0,bg=0},x1,y1,x2,y2)
    end

    paint_border(x1,y1,x2,y2,color,title)
    return x1+1,y1+1,width,height,x2-1,y2-1
end

return _ENV
