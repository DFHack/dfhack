-- The DFHack in-game command launcher
--@module=true

local dialogs = require('gui.dialogs')
local gui = require('gui')
local helpdb = require('helpdb')
local json = require('json')
local utils = require('utils')
local widgets = require('gui.widgets')

local AUTOCOMPLETE_PANEL_WIDTH = 25
local EDIT_PANEL_HEIGHT = 4

local HISTORY_SIZE = 5000
local HISTORY_ID = 'gui/launcher'
local HISTORY_FILE = 'dfhack-config/launcher.history'
local CONSOLE_HISTORY_FILE = 'dfhack-config/dfhack.history'
local CONSOLE_HISTORY_FILE_OLD = 'dfhack.history'
local TITLE = 'DFHack Launcher'

-- this size chosen since it's reasonably large and it also keeps the latency
-- within 1s when adding text to a full scrollback buffer
local SCROLLBACK_CHARS = 2^18

config = config or json.open('dfhack-config/launcher.json')
base_freq = base_freq or json.open('hack/data/base_command_counts.json')
user_freq = user_freq or json.open('dfhack-config/command_counts.json')

-- track whether the user has enabled dev mode
dev_mode = dev_mode or false

-- trims the history down to its maximum size, if needed
local function trim_history(hist, hist_set)
    if #hist <= HISTORY_SIZE then return end
    -- we can only ever go over by one, so no need to loop
    -- This is O(N) in the HISTORY_SIZE. if we need to make this more efficient,
    -- we can use a ring buffer.
    local line = table.remove(hist, 1)
    -- since all lines are guaranteed to be unique, we can just remove the hash
    -- from the set instead of, say, decrementing a counter
    hist_set[line] = nil
end

-- removes duplicate existing history lines and adds the given line to the front
local function add_history(hist, hist_set, line)
    if hist_set[line] then
        for i,v in ipairs(hist) do
            if v == line then
                table.remove(hist, i)
                break
            end
        end
    end
    table.insert(hist, line)
    hist_set[line] = true
    trim_history(hist, hist_set)
end

local function file_exists(fname)
    return dfhack.filesystem.mtime(fname) ~= -1
end

-- history files are written with the most recent entry on *top*, which the
-- opposite of what we want. add the file contents to our history in reverse.
local function add_history_lines(lines, hist, hist_set)
    for i=#lines,1,-1 do
        add_history(hist, hist_set, lines[i])
    end
end

local function add_history_file(fname, hist, hist_set)
    if not file_exists(fname) then
        return
    end
    local lines = {}
    for line in io.lines(fname) do
        table.insert(lines, line)
    end
    add_history_lines(lines, hist, hist_set)
end

local function init_history()
    local hist, hist_set = {}, {}
    -- snarf the console history into our active history. it would be better if
    -- both the launcher and the console were using the same history object so
    -- the sharing would be "live", but we can address that later.
    add_history_file(CONSOLE_HISTORY_FILE_OLD, hist, hist_set)
    add_history_file(CONSOLE_HISTORY_FILE, hist, hist_set)

    -- read in our own command history
    add_history_lines(dfhack.getCommandHistory(HISTORY_ID, HISTORY_FILE),
                      hist, hist_set)

    return hist, hist_set
end

if not history then
    history, history_set = init_history()
end

local function get_first_word(text)
    local word = text:trim():split(' +')[1]
    if word:startswith(':') then word = word:sub(2) end
    return word
end

local function get_command_count(command)
    return (base_freq.data[command] or 0) + (user_freq.data[command] or 0)
end

local function record_command(line)
    add_history(history, history_set, line)
    local firstword = get_first_word(line)
    user_freq.data[firstword] = (user_freq.data[firstword] or 0) + 1
    user_freq:write()
end

----------------------------------
-- AutocompletePanel
--
AutocompletePanel = defclass(AutocompletePanel, widgets.Panel)
AutocompletePanel.ATTRS{
    on_autocomplete=DEFAULT_NIL,
    on_double_click=DEFAULT_NIL,
    on_double_click2=DEFAULT_NIL,
}

function AutocompletePanel:init()
    self:addviews{
        widgets.Label{
            frame={l=0, t=0},
            text='Click or select via'
        },
        widgets.Label{
            frame={l=1, t=1},
            text={{text='Shift+Left', pen=COLOR_LIGHTGREEN},
                  {text='/'},
                  {text='Shift+Right', pen=COLOR_LIGHTGREEN}}
        },
        widgets.Label{
            frame={l=0, t=3},
            text='Showing:',
        },
        widgets.Label{
            view_id="autocomplete_label",
            frame={l=9, t=3},
            text='All scripts'
        },
        widgets.List{
            view_id='autocomplete_list',
            scroll_keys={},
            on_select=self:callback('on_list_select'),
            on_double_click=self.on_double_click,
            on_double_click2=self.on_double_click2,
            frame={l=0, r=0, t=5, b=0}},
    }
end

function AutocompletePanel:set_options(options, initially_selected)
    local list = self.subviews.autocomplete_list
    -- disable on_select while we reset the options so we don't automatically
    -- trigger the callback
    list.on_select = nil
    list:setChoices(options, 1)
    list.on_select = self:callback('on_list_select')
    list.cursor_pen = initially_selected and COLOR_LIGHTCYAN or COLOR_CYAN
    self.first_advance = not initially_selected
end

function AutocompletePanel:advance(delta)
    local list = self.subviews.autocomplete_list
    if self.first_advance then
        if list.cursor_pen == COLOR_CYAN and delta > 0 then
            delta = 0
        end
        self.first_advance = false
    end
    list.cursor_pen = COLOR_LIGHTCYAN -- enable highlight
    list:moveCursor(delta, true)
end

function AutocompletePanel:on_list_select(idx, option)
    -- enable highlight
    self.subviews.autocomplete_list.cursor_pen = COLOR_LIGHTCYAN
    self.first_advance = false
    if self.on_autocomplete then self.on_autocomplete(idx, option) end
end

----------------------------------
-- EditPanel
--
EditPanel = defclass(EditPanel, widgets.Panel)
EditPanel.ATTRS{
    on_change=DEFAULT_NIL,
    on_submit=DEFAULT_NIL,
    on_submit2=DEFAULT_NIL,
    on_toggle_minimal=DEFAULT_NIL,
    prefix_visible=DEFAULT_NIL,
}

function EditPanel:init()
    self.stack = {}
    self:reset_history_idx()

    self:addviews{
        widgets.Label{
            view_id='prefix',
            frame={l=0, t=0},
            text='[DFHack]#',
            visible=self.prefix_visible},
        widgets.EditField{
            view_id='editfield',
            frame={l=1, t=1, r=1},
            -- ignore the backtick from the hotkey. otherwise if it is still
            -- held down as the launcher appears, it will be read and be added
            -- to the commandline
            ignore_keys={'STRING_A096'},
            on_char=function(ch, text)
                if ch == ' ' then return text:match('%S$') end
                return true
            end,
            on_change=self.on_change,
            on_submit=self.on_submit,
            on_submit2=self.on_submit2},
        widgets.HotkeyLabel{
            frame={l=1, t=3, w=10},
            key='SELECT',
            label='run',
            on_activate=function()
                if dfhack.internal.getModifiers().shift then
                    self.on_submit2(self.subviews.editfield.text)
                else
                    self.on_submit(self.subviews.editfield.text)
                end
                end},
        widgets.HotkeyLabel{
            frame={r=0, t=0, w=10},
            key='CUSTOM_ALT_M',
            label=string.char(31)..string.char(30),
            on_activate=self.on_toggle_minimal},
        widgets.EditField{
            view_id='search',
            frame={l=13, t=3, r=1},
            key='CUSTOM_ALT_S',
            label_text='history search: ',
            on_change=function(text) self:on_search_text(text) end,
            on_focus=function()
                local text = self.subviews.editfield.text
                if #text:trim() > 0 then
                    self.subviews.search:setText(text)
                    self:on_search_text(text)
                end end,
            on_unfocus=function()
                self.subviews.search:setText('')
                self.subviews.editfield:setFocus(true) end,
            on_submit=function()
                self.on_submit(self.subviews.editfield.text) end,
            on_submit2=function()
                self.on_submit2(self.subviews.editfield.text) end},
    }
end

function EditPanel:reset_history_idx()
    self.history_idx = #history + 1
end

function EditPanel:set_text(text, inhibit_change_callback)
    local edit = self.subviews.editfield
    if inhibit_change_callback then
        edit.on_change = nil
    end
    edit:setText(text)
    edit.on_change = self.on_change
    self:reset_history_idx()
end

function EditPanel:move_history(delta)
    local history_idx = self.history_idx + delta
    if history_idx < 1 or history_idx > #history + 1 or delta == 0 then
        return
    end
    local editfield = self.subviews.editfield
    if self.history_idx == #history + 1 then
        -- we're moving off the initial buffer. save it so we can get it back.
        self.saved_buffer = editfield.text
    end
    self.history_idx = history_idx
    local text
    if history_idx == #history + 1 then
        -- we're moving onto the initial buffer. restore it.
        text = self.saved_buffer
    else
        text = history[history_idx]
    end
    editfield:setText(text)
    self.on_change(text)
end

function EditPanel:on_search_text(search_str, next_match)
    if not search_str or #search_str == 0 then return end
    local start_idx = math.min(self.history_idx - (next_match and 1 or 0),
                               #history)
    for history_idx = start_idx, 1, -1 do
        if history[history_idx]:find(search_str, 1, true) then
            self:move_history(history_idx - self.history_idx)
            return
        end
    end
    -- no matches. restart at the saved input buffer for the next search.
    self:move_history(#history + 1 - self.history_idx)
end

function EditPanel:onInput(keys)
    if EditPanel.super.onInput(self, keys) then return true end

    if keys.STANDARDSCROLL_UP then
        self:move_history(-1)
        return true
    elseif keys.STANDARDSCROLL_DOWN then
        self:move_history(1)
        return true
    elseif keys.CUSTOM_ALT_S then
        -- search to the next match with the current search string
        -- only reaches here if the search field is already active
        self:on_search_text(self.subviews.search.text, true)
        return true
    end
end

----------------------------------
-- HelpPanel
--

HelpPanel = defclass(HelpPanel, widgets.Panel)
HelpPanel.ATTRS{
    autoarrange_subviews=true,
    autoarrange_gap=1,
    frame_inset={t=0, l=1, r=1, b=0},
}

-- this text is intentionally unwrapped so the in-UI wrapping can do the job
local DEFAULT_HELP_TEXT = [[Welcome to DFHack!

Type a command to see its help text here. Hit ENTER to run the command, or tap backtick (`) or hit ESC to close this dialog. This dialog also closes automatically if you run a command that shows a new GUI screen.

Not sure what to do? First, try running "quickstart-guide" to get oriented with DFHack and its capabilities. Then maybe try the "tags" command to see the different categories of tools DFHack has to offer! Run "tags <tagname>" (e.g. "tags design") to see the tools in that category.

To see help for this command launcher (including info on mouse controls), type "launcher" and click on "gui/launcher" to autocomplete.

You're running DFHack ]] .. dfhack.getDFHackVersion() ..
            (dfhack.isRelease() and '' or (' (git: %s)'):format(dfhack.getGitCommit(true)))

function HelpPanel:init()
    self.cur_entry = ''

    self:addviews{
        widgets.TabBar{
            frame={t=0, l=0},
            labels={
                'Help',
                'Output',
            },
            on_select=function(idx) self.subviews.pages:setSelected(idx) end,
            get_cur_page=function() return self.subviews.pages:getSelected() end,
        },
        widgets.Pages{
            view_id='pages',
            frame={t=2, l=0, b=0, r=0},
            subviews={
                widgets.WrappedLabel{
                    view_id='help_label',
                    auto_height=false,
                    scroll_keys={
                        KEYBOARD_CURSOR_UP_FAST=-1,  -- Shift-Up
                        KEYBOARD_CURSOR_DOWN_FAST=1, -- Shift-Down
                        STANDARDSCROLL_PAGEUP='-halfpage',
                        STANDARDSCROLL_PAGEDOWN='+halfpage',
                    },
                    text_to_wrap=DEFAULT_HELP_TEXT},
                widgets.WrappedLabel{
                    view_id='output_label',
                    auto_height=false,
                    scroll_keys={
                        KEYBOARD_CURSOR_UP_FAST=-1,  -- Shift-Up
                        KEYBOARD_CURSOR_DOWN_FAST=1, -- Shift-Down
                        STANDARDSCROLL_PAGEUP='-halfpage',
                        STANDARDSCROLL_PAGEDOWN='+halfpage',
                    },
                    text_to_wrap=''},
            },
        },
    }
end

local function HelpPanel_update_label(label, text)
    label.text_to_wrap = text
    label:postComputeFrame() -- wrap
    label:updateLayout() -- update the scroll arrows after rewrapping text
end

function HelpPanel:add_output(output)
    self.subviews.pages:setSelected('output_label')
    local label = self.subviews.output_label
    local text_height = label:getTextHeight()
    label:scroll('end')
    local line_num = label.start_line_num
    local text = output
    if label.text_to_wrap ~= '' then
        text = label.text_to_wrap .. NEWLINE .. output
    end
    local text_len = #text
    if text_len > SCROLLBACK_CHARS then
        text = text:sub(-SCROLLBACK_CHARS)
        local text_diff = text_len - #text
        HelpPanel_update_label(label, label.text_to_wrap:sub(text_len - #text))
        text_height = label:getTextHeight()
        label:scroll('end')
        line_num = label.start_line_num
    end
    HelpPanel_update_label(label, text)
    if line_num == 1 then
        label:scroll(text_height - 1)
    else
        label:scroll('home')
        label:scroll(line_num - 1)
        label:scroll('+page')
    end
end

function HelpPanel:set_entry(entry_name, show_help)
    if show_help then
        self.subviews.pages:setSelected('help_label')
    end
    local label = self.subviews.help_label
    if #entry_name == 0 then
        HelpPanel_update_label(label, DEFAULT_HELP_TEXT)
        self.cur_entry = ''
        return
    end
    if not helpdb.is_entry(entry_name) or entry_name == self.cur_entry then
        return
    end
    local wrapped_help = helpdb.get_entry_long_help(entry_name,
                                                    self.frame_body.width - 5)
    HelpPanel_update_label(label, wrapped_help)
    self.cur_entry = entry_name
end

function HelpPanel:postComputeFrame()
    if #self.cur_entry == 0 then return end
    local wrapped_help = helpdb.get_entry_long_help(self.cur_entry,
                                                    self.frame_body.width - 5)
    HelpPanel_update_label(self.subviews.help_label, wrapped_help)
end

----------------------------------
-- MainPanel
--

MainPanel = defclass(MainPanel, widgets.Window)
MainPanel.ATTRS{
    frame_title=TITLE,
    frame_inset=0,
    resizable=true,
    resize_min={w=AUTOCOMPLETE_PANEL_WIDTH+49, h=EDIT_PANEL_HEIGHT+20},
    get_minimal=DEFAULT_NIL,
    update_autocomplete=DEFAULT_NIL,
    on_edit_input=DEFAULT_NIL,
}

function MainPanel:postUpdateLayout()
    if self.get_minimal() then return end
    config:write(self.frame)
end

local texpos = dfhack.textures.getThinBordersTexposStart()
local tp = function(offset)
    if texpos == -1 then return nil end
    return texpos + offset
end

local H_SPLIT_PEN = dfhack.pen.parse{tile=tp(5), ch=196, fg=COLOR_GREY, bg=COLOR_BLACK}
local V_SPLIT_PEN = dfhack.pen.parse{tile=tp(4), ch=179, fg=COLOR_GREY, bg=COLOR_BLACK}
local TOP_SPLIT_PEN = dfhack.pen.parse{tile=gui.WINDOW_FRAME.t_frame_pen.tile,
        ch=209, fg=COLOR_GREY, bg=COLOR_BLACK}
local BOTTOM_SPLIT_PEN = dfhack.pen.parse{tile=gui.WINDOW_FRAME.b_frame_pen.tile,
        ch=207, fg=COLOR_GREY, bg=COLOR_BLACK}
local LEFT_SPLIT_PEN = dfhack.pen.parse{tile=gui.WINDOW_FRAME.l_frame_pen.tile,
        ch=199, fg=COLOR_GREY, bg=COLOR_BLACK}
local RIGHT_SPLIT_PEN = dfhack.pen.parse{tile=tp(17), ch=180, fg=COLOR_GREY, bg=COLOR_BLACK}

-- paint autocomplete panel border
local function paint_vertical_border(rect)
    local x = rect.x2 - (AUTOCOMPLETE_PANEL_WIDTH + 2)
    local y1, y2 = rect.y1, rect.y2
    dfhack.screen.paintTile(TOP_SPLIT_PEN, x, y1)
    dfhack.screen.paintTile(BOTTOM_SPLIT_PEN, x, y2)
    for y=y1+1,y2-1 do
        dfhack.screen.paintTile(V_SPLIT_PEN, x, y)
    end
end

-- paint border between edit area and help area
local function paint_horizontal_border(rect)
    local panel_height = EDIT_PANEL_HEIGHT + 1
    local x1, x2 = rect.x1, rect.x2
    local v_border_x = x2 - (AUTOCOMPLETE_PANEL_WIDTH + 2)
    local y = rect.y1 + panel_height
    dfhack.screen.paintTile(LEFT_SPLIT_PEN, x1, y)
    dfhack.screen.paintTile(RIGHT_SPLIT_PEN, v_border_x, y)
    for x=x1+1,v_border_x-1 do
        dfhack.screen.paintTile(H_SPLIT_PEN, x, y)
    end
end

function MainPanel:onRenderFrame(dc, rect)
    MainPanel.super.onRenderFrame(self, dc, rect)
    if self.get_minimal() then return end
    paint_vertical_border(rect)
    paint_horizontal_border(rect)
end

function MainPanel:onInput(keys)
    if MainPanel.super.onInput(self, keys) then
        return true
    elseif keys.CUSTOM_CTRL_D then
        dev_mode = not dev_mode
        self.update_autocomplete(get_first_word(self.subviews.editfield.text))
        return true
    elseif keys.KEYBOARD_CURSOR_RIGHT_FAST then
        self.subviews.autocomplete:advance(1)
        return true
    elseif keys.KEYBOARD_CURSOR_LEFT_FAST then
        self.subviews.autocomplete:advance(-1)
        return true
    end
end


----------------------------------
-- LauncherUI
--

LauncherUI = defclass(LauncherUI, gui.ZScreen)
LauncherUI.ATTRS{
    focus_path='launcher',
    defocusable=false,
    minimal=false,
}

local function get_frame_r()
    -- scan for anchor elements and do our best to avoid them
    local gps = df.global.gps
    local dimy = gps.dimy
    local maxx = gps.dimx - 1
    for x = 0,maxx do
        local index = x * dimy
        if (gps.top_in_use and gps.screentexpos_top_anchored[index] ~= 0) or
                gps.screentexpos_anchored[index] ~= 0 then
            return maxx - x + 1
        end
    end
    return 0
end

function LauncherUI:init(args)
    self.firstword = ""

    local main_panel = MainPanel{
        view_id='main',
        get_minimal=function() return self.minimal end,
        update_autocomplete=self:callback('update_autocomplete'),
        on_edit_input=self:callback('on_edit_input'),
    }

    local frame_r = get_frame_r()

    local update_frames = function()
        local new_frame = {}
        if self.minimal then
            new_frame.l = 0
            new_frame.r = frame_r
            new_frame.t = 0
            new_frame.h = 1
        else
            new_frame = config.data
            if not next(new_frame) then
                new_frame = {w=110, h=36}
            end
        end
        main_panel.frame = new_frame
        main_panel.frame_style = not self.minimal and gui.WINDOW_FRAME or nil

        local edit_frame = self.subviews.edit.frame
        edit_frame.r = self.minimal and
                0 or AUTOCOMPLETE_PANEL_WIDTH+2
        edit_frame.h = self.minimal and 1 or EDIT_PANEL_HEIGHT

        local editfield_frame = self.subviews.editfield.frame
        editfield_frame.t = self.minimal and 0 or 1
        editfield_frame.l = self.minimal and 10 or 1
        editfield_frame.r = self.minimal and 11 or 1
    end

    main_panel:addviews{
        AutocompletePanel{
            view_id='autocomplete',
            frame={t=0, r=0, w=AUTOCOMPLETE_PANEL_WIDTH},
            on_autocomplete=self:callback('on_autocomplete'),
            on_double_click=function(_,c) self:run_command(true, c.text) end,
            on_double_click2=function(_,c) self:run_command(false, c.text) end,
            visible=function() return not self.minimal end},
        EditPanel{
            view_id='edit',
            frame={t=0, l=0},
            on_change=function(text) self:on_edit_input(text, false) end,
            on_submit=self:callback('run_command', true),
            on_submit2=self:callback('run_command', false),
            on_toggle_minimal=function()
                self.minimal = not self.minimal
                update_frames()
                self:updateLayout()
            end,
            prefix_visible=function() return self.minimal end},
        HelpPanel{
            view_id='help',
            frame={t=EDIT_PANEL_HEIGHT+1, l=0, r=AUTOCOMPLETE_PANEL_WIDTH+1},
            visible=function() return not self.minimal end},
    }
    self:addviews{main_panel}

    update_frames()
    self:on_edit_input('')
end

function LauncherUI:update_help(text, firstword, show_help)
    firstword = firstword or get_first_word(text)
    if firstword == self.firstword then
        return
    end
    self.firstword = firstword
    self.subviews.help:set_entry(firstword, show_help)
end

local function extract_entry(entries, firstword)
    for i,v in ipairs(entries) do
        if v == firstword then
            table.remove(entries, i)
            return true
        end
    end
end

local function sort_by_freq(entries)
    -- remember starting position of each entry so we can sort stably
    local indices = utils.invert(entries)
    local stable_sort_by_frequency = function(a, b)
        local acount, bcount = get_command_count(a), get_command_count(b)
        if acount > bcount then return true
        elseif acount == bcount then
            return indices[a] < indices[b]
        end
        return false
    end
    table.sort(entries, stable_sort_by_frequency)
end

-- adds the n most closely affiliated peer entries for the given entry that
-- aren't already in the entries list. affiliation is determined by how many
-- tags the entries share.
local function add_top_related_entries(entries, entry, n)
    local dev_ok = dev_mode or helpdb.get_entry_tags(entry).dev
    local tags = helpdb.get_entry_tags(entry)
    local affinities, buckets = {}, {}
    for tag in pairs(tags) do
        for _,peer in ipairs(helpdb.get_tag_data(tag)) do
            affinities[peer] = (affinities[peer] or 0) + 1
        end
        buckets[#buckets + 1] = {}
    end
    for peer,affinity in pairs(affinities) do
        if helpdb.get_entry_types(peer).command then
            table.insert(buckets[affinity], peer)
        end
    end
    local entry_set = utils.invert(entries)
    for i=#buckets,1,-1 do
        sort_by_freq(buckets[i])
        for _,peer in ipairs(buckets[i]) do
            if not entry_set[peer] then
                entry_set[peer] = true
                if dev_ok or not helpdb.get_entry_tags(peer).dev then
                    table.insert(entries, peer)
                    n = n - 1
                    if n < 1 then return end
                end
            end
        end
    end
end

function LauncherUI:update_autocomplete(firstword)
    local excludes
    if not dev_mode then
        excludes = {tag={'dev', 'unavailable'}}
        if dfhack.getHideArmokTools() then
            table.insert(excludes.tag, 'armok')
        end
    end
    local entries = helpdb.search_entries({str=firstword, types='command'}, excludes)
    -- if firstword is in the list, extract it so we can add it to the top later
    -- even if it's not in the list, add it back anyway if it's a valid db entry
    -- (e.g. if it's a dev script that we masked out) to show that it's a valid
    -- command
    local found = extract_entry(entries,firstword) or helpdb.is_entry(firstword)
    sort_by_freq(entries)
    if found then
        table.insert(entries, 1, firstword)
        self.subviews.autocomplete_label:setText("Similar scripts")
        add_top_related_entries(entries, firstword, 20)
    else
        self.subviews.autocomplete_label:setText("Suggestions")
    end

    if #firstword == 0 then
        self.subviews.autocomplete_label:setText("All scripts")
    end

    self.subviews.autocomplete:set_options(entries, found)
end

function LauncherUI:on_edit_input(text, show_help)
    local firstword = get_first_word(text)
    self:update_help(text, firstword, show_help)
    self:update_autocomplete(firstword)
end

function LauncherUI:on_autocomplete(_, option)
    if option then
        self.subviews.edit:set_text(option.text..' ', true)
        self:update_help(option.text)
    end
end

function LauncherUI:run_command(reappear, command)
    command = command:trim()
    if #command == 0 then return end
    dfhack.addCommandToHistory(HISTORY_ID, HISTORY_FILE, command)
    record_command(command)
    -- remember the previous parent screen address so we can detect changes
    local _,prev_parent_addr = self._native.parent:sizeof()
    -- propagate saved unpaused status to the new ZScreen
    local saved_pause_state = df.global.pause_state
    if not self.saved_pause_state then
        df.global.pause_state = false
    end
    -- remove our viewscreen from the stack while we run the command. this
    -- allows hotkey guards and tools that interact with the top viewscreen
    -- without checking whether it is active to work reliably.
    local output = dfhack.screen.hideGuard(self, dfhack.run_command_silent,
                                           command)
    df.global.pause_state = saved_pause_state
    if #output > 0 then
        print('Output from command run from gui/launcher:')
        print('> ' .. command)
        print()
        print(output)
    end
    -- if we displayed a different screen, don't come back up even if reappear
    -- is true so the user can interact with the new screen.
    local _,parent_addr = self._native.parent:sizeof()
    if not reappear or self.minimal or parent_addr ~= prev_parent_addr then
        self:dismiss()
        if self.minimal and #output > 0 then
            dialogs.showMessage(TITLE, output)
        end
        return
    end
    -- reappear and show the command output
    self.subviews.edit:set_text('')
    if #output == 0 then
        output = 'Command finished successfully'
    else
        output = output:gsub('\t', ' ')
    end
    self.subviews.help:add_output(('> %s\n\n%s'):format(command, output))
end

function LauncherUI:onDismiss()
    view = nil
end

if dfhack_flags.module then
    return
end

if view then
    if not view:hasFocus() then
        view:raise()
    else
        -- running the launcher while it is open (e.g. from hitting the launcher
        -- hotkey a second time) should close the dialog
        view:dismiss()
        return
    end
end

local args = {...}
local minimal
if args[1] == '--minimal' or args[1] == '-m' then
    table.remove(args, 1)
    minimal = true
end
if not view then
    view = LauncherUI{minimal=minimal}:show()
elseif minimal and not view.minimal then
    view.subviews.edit.on_toggle_minimal()
end
local initial_command = table.concat(args, ' ')
if #initial_command > 0 then
    view.subviews.edit:set_text(initial_command)
    view:on_edit_input(initial_command, true)
end
