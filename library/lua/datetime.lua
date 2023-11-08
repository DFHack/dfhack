local _ENV = mkmodule('datetime')

DAYS_PER_MONTH    = 28
DAYS_PER_YEAR     = 336
MONTHS_PER_YEAR   = 12

DWARF_TICKS_PER_DAY     = 1200
--local DWARF_TICKS_PER_MONTH = DWARF_TICKS_PER_DAY * DAYS_PER_MONTH
--local DWARF_TICKS_PER_YEAR  = DWARF_TICKS_PER_MONTH * MONTHS_PER_YEAR

ADVENTURE_TICKS_PER_DAY = 172800
--local ADVENTURE_TICKS_PER_MONTH = ADVENTURE_TICKS_PER_DAY * DAYS_PER_MONTH
--local ADVENTURE_TICKS_PER_YEAR  = ADVENTURE_TICKS_PER_MONTH * MONTHS_PER_YEAR

--local TICKS_PER_DAY   = DWARF_TICKS_PER_DAY
--local TICKS_PER_MONTH = DWARF_TICKS_PER_MONTH
--local TICKS_PER_YEAR  = TICKS_PER_MONTH * MONTHS_PER_YEAR

CALENDAR_MONTHS = {
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
    'Obsidian'
}

-- day -> month
local CALENDAR_FULLMOON_MAP = {
    [25] = 1,
    [23] = 2,
    [21] = 3,
    [19] = 4,
    [17] = 5,
    [15] = 6,
    [13] = 7,
    [10] = 8,
    [8]  = 9,
    [6]  = 10,
    [4]  = 11,
    [2]  = 12,
    [28] = 12
}

-- Ordinal suffix rules found here: https://en.wikipedia.org/wiki/Ordinal_indicator
-- global for scripts to use
function getOrdinalSuffix(ordinal)
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

DwarfCalendar = defclass(DwarfCalendar)

DwarfCalendar.ATTRS{
    year=0,
    year_tick=0,
    ticks_per_day=DWARF_TICKS_PER_DAY
}

function DwarfCalendar:init()
    self:setTickRate(self.ticks_per_day)
    self:normalize()
end

function DwarfCalendar:setTickRate(ticks_per_day)
    -- game_mode.DWARF and .ADVENTURE values are < 10
    -- too low for sane tick rates, so we can utilize em.
    -- this might be useful if the caller wants to set by game mode
    if (ticks_per_day == df.game_mode.DWARF) then
        self.ticks_per_day = DWARF_TICKS_PER_DAY
    elseif (ticks_per_day == df.game_mode.ADVENTURE) then
        self.ticks_per_day = ADVENTURE_TICKS_PER_DAY
    else
        self.ticks_per_day = (ticks_per_day > 0) and ticks_per_day or DWARF_TICKS_PER_DAY
    end
    self.ticks_per_month = self.ticks_per_day * DAYS_PER_MONTH
    self.ticks_per_year  = self.ticks_per_day * DAYS_PER_YEAR
    return self
end

function DwarfCalendar:addTicks(ticks)
    self.year_tick = self.year_tick + ticks
    self:normalize()
    return self
end

function DwarfCalendar:addDays(days)
    self.year_tick = self.year_tick + self:daysToTicks(days)
    self:normalize()
    return self
end

function DwarfCalendar:addMonths(months)
    self.year_tick = self.year_tick + self:monthsToTicks(months)
    self:normalize()
    return self
end

function DwarfCalendar:addYears(years)
    self.year = self.year + years
    return self
end

function DwarfCalendar:setDayOfMonth(day, month)
    month = month or self:getMonth()
    -- zero based when converting to ticks
    self.year_tick = self:monthsToTicks(month-1) + self:daysToTicks(day-1)
    return self
end

-- should this be named getYear()?
-- returns an integer pair: (year), (year tick count)
function DwarfCalendar:getYears()
    return self.year, self.year_tick
end

-- returns an integer pair: (day of month starting from 1), (day tick count)
function DwarfCalendar:getDayOfMonth()
    return self:ticksToDayOfMonth() + 1, self:getDayTicks()
end

-- returns a string in ordinal form (e.g. 1st, 12th, 22nd, 101st, 111th, 133rd)
function DwarfCalendar:getDayOfMonthWithSuffix()
    local d = self:getDayOfMonth()
    return tostring(d)..getOrdinalSuffix(d)
end

-- returns an integer pair: (current day of year, from 1), (day tick count)
function DwarfCalendar:getDayOfYear()
    return self:ticksToDays() + 1, self:getDayTicks()
end

-- returns an integer pair: (current month of the year, from 1), (month tick count)
function DwarfCalendar:getMonth()
    return self:ticksToMonths() + 1, self:getMonthTicks()
end

-- returns a string of the current month of the year
function DwarfCalendar:getNameOfMonth()
    return CALENDAR_MONTHS[self:getMonth()] or error("bad index?")
end

-- returns days since beginning of a year, starting from zero
function DwarfCalendar:ticksToDays()
    return self.year_tick // self.ticks_per_day
end

-- returns days since the beginning of a month, starting from zero
function DwarfCalendar:ticksToDayOfMonth()
    return self:ticksToDays() % DAYS_PER_MONTH
end

-- returns months since the beginning of a year, starting from zero
function DwarfCalendar:ticksToMonths()
    return self.year_tick // self.ticks_per_month
end

-- returns ticks since the beginning of a day
function DwarfCalendar:getDayTicks()
    return self.year_tick % self.ticks_per_day
end

-- returns ticks since the beginning of a month
function DwarfCalendar:getMonthTicks()
    return self.year_tick % self.ticks_per_month
end

function DwarfCalendar:daysToTicks(days)
    return days * self.ticks_per_day
end

function DwarfCalendar:monthsToTicks(months)
    return months * self.ticks_per_month
end

function DwarfCalendar:isFullMoon()
    return (self:getMonth() == CALENDAR_FULLMOON_MAP[self:getDayOfMonth()])
        and true or false
end

function DwarfCalendar:nextFullMoon()
    local dateT = DateTime{ year = self.year,
                            year_tick = self.year_tick,
                            ticks_per_day = self.ticks_per_day }
    if (dateT:isFullMoon()) then dateT:addDays(1) end

    local cur_m = dateT:getMonth()
    local fm = {
        25,
        23,
        21,
        19,
        17,
        15,
        13,
        10,
        8,
        6,
        4,
        2, 28
    }

    if (dateT:getDayOfMonth() < fm[cur_m]) then
        dateT:setDayOfMonth(fm[cur_m], cur_m)
    else
        -- Next full moon is on the next month
        -- or next year if addDays() rolled us over.
        -- Obsidian is special since it has 2 full moons
        -- also handles the case when Obsidian day is between 2 and 28 exclusive
        dateT:setDayOfMonth(fm[cur_m+1], cur_m)
    end

    return dateT
end

function DwarfCalendar:normalize()
    if (self.year_tick > self.ticks_per_year) then
        self.year = self.year + (self.year_tick // self.ticks_per_year)
        self.year_tick = self.year_tick % self.ticks_per_year
    elseif (self.year_tick < 0) then
        -- going backwards in time, subtract year by at least one.
        self.year = self.year - math.max(1, math.abs(self.year_tick) // self.ticks_per_year)
        -- Lua's modulo operator applies floor division,
        -- hence year_tick will always be positive after assignment
        -- equivalent to: year_tick - (TICKS_PER_YEAR * (year_tick // TICKS_PER_YEAR))
        self.year_tick = self.year_tick % self.ticks_per_year
    end
end

function DwarfCalendar:__add(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return DwarfCalendar{ year = (self.year + other.year),
                          year_tick = (self.year_tick + other.year_tick),
                          ticks_per_day = self.ticks_per_day }
end

function DwarfCalendar:__sub(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return DwarfCalendar{ year = (self.year - other.year),
                          year_tick = (self.year_tick - other.year_tick),
                          ticks_per_day = self.ticks_per_day }
end

function DwarfCalendar:_debugOps(other)
    print('first: '..self.year,self.year_tick)
    print('second: '..other.year,other.year_tick)
end

DateTime = defclass(DateTime, DwarfCalendar)

-- returns hours (24 hour format), minutes, seconds
function DateTime:getTime()
    -- probably only useful in adv mode where a day is 144x longer
    local h = self:getDayTicks() / (self.ticks_per_day / 24)
    local m = (h * 60) % 60
    local s = (m * 60) % 60
    -- return as integers, rounded down
    return h//1, m//1, s//1
end

-- TODO: maybe add setTime()

--function DateTime:daysTo(other)
--    local dateT = other - self
--    return dateT.year * DAYS_PER_YEAR + dateT:ticksToDays()
--end
-- maybe useful addition
--function DateTime:daysTo(other)
--    return (other - self):toDuration().getDays()
--end

-- Alternatively, instead of a Duration object,
-- we can simply synthesize a table with
-- key value pairs of years, months, days, ticks.
-- If table, it could optionally take an argument or variadic
-- where the caller provides the time unit specifiers
-- i.e. getDuration/toDuration('ymd') or toDuration('y', 'm', 'd'), etc
function DateTime:toDuration()
    return Duration{ year = self.year,
                     year_tick = self.year_tick,
                     ticks_per_day = self.ticks_per_day }
end

function DateTime:__add(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return DateTime{ year = (self.year + other.year),
                     year_tick = (self.year_tick + other.year_tick),
                     ticks_per_day = self.ticks_per_day }
end

-- might make sense to return a Duration here
function DateTime:__sub(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return DateTime{ year = (self.year - other.year),
                     year_tick = (self.year_tick - other.year_tick),
                     ticks_per_day = self.ticks_per_day }
end

function DateTime.now(game_mode)
    game_mode = game_mode or df.global.gamemode
    -- if game_mode is not given or not ADVENTURE then default to DWARF mode
    local ticks = (game_mode == df.game_mode.ADVENTURE) and
            (df.global.cur_year_tick_advmode) or (df.global.cur_year_tick)
    -- Tick rate defaults to DWARF mode, we should set the tick rate here as well
    -- For a custom rate the caller can use setTickRate() or we can add a second
    -- optional parameter
    return DateTime{ year = df.global.cur_year,
                     year_tick = ticks }:setTickRate(game_mode)
end

Duration = defclass(Duration, DwarfCalendar)

-- returns ticks since year zero
function Duration:getTicks()
    return self.year * self.ticks_per_year + self.year_tick
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
    return Duration{ year = (self.year + other.year),
                     year_tick = (self.year_tick + other.year_tick),
                     ticks_per_day = self.ticks_per_day }
end

function Duration:__sub(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return Duration{ year = (self.year - other.year),
                     year_tick = (self.year_tick - other.year_tick),
                     ticks_per_day = self.ticks_per_day }
end

return _ENV
