-- A replacement for the "load game" screen
--@ enable = true

VERSION = '0.8.5'

function usage()
    print(dfhack.script_help())
end

local gui = require 'gui'
local dialog = require 'gui.dialogs'
local widgets = require 'gui.widgets'
local save_version = reqscript 'devel/save-version'

function showError(title, text)
    if text == nil then title, text = 'Error', title end
    dialog.showMessage(title, text, COLOR_LIGHTRED)
end

function keyStringLength(key, str)
    return #(gui.getKeyDisplay(df.interface_key[key]) .. ": " .. str)
end
function paintKeyString(x, y, key, str, opts)
    opts = opts or {}
    local key_str = gui.getKeyDisplay(df.interface_key[key])
    paintString(opts.key_color or COLOR_LIGHTRED, x, y, key_str)
    paintString(COLOR_WHITE, x + #key_str, y, ": " .. str)
end

function gametypeString(gametype, overrides)
    overrides = overrides or {}
    if overrides[df.game_type[gametype]] then return overrides[df.game_type[gametype]] end
    if gametype == df.game_type.DWARF_MAIN then
        return "Fortress mode"
    elseif gametype == df.game_type.DWARF_RECLAIM then
        return "Reclaim fortress"
    elseif gametype == df.game_type.DWARF_UNRETIRE then
        return "Unretire fortress"
    elseif gametype == df.game_type.ADVENTURE_MAIN then
        return "Adventure mode"
    elseif gametype == df.game_type.DWARF_ARENA then
        return "Arena (fortress)"
    elseif gametype == df.game_type.ADVENTURE_ARENA then
        return "Arena (adventurer)"
    elseif gametype == df.game_type.NONE then
        return "None"
    else
        return "Unknown mode"
    end
end
gametypeMap = (function()
    local gametypes = {'NONE', 'DWARF_MAIN', 'DWARF_RECLAIM',  'DWARF_UNRETIRE', 'ADVENTURE_MAIN'}
    local ret = {}
    for i, t in pairs(gametypes) do
        ret[t] = gametypes[i + 1] or gametypes[1]
    end
    return ret
end)()

function copy_table(t)
    local t2 = {}
    for k, v in pairs(t) do
        t2[k] = v
    end
    return t2
end

paintString = dfhack.screen.paintString
function paintStringCenter(pen, y, str)
    if tonumber(pen) ~= nil then
        pen = math.max(0, math.min(15, pen))
        pen = {ch=' ', fg=pen}
    end
    local cols, rows = dfhack.screen.getWindowSize()
    paintString(pen, math.floor((cols - #str) / 2), y, str)
end

load_screen = defclass(load_screen, gui.Screen)
load_screen.focus_path = 'load_screen'

function load_screen:init()
    self.saves = nil
    self.backup_opts = {[0] = "No backups", "Backups visible", "Backups only"}
    self.old_fps = df.global.gps.display_frames
    df.global.gps.display_frames = 0
    self:reset()
end

function load_screen:reset()
    self.sel_idx = 1
    self.opts = {
        backups = 0,
        filter = '',
        filter_mode = df.game_type.NONE,
    }
    self.search_active = false
end

function load_screen:is_backup(folder_name)
    if folder_name:match('%-%d%d%d%d%d%-%d%d%-%d%d$') ~= nil then
        return true
    end
    return false
end

function load_screen:init_saves()
    self.saves = {}
    local parent_saves = self._native.parent.saves
    for i = 0, #parent_saves - 1 do
        table.insert(self.saves, parent_saves[i])
    end
end

function load_screen:get_saves()
    if not self.saves then self:init_saves() end
    local saves = {}
    for i = 1, #self.saves do
        save = self.saves[i]
        if (self:is_backup(save.folder_name) and self.opts.backups == 0) or
            (not self:is_backup(save.folder_name) and self.opts.backups == 2) or
            (#self.opts.filter and not save.folder_name:lower():find(self.opts.filter:lower(), 1, true)) or
            (self.opts.filter_mode ~= df.game_type.NONE and self.opts.filter_mode ~= save.game_type) then
            --pass
        else
            table.insert(saves, save)
        end
    end
    return saves
end

function load_screen:visible_save_bounds()
    local saves = self:get_saves()
    local cols, rows = dfhack.screen.getWindowSize()
    local max_rows = math.floor((rows - 5) / 2)
    local min = self.sel_idx - math.floor(max_rows / 2)
    local max = self.sel_idx + math.ceil(max_rows / 2)
    local d
    if max > #saves then
        d = max - #saves
        max = max - d
        min = min - d
    end
    if min < 1 then
        d = 1 - min
        min = min + d
        max = max + d
    end
    max = math.min(max, #saves)
    return min, max
end
function load_screen:visible_save_count()
    local min, max = self:visible_save_bounds()
    return max - min + 1
end

function load_screen:draw_scrollbar()
    local cols, rows = dfhack.screen.getWindowSize()
    local x = cols - 1
    local y1, y2
    local min, max = self:visible_save_bounds()
    local saves = self:get_saves()
    local pen = {fg = COLOR_CYAN, bg = COLOR_LIGHTGREEN}
    local r_pen = {fg = pen.bg, bg = pen.fg}
    if #saves > max - min + 1 then
        paintString(r_pen, x, 0, (min > 1 and string.char(24)) or ' ')  -- up arrow
        paintString(r_pen, x, rows - 1, (max < #saves and string.char(25)) or ' ')  -- down arrow
        y1 = ((min - 1) / #saves) * (rows - 2) + 1
        y2 = (max / #saves) * (rows - 2) + 1
        for y = 1, rows - 2 do
            if y >= y1 and y <= y2 then
                paintString(pen, x, y, string.char(8))
            else
                paintString(COLOR_CYAN, x, y, string.char(179))
            end
        end
    end
end

function load_screen:onRender()
    local pen = {ch=' ', fg=COLOR_GREY}
    local saves = self:get_saves()
    self.sel_idx = math.max(1, math.min(#saves, self.sel_idx))
    dfhack.screen.clear()
    local cols, rows = dfhack.screen.getWindowSize()
    local min, max = self:visible_save_bounds()
    paintStringCenter(pen, 0, "Load game (DFHack)")
    paintString(pen, cols - #VERSION - 3, 0, "v" .. VERSION)
    local y = 0
    local max_x = 77
    for i = min, max do
        local save = saves[i]
        pen.fg = COLOR_GREY
        if self:is_backup(save.folder_name) then pen.fg = COLOR_RED end
        if save.game_type == df.game_type.DWARF_RECLAIM then
            pen.fg = COLOR_MAGENTA
        elseif save.game_type == df.game_type.DWARF_UNRETIRE then
            pen.fg = COLOR_GREEN
        elseif save.game_type == df.game_type.ADVENTURE_MAIN then
            pen.fg = COLOR_CYAN
        end
        if i == self.sel_idx then
            pen.fg = pen.fg + 8
        end
        pen.bg = (i == self.sel_idx and COLOR_BLUE) or COLOR_BLACK

        y = y + 2
        local year = tostring(save.year)
        dfhack.screen.fillRect(pen, 2, y, max_x, y + 1)
        paintString(pen, 2, y, save.fort_name .. (#save.fort_name > 0 and " - " or "") .. gametypeString(save.game_type))
        paintString(pen, max_x - #save.world_name, y, save.world_name)
        paintString(pen, 3, y + 1, "Folder: " .. save.folder_name)
        paintString(pen, max_x - #year, y + 1, year)
    end
    self:draw_scrollbar()
    if #saves == 0 then
        paintString(COLOR_WHITE, 1, 3, "No results found")
        paintKeyString(1, 5, "CUSTOM_ALT_C",
            "Clear " .. (self.search_active and "search" or "filters"))
    end
    local label = self.opts.filter
    if #label > 20 then
        label = '\027' .. label:sub(-20 + 1)
    end
    if self.search_active then
        paintKeyString(1, rows - 1, 'CUSTOM_S', label, {key_color = COLOR_RED})
        local x = keyStringLength('CUSTOM_S', label) + 1
        paintString(COLOR_LIGHTGREEN, x, rows - 1, '_')
    else
        paintKeyString(1, rows - 1, 'CUSTOM_S', #label > 0 and label or "Search")
    end
    paintKeyString(27, rows - 1, 'CUSTOM_T', gametypeString(self.opts.filter_mode, {NONE = "Any mode"}))
    paintKeyString(52, rows - 1, 'CUSTOM_B', self.backup_opts[self.opts.backups])
end

function load_screen:onIdle()
    self.text_input_mode = self.search_active
end

function load_screen:onInput(keys)
    if keys._MOUSE_L_DOWN then
        return self:onMouseInput(df.global.gps.mouse_x, df.global.gps.mouse_y)
    end
    if self.search_active then
        if keys.LEAVESCREEN then
            self.search_active = false
            self.opts.filter = ''
        elseif keys.SELECT then
            self.search_active = false
        elseif keys.STRING_A000 then
            self.opts.filter = self.opts.filter:sub(0, -2)
        elseif keys._STRING then
            self.opts.filter = self.opts.filter .. string.char(keys._STRING)
        elseif keys.STANDARDSCROLL_DOWN or keys.STANDARDSCROLL_UP or
                keys.STANDARDSCROLL_PAGEDOWN or keys.STANDARDSCROLL_PAGEUP then
            self.search_active = false
            self:onInput(keys)
        elseif keys.CUSTOM_ALT_C then
            self.opts.filter = ''
        end
        return
    end
    if keys.LEAVESCREEN then
        self:dismiss()
        dfhack.screen.dismiss(self._native.parent)
    elseif keys.SELECT then
        load_screen_options:display(self, self:get_saves()[self.sel_idx])
    elseif keys.STANDARDSCROLL_DOWN then
        self:scroll(1)
    elseif keys.STANDARDSCROLL_UP then
        self:scroll(-1)
    elseif keys.STANDARDSCROLL_PAGEDOWN then
        self:scroll(self:visible_save_count())
    elseif keys.STANDARDSCROLL_PAGEUP then
        self:scroll(-self:visible_save_count())
    elseif keys.CUSTOM_B then
        self.opts.backups = self.opts.backups + 1
        if self.opts.backups > 2 then self.opts.backups = 0 end
    elseif keys.CUSTOM_S then
        self.search_active = true
    elseif keys.CUSTOM_T then
        self.opts.filter_mode = df.game_type[gametypeMap[df.game_type[self.opts.filter_mode]]]
    elseif keys.CUSTOM_ALT_C then
        self:reset()
    end
end

function load_screen:onMouseInput(x, y)
    local cols, rows = dfhack.screen.getWindowSize()
    if y == rows - 1 then
        if x <= 26 then
            self.search_active = true
        else
            self.search_active = false
            if x <= 51 then
                self:onInput({CUSTOM_T = true})
            else
                self:onInput({CUSTOM_B = true})
            end
        end
    end
end

function load_screen:scroll(dist)
    local old_idx = self.sel_idx
    self.sel_idx = self.sel_idx + dist
    local saves = self:get_saves()
    if self.sel_idx > #saves then
        if old_idx == #saves then
            self.sel_idx = 1
        else
            self.sel_idx = #saves
        end
    elseif self.sel_idx < 1 then
        if old_idx == 1 then
            self.sel_idx = #saves
        else
            self.sel_idx = 1
        end
    end
end

function load_screen:load_game(folder_name)
    if not folder_name then return false end
    local parent = self._native.parent
    if #parent.saves < 1 then return false end
    parent.sel_idx = 0
    parent.saves[0].folder_name = folder_name
    self:dismiss()
    gui.simulateInput(parent, {df.interface_key.SELECT})
    return true
end

function load_screen:onDismiss()
    df.global.gps.display_frames = self.old_fps
end

load_screen_options = gui.FramedScreen{
    frame_width = 42,
    frame_height = 6,
    frame_title = "",
    frame_inset = 1,
}

function load_screen_options:onRenderBody(p)
    p:key_pen(COLOR_LIGHTRED)
    if self.loading == true then
        self.loading = false
        self.parent:load_game(self.save.folder_name)
        self:dismiss()
        return
    end
    p:string(self.last_saved_message):newline()
    p:string('Last DF Version: ' .. self.save_version):newline()
    p:key_string('CUSTOM_R', 'Rename'):newline()
    -- p:key_string('CUSTOM_C', 'Copy')
    p:seek(0, self.frame_height - 1)
    p:key_string('LEAVESCREEN', 'Cancel')
    p:seek(self.frame_width - keyStringLength('SELECT', 'Play now'), self.frame_height - 1)
    p:key_string('SELECT', 'Play now')
end

function load_screen_options:onInput(keys)
    if keys.LEAVESCREEN then
        self:dismiss()
    elseif keys.SELECT then
        self.loading = true
    elseif keys.CUSTOM_R then
        self:dialog(
            'Rename "' .. self.save.folder_name .. '"',
            "New folder name:",
            self.save.folder_name,
            'do_rename'
        )
    elseif keys.CUSTOM_C then
        self:dialog(
            'Copy "' .. self.save.folder_name .. '"',
            "New folder name:",
            self.save.folder_name .. '-copy',
            'do_copy'
        )
    end
end

function load_screen_options:refresh()
    self.frame_title = "Load game: " .. self.save.folder_name
    self.frame_width = math.min(df.global.gps.dimx, math.max(40, #self.frame_title + 4, #self.last_saved_message))
end

function load_screen_options:dialog(title, text, input, callback)
    dialog.InputBox{
        frame_title = title,
        text = text,
        text_pen = COLOR_WHITE,
        input = input,
        on_input = self:callback(callback),
        frame_width = self.frame_width - 2,
        frame_height = self.frame_height,
    }:show()
end

function load_screen_options:validate_folders(old_folder, new_folder)
    if old_folder == new_folder then
        -- no change - treat as ''esc', fail silently
        return false
    end
    if (old_folder .. new_folder):match('[/\\:]') ~= nil then
        showError('Invalid path!')
        return false
    end
    local old_path = 'save/' .. old_folder
    local new_path = 'save/' .. new_folder
    if not dfhack.filesystem.isdir(old_path) then
        showError('Folder missing', 'Cannot find ' .. old_path)
        return false
    end
    if dfhack.filesystem.exists(new_path) then
        showError('Destination folder exists', new_path .. ' already exists!')
        return false
    end
    return true, old_path, new_path
end

function load_screen_options:do_rename(new_folder)
    local ok, old_path, new_path = self:validate_folders(self.save.folder_name, new_folder)
    if not ok then return false end
    if not os.rename(old_path, new_path) then
        showError('Rename failed!')
        return
    end
    self.save.folder_name = new_folder
    self:refresh()
    return true
end

function load_screen_options:do_copy(new_folder)
    local ok, old_path, new_path = self:validate_folders(self.save.folder_name, new_folder)
    if not ok then return false end
    showError('Not implemented')
end

function load_screen_options:display(parent, save)
    if not save then return end
    self.parent = parent
    self.save = save

    local world_sav_path = 'save/' .. save.folder_name .. '/world.sav'
    self.save_mtime = os.date('%c', dfhack.filesystem.mtime(world_sav_path))
    self.last_saved_message = 'Last saved: ' .. self.save_mtime
    local f = io.open(world_sav_path, 'rb')
    if f then
        local v_raw = {f:read(4):byte(1, 4)}
        f:close()
        local v = v_raw[1] + (1 << 8) * v_raw[2] + (1 << 16) * v_raw[3] + (1 << 24) * v_raw[4]
        self.save_version = save_version.versions[v] or 'unknown'
    else
        self.save_version = 'error'
    end

    self:refresh()
    self:show()
end

focus_stack = focus_stack or {'', ''}
function state_change_handler(evt)
    if evt ~= SC_VIEWSCREEN_CHANGED then return end
    table.remove(focus_stack, 1)
    table.insert(focus_stack, dfhack.gui.getCurFocus())
    if enabled and focus_stack[1] == 'title' and focus_stack[2] == 'loadgame' then
        mkscreen()
    end
end

function init()
    dfhack.onStateChange.load_screen = state_change_handler
    -- populate focus_stack
    state_change_handler(SC_VIEWSCREEN_CHANGED)
end

function mkscreen()
    local parent = dfhack.gui.getCurViewscreen()
    -- Create this screen only above a viewscreen_loadgamest that isn't loading a game
    if df.viewscreen_loadgamest:is_instance(parent) and parent.loading == 0 then
        scr = load_screen()
        scr:show()
    end
end

if initialized == nil then
    local major_version = tonumber(dfhack.getDFVersion():split('.', true)[2])
    if not major_version or major_version < 40 then
        qerror('gui/load-screen only supports DF 0.40.xx and above')
    end
    init()
    initialized = true
    enabled = false
end

args = {...}
if dfhack_flags and dfhack_flags.enable then
    table.insert(args, dfhack_flags.enable_state and 'enable' or 'disable')
end
if #args == 1 then
    if args[1] == 'enable' then
        enabled = true
        if dfhack.gui.getCurFocus() == 'loadgame' then
            mkscreen()
        end
    elseif args[1] == 'disable' then enabled = false
    elseif args[1] == 'version' then print('load-screen version ' .. VERSION)
    else usage()
    end
else usage()
end
