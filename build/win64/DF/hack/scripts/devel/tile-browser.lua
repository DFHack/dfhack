local gui = require('gui')
local widgets = require('gui.widgets')

TileBrowser = defclass(TileBrowser, gui.ZScreen)
TileBrowser.ATTRS{
    focus_string='tile-browser',
}

function TileBrowser:init()
    self.max_texpos = #df.global.enabler.textures.raws
    self.tile_size = 1

    local main_panel = widgets.Window{
        view_id='main',
        frame={w=32, h=30},
        drag_anchors={title=true, body=true},
        resizable=true,
        resize_min={h=20},
        on_layout=self:callback('on_resize'),
        frame_title='Tile Browser',
    }

    main_panel:addviews{
        widgets.EditField{
            view_id='start_index',
            frame={t=0, l=0},
            key='CUSTOM_CTRL_A',
            label_text='Start index: ',
            text='0',
            on_submit=self:callback('set_start_index'),
            on_char=function(ch) return ch:match('%d') end},
        widgets.HotkeyLabel{
            frame={t=1, l=0},
            label='Prev',
            key='KEYBOARD_CURSOR_UP_FAST',
            on_activate=self:callback('shift_start_index', -1000)},
        widgets.Label{
            frame={t=1, l=6, w=1},
            text={{text=string.char(24), pen=COLOR_LIGHTGREEN}}},
        widgets.HotkeyLabel{
            frame={t=1, l=15},
            label='Next',
            key='KEYBOARD_CURSOR_DOWN_FAST',
            on_activate=self:callback('shift_start_index', 1000)},
        widgets.Label{
            frame={t=1, l=21, w=1},
            text={{text=string.char(25), pen=COLOR_LIGHTGREEN}}},
        widgets.Label{
            view_id='header',
            frame={t=3, l=0, r=2}},
        widgets.Panel{
            view_id='transparent_area',
            frame={t=4, b=4, l=5, r=2},
            frame_background=gui.TRANSPARENT_PEN},
        widgets.Label{
            view_id='report',
            frame={t=4, b=4},
            scroll_keys={
                STANDARDSCROLL_UP = -1,
                KEYBOARD_CURSOR_UP = -1,
                STANDARDSCROLL_DOWN = 1,
                KEYBOARD_CURSOR_DOWN = 1,
                STANDARDSCROLL_PAGEUP = '-page',
                STANDARDSCROLL_PAGEDOWN = '+page'}},
        widgets.Label{
            view_id='footer',
            frame={b=3, l=0, r=2}},
        widgets.WrappedLabel{
            frame={b=0},
            text_to_wrap='Please resize window to change visible tile size.'},
    }
    self:addviews{main_panel}

    self:set_start_index('0')
end

function TileBrowser:shift_start_index(amt)
    self:set_start_index(tonumber(self.subviews.start_index.text) + amt)
end

function TileBrowser:set_start_index(idx)
    idx = tonumber(idx)
    if not idx then return end

    idx = math.max(0, math.min(self.max_texpos - 980, idx))

    idx = idx - (idx % 20) -- floor to nearest multiple of 20
    self.subviews.start_index:setText(tostring(idx))
    self.subviews.transparent_area.frame.l = #tostring(idx) + 4
    self.dirty = true
end

function TileBrowser:update_report()
    local idx = tonumber(self.subviews.start_index.text)
    local end_idx = math.min(self.max_texpos, idx + 999)
    local prefix_len = #tostring(idx) + 4

    local guide = {}
    table.insert(guide, {text='', width=prefix_len})
    for n=0,9 do
        table.insert(guide, {text=tostring(n), width=self.tile_size})
    end
    table.insert(guide, {text='', width=self.tile_size})
    for n=0,9 do
        table.insert(guide, {text=tostring(n), width=self.tile_size})
    end
    self.subviews.header:setText(guide)
    self.subviews.footer:setText(guide)

    local report = {}
    for texpos=idx,end_idx do
        if texpos % 20 == 0 then
            table.insert(report, {text=tostring(texpos), width=prefix_len})
        elseif texpos % 10 == 0 then
            table.insert(report, {text='', pad_char=' ', width=self.tile_size})
        end
        table.insert(report, {tile=texpos, pen=gui.KEEP_LOWER_PEN, width=self.tile_size})
        if (texpos+1) % 20 == 0 then
            for n=1,self.tile_size do
                table.insert(report, NEWLINE)
            end
        end
    end

    self.subviews.report:setText(report)
    self.subviews.main:updateLayout()
end

function TileBrowser:on_resize(body)
    local report_width = body.width - 15
    local prev_tile_size = self.tile_size
    self.tile_size = (report_width // 21) + 1
    if prev_tile_size ~= self.tile_size then
        self.dirty = true
    end
end

function TileBrowser:onRenderFrame()
    if self.dirty then
        self:update_report()
        self.dirty = false
    end
end

function TileBrowser:onDismiss()
    view = nil
end

view = view and view:raise() or TileBrowser{}:show()
