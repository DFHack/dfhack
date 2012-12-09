-- Viewscreen implementation utility collection.

local _ENV = mkmodule('gui')

local dscreen = dfhack.screen

USE_GRAPHICS = dscreen.inGraphicsMode()

local to_pen = dfhack.pen.parse

CLEAR_PEN = to_pen{ch=32,fg=0,bg=0}

local FAKE_INPUT_KEYS = {
    _MOUSE_L = true,
    _MOUSE_R = true,
    _MOUSE_L_DOWN = true,
    _MOUSE_R_DOWN = true,
    _STRING = true,
}

function simulateInput(screen,...)
    local keys = {}
    local function push_key(arg)
        local kv = arg
        if type(arg) == 'string' then
            kv = df.interface_key[arg]
            if kv == nil and not FAKE_INPUT_KEYS[arg] then
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
function is_in_rect(rect,x,y)
    return x and y and x >= rect.x1 and x <= rect.x2 and y >= rect.y1 and y <= rect.y2
end

local function align_coord(gap,align,lv,rv)
    if gap <= 0 then
        return 0
    end
    if not align then
        if rv and not lv then
            align = 1.0
        elseif lv and not rv then
            align = 0.0
        else
            align = 0.5
        end
    end
    return math.floor(gap*align)
end

function compute_frame_rect(wavail,havail,spec,xgap,ygap)
    if not spec then
        return mkdims_wh(0,0,wavail,havail)
    end

    local sw = wavail - (spec.l or 0) - (spec.r or 0)
    local sh = havail - (spec.t or 0) - (spec.b or 0)
    local rqw = math.min(sw, (spec.w or sw)+xgap)
    local rqh = math.min(sh, (spec.h or sh)+ygap)
    local ax = align_coord(sw - rqw, spec.xalign, spec.l, spec.r)
    local ay = align_coord(sh - rqh, spec.yalign, spec.t, spec.b)

    local rect = mkdims_wh((spec.l or 0) + ax, (spec.t or 0) + ay, rqw, rqh)
    rect.wgap = sw - rqw
    rect.hgap = sh - rqh
    return rect
end

local function parse_inset(inset)
    local l,r,t,b
    if type(inset) == 'table' then
        l,r = inset.l or inset.x, inset.r or inset.x
        t,b = inset.t or inset.y, inset.b or inset.y
    else
        l = inset or 0
        t,r,b = l,l,l
    end
    return l,r,t,b
end

function inset_frame(rect, inset, gap)
    gap = gap or 0
    local l,t,r,b = parse_inset(inset)
    return mkdims_xy(rect.x1+l+gap, rect.y1+t+gap, rect.x2-r-gap, rect.y2-b-gap)
end

function compute_frame_body(wavail, havail, spec, inset, gap, inner_frame)
    gap = gap or 0
    local l,t,r,b = parse_inset(inset)
    local xgap,ygap = 0,0
    if inner_frame then
        xgap,ygap = gap*2+l+r, gap*2+t+b
    end
    local rect = compute_frame_rect(wavail, havail, spec, xgap, ygap)
    local body = mkdims_xy(rect.x1+l+gap, rect.y1+t+gap, rect.x2-r-gap, rect.y2-b-gap)
    return rect, body
end

function blink_visible(delay)
    return math.floor(dfhack.getTickCount()/delay) % 2 == 0
end

function getKeyDisplay(code)
    if type(code) == 'string' then
        code = df.interface_key[code]
    end
    return dscreen.getKeyDisplay(code)
end

-----------------------------------
-- Clipped view rectangle object --
-----------------------------------

ViewRect = defclass(ViewRect, nil)

function ViewRect:init(args)
    if args.view_rect then
        self:assign(args.view_rect)
    else
        local rect = args.rect or mkdims_wh(0,0,dscreen.getWindowSize())
        local crect = args.clip_rect or rect
        self:assign{
            x1 = rect.x1, clip_x1 = crect.x1,
            y1 = rect.y1, clip_y1 = crect.y1,
            x2 = rect.x2, clip_x2 = crect.x2,
            y2 = rect.y2, clip_y2 = crect.y2,
            width = rect.x2-rect.x1+1,
            height = rect.y2-rect.y1+1,
        }
    end
    if args.clip_view then
        local cr = args.clip_view
        self:assign{
            clip_x1 = math.max(self.clip_x1, cr.clip_x1),
            clip_y1 = math.max(self.clip_y1, cr.clip_y1),
            clip_x2 = math.min(self.clip_x2, cr.clip_x2),
            clip_y2 = math.min(self.clip_y2, cr.clip_y2),
        }
    end
end

function ViewRect:isDefunct()
    return (self.clip_x1 > self.clip_x2 or self.clip_y1 > self.clip_y2)
end

function ViewRect:inClipGlobalXY(x,y)
    return x >= self.clip_x1 and x <= self.clip_x2
       and y >= self.clip_y1 and y <= self.clip_y2
end

function ViewRect:inClipLocalXY(x,y)
    return (x+self.x1) >= self.clip_x1 and (x+self.x1) <= self.clip_x2
       and (y+self.y1) >= self.clip_y1 and (y+self.y1) <= self.clip_y2
end

function ViewRect:localXY(x,y)
    return x-self.x1, y-self.y1
end

function ViewRect:globalXY(x,y)
    return x+self.x1, y+self.y1
end

function ViewRect:viewport(x,y,w,h)
    if type(x) == 'table' then
        x,y,w,h = x.x1, x.y1, x.width, x.height
    end
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
    }
    return mkinstance(ViewRect, vp)
end

----------------------------
-- Clipped painter object --
----------------------------

Painter = defclass(Painter, ViewRect)

function Painter:init(args)
    self.x = self.x1
    self.y = self.y1
    self.cur_pen = to_pen(args.pen or COLOR_GREY)
    self.cur_key_pen = to_pen(args.key_pen or COLOR_LIGHTGREEN)
end

function Painter.new(rect, pen)
    return Painter{ rect = rect, pen = pen }
end

function Painter.new_view(view_rect, pen)
    return Painter{ view_rect = view_rect, pen = pen }
end

function Painter.new_xy(x1,y1,x2,y2,pen)
    return Painter{ rect = mkdims_xy(x1,y1,x2,y2), pen = pen }
end

function Painter.new_wh(x,y,w,h,pen)
    return Painter{ rect = mkdims_wh(x,y,w,h), pen = pen }
end

function Painter:isValidPos()
    return self:inClipGlobalXY(self.x, self.y)
end

function Painter:viewport(x,y,w,h)
    local vp = ViewRect.viewport(x,y,w,h)
    vp.cur_pen = self.cur_pen
    vp.cur_key_pen = self.cur_key_pen
    return mkinstance(Painter, vp):seek(0,0)
end

function Painter:cursor()
    return self.x - self.x1, self.y - self.y1
end

function Painter:cursorX()
    return self.x - self.x1
end

function Painter:cursorY()
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
    self.cur_pen = to_pen(self.cur_pen, fg, bg, bold)
    return self
end

function Painter:key_pen(pen,...)
    self.cur_key_pen = to_pen(self.cur_key_pen, pen, ...)
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
    x1 = math.max(x1+self.x1,self.clip_x1)
    y1 = math.max(y1+self.y1,self.clip_y1)
    x2 = math.min(x2+self.x1,self.clip_x2)
    y2 = math.min(y2+self.y1,self.clip_y2)
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

function Painter:key(code,pen,...)
    return self:string(
        getKeyDisplay(code),
        to_pen(self.cur_key_pen, pen, ...)
    )
end

--------------------------
-- Abstract view object --
--------------------------

View = defclass(View)

View.ATTRS {
    active = true,
    visible = true,
    view_id = DEFAULT_NIL,
}

function View:init(args)
    self.subviews = {}
end

function View:addviews(list)
    if not list then return end

    local sv = self.subviews

    for _,obj in ipairs(list) do
        table.insert(sv, obj)

        local id = obj.view_id
        if id and type(id) ~= 'number' and sv[id] == nil then
            sv[id] = obj
        end
    end

    for _,dir in ipairs(list) do
        for id,obj in pairs(dir.subviews) do
            if id and type(id) ~= 'number' and sv[id] == nil then
                sv[id] = obj
            end
        end
    end
end

function View:getWindowSize()
    local rect = self.frame_body
    return rect.width, rect.height
end

function View:getMousePos()
    local rect = self.frame_body
    local x,y = dscreen.getMousePos()
    if rect and rect:inClipGlobalXY(x,y) then
        return rect:localXY(x,y)
    end
end

function View:computeFrame(parent_rect)
    return mkdims_wh(0,0,parent_rect.width,parent_rect.height)
end

function View:updateSubviewLayout(frame_body)
    for _,child in ipairs(self.subviews) do
        child:updateLayout(frame_body)
    end
end

function View:updateLayout(parent_rect)
    if not parent_rect then
        parent_rect = self.frame_parent_rect
    else
        self.frame_parent_rect = parent_rect
    end

    self:invoke_before('preUpdateLayout', parent_rect)

    local frame_rect,body_rect = self:computeFrame(parent_rect)

    self.frame_rect = frame_rect
    self.frame_body = parent_rect:viewport(body_rect or frame_rect)

    self:invoke_after('postComputeFrame', self.frame_body)

    self:updateSubviewLayout(self.frame_body)

    self:invoke_after('postUpdateLayout', self.frame_body)
end

function View:renderSubviews(dc)
    for _,child in ipairs(self.subviews) do
        if child.visible then
            child:render(dc)
        end
    end
end

function View:render(dc)
    self:onRenderFrame(dc, self.frame_rect)

    local sub_dc = Painter{
        view_rect = self.frame_body,
        clip_view = dc
    }

    self:onRenderBody(sub_dc)

    self:renderSubviews(sub_dc)
end

function View:onRenderFrame(dc,rect)
end

function View:onRenderBody(dc)
end

function View:inputToSubviews(keys)
    local children = self.subviews

    for i=#children,1,-1 do
        local child = children[i]
        if child.visible and child.active and child:onInput(keys) then
            return true
        end
    end

    return false
end

function View:onInput(keys)
    return self:inputToSubviews(keys)
end

------------------------
-- Base screen object --
------------------------

Screen = defclass(Screen, View)

Screen.text_input_mode = false

function Screen:postinit()
    self:onResize(dscreen.getWindowSize())
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

function Screen:show(parent)
    if self._native then
        error("This screen is already on display")
    end
    parent = parent or dfhack.gui.getCurViewscreen(true)
    self:onAboutToShow(parent)
    if not dscreen.show(self, parent.child) then
        error('Could not show screen')
    end
end

function Screen:onAboutToShow(parent)
end

function Screen:onShow()
    self:onResize(dscreen.getWindowSize())
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
    self:updateLayout(ViewRect{ rect = mkdims_wh(0,0,w,h) })
end

function Screen:onRender()
    self:render(Painter.new())
end

------------------------
-- Framed screen object --
------------------------

-- Plain grey-colored frame.
GREY_FRAME = {
    frame_pen = to_pen{ ch = ' ', fg = COLOR_BLACK, bg = COLOR_GREY },
    title_pen = to_pen{ fg = COLOR_BLACK, bg = COLOR_WHITE },
    signature_pen = to_pen{ fg = COLOR_BLACK, bg = COLOR_GREY },
}

-- The usual boundary used by the DF screens. Often has fancy pattern in tilesets.
BOUNDARY_FRAME = {
    frame_pen = to_pen{ ch = 0xDB, fg = COLOR_DARKGREY, bg = COLOR_BLACK },
    title_pen = to_pen{ fg = COLOR_BLACK, bg = COLOR_GREY },
    signature_pen = to_pen{ fg = COLOR_BLACK, bg = COLOR_DARKGREY },
}

GREY_LINE_FRAME = {
    frame_pen = to_pen{ ch = 206, fg = COLOR_GREY, bg = COLOR_BLACK },
    h_frame_pen = to_pen{ ch = 205, fg = COLOR_GREY, bg = COLOR_BLACK },
    v_frame_pen = to_pen{ ch = 186, fg = COLOR_GREY, bg = COLOR_BLACK },
    lt_frame_pen = to_pen{ ch = 201, fg = COLOR_GREY, bg = COLOR_BLACK },
    lb_frame_pen = to_pen{ ch = 200, fg = COLOR_GREY, bg = COLOR_BLACK },
    rt_frame_pen = to_pen{ ch = 187, fg = COLOR_GREY, bg = COLOR_BLACK },
    rb_frame_pen = to_pen{ ch = 188, fg = COLOR_GREY, bg = COLOR_BLACK },
    title_pen = to_pen{ fg = COLOR_BLACK, bg = COLOR_GREY },
    signature_pen = to_pen{ fg = COLOR_DARKGREY, bg = COLOR_BLACK },
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

FramedScreen.ATTRS{
    frame_style = BOUNDARY_FRAME,
    frame_title = DEFAULT_NIL,
    frame_width = DEFAULT_NIL,
    frame_height = DEFAULT_NIL,
    frame_inset = 0,
    frame_background = CLEAR_PEN,
}

function FramedScreen:getWantedFrameSize()
    return self.frame_width, self.frame_height
end

function FramedScreen:computeFrame(parent_rect)
    local sw, sh = parent_rect.width, parent_rect.height
    local fw, fh = self:getWantedFrameSize(parent_rect)
    return compute_frame_body(sw, sh, { w = fw, h = fh }, self.frame_inset, 1, true)
end

function FramedScreen:onRenderFrame(dc, rect)
    local x1,y1,x2,y2 = rect.x1, rect.y1, rect.x2, rect.y2

    if rect.wgap <= 0 and rect.hgap <= 0 then
        dc:clear()
    else
        self:renderParent()
        dc:fill(rect, self.frame_background)
    end

    paint_frame(x1,y1,x2,y2,self.frame_style,self.frame_title)
end

return _ENV
