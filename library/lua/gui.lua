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

function mkdims_xy(x1,y1,x2,y2)
    return { x1=x1, y1=y1, x2=x2, y2=y2, width=x2-x1+1, height=y2-y1+1 }
end
function mkdims_wh(x1,y1,w,h)
    return { x1=x1, y1=y1, x2=x1+w-1, y2=y1+h-1, width=w, height=h }
end
function inset(rect,dx1,dy1,dx2,dy2)
    return mkdims_xy(
        rect.x1+dx1, rect.y1+dy1,
        rect.x2-(dx2 or dx1), rect.y2-(dy2 or dy1)
    )
end

function to_pen(pen, default)
    if pen == nil then
        return default or {}
    elseif type(pen) ~= 'table' then
        return {fg=pen}
    else
        return pen
    end
end

----------------------------
-- Clipped painter object --
----------------------------

Painter = defclass(Painter, nil)

function Painter.new(rect, pen)
    rect = rect or {}
    local sw, sh = dscreen.getWindowSize()
    local self = mkdims_xy(
        math.max(rect.x1 or 0, 0),
        math.max(rect.y1 or 0, 0),
        math.min(rect.x2 or sw-1, sw-1),
        math.min(rect.y2 or sh-1, sh-1)
    )
    self.cur_pen = to_pen(pen or COLOR_GREY)
    self.x = self.x1
    self.y = self.y1
    return mkinstance(Painter, self)
end

function Painter:setRect(x1,y1,x2,y2)
    local rect = mkdims_xy(x1,y1,x2,y2)
    self.x1 = rect.x1
    self.y1 = rect.y1
    self.x2 = rect.x2
    self.y2 = rect.y2
    self.width = rect.width
    self.height = rect.height
end

function Painter:isValidPos()
    return self.x >= self.x1 and self.x <= self.x2 and self.y >= self.y1 and self.y <= self.y2
end

function Painter:inset(dx1,dy1,dx2,dy1)
    self:setRect(
        self.x1+dx1, self.y1+dy1,
        self.x2-(dx2 or dx1), self.y2-(dy2 or dy1)
    )
    return self
end

function Painter:seek(x,y)
    if x then self.x = self.x1 + x end
    if y then self.y = self.y1 + y end
    return self
end

function Painter:advance(dx,dy)
    if dx then self.x = self.x + dx end
    if dy then self.y = self.y + dy end
    return self
end

function Painter:newline(dx)
    self.y = self.y + 1
    self.x = self.x1 + (dx or 0)
    return self
end

function Painter:pen(pen)
    self.cur_pen = to_pen(pen, self.cur_pen)
    return self
end

function Painter:clear()
    dscreen.fillRect(CLEAR_PEN, self.x1, self.y1, self.x2, self.y2)
    return self
end

function Painter:fill(x1,y1,x2,y2,pen)
    if type(x1) == 'table' then
        x1, y1, x2, y2, pen = x1.x1, x1.y1, x1.x2, x1.y2, x2
    end
    x1 = math.max(x1,self.x1)
    y1 = math.max(y1,self.y1)
    x2 = math.min(x2,self.x2)
    y2 = math.min(y2,self.y2)
    dscreen.fillRect(to_pen(pen, self.cur_pen),x1,y1,x2,y2)
    return self
end

function Painter:char(char,pen)
    if self:isValidPos() then
        dscreen.paintTile(to_pen(pen, self.cur_pen), self.x, self.y, char)
    end
    return self:advance(1, nil)
end

function Painter:tile(char,tile,pen)
    if self:isValidPos() then
        dscreen.paintTile(to_pen(pen, self.cur_pen), self.x, self.y, char, tile)
    end
    return self:advance(1, nil)
end

function Painter:string(text,pen)
    if self.y >= self.y1 and self.y <= self.y2 then
        local dx = 0
        if self.x < self.x1 then
            dx = self.x1 - self.x
        end
        local len = #text
        if self.x + len - 1 > self.x2 then
            len = self.x2 - self.x + 1
        end
        if len > dx then
            dscreen.paintString(
                to_pen(pen, self.cur_pen),
                self.x+dx, self.y,
                string.sub(text,dx+1,len)
            )
        end
    end
    return self:advance(#text, nil)
end

------------------------
-- Base screen object --
------------------------

Screen = defclass(Screen, dfhack.screen)

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

function Screen:show()
    if self._native then
        error("This screen is already on display")
    end
    self:onAboutToShow()
    if dscreen.show(self) then
        self:onShown()
    end
end

function Screen:onAboutToShow()
end

function Screen:onShown()
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

    return Painter.new(mkdims_wh(x1+1,y1+1,width,height))
end

return _ENV
