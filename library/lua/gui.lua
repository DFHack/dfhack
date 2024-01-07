-- Viewscreen implementation utility collection.

local _ENV = mkmodule('gui')

local textures = require('gui.textures')
local utils = require('utils')

local dscreen = dfhack.screen
local getval = utils.getval

local to_pen = dfhack.pen.parse

CLEAR_PEN = to_pen{tile=dfhack.internal.getAddress('init') and df.global.init.texpos_border_interior, ch=32, fg=0, bg=0, write_to_lower=true}
TRANSPARENT_PEN = to_pen{tile=0, ch=0}
KEEP_LOWER_PEN = to_pen{ch=32, fg=0, bg=0, keep_lower=true}

local function set_and_get_undo(field, is_set)
    local prev_value = df.global.enabler[field]
    df.global.enabler[field] = is_set and 1 or 0
    return function() df.global.enabler[field] = prev_value end
end

local MOUSE_KEYS = {
    _MOUSE_L = curry(set_and_get_undo, 'mouse_lbut'),
    _MOUSE_R = curry(set_and_get_undo, 'mouse_rbut'),
    _MOUSE_M = true,
    _MOUSE_L_DOWN = true,
    _MOUSE_R_DOWN = true,
    _MOUSE_M_DOWN = true,
}

local FAKE_INPUT_KEYS = copyall(MOUSE_KEYS)
FAKE_INPUT_KEYS._STRING = true

function simulateInput(screen,...)
    local keys, enabled_mouse_keys = {}, {}
    local function push_key(arg)
        local kv = arg
        if type(arg) == 'string' then
            kv = df.interface_key[arg]
            if kv == nil and not FAKE_INPUT_KEYS[arg] then
                error('Invalid keycode: '..arg)
            end
            if MOUSE_KEYS[arg] then
                df.global.enabler.tracking_on = 1
                enabled_mouse_keys[arg] = true
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
    local undo_fns = {}
    for mk, fn in pairs(MOUSE_KEYS) do
        if type(fn) == 'function' then
            table.insert(undo_fns, fn(enabled_mouse_keys[mk]))
        end
    end
    dscreen._doSimulateInput(screen, keys)
    for _, undo_fn in ipairs(undo_fns) do
        undo_fn()
    end
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

function parse_inset(inset)
    local l,r,t,b
    if type(inset) == 'table' then
        l,r = inset.l or inset.x or 0, inset.r or inset.x or 0
        t,b = inset.t or inset.y or 0, inset.b or inset.y or 0
    else
        l = inset or 0
        t,r,b = l,l,l
    end
    return l,t,r,b
end

function inset_frame(rect, inset, gap)
    if not rect then return mkdims_wh(0, 0, 0, 0) end
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
    self.to_map = false
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
    local vp = ViewRect.viewport(self,x,y,w,h)
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

function Painter:map(to_map)
    self.to_map = to_map
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
    dscreen.fillRect(to_pen(self.cur_pen,pen,bg,bold),x1,y1,x2,y2,self.to_map)
    return self
end

function Painter:char(char,pen,...)
    if self:isValidPos() then
        dscreen.paintTile(to_pen(self.cur_pen, pen, ...), self.x, self.y, char, nil, self.to_map)
    end
    return self:advance(1, nil)
end

function Painter:tile(char,tile,pen,...)
    if self:isValidPos() then
        dscreen.paintTile(to_pen(self.cur_pen, pen, ...), self.x, self.y, char, tile, self.to_map)
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
                string.sub(text,dx+1,len),
                self.to_map
            )
        end
    end
    return self:advance(#text, nil)
end

function Painter:key(keycode,pen,...)
    return self:string(
        getKeyDisplay(keycode),
        to_pen(self.cur_key_pen, pen, ...)
    )
end

function Painter:key_string(keycode, text, ...)
    return self:key(keycode):string(': '):string(text, ...)
end

--------------------------
-- Abstract view object --
--------------------------

View = defclass(View)

View.ATTRS {
    active = true,
    visible = true,
    view_id = DEFAULT_NIL,
    on_focus = DEFAULT_NIL,
    on_unfocus = DEFAULT_NIL,
}

function View:init(args)
    self.subviews = {}
    self.focus_group = {self}
    self.focus = false
end

local function inherit_focus_group(view, focus_group)
    for _,child in ipairs(view.subviews) do
        inherit_focus_group(child, focus_group)
    end
    view.focus_group = focus_group
end

function View:addviews(list)
    if not list then return end

    local sv = self.subviews

    for _,obj in ipairs(list) do
        -- absorb the focus groups of new children
        for _,focus_obj in ipairs(obj.focus_group) do
            table.insert(self.focus_group, focus_obj)
        end
        -- if the child's focus group has a focus owner, absorb it if we don't
        -- already have one. otherwise keep the focus owner we have and clear
        -- the focus of the child.
        if obj.focus_group.cur then
            if not self.focus_group.cur then
                self.focus_group.cur = obj.focus_group.cur
            else
                obj.focus_group.cur:setFocus(false)
            end
        end
        -- overwrite the child's focus_group hierarchy with ours
        inherit_focus_group(obj, self.focus_group)
        -- if we don't have a focus owner, give it to the new child if they want
        if not self.focus_group.cur and obj:getPreferredFocusState() then
            obj:setFocus(true)
        end

        -- set ourselves as the parent view of the new child
        obj.parent_view = self

        -- add the subview to our child list
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

-- should be overridden by widgets that care about capturing keyboard focus
-- (e.g. widgets.EditField)
function View:getPreferredFocusState()
    return false
end

function View:setFocus(focus)
    if focus then
        if self.focus then return end -- nothing to do if we already have focus
        if self.focus_group.cur then
            -- steal focus from current owner
            self.focus_group.cur:setFocus(false)
        end
        self.focus_group.cur = self
        self.focus = true
        if self.on_focus then
            self.on_focus()
        end
    elseif self.focus then
        self.focus = false
        self.focus_group.cur = nil
        if self.on_unfocus then
            self.on_unfocus()
        end
    end
end

function View:getWindowSize()
    local rect = self.frame_body
    return rect.width, rect.height
end

function View:getMousePos(view_rect)
    local rect = view_rect or self.frame_body
    local x,y = dscreen.getMousePos()
    if rect and x and rect:inClipGlobalXY(x,y) then
        return rect:localXY(x,y)
    end
end

function View:getMouseFramePos()
    return self:getMousePos(ViewRect{rect=self.frame_rect})
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
        if getval(child.visible) then
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

-- Returns whether we should invoke the focus owner's onInput() function from
-- the given view's inputToSubviews() function. That is, returns true if:
-- - the view is not itself the focus owner since that would be an infinite loop
-- - the view is not a descendent of the focus owner (same as above)
-- - the focus owner and all of its ancestors are visible and active, since if
--   the focus owner is not (directly or transitively) visible or active, then
--   it shouldn't be getting input.
local function should_send_input_to_focus_owner(view, focus_owner)
    local iter = view
    while iter do
        if iter == focus_owner then
            return false
        end
        iter = iter.parent_view
    end
    iter = focus_owner
    while iter do
        if not getval(iter.visible) or not getval(iter.active) then
            return false
        end
        iter = iter.parent_view
    end
    return true
end

function View:inputToSubviews(keys)
    local children = self.subviews

    -- give focus owner first dibs on the input
    local focus_owner = self.focus_group.cur
    if focus_owner and should_send_input_to_focus_owner(self, focus_owner) and
            focus_owner:onInput(keys) then
        return true
    end

    for i=#children,1,-1 do
        local child = children[i]
        if getval(child.visible) and getval(child.active)
                and child ~= focus_owner and child:onInput(keys) then
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
Screen.request_full_screen_refresh = false

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
    if Screen.request_full_screen_refresh then
        df.global.gps.force_full_display_count = 1
        Screen.request_full_screen_refresh = false
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
    self:onAboutToShow(parent or dfhack.gui.getCurViewscreen(true))

    -- if we're autodetecting the parent, refresh the parent handle in case the
    -- original parent has been dismissed by onAboutToShow()'s actions
    parent = parent or dfhack.gui.getCurViewscreen(true)
    if not dscreen.show(self, parent.child) then
        error('Could not show screen')
    end
    return self
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
    -- don't leave artifacts behind on the parent screen when we disappear
    Screen.request_full_screen_refresh = true
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

-----------------------------
-- Z-order swapping screen --
-----------------------------

DEFAULT_INITIAL_PAUSE = true

ZScreen = defclass(ZScreen, Screen)
ZScreen.ATTRS{
    defocusable=true,
    initial_pause=DEFAULT_NIL,
    force_pause=false,
    pass_pause=true,
    pass_movement_keys=false,
    pass_mouse_clicks=true,
}

function ZScreen:preinit(args)
    if self.ATTRS.initial_pause == nil then
        args.initial_pause = DEFAULT_INITIAL_PAUSE or
                (self.ATTRS.pass_mouse_clicks == false)
    end
end

function ZScreen:init()
    self.saved_pause_state = df.global.pause_state
    if self.initial_pause and dfhack.isMapLoaded() then
        df.global.pause_state = true
    end
    self.defocused = false
end

function ZScreen:dismiss()
    ZScreen.super.dismiss(self)
    if (self.force_pause or self.initial_pause) and dfhack.isMapLoaded() then
        -- never go from unpaused to paused, just from paused to unpaused
        df.global.pause_state = df.global.pause_state and self.saved_pause_state
    end
end

local NO_LOGIC_SCREENS = {
    'viewscreen_loadgamest',
    'viewscreen_adopt_regionst',
    'viewscreen_export_regionst',
    'viewscreen_choose_game_typest',
    'viewscreen_worldst',
}
for _,v in ipairs(NO_LOGIC_SCREENS) do
    if not df[v] then
        error('invalid class name: ' .. v)
    end
    NO_LOGIC_SCREENS[df[v]] = true
end

-- this is necessary for middle-click map scrolling to function
function ZScreen:onIdle()
    if self.force_pause and dfhack.isMapLoaded() then
        if not df.global.pause_state and self.force_pause ~= 'blink' then
            self.force_pause = 'blink'
            local end_ms = dfhack.getTickCount() + 1000
            local function blink_reset()
                if dfhack.getTickCount() < end_ms then
                    dfhack.timeout(10, 'frames', blink_reset)
                else
                    self.force_pause = true
                end
            end
            blink_reset()
        end
        df.global.pause_state = true
    end
    if self._native and self._native.parent then
        local vs_type = dfhack.gui.getDFViewscreen(true)._type
        if NO_LOGIC_SCREENS[vs_type] then
            self.force_pause = true
            self.pass_movement_keys = false
            self.pass_mouse_clicks = false
        else
            self._native.parent:logic()
        end
    end
end

function ZScreen:render(dc)
    self:renderParent()
    ZScreen.super.render(self, dc)
end

function ZScreen:hasFocus()
    return not self.defocused
            and dfhack.gui.getCurViewscreen(true) == self._native
end

function ZScreen:onInput(keys)
    local has_mouse = self:isMouseOver()
    if not self:hasFocus() then
        if has_mouse and
                (keys._MOUSE_L or keys._MOUSE_R or
                 keys.CONTEXT_SCROLL_UP or keys.CONTEXT_SCROLL_DOWN or
                 keys.CONTEXT_SCROLL_PAGEUP or keys.CONTEXT_SCROLL_PAGEDOWN) then
            self:raise()
        else
            self:sendInputToParent(keys)
            return true
        end
    end

    if ZScreen.super.onInput(self, keys) then
        -- noop
    elseif self.pass_mouse_clicks and keys._MOUSE_L and not has_mouse then
        self.defocused = self.defocusable
        self:sendInputToParent(keys)
    elseif keys.LEAVESCREEN or keys._MOUSE_R then
        self:dismiss()
    else
        local passit = self.pass_pause and keys.D_PAUSE
        if not passit and self.pass_mouse_clicks then
            if keys.CONTEXT_SCROLL_UP or keys.CONTEXT_SCROLL_DOWN or
                    keys.CONTEXT_SCROLL_PAGEUP or keys.CONTEXT_SCROLL_PAGEDOWN then
                passit = true
            else
                for key in pairs(MOUSE_KEYS) do
                    if keys[key] then
                        passit = true
                        break
                    end
                end
            end
        end
        if not passit and self.pass_movement_keys then
            passit = require('gui.dwarfmode').getMapKey(keys)
        end
        if passit then
            self:sendInputToParent(keys)
        end
    end
    return true
end

function ZScreen:raise()
    if self:isDismissed() or self:hasFocus() then
        return self
    end
    dscreen.raise(self)
    self.defocused = false
    return self
end

function ZScreen:isMouseOver()
    for _,sv in ipairs(self.subviews) do
        if sv:getMouseFramePos() then return true end
    end
end

local function zscreen_get_any(scr, thing)
    if not scr._native or not scr._native.parent then return nil end
    return dfhack.gui['getAny'..thing](scr._native.parent)
end
function ZScreen:onGetSelectedUnit()
    return zscreen_get_any(self, 'Unit')
end
function ZScreen:onGetSelectedItem()
    return zscreen_get_any(self, 'Item')
end
function ZScreen:onGetSelectedBuilding()
    return zscreen_get_any(self, 'Building')
end
function ZScreen:onGetSelectedStockpile()
    return zscreen_get_any(self, 'Stockpile')
end
function ZScreen:onGetSelectedCivZone()
    return zscreen_get_any(self, 'CivZone')
end
function ZScreen:onGetSelectedPlant()
    return zscreen_get_any(self, 'Plant')
end

-- convenience subclass for modal dialogs
ZScreenModal = defclass(ZScreenModal, ZScreen)
ZScreenModal.ATTRS{
    defocusable = false,
    force_pause = true,
    pass_movement_keys = false,
    pass_mouse_clicks = false,
}

-- Framed screen object
--------------------------

-- Plain grey-colored frame.
-- deprecated
GREY_FRAME = {
    frame_pen = to_pen{ ch = ' ', fg = COLOR_BLACK, bg = COLOR_GREY },
    title_pen = to_pen{ fg = COLOR_BLACK, bg = COLOR_WHITE },
    signature_pen = to_pen{ fg = COLOR_BLACK, bg = COLOR_GREY },
}

-- The boundary used by the pre-steam DF screens.
-- deprecated
BOUNDARY_FRAME = {
    frame_pen = to_pen{ ch = 0xDB, fg = COLOR_GREY, bg = COLOR_BLACK },
    title_pen = to_pen{ fg = COLOR_BLACK, bg = COLOR_GREY },
    signature_pen = to_pen{ fg = COLOR_BLACK, bg = COLOR_GREY },
}

local BASE_FRAME = {
    frame_pen = to_pen{ ch=206, fg=COLOR_GREY, bg=COLOR_BLACK },
    title_pen = to_pen{ fg=COLOR_BLACK, bg=COLOR_GREY },
    inactive_title_pen = to_pen{ fg=COLOR_GREY, bg=COLOR_BLACK },
    signature_pen = to_pen{ fg=COLOR_GREY, bg=COLOR_BLACK },
    paused_pen = to_pen{fg=COLOR_RED, bg=COLOR_BLACK},
}


local function make_frame(tp, double_line)
    local frame = copyall(BASE_FRAME)
    frame.t_frame_pen = to_pen{ tile=curry(tp, 2), ch=double_line and 205 or 196, fg=COLOR_GREY, bg=COLOR_BLACK }
    frame.l_frame_pen = to_pen{ tile=curry(tp, 8), ch=double_line and 186 or 179, fg=COLOR_GREY, bg=COLOR_BLACK }
    frame.b_frame_pen = to_pen{ tile=curry(tp, 16), ch=double_line and 205 or 196, fg=COLOR_GREY, bg=COLOR_BLACK }
    frame.r_frame_pen = to_pen{ tile=curry(tp, 10), ch=double_line and 186 or 179, fg=COLOR_GREY, bg=COLOR_BLACK }
    frame.lt_frame_pen = to_pen{ tile=curry(tp, 1), ch=double_line and 201 or 218, fg=COLOR_GREY, bg=COLOR_BLACK }
    frame.lb_frame_pen = to_pen{ tile=curry(tp, 15), ch=double_line and 200 or 192, fg=COLOR_GREY, bg=COLOR_BLACK }
    frame.rt_frame_pen = to_pen{ tile=curry(tp, 3), ch=double_line and 187 or 191, fg=COLOR_GREY, bg=COLOR_BLACK }
    frame.rb_frame_pen = to_pen{ tile=curry(tp, 17), ch=double_line and 188 or 217, fg=COLOR_GREY, bg=COLOR_BLACK }
    return frame
end

function FRAME_WINDOW(resizable)
    local frame = make_frame(textures.tp_border_window, true)
    if not resizable then
        frame.rb_frame_pen = to_pen{ tile=curry(textures.tp_border_panel, 17), ch=double_line and 188 or 217, fg=COLOR_GREY, bg=COLOR_BLACK }
    end
    return frame
end
function FRAME_PANEL()
    return make_frame(textures.tp_border_panel, false)
end
function FRAME_MEDIUM()
    return make_frame(textures.tp_border_medium, false)
end
function FRAME_BOLD()
    return make_frame(textures.tp_border_bold, true)
end
function FRAME_THIN()
    return make_frame(textures.tp_border_thin, false)
end
function FRAME_INTERIOR()
    local frame = make_frame(textures.tp_border_thin, false)
    frame.signature_pen = false
    return frame
end
function FRAME_INTERIOR_MEDIUM()
    local frame = make_frame(textures.tp_border_medium, false)
    frame.signature_pen = false
    return frame
end

-- for compatibility with pre-steam code
GREY_LINE_FRAME = FRAME_PANEL

-- for compatibility with deprecated frame naming scheme
WINDOW_FRAME = FRAME_WINDOW
PANEL_FRAME = FRAME_PANEL
MEDIUM_FRAME = FRAME_MEDIUM
BOLD_FRAME = FRAME_BOLD
INTERIOR_FRAME = FRAME_INTERIOR
INTERIOR_MEDIUM_FRAME = FRAME_INTERIOR_MEDIUM

function paint_frame(dc, rect, style, title, inactive, pause_forced, resizable)
    if type(style) == 'function' then
        style = style(resizable)
    end
    local pen = style.frame_pen
    local x1,y1,x2,y2 = dc.x1+rect.x1, dc.y1+rect.y1, dc.x1+rect.x2, dc.y1+rect.y2
    dscreen.paintTile(style.lt_frame_pen or pen, x1, y1)
    dscreen.paintTile(style.rt_frame_pen or pen, x2, y1)
    dscreen.paintTile(style.lb_frame_pen or pen, x1, y2)
    dscreen.paintTile(style.rb_frame_pen or pen, x2, y2)
    dscreen.fillRect(style.t_frame_pen or style.h_frame_pen or pen,x1+1,y1,x2-1,y1)
    dscreen.fillRect(style.b_frame_pen or style.h_frame_pen or pen,x1+1,y2,x2-1,y2)
    dscreen.fillRect(style.l_frame_pen or style.v_frame_pen or pen,x1,y1+1,x1,y2-1)
    dscreen.fillRect(style.r_frame_pen or style.v_frame_pen or pen,x2,y1+1,x2,y2-1)
    if style.signature_pen ~= false then
        dscreen.paintString(style.signature_pen or style.title_pen or pen,x2-7,y2,"DFHack")
    end

    if title then
        local x = math.max(0,math.floor((x2-x1-3-#title)/2)) + x1
        local tstr = '  '..title..'  '
        if #tstr > x2-x1-1 then
            tstr = string.sub(tstr,1,x2-x1-1)
        end
        dscreen.paintString(inactive and style.inactive_title_pen or style.title_pen or pen,
                            x, y1, tstr)
    end

    if pause_forced then
        local pause_label = ' PAUSE FORCED '
        if pause_forced == 'blink' and blink_visible(100) then
            pause_label = '              '
        end
        dscreen.paintString(style.paused_pen or style.title_pen or pen,
                            x1+2, y2, pause_label)
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
    if rect.wgap <= 0 and rect.hgap <= 0 then
        dc:clear()
    else
        self:renderParent()
        dc:fill(rect, self.frame_background)
    end
    paint_frame(dc,rect,self.frame_style,self.frame_title)
end

function FramedScreen:onInput(keys)
    FramedScreen.super.onInput(self, keys)
    return true -- FramedScreens are modal
end

-- Inverts the brightness of the color, optionally taking a "bold" parameter,
-- which you should include if you're reading the fg color of a pen.
function invert_color(color, bold)
    color = bold and (color + 8) or color
    return (color + 8) % 16
end
return _ENV
