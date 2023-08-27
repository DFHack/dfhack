--- Simple menu to quickly execute common commands.
--[[
Copyright (c) 2014, Michon van Dooren
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the {organization} nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
]]

local dlg = require('gui.dialogs')
local json = require('json')
local gui = require('gui')
local widgets = require('gui.widgets')

local CONFIG_FILE = 'dfhack-config/quickcmd.json'
local HOTKEYWIDTH = 7
local HOTKEYS = 'asdfghjklqwertyuiopzxcvbnm'

local function load_commands()
    local ok, data = pcall(json.decode_file, CONFIG_FILE)
    return ok and data or {}
end

local function save_commands(data)
    json.encode_file(data, CONFIG_FILE)
end

QCMDDialog = defclass(QCMDDialog, widgets.Window)
QCMDDialog.ATTRS {
    frame_title='Quick Command',
    frame={w=40, h=28},
    resizable=true,
    resize_min={h=10},
}

function QCMDDialog:init(info)
    self.commands = load_commands()

    self:addviews{
        widgets.Label{
            frame={t=0},
            text={{text='Hotkey', width=HOTKEYWIDTH}, ' Command'},
            visible=function() return #self.commands > 0 end,
        },
        widgets.List{
            view_id='list',
            frame={t=2, b=3},
            on_submit=self:callback('submit'),
        },
        widgets.Label{
            frame={t=0},
            text={'Command list is empty.', NEWLINE, 'Hit "A" to add one!'},
            visible=function() return #self.commands == 0 end,
        },
        widgets.Label{
            frame={b=0, h=2},
            text={
                {key='CUSTOM_SHIFT_A', text=': Add command',
                 on_activate=self:callback('onAddCommand')}, ' ',
                {key='CUSTOM_SHIFT_D', text=': Delete command',
                 on_activate=self:callback('onDelCommand')}, NEWLINE,
                {key='CUSTOM_SHIFT_E', text=': Edit command',
                 on_activate=self:callback('onEditCommand')},
            },
        },
    }

    self:updateList()
end

function QCMDDialog:submit(idx, choice)
    local screen = self.parent_view
    local parent = screen._native.parent
    dfhack.screen.hideGuard(screen, function()
        dfhack.run_command(choice.command)
    end)
    screen:dismiss()
end

function QCMDDialog:updateList()
    -- Build the list entries.
    local choices = {}
    for i,command in ipairs(self.commands) do
        -- Get the hotkey for this entry.
        local hotkey = nil
        if i <= HOTKEYS:len() then
            hotkey = HOTKEYS:sub(i, i)
        end

        -- Store the entry.
        table.insert(choices, {
            text={{text=hotkey or '', width=HOTKEYWIDTH}, ' ', command},
            command=command,
            hotkey=hotkey and ('CUSTOM_' .. hotkey:upper()) or '',
        })
    end
    self.subviews.list:setChoices(choices);
end

function QCMDDialog:onInput(keys)
    -- If the pressed key is a hotkey, perform that command and close.
    for idx,choice in ipairs(self.subviews.list:getChoices()) do
        if keys[choice.hotkey] then
            self:submit(idx, choice)
            return true
        end
    end

    -- Else, let the parent handle it.
    return QCMDDialog.super.onInput(self, keys)
end

function QCMDDialog:onAddCommand()
    dlg.showInputPrompt(
        'Add command',
        'Enter new command:',
        COLOR_GREEN,
        '',
        function(command)
            table.insert(self.commands, command)
            save_commands(self.commands)
            self:updateList()
        end
    )
end

function QCMDDialog:onDelCommand()
    -- Get the selected command.
    local index, item = self.subviews.list:getSelected()
    if not item then
        return
    end

    -- Prompt for confirmation.
    dlg.showYesNoPrompt(
        'Delete command',
        'Are you sure you want to delete this command: ' .. NEWLINE .. item.command,
        COLOR_GREEN,
        function()
            table.remove(self.commands, index)
            save_commands(self.commands)
            self:updateList()
        end
    )
end

function QCMDDialog:onEditCommand()
    -- Get the selected command.
    local index, item = self.subviews.list:getSelected()
    if not item then
        return
    end

    -- Prompt for new value.
    dlg.showInputPrompt(
        'Edit command',
        'Enter command:',
        COLOR_GREEN,
        item.command,
        function(command)
            self.commands[index] = command
            save_commands(self.commands)
            self:updateList()
        end
    )
end

QCMDScreen = defclass(QCMDScreen, gui.ZScreen)
QCMDScreen.ATTRS {
    focus_path='quickcmd',
}

function QCMDScreen:init()
    self:addviews{QCMDDialog{}}
end

function QCMDScreen:onDismiss()
    view = nil
end

view = view and view:raise() or QCMDScreen{}:show()
