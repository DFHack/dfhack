-- Viewscreen implementation utility collection.

local _ENV = mkmodule('gui')

local dscreen = dfhack.screen

CLEAR_PEN = {ch=32,fg=0,bg=0}

function simulateInput(screen,...)
    local keys = {}
    local function push_key(arg)
        local kv = arg
        if type(arg) == 'string' then
            kv = df.interface_key[arg]
            if kv == nil then
                error('Invalid keycode: '..arg)
            end
        end
        if type(arg) == 'number' then
            keys[#keys+1] = kv
        end
    end
    for i = 1,select('#',...) do
        local arg = select(i,...)
        if arg ~= nil then
            local t = type(arg)
            if type(arg) == 'table' then
                for k,v in pairs(arg) do
                    if v == true then
                        push_key(k)
                    else
                        push_key(v)
                    end
                end
            else
                push_key(arg)
            end
        end
    end
    dscreen._doSimulateInput(screen, keys)
end

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

function Screen:inputToParent(...)
    if self._native and self._native.parent then
        simulateInput(self._native.parent, ...)
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

local function hint_coord(gap,hint)
    if hint and hint > 0 then
        return math.min(hint,gap)
    elseif hint and hint < 0 then
        return math.max(0,gap-hint)
    else
        return math.floor(gap/2)
    end
end

function mkdims_xy(x1,y1,x2,y2)
    return { x1=x1, y1=y1, x2=x2, y2=y2, width=x2-x1+1, height=y2-y1+1 }
end
function mkdims_wh(x1,y1,w,h)
    return { x1=x1, y1=y1, x2=x1+w-1, y2=y1+h-1, width=w, height=h }
end

function Screen:renderFrame(color,title,width,height,xhint,yhint)
    local sw, sh = dscreen.getWindowSize()
    local iw, ih = sw-2, sh-2
    width = math.min(width or iw, iw)
    height = math.min(height or ih, ih)
    local gw, gh = iw-width, ih-height
    local x1, y1 = hint_coord(gw,xhint), hint_coord(gh,yhint)
    local x2, y2 = x1+width+1, y1+height+1

    if gw == 0 and gh == 0 then
        dscreen.clear()
    else
        self:renderParent()
        dscreen.fillRect(CLEAR_PEN,x1,y1,x2,y2)
    end

    paint_border(x1,y1,x2,y2,color,title)
    return x1+1,y1+1,width,height,x2-1,y2-1
end

return _ENV
