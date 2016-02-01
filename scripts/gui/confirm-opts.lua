-- confirm plugin options
--[[=begin

gui/confirm-opts
================
A basic configuration interface for the `confirm` plugin.

=end]]


confirm = require 'plugins.confirm'
gui = require 'gui'

Opts = defclass(Opts, gui.FramedScreen)
Opts.ATTRS = {
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = 'Confirmation dialogs',
    frame_width = 32,
    frame_height = 20,
    frame_inset = 1,
    focus_path = 'confirm/opts',
}

function Opts:init()
    self:refresh()
    self.cursor = 1
    local active_id = confirm.get_active_id()
    for i, c in pairs(self.data) do
        if c.id == active_id then
            self.cursor = i
            break
        end
    end
end

function Opts:refresh()
    self.data = confirm.get_conf_data()
    self.frame_height = #self.data
end

function Opts:onRenderBody(p)
    for i, c in pairs(self.data) do
        local highlight = (i == self.cursor and 8 or 0)
        p:pen(COLOR_GREY + highlight)
        p:string(c.id .. ': ')
        p:pen((c.enabled and COLOR_GREEN or COLOR_RED) + highlight)
        p:string(c.enabled and 'Enabled' or 'Disabled')
        p:newline()
    end
end

function Opts:onInput(keys)
    local conf = self.data[self.cursor]
    if keys.LEAVESCREEN then
        self:dismiss()
    elseif keys.SELECT then
        confirm.set_conf_state(conf.id, not conf.enabled)
        self:refresh()
    elseif keys.SEC_SELECT then
        for _, c in pairs(self.data) do
            confirm.set_conf_state(c.id, not conf.enabled)
        end
        self:refresh()
    elseif keys.STANDARDSCROLL_UP or keys.STANDARDSCROLL_DOWN then
        self.cursor = self.cursor + (keys.STANDARDSCROLL_UP and -1 or 1)
        if self.cursor < 1 then
            self.cursor = #self.data
        elseif self.cursor > #self.data then
            self.cursor = 1
        end
    end
end

Opts():show()
