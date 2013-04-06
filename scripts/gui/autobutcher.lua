-- A GUI front-end for the autobutcher plugin.

local gui = require 'gui'
local utils = require 'utils'
local widgets = require 'gui.widgets'
local dlg = require 'gui.dialogs'

local plugin = require 'plugins.zone'

WatchList = defclass(WatchList, gui.FramedScreen)

WatchList.ATTRS {
    frame_title = 'Autobutcher Watchlist',
    frame_inset = 0, -- cover full DF window
    frame_background = COLOR_BLACK,
    frame_style = gui.BOUNDARY_FRAME,
}

-- width of the race name column in the UI
local racewidth = 25

function nextAutowatchState()
    if(plugin.autowatch_isEnabled()) then
        return 'Stop '
    end
    return 'Start'
end

function nextAutobutcherState()
    if(plugin.autobutcher_isEnabled()) then
        return 'Stop '
    end
    return 'Start'
end

function getSleepTimer()
    return plugin.autobutcher_getSleep()
end

function setSleepTimer(ticks)
    plugin.autobutcher_setSleep(ticks)
end

function WatchList:init(args)
    local colwidth = 7
    self:addviews{
        widgets.Panel{
            frame = { l = 0, r = 0 },
            frame_inset = 1,
            subviews = {
                widgets.Label{
                    frame = { l = 0, t = 0 },
                    text_pen = COLOR_CYAN,
                    text = {
                        { text = 'Race', width = racewidth }, ' ',
                        { text = 'female', width = colwidth }, ' ',
                        { text = ' male', width = colwidth }, ' ',
                        { text = 'Female', width = colwidth }, ' ',
                        { text = ' Male', width = colwidth }, ' ',
                        { text = 'watch? ' },
                        { text = ' butchering' },
                        NEWLINE,
                        { text = '', width = racewidth }, ' ',
                        { text = ' kids', width = colwidth }, ' ',
                        { text = ' kids', width = colwidth }, ' ',
                        { text = 'adults', width = colwidth }, ' ',
                        { text = 'adults', width = colwidth }, ' ',
                        { text = '       ' },
                        { text = '  ordered' },
                    }
                },
                widgets.List{
                    view_id = 'list',
                    frame = { t = 3, b = 5 },
                    not_found_label = 'Watchlist is empty.',
                    edit_pen = COLOR_LIGHTCYAN,
                    text_pen = { fg = COLOR_GREY, bg = COLOR_BLACK },
                    cursor_pen = { fg = COLOR_WHITE, bg = COLOR_GREEN },
                    --on_select = self:callback('onSelectEntry'),
                },
                widgets.Label{
                    view_id = 'bottom_ui',
                    frame = { b = 0, h = 1 },
                    text = 'filled by updateBottom()'
                }
            }
        },
    }

    self:initListChoices()
    self:updateBottom()
end

-- change the viewmode for stock data displayed in left section of columns
local viewmodes = { 'total stock', 'protected stock', 'butcherable', 'butchering ordered' }
local viewmode = 1
function WatchList:onToggleView()
    if viewmode < #viewmodes then
        viewmode = viewmode + 1
    else
        viewmode = 1
    end
    self:initListChoices()
    self:updateBottom()
end

-- update the bottom part of the UI (after sleep timer changed etc)
function WatchList:updateBottom()
    self.subviews.bottom_ui:setText(
        {
            { key = 'CUSTOM_SHIFT_V', text = ': View in colums shows: '..viewmodes[viewmode]..' / target max',
              on_activate = self:callback('onToggleView') }, NEWLINE,
            { key = 'CUSTOM_F', text = ': f kids',
              on_activate = self:callback('onEditFK') }, ', ',
            { key = 'CUSTOM_M', text = ': m kids',
              on_activate = self:callback('onEditMK') }, ', ',
            { key = 'CUSTOM_SHIFT_F', text = ': f adults',
              on_activate = self:callback('onEditFA') }, ', ',
            { key = 'CUSTOM_SHIFT_M', text = ': m adults',
              on_activate = self:callback('onEditMA') }, '. ',
            { key = 'CUSTOM_W', text = ': Toggle watch',
              on_activate = self:callback('onToggleWatching') }, '. ',
            { key = 'CUSTOM_X', text = ': Delete',
              on_activate = self:callback('onDeleteEntry') }, '. ', NEWLINE,
            --{ key = 'CUSTOM_A', text = ': Add race',
            --  on_activate = self:callback('onAddRace') }, ', ',
            { key = 'CUSTOM_SHIFT_R', text = ': Set whole row',
              on_activate = self:callback('onSetRow') }, '.     ',
            { key = 'CUSTOM_B', text = ': Remove butcher orders',
              on_activate = self:callback('onUnbutcherRace') }, '. ',
            { key = 'CUSTOM_SHIFT_B', text = ': Butcher race',
              on_activate = self:callback('onButcherRace') }, '. ', NEWLINE,
            { key = 'CUSTOM_SHIFT_A', text = ': '..nextAutobutcherState()..' Autobutcher',
              on_activate = self:callback('onToggleAutobutcher') }, '. ',
            { key = 'CUSTOM_SHIFT_W', text = ': '..nextAutowatchState()..' Autowatch',
              on_activate = self:callback('onToggleAutowatch') }, '.       ',
            { key = 'CUSTOM_SHIFT_S', text = ': Sleep ('..getSleepTimer()..' ticks)',
              on_activate = self:callback('onEditSleepTimer') }, '. ',
        })
end

function stringify(number)
    -- cap displayed number to 3 digits
    -- after population of 50 per race is reached pets stop breeding anyways
    -- so probably this could safely be reduced to 99
    local max = 999
    if number > max then number = max end
    return tostring(number)
end

function WatchList:initListChoices()

    local choices = {}

    -- first two rows are for "edit all races" and "edit new races"
    local settings = plugin.autobutcher_getSettings()
    local fk = stringify(settings.fk)
    local fa = stringify(settings.fa)
    local mk = stringify(settings.mk)
    local ma = stringify(settings.ma)

    local watched = ''

    local colwidth = 7

    table.insert (choices, {
        text = {
            { text = '!! ALL RACES PLUS NEW', width = racewidth, pad_char = ' ' }, --' ',
            { text = '   ', width = 3, rjustify = true,  pad_char = ' ' }, ' ',
            { text = fk,    width = 3, rjustify = false, pad_char = ' ' }, ' ',
            { text = '   ', width = 3, rjustify = true,  pad_char = ' ' }, ' ',
            { text = mk,    width = 3, rjustify = false, pad_char = ' ' }, ' ',
            { text = '   ', width = 3, rjustify = true,  pad_char = ' ' }, ' ',
            { text = fa,    width = 3, rjustify = false, pad_char = ' ' }, ' ',
            { text = '   ', width = 3, rjustify = true,  pad_char = ' ' }, ' ',
            { text = ma,    width = 3, rjustify = false, pad_char = ' ' }, ' ',
            { text = watched, width = 6, rjustify = true }
        }
    })

    table.insert (choices, {
        text = {
            { text = '!! ONLY NEW RACES', width = racewidth, pad_char = ' ' }, --' ',
            { text = '   ', width = 3, rjustify = true,  pad_char = ' ' }, ' ',
            { text = fk,    width = 3, rjustify = false, pad_char = ' ' }, ' ',
            { text = '   ', width = 3, rjustify = true,  pad_char = ' ' }, ' ',
            { text = mk,    width = 3, rjustify = false, pad_char = ' ' }, ' ',
            { text = '   ', width = 3, rjustify = true,  pad_char = ' ' }, ' ',
            { text = fa,    width = 3, rjustify = false, pad_char = ' ' }, ' ',
            { text = '   ', width = 3, rjustify = true,  pad_char = ' ' }, ' ',
            { text = ma,    width = 3, rjustify = false, pad_char = ' ' }, ' ',
            { text = watched, width = 6, rjustify = true }
        }
    })

    local watchlist = plugin.autobutcher_getWatchList()

    for i,entry in ipairs(watchlist) do
        fk = stringify(entry.fk)
        fa = stringify(entry.fa)
        mk = stringify(entry.mk)
        ma = stringify(entry.ma)
        if viewmode == 1 then
            fkc = stringify(entry.fk_total)
            fac = stringify(entry.fa_total)
            mkc = stringify(entry.mk_total)
            mac = stringify(entry.ma_total)
        end
        if viewmode == 2 then
            fkc = stringify(entry.fk_protected)
            fac = stringify(entry.fa_protected)
            mkc = stringify(entry.mk_protected)
            mac = stringify(entry.ma_protected)
        end
        if viewmode == 3 then
            fkc = stringify(entry.fk_butcherable)
            fac = stringify(entry.fa_butcherable)
            mkc = stringify(entry.mk_butcherable)
            mac = stringify(entry.ma_butcherable)
        end
        if viewmode == 4 then
            fkc = stringify(entry.fk_butcherflag)
            fac = stringify(entry.fa_butcherflag)
            mkc = stringify(entry.mk_butcherflag)
            mac = stringify(entry.ma_butcherflag)
        end
        local butcher_ordered = entry.fk_butcherflag + entry.fa_butcherflag + entry.mk_butcherflag + entry.ma_butcherflag
        local bo = ' '
        if butcher_ordered > 0 then bo = stringify(butcher_ordered) end

        local watched = 'no'
        if entry.watched then watched = 'yes' end

        local racestr = entry.name

        -- highlight entries where the target quota can't be met because too many are protected
        bad_pen = COLOR_LIGHTRED
        good_pen = NONE -- this is stupid, but it works. sue me
        fk_pen = good_pen
        fa_pen = good_pen
        mk_pen = good_pen
        ma_pen = good_pen
        if entry.fk_protected > entry.fk then fk_pen = bad_pen end
        if entry.fa_protected > entry.fa then fa_pen = bad_pen end
        if entry.mk_protected > entry.mk then mk_pen = bad_pen end
        if entry.ma_protected > entry.ma then ma_pen = bad_pen end

        table.insert (choices, {
            text = {
                { text = racestr, width = racewidth, pad_char = ' ' }, --' ',
                { text = fkc, width = 3, rjustify = true,  pad_char = ' ' }, '/',
                { text = fk,  width = 3, rjustify = false, pad_char = ' ', pen = fk_pen }, ' ',
                { text = mkc, width = 3, rjustify = true,  pad_char = ' ' }, '/',
                { text = mk,  width = 3, rjustify = false, pad_char = ' ', pen = mk_pen }, ' ',
                { text = fac, width = 3, rjustify = true,  pad_char = ' ' }, '/',
                { text = fa,  width = 3, rjustify = false, pad_char = ' ', pen = fa_pen }, ' ',
                { text = mac, width = 3, rjustify = true,  pad_char = ' ' }, '/',
                { text = ma,  width = 3, rjustify = false, pad_char = ' ', pen = ma_pen }, ' ',
                { text = watched, width = 6, rjustify = true, pad_char = ' ' }, ' ',
                { text = bo,  width = 8, rjustify = true, pad_char = ' ' }
            },
            obj = entry,
        })
    end

    local list = self.subviews.list
    list:setChoices(choices)
end

function WatchList:onInput(keys)
    if keys.LEAVESCREEN then
        self:dismiss()
    else
        WatchList.super.onInput(self, keys)
    end
end

-- check the user input for target population values
function WatchList:checkUserInput(count, text)
    if count == nil then
        dlg.showMessage('Invalid Number', 'This is not a number: '..text..NEWLINE..'(for zero enter a 0)', COLOR_LIGHTRED)
        return false
    end
    if count < 0 then
        dlg.showMessage('Invalid Number', 'Negative numbers make no sense!', COLOR_LIGHTRED)
        return false
    end
    return true
end

-- check the user input for sleep timer
function WatchList:checkUserInputSleep(count, text)
    if count == nil then
        dlg.showMessage('Invalid Number', 'This is not a number: '..text..NEWLINE..'(for zero enter a 0)', COLOR_LIGHTRED)
        return false
    end
    if count < 1000 then
        dlg.showMessage('Invalid Number',
            'Minimum allowed timer value is 1000!'..NEWLINE..'Too low values could decrease performance'..NEWLINE..'and are not necessary!',
            COLOR_LIGHTRED)
        return false
    end
    return true
end

function WatchList:onEditFK()
    local selidx,selobj = self.subviews.list:getSelected()
    local settings = plugin.autobutcher_getSettings()
    local fk = settings.fk
    local mk = settings.mk
    local fa = settings.fa
    local ma = settings.ma
    local race = 'ALL RACES PLUS NEW'
    local id = -1
    local watched = false

    if selidx == 2 then
        race = 'ONLY NEW RACES'
    end

    if selidx > 2 then
        local entry = selobj.obj
        fk = entry.fk
        mk = entry.mk
        fa = entry.fa
        ma = entry.ma
        race = entry.name
        id = entry.id
        watched = entry.watched
    end

    dlg.showInputPrompt(
        'Race: '..race,
        'Enter desired maximum of female kids:',
        COLOR_WHITE,
        ' '..fk,
        function(text)
            local count = tonumber(text)
            if self:checkUserInput(count, text) then
                fk = count
                if selidx == 1 then
                    plugin.autobutcher_setDefaultTargetAll( fk, mk, fa, ma )
                end
                if selidx == 2 then
                    plugin.autobutcher_setDefaultTargetNew( fk, mk, fa, ma )
                end
                if selidx > 2 then
                    plugin.autobutcher_setWatchListRace(id, fk, mk, fa, ma, watched)
                end
                self:initListChoices()
            end
        end
    )
end

function WatchList:onEditMK()
    local selidx,selobj = self.subviews.list:getSelected()
    local settings = plugin.autobutcher_getSettings()
    local fk = settings.fk
    local mk = settings.mk
    local fa = settings.fa
    local ma = settings.ma
    local race = 'ALL RACES PLUS NEW'
    local id = -1
    local watched = false

    if selidx == 2 then
        race = 'ONLY NEW RACES'
    end

    if selidx > 2 then
        local entry = selobj.obj
        fk = entry.fk
        mk = entry.mk
        fa = entry.fa
        ma = entry.ma
        race = entry.name
        id = entry.id
        watched = entry.watched
    end

    dlg.showInputPrompt(
        'Race: '..race,
        'Enter desired maximum of male kids:',
        COLOR_WHITE,
        ' '..mk,
        function(text)
            local count = tonumber(text)
            if self:checkUserInput(count, text) then
                mk = count
                if selidx == 1 then
                    plugin.autobutcher_setDefaultTargetAll( fk, mk, fa, ma )
                end
                if selidx == 2 then
                    plugin.autobutcher_setDefaultTargetNew( fk, mk, fa, ma )
                end
                if selidx > 2 then
                    plugin.autobutcher_setWatchListRace(id, fk, mk, fa, ma, watched)
                end
                self:initListChoices()
            end
        end
    )
end

function WatchList:onEditFA()
    local selidx,selobj = self.subviews.list:getSelected()
    local settings = plugin.autobutcher_getSettings()
    local fk = settings.fk
    local mk = settings.mk
    local fa = settings.fa
    local ma = settings.ma
    local race = 'ALL RACES PLUS NEW'
    local id = -1
    local watched = false

    if selidx == 2 then
        race = 'ONLY NEW RACES'
    end

    if selidx > 2 then
        local entry = selobj.obj
        fk = entry.fk
        mk = entry.mk
        fa = entry.fa
        ma = entry.ma
        race = entry.name
        id = entry.id
        watched = entry.watched
    end

    dlg.showInputPrompt(
        'Race: '..race,
        'Enter desired maximum of female adults:',
        COLOR_WHITE,
        ' '..fa,
        function(text)
            local count = tonumber(text)
            if self:checkUserInput(count, text) then
                fa = count
                if selidx == 1 then
                    plugin.autobutcher_setDefaultTargetAll( fk, mk, fa, ma )
                end
                if selidx == 2 then
                    plugin.autobutcher_setDefaultTargetNew( fk, mk, fa, ma )
                end
                if selidx > 2 then
                    plugin.autobutcher_setWatchListRace(id, fk, mk, fa, ma, watched)
                end
                self:initListChoices()
            end
        end
    )
end

function WatchList:onEditMA()
    local selidx,selobj = self.subviews.list:getSelected()
    local settings = plugin.autobutcher_getSettings()
    local fk = settings.fk
    local mk = settings.mk
    local fa = settings.fa
    local ma = settings.ma
    local race = 'ALL RACES PLUS NEW'
    local id = -1
    local watched = false

    if selidx == 2 then
        race = 'ONLY NEW RACES'
    end

    if selidx > 2 then
        local entry = selobj.obj
        fk = entry.fk
        mk = entry.mk
        fa = entry.fa
        ma = entry.ma
        race = entry.name
        id = entry.id
        watched = entry.watched
    end

    dlg.showInputPrompt(
        'Race: '..race,
        'Enter desired maximum of male adults:',
        COLOR_WHITE,
        ' '..ma,
        function(text)
            local count = tonumber(text)
            if self:checkUserInput(count, text) then
                ma = count
                if selidx == 1 then
                    plugin.autobutcher_setDefaultTargetAll( fk, mk, fa, ma )
                end
                if selidx == 2 then
                    plugin.autobutcher_setDefaultTargetNew( fk, mk, fa, ma )
                end
                if selidx > 2 then
                    plugin.autobutcher_setWatchListRace(id, fk, mk, fa, ma, watched)
                end
                self:initListChoices()
            end
        end
    )
end

function WatchList:onEditSleepTimer()
    local sleep = getSleepTimer()
    dlg.showInputPrompt(
        'Edit Sleep Timer',
        'Enter new sleep timer in ticks:'..NEWLINE..'(1 ingame day equals 1200 ticks)',
        COLOR_WHITE,
        ' '..sleep,
        function(text)
            local count = tonumber(text)
            if self:checkUserInputSleep(count, text) then
                sleep = count
                setSleepTimer(sleep)
                self:updateBottom()
            end
        end
    )
end

function WatchList:onToggleWatching()
    local selidx,selobj = self.subviews.list:getSelected()
    if selidx > 2 then
        local entry = selobj.obj
        plugin.autobutcher_setWatchListRace(entry.id, entry.fk, entry.mk, entry.fa, entry.ma, not entry.watched)
    end
    self:initListChoices()
end

function WatchList:onDeleteEntry()
    local selidx,selobj = self.subviews.list:getSelected()
    if(selidx < 3 or selobj == nil) then
        return
    end
    dlg.showYesNoPrompt(
        'Delete from Watchlist',
        'Really delete the selected entry?'..NEWLINE..'(you could just toggle watch instead)',
        COLOR_YELLOW,
        function()
            plugin.autobutcher_removeFromWatchList(selobj.obj.id)
            self:initListChoices()
        end
    )
end

function WatchList:onAddRace()
    print('onAddRace - not implemented yet')
end

function WatchList:onUnbutcherRace()
    local selidx,selobj = self.subviews.list:getSelected()
    if selidx < 3 then dlg.showMessage('Error', 'Select a specific race.',    COLOR_LIGHTRED)    end
    if selidx > 2 then
        local entry = selobj.obj
        local race = entry.name
        plugin.autobutcher_unbutcherRace(entry.id)
        self:initListChoices()
        self:updateBottom()
    end
end

function WatchList:onButcherRace()
    local selidx,selobj = self.subviews.list:getSelected()
    if selidx < 3 then dlg.showMessage('Error', 'Select a specific race.',    COLOR_LIGHTRED) end
    if selidx > 2 then
        local entry = selobj.obj
        local race = entry.name
        plugin.autobutcher_butcherRace(entry.id)
        self:initListChoices()
        self:updateBottom()
    end
end

-- set whole row (fk, mk, fa, ma) to one value
function WatchList:onSetRow()
    local selidx,selobj = self.subviews.list:getSelected()
    local race = 'ALL RACES PLUS NEW'
    local id = -1
    local watched = false

    if selidx == 2 then
        race = 'ONLY NEW RACES'
    end

    local watchindex = selidx - 3
    if selidx > 2 then
        local entry = selobj.obj
        race = entry.name
        id = entry.id
        watched = entry.watched
    end

    dlg.showInputPrompt(
        'Set whole row for '..race,
        'Enter desired maximum for all subtypes:',
        COLOR_WHITE,
        ' ',
        function(text)
            local count = tonumber(text)
            if self:checkUserInput(count, text) then
                if selidx == 1 then
                    plugin.autobutcher_setDefaultTargetAll( count, count, count, count )
                end
                if selidx == 2 then
                    plugin.autobutcher_setDefaultTargetNew( count, count, count, count )
                end
                if selidx > 2 then
                    plugin.autobutcher_setWatchListRace(id, count, count, count, count, watched)
                end
                self:initListChoices()
            end
        end
    )
end

function WatchList:onToggleAutobutcher()
    if(plugin.autobutcher_isEnabled()) then
        plugin.autobutcher_setEnabled(false)
        plugin.autobutcher_sortWatchList()
    else
        plugin.autobutcher_setEnabled(true)
    end
    self:initListChoices()
    self:updateBottom()
end

function WatchList:onToggleAutowatch()
    if(plugin.autowatch_isEnabled()) then
        plugin.autowatch_setEnabled(false)
    else
        plugin.autowatch_setEnabled(true)
    end
    self:initListChoices()
    self:updateBottom()
end

if not dfhack.isMapLoaded() then
    qerror('Map is not loaded.')
end

if string.match(dfhack.gui.getCurFocus(), '^dfhack/lua') then
    qerror("This script must not be called while other lua gui stuff is running.")
end

-- maybe this is too strict, there is not really a reason why it can only be called from the status screen
-- (other than the hotkey might overlap with other scripts)
if (not string.match(dfhack.gui.getCurFocus(), '^overallstatus') and not string.match(dfhack.gui.getCurFocus(), '^pet/List/Unit')) then
    qerror("This script must either be called from the overall status screen or the animal list screen.")
end


local screen = WatchList{ }
screen:show()
