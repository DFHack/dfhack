-- An in-game CP437 table

local dialog = require('gui.dialogs')
local gui = require('gui')
local widgets = require('gui.widgets')

local to_pen = dfhack.pen.parse

local tb_texpos = dfhack.textures.getThinBordersTexposStart()
local tp = function(offset)
    if tb_texpos == -1 then return nil end
    return tb_texpos + offset
end

local function get_key_pens(ch)
    return {
        lt=to_pen{tile=tp(0), write_to_lower=true},
        t=to_pen{tile=tp(1), ch=ch, write_to_lower=true, top_of_text=true},
        t_ascii=to_pen{ch=32},
        rt=to_pen{tile=tp(2), write_to_lower=true},
        lb=to_pen{tile=tp(14), write_to_lower=true},
        b=to_pen{tile=tp(15), ch=ch, write_to_lower=true, bottom_of_text=true},
        rb=to_pen{tile=tp(16), write_to_lower=true},
    }
end

local function get_key_hover_pens(ch)
    return {
        t=to_pen{tile=tp(1), fg=COLOR_WHITE, bg=COLOR_RED, ch=ch, write_to_lower=true, top_of_text=true},
        t_ascii=to_pen{fg=COLOR_WHITE, bg=COLOR_RED, ch=ch == 0 and 0 or 32},
        b=to_pen{tile=tp(15), fg=COLOR_WHITE, bg=COLOR_RED, ch=ch, write_to_lower=true, bottom_of_text=true},
    }
end

CPDialog = defclass(CPDialog, widgets.Window)
CPDialog.ATTRS {
    frame_title='CP437 table',
    frame={w=100, h=26},
}

function CPDialog:init(info)
    self:addviews{
        widgets.EditField{
            view_id='edit',
            frame={t=0, l=0},
        },
        widgets.Panel{
            view_id='board',
            frame={t=2, l=0, w=96, h=18},
        },
        widgets.Label{
            frame={b=2, l=0},
            text='Click characters or type',
        },
        widgets.HotkeyLabel{
            frame={b=0, l=0},
            key='SELECT',
            label='Send text to parent',
            auto_width=true,
            on_activate=self:callback('submit'),
        },
        widgets.HotkeyLabel{
            frame={b=0},
            key='STRING_A000',
            key_sep='',
            label='Click here to Backspace',
            auto_width=true,
            on_activate=function() self.subviews.edit:onInput{_STRING=0} end,
        },
        widgets.HotkeyLabel{
            frame={b=0, r=0},
            key='LEAVESCREEN',
            label='Cancel',
            auto_width=true,
            on_activate=function() self.parent_view:dismiss() end,
        },
    }

    local board = self.subviews.board
    local edit = self.subviews.edit
    for ch = 0,255 do
        local xoff, yoff = (ch%32) * 3, (ch//32) * 2
        if not dfhack.screen.charToKey(ch) then ch = 0 end
        local pens = get_key_pens(ch)
        local hpens = get_key_hover_pens(ch)
        local function get_top_tile()
            return dfhack.screen.inGraphicsMode() and pens.t or pens.t_ascii
        end
        local function get_top_htile()
            return dfhack.screen.inGraphicsMode() and hpens.t or hpens.t_ascii
        end
        board:addviews{
            widgets.Label{
                frame={t=yoff, l=xoff, w=3, h=2},
                auto_height=false,
                text={
                    {tile=pens.lt},
                    {tile=get_top_tile, htile=get_top_htile},
                    {tile=pens.rt},
                    NEWLINE,
                    {tile=pens.lb},
                    {tile=pens.b, htile=hpens.b},
                    {tile=pens.rb},
                },
                on_click=function() if ch ~= 0 then edit:insert(string.char(ch)) end end,
            },
        }
    end
end

function CPDialog:submit()
    local keys = {}
    local text = self.subviews.edit.text
    for i = 1,#text do
        local k = dfhack.screen.charToKey(string.byte(text:sub(i, i)))
        if not k then
            dialog.showMessage('Error',
                ('Invalid character at position %d: "%s"'):
                    format(i, text:sub(i, i)),
                COLOR_LIGHTRED)
            return
        end
        keys[i] = k
    end

    -- ensure clicks on "submit" don't bleed through
    df.global.enabler.mouse_lbut = 0
    df.global.enabler.mouse_lbut_down = 0

    local screen = self.parent_view
    local parent = screen._native.parent
    dfhack.screen.hideGuard(screen, function()
        for i, k in pairs(keys) do
            gui.simulateInput(parent, k)
        end
    end)
    screen:dismiss()
end

CPScreen = defclass(CPScreen, gui.ZScreen)
CPScreen.ATTRS {
    focus_path='cp437-table',
}

function CPScreen:init()
    self:addviews{CPDialog{}}
end

function CPScreen:onDismiss()
    view = nil
end

view = view and view:raise() or CPScreen{}:show()
