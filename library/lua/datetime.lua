local _ENV = mkmodule('datetime')

--[[
TODO: investigate applicability for adv mode.
if advmode then TICKS_PER_DAY = 86400 ? or only for cur_year_tick?
advmod_TU / 72 = ticks
--]]

-- are these locals better in DwarfCalendar?
local DAYS_PER_MONTH = 28
local DAYS_PER_YEAR = 336
local MONTHS_PER_YEAR = 12
local TICKS_PER_DAY = 1200
local TICKS_PER_MONTH = TICKS_PER_DAY * DAYS_PER_MONTH
local TICKS_PER_YEAR = TICKS_PER_MONTH * MONTHS_PER_YEAR

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

DwarfCalendar = defclass(DwarfCalendar)

DwarfCalendar.ATTRS{
    year=0,
    year_tick=0,
}

function DwarfCalendar:init()
    self:normalize()
end

function DwarfCalendar:addTicks(ticks)
    self.year_tick = self.year_tick + ticks
    self:normalize()
end

function DwarfCalendar:addDays(days)
    self.year_tick = self.year_tick + self.daysToTicks(days)
    self:normalize()
end

function DwarfCalendar:addMonths(months)
    self.year_tick = self.year_tick + self.monthsToTicks(months)
    self:normalize()
end

function DwarfCalendar:addYears(years)
    self.year = self.year + years
end

-- returns an integer pair: (year), (year tick count)
function DwarfCalendar:getYears()
    return self.year, self.year_tick
end

-- returns days since beginning of a year, starting from zero
function DwarfCalendar:ticksToDays()
    return self.year_tick // TICKS_PER_DAY
end

-- returns days since the beginning of a month, starting from zero
function DwarfCalendar:ticksToDayOfMonth()
    return self:ticksToDays() % DAYS_PER_MONTH
end

-- returns months since the beginning of a year, starting from zero
function DwarfCalendar:ticksToMonths()
    return self.year_tick // TICKS_PER_MONTH
end

-- returns ticks since the beginning of a day
function DwarfCalendar:getDayTicks()
    return self.year_tick % TICKS_PER_DAY
end

-- returns ticks since the beginning of a month
function DwarfCalendar:getMonthTicks()
    return self.year_tick % TICKS_PER_MONTH
end

function DwarfCalendar:normalize()
    if (self.year_tick > TICKS_PER_YEAR) then
        self.year = self.year + (self.year_tick // TICKS_PER_YEAR)
        self.year_tick = self.year_tick % TICKS_PER_YEAR
    elseif (self.year_tick < 0) then
        -- going backwards in time, subtract year by at least one.
        self.year = self.year - math.max(1, math.abs(self.year_tick) // TICKS_PER_YEAR)
        -- Lua's modulo operator applies floor division,
        -- hence year_tick will always be positive after assignment
        -- equivalent to: year_tick - (TICKS_PER_YEAR * (year_tick // TICKS_PER_YEAR))
        self.year_tick = self.year_tick % TICKS_PER_YEAR
    end
end

function DwarfCalendar:__add(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return DwarfCalendar{ year = (self.year + other.year), year_tick = (self.year_tick + other.year_tick) }
end

function DwarfCalendar:__sub(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return DwarfCalendar{ year = (self.year - other.year) , year_tick = (self.year_tick - other.year_tick) }
end

function DwarfCalendar:_debugOps(other)
    print('first: '..self.year,self.year_tick)
    print('second: '..other.year,other.year_tick)
end

-- utility functions
function DwarfCalendar.daysToTicks(days)
    return days * TICKS_PER_DAY
end

function DwarfCalendar.monthsToTicks(months)
    return months * TICKS_PER_MONTH
end

function DwarfCalendar.getMonthNames()
    return MONTHS
end


DateTime = defclass(DateTime, DwarfCalendar)

-- returns an integer pair: (day of month starting from 1), (day tick count)
function DateTime:getDayOfMonth()
    return self:ticksToDayOfMonth() + 1, self:getDayTicks()
end

-- returns a string in ordinal form (e.g. 1st, 12th, 22nd, 101st, 111th, 133rd)
function DateTime:getDayOfMonthWithSuffix()
    local d = self:getDayOfMonth()
    return tostring(d)..self.getOrdinalSuffix(d)
end

-- returns an integer pair: (current day of year, from 1), (day tick count)
function DateTime:getDayOfYear()
    return self:ticksToDays() + 1, self:getDayTicks()
end

-- returns an integer pair: (current month of the year, from 1), (month tick count)
function DateTime:getMonth()
    return self:ticksToMonths() + 1, self:getMonthTicks()
end

-- returns a string of the current month of the year
function DateTime:getNameOfMonth()
    return MONTHS[self:getMonth()] or error("getMonth(): bad index?")
end

function DateTime:toDuration()
    return Duration{ year = self.year, year_tick = self.year_tick }
end

function DateTime:__add(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return DateTime{ year = (self.year + other.year), year_tick = (self.year_tick + other.year_tick) }
end

-- might make sense to return a Duration here
function DateTime:__sub(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return DateTime{ year = (self.year - other.year) , year_tick = (self.year_tick - other.year_tick) }
end

-- Static functions
function DateTime.now()
    return DateTime{ year = df.global.cur_year, year_tick = df.global.cur_year_tick }
end

-- Ordinal suffix rules found here: https://en.wikipedia.org/wiki/Ordinal_indicator
function DateTime.getOrdinalSuffix(ordinal)
    if (ordinal < 0) then
        ordinal = math.abs(ordinal)
    end

    local rem = (ordinal < 100) and (ordinal % 10) or (ordinal % 100)
    -- rem can be between 11 and 13 only when ordinal is > 100
    if (ordinal >= 11 and ordinal <= 13) or (rem >= 11 and rem <= 13) then
        return 'th'
    end
    -- modulo again to handle the case when ordinal is > 100
    return ({'st', 'nd', 'rd'})[rem % 10] or 'th'
end

Duration = defclass(Duration, DwarfCalendar)

-- returns ticks since year zero
function Duration:getTicks()
    return self.year * TICKS_PER_YEAR + self.year_tick
end

-- returns an integer pair: (days since year zero), (day tick count)
function Duration:getDays()
    return self.year * DAYS_PER_YEAR + self:ticksToDays(), self:getDayTicks()
end

-- returns an integer pair: (months since year zero), (month tick count)
function Duration:getMonths()
    return self.year * MONTHS_PER_YEAR + self:ticksToMonths(), self:getMonthTicks()
end

-- returns parts of an elapsed time:
-- years,
-- months,          - since start of year
-- days,            - since start of month
-- day tick count   - since start of day
function Duration:getYearsMonthsDays()
    return self.year, self:ticksToMonths(), self:ticksToDayOfMonth(), self:getDayTicks()
end

function Duration:__add(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return Duration{ year = (self.year + other.year), year_tick = (self.year_tick + other.year_tick) }
end

function Duration:__sub(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return Duration{ year = (self.year - other.year) , year_tick = (self.year_tick - other.year_tick) }
end

return _ENV
