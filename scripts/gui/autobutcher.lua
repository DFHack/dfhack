-- A GUI front-end for the autobutcher plugin.
-- requires to be called from the stock screen (z)


--[[

	API overview (zone/autobutcher plugin functions which can be used by this lua script):

	autobutcher_isEnabled() 				- returns true if autobutcher is running
	autowatch_isEnabled()					- returns true if autowatch is running
	autobutcher_getSleep()					- get sleep timer in ticks
	autobutcher_setSleep(int)				- set sleep timer in ticks
	autobutcher_getWatchListSize()			- return size of watchlist
	autobutcher_getWatchListRace(idx)		- return race name for this watchlist index

	autobutcher_isWatchListRaceWatched(idx)	- true if this watchlist index is watched
	autobutcher_setWatchListRaceWatched(idx, bool) - set watchlist index to watched/unwatched

	autobutcher_getWatchListRaceFK()		- get target fk
	autobutcher_getWatchListRaceFA()		- get target fa
	autobutcher_getWatchListRaceMK()		- get target mk
	autobutcher_getWatchListRaceMA()		- get target ma

	autobutcher_setWatchListRaceFK(value)	- set fk
	autobutcher_setWatchListRaceFA(value)	- set fa
	autobutcher_setWatchListRaceMK(value)	- set mk
	autobutcher_setWatchListRaceMA(value)	- set ma

	autobutcher_removeFromWatchList(idx)	- remove watchlist entry
	autobutcher_sortWatchList()				- sort the watchlist alphabetically

	testString(id) - returns race name for this race id (gonna be removed soon)

--]]


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
		return 'stop'
	end
	return 'start'
end

function nextAutobutcherState()
	if(plugin.autobutcher_isEnabled()) then 
		return 'stop'
	end
	return 'start'
end

function getSleepTimer()
	return plugin.autobutcher_getSleep()
end

function setSleepTimer(ticks)
	plugin.autobutcher_setSleep(ticks)
end

function WatchList:init(args)
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
                        { text = 'female', width = 6 }, ' ', 
                        { text = '  male', width = 6 }, ' ', 
                        { text = 'Female', width = 6 }, ' ', 
                        { text = '  Male', width = 6 }, ' ', 
						{ text = 'watching?' },
						NEWLINE,
                        { text = '', width = racewidth }, ' ',
                        { text = '  kids', width = 6 }, ' ', 
                        { text = '  kids', width = 6 }, ' ', 
                        { text = 'adults', width = 6 }, ' ', 
                        { text = 'adults', width = 6 }, ' ', 
                    }
                },
                widgets.List{
                    view_id = 'list',
                    frame = { t = 3, b = 3 },
                    not_found_label = 'Watchlist is empty.',
                    edit_pen = COLOR_LIGHTCYAN,
                    text_pen = { fg = COLOR_GREY, bg = COLOR_BLACK },
                    cursor_pen = { fg = COLOR_WHITE, bg = COLOR_GREEN },
                    --on_select = self:callback('onSelectConstraint'),
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

-- update the bottom part of the UI (after sleep timer changed etc)
function WatchList:updateBottom()
	self.subviews.bottom_ui:setText(
		{
			{ key = 'CUSTOM_F', text = ': f kids',
			  on_activate = self:callback('onEditFK') }, ', ',
			{ key = 'CUSTOM_M', text = ': m kids',
			  on_activate = self:callback('onEditMK') }, ', ',
			{ key = 'CUSTOM_SHIFT_F', text = ': f adults',
			  on_activate = self:callback('onEditFA') }, ', ',
			{ key = 'CUSTOM_SHIFT_M', text = ': m adults',
			  on_activate = self:callback('onEditMA') }, ', ',
			{ key = 'CUSTOM_SHIFT_X', text = ': Delete',
			  on_activate = self:callback('onDeleteConstraint') }, ', ', 
			{ key = 'CUSTOM_W', text = ': toggle watch',
			  on_activate = self:callback('onToggleWatching') }, ', ', NEWLINE,
			{ key = 'CUSTOM_SHIFT_A', text = ': '..nextAutobutcherState()..' Autobutcher',
			  on_activate = self:callback('onToggleAutobutcher') }, ', ',
			{ key = 'CUSTOM_SHIFT_W', text = ': '..nextAutowatchState()..' Autowatch',
			  on_activate = self:callback('onToggleAutowatch') }, ', ',
			{ key = 'CUSTOM_SHIFT_S', text = ': sleep timer ('..getSleepTimer()..' ticks)',
			  on_activate = self:callback('onEditSleepTimer') }, ', ',
		})
end

function WatchList:initListChoices()

	local choices = {}
	
	-- first two rows are for "edit all races" and "edit new races"
	local fk = plugin.autobutcher_getDefaultFK()
	local fa = plugin.autobutcher_getDefaultFA()
	local mk = plugin.autobutcher_getDefaultMK()
	local ma = plugin.autobutcher_getDefaultMA()
	local watched = '---'
	
	table.insert (choices, {
		text = {
			{ text = '!! ALL RACES PLUS NEW', width = racewidth, pad_char = ' ' }, --' ',
			{ text = tostring(fk), width = 7, rjustify = true }, 
			{ text = tostring(mk), width = 7, rjustify = true }, 
			{ text = tostring(fa), width = 7, rjustify = true }, 
			{ text = tostring(ma), width = 7, rjustify = true }, 
			{ text = watched, width = 6, rjustify = true }
		}
	})

	table.insert (choices, {
		text = {
			{ text = '!! ONLY NEW RACES', width = racewidth, pad_char = ' ' }, --' ',
			{ text = tostring(fk), width = 7, rjustify = true }, 
			{ text = tostring(mk), width = 7, rjustify = true }, 
			{ text = tostring(fa), width = 7, rjustify = true }, 
			{ text = tostring(ma), width = 7, rjustify = true }, 
			{ text = watched, width = 6, rjustify = true }
		}
	})

	-- fill with watchlist
	for i=0, plugin.autobutcher_getWatchListSize()-1 do
		local racestr = plugin.autobutcher_getWatchListRace(i)
		fk = plugin.autobutcher_getWatchListRaceFK(i)
		fa = plugin.autobutcher_getWatchListRaceFA(i)
		mk = plugin.autobutcher_getWatchListRaceMK(i)
		ma = plugin.autobutcher_getWatchListRaceMA(i)
		local watched = 'no'
		if plugin.autobutcher_isWatchListRaceWatched(i) then
			watched = 'yes'
		end
		
		table.insert (choices, {
			text = {
                { text = racestr, width = racewidth, pad_char = ' ' }, --' ',
                { text = tostring(fk), width = 7, rjustify = true }, 
                { text = tostring(mk), width = 7, rjustify = true }, 
                { text = tostring(fa), width = 7, rjustify = true }, 
                { text = tostring(ma), width = 7, rjustify = true }, 
				{ text = watched, width = 6, rjustify = true }
            }
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
	local fk = plugin.autobutcher_getDefaultFK();
	local mk = plugin.autobutcher_getDefaultMK();
	local fa = plugin.autobutcher_getDefaultFA();
	local ma = plugin.autobutcher_getDefaultMA();
	local race = 'ALL RACES PLUS NEW'
	
	if selidx == 2 then
		race = 'ONLY NEW RACES'
	end
	
	local watchindex = selidx - 3
	if selidx > 2 then
		fk = plugin.autobutcher_getWatchListRaceFK(watchindex)
		race = plugin.autobutcher_getWatchListRace(watchindex)
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
					plugin.autobutcher_setWatchListRaceFK(watchindex, fk)
				end
				self:initListChoices()
			end
        end
    )
end

function WatchList:onEditMK()
	local selidx = self.subviews.list:getSelected()
	local fk = plugin.autobutcher_getDefaultFK();
	local mk = plugin.autobutcher_getDefaultMK();
	local fa = plugin.autobutcher_getDefaultFA();
	local ma = plugin.autobutcher_getDefaultMA();
	local race = 'ALL RACES PLUS NEW'
	
	if selidx == 2 then
		race = 'ONLY NEW RACES'
	end
	
	local watchindex = selidx - 3
	if selidx > 2 then
		mk = plugin.autobutcher_getWatchListRaceMK(watchindex)
		race = plugin.autobutcher_getWatchListRace(watchindex)
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
					plugin.autobutcher_setWatchListRaceMK(watchindex, mk)
				end
				self:initListChoices()
			end
        end
    )
end

function WatchList:onEditFA()
	local selidx = self.subviews.list:getSelected()
	local fk = plugin.autobutcher_getDefaultFK();
	local mk = plugin.autobutcher_getDefaultMK();
	local fa = plugin.autobutcher_getDefaultFA();
	local ma = plugin.autobutcher_getDefaultMA();
	local race = 'ALL RACES PLUS NEW'
	
	if selidx == 2 then
		race = 'ONLY NEW RACES'
	end
	
	local watchindex = selidx - 3
	if selidx > 2 then
		fa = plugin.autobutcher_getWatchListRaceFA(watchindex)
		race = plugin.autobutcher_getWatchListRace(watchindex)
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
					plugin.autobutcher_setWatchListRaceFA(watchindex, fa)
				end
				self:initListChoices()
			end
        end
    )
end

function WatchList:onEditMA()
	local selidx = self.subviews.list:getSelected()
	local fk = plugin.autobutcher_getDefaultFK();
	local mk = plugin.autobutcher_getDefaultMK();
	local fa = plugin.autobutcher_getDefaultFA();
	local ma = plugin.autobutcher_getDefaultMA();
	local race = 'ALL RACES PLUS NEW'
	
	if selidx == 2 then
		race = 'ONLY NEW RACES'
	end
	
	local watchindex = selidx - 3
	if selidx > 2 then
		ma = plugin.autobutcher_getWatchListRaceMA(watchindex)
		race = plugin.autobutcher_getWatchListRace(watchindex)
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
					plugin.autobutcher_setWatchListRaceMA(watchindex, ma)
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
	local selidx = self.subviews.list:getSelected()
	if(selidx == 0) then
		--print('special handling for zero - list empty?')
	end
	if (selidx == 1) then
		--print('special handling for first row - ALL animals')
	end
	if (selidx == 2) then
		--print('special handling for second row - NEW animals')
	end
	if selidx > 2 then
		--print('handling for single animal on watchlist')
		local idx = selidx - 3
		if plugin.autobutcher_isWatchListRaceWatched(idx) then
			plugin.autobutcher_setWatchListRaceWatched(idx, false)
		else
			plugin.autobutcher_setWatchListRaceWatched(idx, true)
		end		
	end
	self:initListChoices()
end

function WatchList:onDeleteConstraint()
	local selidx,selobj = self.subviews.list:getSelected()
	if(selidx < 3) then
		-- print('cannot delete this entry')
		return
	end	
	local idx = selidx - 3
    dlg.showYesNoPrompt(
        'Delete from Watchlist',
        'Really delete the selected entry?'..NEWLINE..'(you could just toggle watch instead)',
        COLOR_YELLOW,
        function()
			plugin.autobutcher_removeFromWatchList(idx)
            self:initListChoices()
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


local screen = WatchList{ }
screen:show()
