-- Show fort's petitions, pending and fulfilled.
--[====[

gui/petitions
=============
Show fort's petitions, pending and fulfilled.

For best experience add following to your ``dfhack*.init``::

    keybinding add Alt-P@dwarfmode/Default gui/petitions

]====]

local gui = require 'gui'
local widgets = require 'gui.widgets'
local utils = require 'utils'

-- local args = utils.invert({...})

--[[
[lua]# @ df.agreement_details_type
<type: agreement_details_type>
0                        = JoinParty
1                        = DemonicBinding
2                        = Residency
3                        = Citizenship
4                        = Parley
5                        = PositionCorruption
6                        = PlotStealArtifact
7                        = PromisePosition
8                        = PlotAssassination
9                        = PlotAbduct
10                       = PlotSabotage
11                       = PlotConviction
12                       = Location
13                       = PlotInfiltrationCoup
14                       = PlotFrameTreason
15                       = PlotInduceWar
]]

if not dfhack.world.isFortressMode() then return end

-- from gui/unit-info-viewer.lua
do -- for code folding
--------------------------------------------------
---------------------- Time ----------------------
--------------------------------------------------
local TU_PER_DAY = 1200
--[[
if advmode then TU_PER_DAY = 86400 ? or only for cur_year_tick?
advmod_TU / 72 = ticks
--]]
local TU_PER_MONTH = TU_PER_DAY * 28
local TU_PER_YEAR = TU_PER_MONTH * 12

local MONTHS = {
 'Granite',
 'Slate',
 'Felsite',
 'Hematite',
 'Malachite',
 'Galena',
 'Limestone',
 'Sandstone',
 'Timber',
 'Moonstone',
 'Opal',
 'Obsidian',
}
Time = defclass(Time)
function Time:init(args)
 self.year = args.year or 0
 self.ticks = args.ticks or 0
end
function Time:getDays() -- >>float<< Days as age (including years)
 return self.year * 336 + (self.ticks / TU_PER_DAY)
end
function Time:getDayInMonth()
 return math.floor ( (self.ticks % TU_PER_MONTH) / TU_PER_DAY ) + 1
end
function Time:getMonths() -- >>int<< Months as age (not including years)
 return math.floor (self.ticks / TU_PER_MONTH)
end
function Time:getYears() -- >>int<<
 return self.year
end
function Time:getMonthStr() -- Month as date
 return MONTHS[self:getMonths()+1] or 'error'
end
function Time:getDayStr() -- Day as date
 local d = math.floor ( (self.ticks % TU_PER_MONTH) / TU_PER_DAY ) + 1
 if d == 11 or d == 12 or d == 13 then
  d = tostring(d)..'th'
 elseif d % 10 == 1 then
  d = tostring(d)..'st'
 elseif d % 10 == 2 then
  d = tostring(d)..'nd'
 elseif d % 10 == 3 then
  d = tostring(d)..'rd'
 else
  d = tostring(d)..'th'
 end
 return d
end
--function Time:__add()
--end
function Time:__sub(other)
 if DEBUG then print(self.year,self.ticks) end
 if DEBUG then print(other.year,other.ticks) end
 if self.ticks < other.ticks then
  return Time{ year = (self.year - other.year - 1) , ticks = (TU_PER_YEAR + self.ticks - other.ticks) }
 else
  return Time{ year = (self.year - other.year) , ticks = (self.ticks - other.ticks) }
 end
end
--------------------------------------------------
--------------------------------------------------
end

local we = df.global.plotinfo.group_id

local function getAgreementDetails(a)
    local sb = {} -- StringBuilder

    sb[#sb+1] = {text = "Agreement #" ..a.id, pen = COLOR_RED}
    sb[#sb+1] = NEWLINE

    local us = "Us"
    local them = "Them"
    for i, p in ipairs(a.parties) do
        local e_descr = {}
        local our = false
        for _, e_id in ipairs(p.entity_ids) do
            local e = df.global.world.entities.all[e_id]
            e_descr[#e_descr+1] = table.concat{"The ", df.historical_entity_type[e.type], " ", dfhack.TranslateName(e.name, true)}
            if we == e_id then our = true end
        end
        for _, hf_id in ipairs(p.histfig_ids) do
            local hf = df.global.world.history.figures[hf_id]
            local race = df.creature_raw.find(hf.race)
            local civ = df.historical_entity.find(hf.civ_id)
            e_descr[#e_descr+1] = table.concat{
                                "The ", race.creature_id,
                                " ", df.profession[hf.profession],
                                " ", dfhack.TranslateName(hf.name, true),
                                NEWLINE,
                                "of ", dfhack.TranslateName(civ.name, true)
                            }
        end

        if our then
            us = table.concat(e_descr, ", ")
        else
            them = table.concat(e_descr, ", ")
        end
    end
    sb[#sb+1] = them
    sb[#sb+1] = NEWLINE
    sb[#sb+1] = " petitioned"
    sb[#sb+1] = NEWLINE
    sb[#sb+1] = us
    sb[#sb+1] = NEWLINE
    local expired = false
    for _, d in ipairs (a.details) do
        local petition_date = Time{year = d.year, ticks = d.year_tick}
        local petition_date_str = petition_date:getDayStr()..' of '..petition_date:getMonthStr()..' in the year '..tostring(petition_date.year)
        local cur_date = Time{year = df.global.cur_year, ticks = df.global.cur_year_tick}
        sb[#sb+1] = ("On " .. petition_date_str)
        sb[#sb+1] = NEWLINE
        local diff = (cur_date - petition_date)
        expired = expired or diff:getYears() >= 1

        if diff:getDays() < 1.0 then
            sb[#sb+1] = ("(this was today)")
        elseif diff:getMonths() == 0 then
            sb[#sb+1] = ("(this was " .. math.floor( diff:getDays() ) .. " days ago)" )
        elseif diff:getYears() == 0 then
            sb[#sb+1] = ("(this was " .. diff:getMonths() .. " months and " ..  diff:getDayInMonth() .. " days ago)" )
        elseif diff:getYears() == 1 then
            sb[#sb+1] = ("(this was " .. diff:getYears() .. " year " .. diff:getMonths() .. " months and " ..  diff:getDayInMonth() .. " days ago)" )
        else
            sb[#sb+1] = ("(this was " .. diff:getYears() .. " years " .. diff:getMonths() .. " months and " ..  diff:getDayInMonth() .. " days ago)" )
        end
        sb[#sb+1] = NEWLINE

        sb[#sb+1] = ("Petition type: " .. df.agreement_details_type[d.type])
        sb[#sb+1] = NEWLINE
        if d.type == df.agreement_details_type.Location then
            local details = d.data.Location
            sb[#sb+1] = "Provide a "
            sb[#sb+1] = {text = df.abstract_building_type[details.type], pen = COLOR_LIGHTGREEN}
            sb[#sb+1] = " of tier "
            sb[#sb+1] = {text = details.tier, pen = COLOR_LIGHTGREEN}
            if details.deity_type ~= -1 then
                sb[#sb+1] = " of a "
                -- None/Deity/Religion
                sb[#sb+1] = {text = df.temple_deity_type[details.deity_type], pen = COLOR_LIGHTGREEN}
            else
                sb[#sb+1] = " for "
                sb[#sb+1] = {text = df.profession[details.profession], pen = COLOR_LIGHTGREEN}
            end
            sb[#sb+1] = NEWLINE
        elseif d.type == df.agreement_details_type.Residency then
            local details = d.data.Residency
            sb[#sb+1] = "            to "
            sb[#sb+1] = {text = df.history_event_reason[details.reason], pen = COLOR_LIGHTGREEN}
            sb[#sb+1] = NEWLINE
        end
    end

    local petition = {}

    if a.flags.petition_not_accepted then
        sb[#sb+1] = {text = "This petition wasn't accepted yet!", pen = COLOR_YELLOW}
        petition.status = 'PENDING'
    elseif a.flags.convicted_accepted then
        sb[#sb+1] = {text = "This petition was fulfilled!", pen = COLOR_GREEN}
        petition.status = 'FULFILLED'
    elseif expired then
        sb[#sb+1] = {text = "This petition expired!", pen = COLOR_LIGHTRED}
        petition.status = 'EXPIRED'
    else
        petition.status = 'ACCEPTED'
    end

    petition.text = sb

    return petition
end

local getAgreements = function()
    local list = {}

    local ags = df.global.world.agreements.all
    for i, a in ipairs(ags) do
        for _, p in ipairs(a.parties) do
            for _, e in ipairs(p.entity_ids) do
                if e == we then
                    list[#list+1] = getAgreementDetails(a)
                end
            end
        end
    end

    return list
end

local petitions = defclass(petitions, gui.FramedScreen)
petitions.ATTRS = {
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = 'Petitions',
    frame_width = 21, -- is calculated in :refresh
    min_frame_width = 21,
    frame_height = 26,
    frame_inset = 0,
    focus_path = 'petitions',
}

function petitions:init(args)
    self.list = args.list
    -- self.fulfilled = true
    self:addviews{
        widgets.Label{
            view_id = 'text',
            frame_inset = 0,
        },
    }

    self:refresh()
end

function petitions:refresh()
    local lines = {}
    -- list of petitions
    for _, p in ipairs(self.list) do
        if not self.fulfilled and (p.status == 'FULFILLED' or p.status == 'EXPIRED') then goto continue end
        -- each petition is a status and a text
        for _, tok in ipairs(p.text) do
            -- where text is a list of tokens
            table.insert(lines, tok)
        end
        table.insert(lines, NEWLINE)
    ::continue::
    end
    table.remove(lines, #lines) -- remove last NEWLINE

    local label = self.subviews.text
    label:setText(lines)

    -- changing text doesn't automatically change scroll position
    if label.frame_body then
        local last_visible_line = label.start_line_num + label.frame_body.height - 1
        if last_visible_line > label:getTextHeight() then
            label.start_line_num = math.max(label:getTextHeight() - label.frame_body.height + 1, 1)
        end
    end

    self.frame_width = math.max(label:getTextWidth()+1, self.min_frame_width)
    self.frame_width = math.min(df.global.gps.dimx - 2, self.frame_width)
    self.frame_height = math.min(df.global.gps.dimy - 4, self.frame_height)
    self:onResize(dfhack.screen.getWindowSize()) -- applies new frame_width
end

function petitions:onRenderFrame(painter, frame)
    petitions.super.onRenderFrame(self, painter, frame)

    painter:seek(frame.x1+2, frame.y1 + frame.height-1):key_string('CUSTOM_F', "toggle fulfilled")
end

function petitions:onInput(keys)
    if petitions.super.onInput(self, keys) then return end

    if keys.LEAVESCREEN or keys.SELECT then
        self:dismiss()
    elseif keys.CUSTOM_F then
        self.fulfilled = not self.fulfilled
        self:refresh()
    end
end

df.global.pause_state = true
petitions{list=getAgreements()}:show()
