-- Viewscreen implementation utility collection.

local _ENV = mkmodule('gui')

local dscreen = dfhack.screen

USE_GRAPHICS = dscreen.inGraphicsMode()

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
        if type(kv) == 'number' then
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
function is_in_rect(rect,x,y)
    return x and y and x >= rect.x1 and x <= rect.x2 and y >= rect.y1 and y <= rect.y2
end

local function to_pen(default, pen, bg, bold)
    if pen == nil then
        return default or {}
    elseif type(pen) ~= 'table' then
        return {fg=pen,bg=bg,bold=bold}
    else
        return pen
    end
end

----------------------------
-- Clipped painter object --
----------------------------

Painter = defclass(Painter, nil)

function Painter.new(rect, pen)
    rect = rect or mkdims_wh(0,0,dscreen.getWindowSize())
    local self = {
        x1 = rect.x1, clip_x1 = rect.x1,
        y1 = rect.y1, clip_y1 = rect.y1,
        x2 = rect.x2, clip_x2 = rect.x2,
        y2 = rect.y2, clip_y2 = rect.y2,
        width = rect.x2-rect.x1+1,
        height = rect.y2-rect.y1+1,
        cur_pen = to_pen(nil, pen or COLOR_GREY)
    }
    return mkinstance(Painter, self):seek(0,0)
end

function Painter:isValidPos()
    return self.x >= self.clip_x1 and self.x <= self.clip_x2
       and self.y >= self.clip_y1 and self.y <= self.clip_y2
end

function Painter:viewport(x,y,w,h)
    local x1,y1 = self.x1+x, self.y1+y
    local x2,y2 = x1+w-1, y1+h-1
    local vp = {
        -- Logical viewport
        x1 = x1, y1 = y1, x2 = x2, y2 = y2,
        width = w, height = h,
        -- Actual clipping rect
        clip_x1 = math.max(self.clip_x1, x1),
        clip_y1 = math.max(self.clip_y1, y1),
        clip_x2 = math.min(self.clip_x2, x2),
        clip_y2 = math.min(self.clip_y2, y2),
        -- Pen
        cur_pen = self.cur_pen
    }
    return mkinstance(Painter, vp):seek(0,0)
end

function Painter:localX()
    return self.x - self.x1
end

function Painter:localY()
    return self.y - self.y1
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

function Painter:pen(pen,...)
    self.cur_pen = to_pen(self.cur_pen, pen, ...)
    return self
end

function Painter:color(fg,bold,bg)
    self.cur_pen = copyall(self.cur_pen)
    self.cur_pen.fg = fg
    self.cur_pen.bold = bold
    if bg then self.cur_pen.bg = bg end
    return self
end

function Painter:clear()
    dscreen.fillRect(CLEAR_PEN, self.clip_x1, self.clip_y1, self.clip_x2, self.clip_y2)
    return self
end

function Painter:fill(x1,y1,x2,y2,pen,bg,bold)
    if type(x1) == 'table' then
        x1, y1, x2, y2, pen, bg, bold = x1.x1, x1.y1, x1.x2, x1.y2, y1, x2, y2
    end
    x1 = math.max(x1,self.clip_x1)
    y1 = math.max(y1,self.clip_y1)
    x2 = math.min(x2,self.clip_x2)
    y2 = math.min(y2,self.clip_y2)
    dscreen.fillRect(to_pen(self.cur_pen,pen,bg,bold),x1,y1,x2,y2)
    return self
end

function Painter:char(char,pen,...)
    if self:isValidPos() then
        dscreen.paintTile(to_pen(self.cur_pen, pen, ...), self.x, self.y, char)
    end
    return self:advance(1, nil)
end

function Painter:tile(char,tile,pen,...)
    if self:isValidPos() then
        dscreen.paintTile(to_pen(self.cur_pen, pen, ...), self.x, self.y, char, tile)
    end
    return self:advance(1, nil)
end

function Painter:string(text,pen,...)
    if self.y >= self.clip_y1 and self.y <= self.clip_y2 then
        local dx = 0
        if self.x < self.clip_x1 then
            dx = self.clip_x1 - self.x
        end
        local len = #text
        if self.x + len - 1 > self.clip_x2 then
            len = self.clip_x2 - self.x + 1
        end
        if len > dx then
            dscreen.paintString(
                to_pen(self.cur_pen, pen, ...),
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

Screen = defclass(Screen)

Screen.text_input_mode = false

function Screen:init()
    self:updateLayout()
    return self
end

Screen.isDismissed = dscreen.isDismissed

function Screen:isShown()
    return self._native ~= nil
end

function Screen:isActive()
    return self:isShown() and not self:isDismissed()
end

function Screen:invalidate()
    dscreen.invalidate()
end

function Screen:getWindowSize()
    return dscreen.getWindowSize()
end

function Screen:getMousePos()
    return dscreen.getMousePos()
end

function Screen:renderParent()
    if self._native and self._native.parent then
        self._native.parent:render()
    else
        dscreen.clear()
    end
end

function Screen:sendInputToParent(...)
    if self._native and self._native.parent then
        simulateInput(self._native.parent, ...)
    end
end

function Screen:show(below)
    if self._native then
        error("This screen is already on display")
    end
    self:onAboutToShow(below)
    if not dscreen.show(self, below) then
        error('Could not show screen')
    end
end

function Screen:onAboutToShow()
end

function Screen:onShow()
    self:updateLayout()
end

function Screen:dismiss()
    if self._native then
        dscreen.dismiss(self)
    end
end

function Screen:onDismiss()
end

function Screen:onDestroy()
end

function Screen:onResize(w,h)
    self:updateLayout()
end

function Screen:updateLayout()
end

------------------------
-- Framed screen object --
------------------------

-- Plain grey-colored frame.
GREY_FRAME = {
    frame_pen = { ch = ' ', fg = COLOR_BLACK, bg = COLOR_GREY },
    title_pen = { fg = COLOR_BLACK, bg = COLOR_WHITE },
    signature_pen = { fg = COLOR_BLACK, bg = COLOR_GREY },
}

-- The usual boundary used by the DF screens. Often has fancy pattern in tilesets.
BOUNDARY_FRAME = {
    frame_pen = { ch = 0xDB, fg = COLOR_DARKGREY, bg = COLOR_BLACK },
    title_pen = { fg = COLOR_BLACK, bg = COLOR_GREY },
    signature_pen = { fg = COLOR_BLACK, bg = COLOR_DARKGREY },
}

GREY_LINE_FRAME = {
    frame_pen = { ch = 206, fg = COLOR_GREY, bg = COLOR_BLACK },
    h_frame_pen = { ch = 205, fg = COLOR_GREY, bg = COLOR_BLACK },
    v_frame_pen = { ch = 186, fg = COLOR_GREY, bg = COLOR_BLACK },
    lt_frame_pen = { ch = 201, fg = COLOR_GREY, bg = COLOR_BLACK },
    lb_frame_pen = { ch = 200, fg = COLOR_GREY, bg = COLOR_BLACK },
    rt_frame_pen = { ch = 187, fg = COLOR_GREY, bg = COLOR_BLACK },
    rb_frame_pen = { ch = 188, fg = COLOR_GREY, bg = COLOR_BLACK },
    title_pen = { fg = COLOR_BLACK, bg = COLOR_GREY },
    signature_pen = { fg = COLOR_DARKGREY, bg = COLOR_BLACK },
}

function paint_frame(x1,y1,x2,y2,style,title)
    local pen = style.frame_pen
    dscreen.paintTile(style.lt_frame_pen or pen, x1, y1)
    dscreen.paintTile(style.rt_frame_pen or pen, x2, y1)
    dscreen.paintTile(style.lb_frame_pen or pen, x1, y2)
    dscreen.paintTile(style.rb_frame_pen or pen, x2, y2)
    dscreen.fillRect(style.t_frame_pen or style.h_frame_pen or pen,x1+1,y1,x2-1,y1)
    dscreen.fillRect(style.b_frame_pen or style.h_frame_pen or pen,x1+1,y2,x2-1,y2)
    dscreen.fillRect(style.l_frame_pen or style.v_frame_pen or pen,x1,y1+1,x1,y2-1)
    dscreen.fillRect(style.r_frame_pen or style.v_frame_pen or pen,x2,y1+1,x2,y2-1)
    dscreen.paintString(style.signature_pen or style.title_pen or pen,x2-7,y2,"DFHack")

    if title then
        local x = math.max(0,math.floor((x2-x1-3-#title)/2)) + x1
        local tstr = '  '..title..'  '
        if #tstr > x2-x1-1 then
            tstr = string.sub(tstr,1,x2-x1-1)
        end
        dscreen.paintString(style.title_pen or pen, x, y1, tstr)
    end
end

FramedScreen = defclass(FramedScreen, Screen)

FramedScreen.frame_style = BOUNDARY_FRAME

local function hint_coord(gap,hint)
    if hint and hint > 0 then
        return math.min(hint,gap)
    elseif hint and hint < 0 then
        return math.max(0,gap-hint)
    else
        return math.floor(gap/2)
    end
end

function FramedScreen:updateFrameSize()
    local sw, sh = dscreen.getWindowSize()
    local iw, ih = sw-2, sh-2
    local width = math.min(self.frame_width or iw, iw)
    local height = math.min(self.frame_height or ih, ih)
    local gw, gh = iw-width, ih-height
    local x1, y1 = hint_coord(gw,self.frame_xhint), hint_coord(gh,self.frame_yhint)
    self.frame_rect = mkdims_wh(x1+1,y1+1,width,height)
    self.frame_opaque = (gw == 0 and gh == 0)
end

function FramedScreen:updateLayout()
    self:updateFrameSize()
end

function FramedScreen:getWindowSize()
    local rect = self.frame_rect
    return rect.width, rect.height
end

function FramedScreen:getMousePos()
    local rect = self.frame_rect
    local x,y = dscreen.getMousePos()
    if is_in_rect(rect,x,y) then
        return x-rect.x1, y-rect.y1
    end
end

function FramedScreen:onRender()
    local rect = self.frame_rect
    local x1,y1,x2,y2 = rect.x1-1, rect.y1-1, rect.x2+1, rect.y2+1

    if self.frame_opaque then
        dscreen.clear()
    else
        self:renderParent()
        dscreen.fillRect(CLEAR_PEN,x1,y1,x2,y2)
    end

    paint_frame(x1,y1,x2,y2,self.frame_style,self.frame_title)

    self:onRenderBody(Painter.new(rect))
end

function FramedScreen:onRenderBody(dc)
end

return _ENV
