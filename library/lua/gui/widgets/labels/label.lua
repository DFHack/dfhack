local gui = require('gui')
local utils = require('utils')
local common = require('gui.widgets.common')
local Widget = require('gui.widgets.widget')
local Scrollbar = require('gui.widgets.scrollbar')

local getval = utils.getval
local to_pen = dfhack.pen.parse

-----------
-- Label --
-----------

function parse_label_text(obj)
    local text = obj.text or {}
    if type(text) ~= 'table' then
        text = { text }
    end
    local curline = nil
    local out = { }
    local active = nil
    local idtab = nil
    for _,v in ipairs(text) do
        local vv
        if type(v) == 'string' then
            vv = v:split(NEWLINE)
        else
            vv = { v }
        end

        for i = 1,#vv do
            local cv = vv[i]
            if i > 1 then
                if not curline then
                    table.insert(out, {})
                end
                curline = nil
            end
            if cv ~= '' then
                if not curline then
                    curline = {}
                    table.insert(out, curline)
                end

                if type(cv) == 'string' then
                    table.insert(curline, { text = cv })
                else
                    table.insert(curline, cv)

                    if cv.on_activate then
                        active = active or {}
                        table.insert(active, cv)
                    end

                    if cv.id then
                        idtab = idtab or {}
                        idtab[cv.id] = cv
                    end
                end
            end
        end
    end
    obj.text_lines = out
    obj.text_active = active
    obj.text_ids = idtab
end

local function is_disabled(token)
    return (token.disabled ~= nil and getval(token.disabled)) or
           (token.enabled ~= nil and not getval(token.enabled))
end

-- Make the hover pen -- that is a pen that should render elements that has the
-- mouse hovering over it. if hpen is specified, it just checks the fields and
-- returns it (in parsed pen form)
local function make_hpen(pen, hpen)
    if not hpen then
        pen = to_pen(pen)

        -- Swap the foreground and background
        hpen = dfhack.pen.make(pen.bg, nil, pen.fg + (pen.bold and 8 or 0))
    end

    -- text_hpen needs a character in order to paint the background using
    -- Painter:fill(), so let's make it paint a space to show the background
    -- color
    local hpen_parsed = to_pen(hpen)
    hpen_parsed.ch = string.byte(' ')
    return hpen_parsed
end

---@param obj any
---@param dc gui.Painter
---@param x0 integer
---@param y0 integer
---@param pen dfhack.pen|dfhack.color|fun(): dfhack.pen|dfhack.color
---@param dpen dfhack.pen|dfhack.color|fun(): dfhack.pen|dfhack.color
---@param disabled boolean
---@param hpen dfhack.pen|dfhack.color|fun(): dfhack.pen|dfhack.color
---@param hovered boolean
function render_text(obj,dc,x0,y0,pen,dpen,disabled,hpen,hovered)
    pen, dpen, hpen = getval(pen), getval(dpen), getval(hpen)
    local width = 0
    for iline = dc and obj.start_line_num or 1, #obj.text_lines do
        local x, line = 0, obj.text_lines[iline]
        if dc then
            local offset = (obj.start_line_num or 1) - 1
            local y = y0 + iline - offset - 1
            -- skip text outside of the containing frame
            if y > dc.height - 1 then break end
            dc:seek(x+x0, y)
        end
        for _,token in ipairs(line) do
            token.line = iline
            token.x1 = x

            if token.gap then
                x = x + token.gap
                if dc then
                    dc:advance(token.gap)
                end
            end

            if token.tile or (hovered and token.htile) then
                x = x + 1
                if dc then
                    local tile = hovered and getval(token.htile or token.tile) or getval(token.tile)
                    local tile_pen = tonumber(tile) and to_pen{tile=tile} or tile
                    dc:char(nil, tile_pen)
                    if token.width then
                        dc:advance(token.width-1)
                    end
                end
            end

            if token.text or token.key then
                local text = ''..(getval(token.text) or '')
                local keypen = to_pen(token.key_pen or COLOR_LIGHTGREEN)

                if dc then
                    local tpen = getval(token.pen)
                    local dcpen = to_pen(tpen or pen)

                    -- If disabled, figure out which dpen to use
                    if disabled or is_disabled(token) then
                        dcpen = to_pen(getval(token.dpen) or tpen or dpen)
                        if keypen.fg ~= COLOR_BLACK then
                            keypen.bold = false
                        end

                        -- if hovered *and* disabled, combine both effects
                        if hovered then
                            dcpen = make_hpen(dcpen)
                        end
                    elseif hovered then
                        dcpen = make_hpen(dcpen, getval(token.hpen) or hpen)
                    end

                    dc:pen(dcpen)
                end
                local width = getval(token.width)
                local padstr
                if width then
                    x = x + width
                    if #text > width then
                        text = string.sub(text,1,width)
                    else
                        if token.pad_char then
                            padstr = string.rep(token.pad_char,width-#text)
                        end
                        if dc and token.rjustify then
                            if padstr then dc:string(padstr) else dc:advance(width-#text) end
                        end
                    end
                else
                    x = x + #text
                end

                if token.key then
                    if type(token.key) == 'string' and not df.interface_key[token.key] then
                        error('Invalid interface_key: ' .. token.key)
                    end
                    local keystr = gui.getKeyDisplay(token.key)
                    local sep = token.key_sep or ''

                    x = x + #keystr

                    if sep:startswith('()') then
                        if dc then
                            dc:string(text)
                            dc:string(' ('):string(keystr,keypen)
                            dc:string(sep:sub(2))
                        end
                        x = x + 1 + #sep
                    else
                        if dc then
                            dc:string(keystr,keypen):string(sep):string(text)
                        end
                        x = x + #sep
                    end
                else
                    if dc then
                        dc:string(text)
                    end
                end

                if width and dc and not token.rjustify then
                    if padstr then dc:string(padstr) else dc:advance(width-#text) end
                end
            end

            token.x2 = x
        end
        width = math.max(width, x)
    end
    obj.text_width = width
end

function check_text_keys(self, keys)
    if self.text_active then
        for _,item in ipairs(self.text_active) do
            if item.key and keys[item.key] and not is_disabled(item) then
                item.on_activate()
                return true
            end
        end
    end
end

---@class widgets.LabelToken
---@field text string|fun(): string
---@field gap? integer
---@field tile? integer|dfhack.pen
---@field htile? integer|dfhack.pen
---@field width? integer|fun(): integer
---@field pad_char? string
---@field key? string
---@field key_sep? string
---@field disabled? boolean|fun(): boolean
---@field enabled? boolean|fun(): boolean
---@field pen? dfhack.color|dfhack.pen
---@field dpen? dfhack.color|dfhack.pen
---@field hpen? dfhack.color|dfhack.pen
---@field on_activiate? fun()
---@field id? string
---@field line? any Internal use only
---@field x1? any Internal use only
---@field x2? any Internal use only

---@class widgets.Label.attrs: widgets.Widget.attrs
---@field text? string|widgets.LabelToken[]
---@field text_pen dfhack.color|dfhack.pen
---@field text_dpen dfhack.color|dfhack.pen
---@field text_hpen? dfhack.color|dfhack.pen
---@field disabled? boolean|fun(): boolean
---@field enabled? boolean|fun(): boolean
---@field auto_height boolean
---@field auto_width boolean
---@field on_click? function
---@field on_rclick? function
---@field scroll_keys table<string, string|integer>

---@class widgets.Label.attrs.partial: widgets.Label.attrs

---@class widgets.Label: widgets.Widget, widgets.Label.attrs
---@field super widgets.Widget
---@field ATTRS widgets.Label.attrs|fun(attributes: widgets.Label.attrs.partial)
---@overload fun(init_table: widgets.Label.attrs.partial): self
Label = defclass(Label, Widget)

Label.ATTRS{
    text_pen = COLOR_WHITE,
    text_dpen = COLOR_DARKGREY, -- disabled
    text_hpen = DEFAULT_NIL, -- hover - default is to invert the fg/bg colors
    disabled = DEFAULT_NIL,
    enabled = DEFAULT_NIL,
    auto_height = true,
    auto_width = false,
    on_click = DEFAULT_NIL,
    on_rclick = DEFAULT_NIL,
    scroll_keys = common.STANDARDSCROLL,
}

---@param args widgets.Label.attrs.partial
function Label:init(args)
    self.scrollbar = Scrollbar{
        frame={r=0},
        on_scroll=self:callback('on_scrollbar')}

    self:addviews{self.scrollbar}

    self:setText(args.text or self.text)
end

local function update_label_scrollbar(label)
    local body_height = label.frame_body and label.frame_body.height or 1
    label.scrollbar:update(label.start_line_num, body_height,
                           label:getTextHeight())
end

function Label:setText(text)
    self.start_line_num = 1
    self.text = text
    parse_label_text(self)

    if self.auto_height then
        self.frame = self.frame or {}
        self.frame.h = self:getTextHeight()
    end

    update_label_scrollbar(self)
end

function Label:preUpdateLayout()
    if self.auto_width then
        self.frame = self.frame or {}
        self.frame.w = self:getTextWidth()
    end
end

function Label:postUpdateLayout()
    update_label_scrollbar(self)
end

function Label:itemById(id)
    if self.text_ids then
        return self.text_ids[id]
    end
end

function Label:getTextHeight()
    return #self.text_lines
end

function Label:getTextWidth()
    render_text(self)
    return self.text_width
end

-- Overridden by subclasses that also want to add new mouse handlers, see
-- HotkeyLabel.
function Label:shouldHover()
    return self.on_click or self.on_rclick
end

function Label:onRenderBody(dc)
    local text_pen = self.text_pen
    local hovered = self:getMousePos() and self:shouldHover()
    render_text(self,dc,0,0,text_pen,self.text_dpen,is_disabled(self), self.text_hpen, hovered)
end

function Label:on_scrollbar(scroll_spec)
    local v = 0
    if tonumber(scroll_spec) then
        v = scroll_spec - self.start_line_num
    elseif scroll_spec == 'down_large' then
        v = '+halfpage'
    elseif scroll_spec == 'up_large' then
        v = '-halfpage'
    elseif scroll_spec == 'down_small' then
        v = 1
    elseif scroll_spec == 'up_small' then
        v = -1
    end

    self:scroll(v)
end

function Label:scroll(nlines)
    if not nlines then return end
    local text_height = math.max(1, self:getTextHeight())
    if type(nlines) == 'string' then
        if nlines == '+page' then
            nlines = self.frame_body.height
        elseif nlines == '-page' then
            nlines = -self.frame_body.height
        elseif nlines == '+halfpage' then
            nlines = math.ceil(self.frame_body.height/2)
        elseif nlines == '-halfpage' then
            nlines = -math.ceil(self.frame_body.height/2)
        elseif nlines == 'home' then
            nlines = 1 - self.start_line_num
        elseif nlines == 'end' then
            nlines = text_height
        else
            error(('unhandled scroll keyword: "%s"'):format(nlines))
        end
    end
    local n = self.start_line_num + nlines
    n = math.min(n, text_height - self.frame_body.height + 1)
    n = math.max(n, 1)
    nlines = n - self.start_line_num
    self.start_line_num = n
    update_label_scrollbar(self)
    return nlines
end

function Label:onInput(keys)
    if is_disabled(self) then return false end
    if self:inputToSubviews(keys) then
        return true
    end
    if keys._MOUSE_L and self:getMousePos() and self.on_click then
        self.on_click()
        return true
    end
    if keys._MOUSE_R and self:getMousePos() and self.on_rclick then
        self.on_rclick()
        return true
    end
    for k,v in pairs(self.scroll_keys) do
        if keys[k] and 0 ~= self:scroll(v) then
            return true
        end
    end
    return check_text_keys(self, keys)
end

--------------------------------
-- makeButtonLabelText
--

local function get_button_token_hover_ch(spec, x, y, ch)
    local ch_hover = ch
    if spec.chars_hover then
        local row = spec.chars_hover[y]
        if type(row) == 'string' then
            ch_hover = row:sub(x, x)
        else
            ch_hover = row[x]
        end
    end
    return ch_hover
end

local function get_button_token_base_pens(spec, x, y)
    local pen, pen_hover = COLOR_GRAY, COLOR_WHITE
    if spec.pens then
        pen = type(spec.pens) == 'table' and (safe_index(spec.pens, y, x) or spec.pens[y]) or spec.pens
        if spec.pens_hover then
            pen_hover = type(spec.pens_hover) == 'table' and (safe_index(spec.pens_hover, y, x) or spec.pens_hover[y]) or spec.pens_hover
        else
            pen_hover = pen
        end
    end
    return pen, pen_hover
end

local function get_button_tileset_idx(spec, x, y, tileset_offset, tileset_stride)
    local idx = (tileset_offset or 1)
    idx = idx + (x - 1)
    idx = idx + (y - 1) * (tileset_stride or #spec.chars[1])
    return idx
end

local function get_asset_tile(asset, x, y)
    return dfhack.screen.findGraphicsTile(asset.page, asset.x+x-1, asset.y+y-1)
end

local function get_button_token_tiles(spec, x, y)
    local tile = safe_index(spec.tiles_override, y, x)
    local tile_hover = safe_index(spec.tiles_hover_override, y, x) or tile
    if not tile and spec.tileset then
        local tileset = spec.tileset
        local idx = get_button_tileset_idx(spec, x, y, spec.tileset_offset, spec.tileset_stride)
        tile = dfhack.textures.getTexposByHandle(tileset[idx])
        if spec.tileset_hover then
            local tileset_hover = spec.tileset_hover
            local idx_hover = get_button_tileset_idx(spec, x, y,
                spec.tileset_hover_offset or spec.tileset_offset,
                spec.tileset_hover_stride or spec.tileset_stride)
            tile_hover = dfhack.textures.getTexposByHandle(tileset_hover[idx_hover])
        else
            tile_hover = tile
        end
    end
    if not tile and spec.asset then
        tile = get_asset_tile(spec.asset, x, y)
        if spec.asset_hover then
            tile_hover = get_asset_tile(spec.asset_hover, x, y)
        else
            tile_hover = tile
        end
    end
    return tile, tile_hover
end

local function get_button_token_pen(base_pen, tile, ch)
    local pen = dfhack.pen.make(base_pen)
    pen.tile = tile
    pen.ch = ch
    return pen
end

local function get_button_token_pens(spec, x, y, ch, ch_hover)
    local base_pen, base_pen_hover = get_button_token_base_pens(spec, x, y)
    local tile, tile_hover = get_button_token_tiles(spec, x, y)
    return get_button_token_pen(base_pen, tile, ch), get_button_token_pen(base_pen_hover, tile_hover, ch_hover)
end

local function make_button_token(spec, x, y, ch)
    local ch_hover = get_button_token_hover_ch(spec, x, y, ch)
    local pen, pen_hover = get_button_token_pens(spec, x, y, ch, ch_hover)
    return {
        tile=pen,
        htile=pen_hover,
        width=1,
    }
end

---@class widgets.ButtonLabelSpec
---@field chars (string|string[])[]
---@field chars_hover? (string|string[])[]
---@field pens? dfhack.color|dfhack.color[][]
---@field pens_hover? dfhack.color|dfhack.color[][]
---@field tiles_override? integer[][]
---@field tiles_hover_override? integer[][]
---@field tileset? TexposHandle[]
---@field tileset_hover? TexposHandle[]
---@field tileset_offset? integer
---@field tileset_hover_offset? integer
---@field tileset_stride? integer
---@field tileset_hover_stride? integer
---@field asset? {page: string, x: integer, y: integer}
---@field asset_hover? {page: string, x: integer, y: integer}

---@nodiscard
---@param spec widgets.ButtonLabelSpec
---@return widgets.LabelToken[]
function makeButtonLabelText(spec)
    local tokens = {}
    for y, row in ipairs(spec.chars) do
        if type(row) == 'string' then
            local x = 1
            for ch in row:gmatch('.') do
                table.insert(tokens, make_button_token(spec, x, y, ch))
                x = x + 1
            end
        else
            for x, ch in ipairs(row) do
                table.insert(tokens, make_button_token(spec, x, y, ch))
            end
        end
        if y < #spec.chars then
            table.insert(tokens, NEWLINE)
        end
    end
    return tokens
end

Label.makeButtonLabelText = makeButtonLabelText
Label.parse_label_text = parse_label_text
Label.render_text = render_text
Label.check_text_keys = check_text_keys

return Label
