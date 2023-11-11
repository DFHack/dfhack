local _ENV = mkmodule('datetime')

DAYS_PER_MONTH    = 28
DAYS_PER_YEAR     = 336
MONTHS_PER_YEAR   = 12

DWARF_TICKS_PER_DAY     = 1200
ADVENTURE_TICKS_PER_DAY = 172800

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

----------------------------
------ DwarfCalendar -------
-- A very simple calendar --
----------------------------

DwarfCalendar = defclass(DwarfCalendar)

function DwarfCalendar.daysPerMonth()
    return DAYS_PER_MONTH
end

function DwarfCalendar.daysPerYear()
    return DAYS_PER_YEAR
end

function DwarfCalendar.monthsPerYear()
    return MONTHS_PER_YEAR
end

function DwarfCalendar.months()
    return CALENDAR_MONTHS
end

function DwarfCalendar.isFullMoon(dateTime)
    return (dateTime:month() == CALENDAR_FULLMOON_MAP[dateTime:dayOfMonth()])
        and true or false
end

function DwarfCalendar.nextFullMoon(dateTime)
    local nextDateT = DateTime.newFrom(dateTime)
    if (DwarfCalendar.isFullMoon(nextDateT)) then nextDateT:addDays(1) end

    local cur_m = nextDateT:month()
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

    if (nextDateT:dayOfMonth() < fm[cur_m]) then
        nextDateT:setDayOfMonth(fm[cur_m], cur_m)
    else
        -- Next full moon is on the next month
        -- or next year if addDays() rolled us over.
        -- Obsidian is special since it has 2 full moons
        -- also handles the case when Obsidian day is between 2 and 28 exclusive
        nextDateT:setDayOfMonth(fm[cur_m+1], cur_m)
    end

    return nextDateT
end

--------------
-- DateTime --
--------------

-- Only supports calendars with the property that
-- each month has the same number of days

DateTime = defclass(DateTime)

DateTime.ATTRS{
    year=0,
    year_tick=0,
    ticks_per_day=DWARF_TICKS_PER_DAY,
    calendar=DwarfCalendar
}

function DateTime:init()
    self:setTickRate(self.ticks_per_day)
    self:normalize()
end

-- sets rate at which time passes
function DateTime:setTickRate(ticks_per_day)
    -- game_mode.DWARF and .ADVENTURE values are < 10
    -- too low for sane tick rates, so we can utilize em.
    -- this might be useful if the caller wants to set by game mode
    if (ticks_per_day == df.game_mode.DWARF) then
        self.ticks_per_day = DWARF_TICKS_PER_DAY
    elseif (ticks_per_day == df.game_mode.ADVENTURE) then
        self.ticks_per_day = ADVENTURE_TICKS_PER_DAY
    else
        self.ticks_per_day = (ticks_per_day > 0)
            and ticks_per_day or DWARF_TICKS_PER_DAY
    end
    self.ticks_per_month = self.ticks_per_day * self.calendar.daysPerMonth()
    self.ticks_per_year  = self.ticks_per_day * self.calendar.daysPerYear()
    return self
end

function DateTime:normalize()
    if (self.year_tick > self.ticks_per_year) then
        self.year = self.year + (self.year_tick // self.ticks_per_year)
        self.year_tick = self.year_tick % self.ticks_per_year
    elseif (self.year_tick < 0) then
        -- going backwards in time, subtract year by at least one.
        self.year = self.year - math.max(1, math.abs(self.year_tick) // self.ticks_per_year)
        -- porting note: Lua's modulo operator applies floor division,
        -- hence year_tick will always be positive after assignment
        -- equivalent to: year_tick - (TICKS_PER_YEAR * (year_tick // TICKS_PER_YEAR))
        self.year_tick = self.year_tick % self.ticks_per_year
    end
end

-- returns an integer pair: (year), (year tick count)
function DateTime:getYear()
    return self.year, self.year_tick
end

function DateTime:setYear(year)
    if not year then qerror('year must be a number') end
    self.year = year
    return self
end

function DateTime:setYearTick(ticks)
    if not ticks then qerror('ticks must be a number') end
    self.year_tick = ticks
    self:normalize()
    return self
end

-- returns an integer pair: (current day of year, from 1), (day tick count)
function DateTime:dayOfYear()
    return self:dayOfYearTick() + 1, self:dayTick()
end

function DateTime:setDayOfYear(day)
    if not day or (day < 1 or day > self.calendar.daysPerYear())
        then qerror('day must be between 1 and %d, inclusive'):format(self.calendar.daysPerYear())
    end
    self.year_tick = self:daysToTicks(day-1)
    return self
end

-- returns an integer pair: (day of month starting from 1), (day tick count)
function DateTime:dayOfMonth()
    return self:dayOfMonthTick() + 1, self:dayTick()
end

function DateTime:setDayOfMonth(day, month)
    if not day or (day < 1 or day > self.calendar.daysPerMonth())
        then qerror('day must be between 1 and %d, inclusive'):format(self.calendar.daysPerMonth())
    end

    if not month then month = self:month()
    elseif (month < 1 or month > self.calendar.monthsPerYear()) then
        qerror('month must be between 1 and %d, inclusive'):format(self.calendar.monthsPerYear())
    end
    -- zero based when converting to ticks
    self.year_tick = self:monthsToTicks(month-1) + self:daysToTicks(day-1)
    return self
end

-- returns a string in ordinal form (e.g. 1st, 12th, 22nd, 101st, 111th, 133rd)
function DateTime:dayOfMonthWithSuffix()
    local d = self:dayOfMonth()
    return tostring(d)..getOrdinalSuffix(d)
end

-- returns an integer pair: (current month of the year, from 1), (month tick count)
function DateTime:month()
    return self:monthOfYearTick() + 1, self:monthTick()
end

function DateTime:setMonth(month)
    if not month or (month < 1 or month > self.calendar.monthsPerYear()) then
        qerror('month must be between 1 and %d, inclusive'):format(self.calendar.monthsPerYear())
    end
    self.year_tick = self:monthsToTicks(month-1)
    return self
end

-- returns a string of the current month of the year
function DateTime:nameOfMonth()
    return self.calendar.months()[self:month()] or error("bad index?")
end

-- returns days since beginning of a year, starting from zero
function DateTime:dayOfYearTick()
    return self.year_tick // self.ticks_per_day
end

-- returns days since the beginning of a month, starting from zero
function DateTime:dayOfMonthTick()
    return self:dayOfYearTick() % self.calendar.daysPerMonth()
end

-- returns ticks since the beginning of a day
function DateTime:dayTick()
    return self.year_tick % self.ticks_per_day
end

-- sets the current day tick count
function DateTime:setDayTick(ticks)
    -- prevent rollover to next day
    if not ticks or (ticks < 0 or ticks > self.ticks_per_day - 1) then
        qerror('ticks must be between 0 and %d, inclusive'):format(self.ticks_per_day - 1)
    end
    self.year_tick = (self.year_tick - self:dayTick()) + ticks
    return self
end

-- returns months since the beginning of a year, starting from zero
function DateTime:monthOfYearTick()
    return self.year_tick // self.ticks_per_month
end

-- returns ticks since the beginning of a month
function DateTime:monthTick()
    return self.year_tick % self.ticks_per_month
end

function DateTime:daysToTicks(days)
    return days * self.ticks_per_day
end

function DateTime:monthsToTicks(months)
    return months * self.ticks_per_month
end

function DateTime:addTicks(ticks)
    if not ticks then qerror('ticks must be a number') end
    self.year_tick = self.year_tick + ticks
    self:normalize()
    return self
end

function DateTime:addDays(days)
    if not days then qerror('days must be a number') end
    self.year_tick = self.year_tick + self:daysToTicks(days)
    self:normalize()
    return self
end

function DateTime:addMonths(months)
    if not months then qerror('months must be a number') end
    self.year_tick = self.year_tick + self:monthsToTicks(months)
    self:normalize()
    return self
end

function DateTime:addYears(years)
    if not years then qerror('years must be a number') end
    self.year = self.year + years
    return self
end

-- returns hours (24 hour format), minutes, seconds
function DateTime:time()
    -- probably only useful in adv mode where a day is 144x longer
    local h = self:dayTick() / (self.ticks_per_day / 24)
    local m = (h * 60) % 60
    local s = (m * 60) % 60
    -- return as integers, rounded down
    return h//1, m//1, s//1
end

function DateTime:setTime(hour, min, sec)
    -- we're setting time of the current day, so we'll have to
    -- ensure we're within bounds
    if not hour or (hour < 0 or hour > 23) then
        qerror('hour must be a number between 0 and 23, inclusive') end
    if not min or (min < 0 or min > 59) then
        qerror('min must be a number between 0 and 59, inclusive') end
    if not sec or (sec < 0 or sec > 59) then
        qerror('sec must be a number between 0 and 59, inclusive') end

    local h = hour * (self.ticks_per_day / 24)
    local m = min * (h / 60)
    local s = sec * (m / 60)
    return self:setDayTick((h + m + s)//1)
end

--function DateTime:daysTo(other)
--    local dateT = other - self
--    return dateT.year * DAYS_PER_YEAR + dateT:dayOfYearTick()
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
                     ticks_per_day = self.ticks_per_day,
                     calendar = self.calendar }
end

function DateTime:__add(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return DateTime{ year = (self.year + other.year),
                     year_tick = (self.year_tick + other.year_tick),
                     ticks_per_day = self.ticks_per_day,
                     calendar = self.calendar }
end

-- might make sense to return a Duration here
function DateTime:__sub(other)
    if DEBUG then self:_debugOps(other) end
    -- normalize() handles adjustments to year and year_tick
    return DateTime{ year = (self.year - other.year),
                     year_tick = (self.year_tick - other.year_tick),
                     ticks_per_day = self.ticks_per_day,
                     calendar = self.calendar }
end

function DateTime:_isEq(other)
    return (self.year == other.year and self.year_tick == other.year_tick)
        and true or false
end

function DateTime:_isLt(other)
    return (self.year < other.year or
        (self.year == other.year and self.year_tick < other.year_tick))
            and true or false
end

function DateTime:__eq(other)
    return self:_isEq(other)
end

function DateTime:__lt(other)
    return self:_isLt(other)
end

function DateTime:__le(other)
    return (self:_isLt(other) or self:_isEq(other))
end

function DateTime.newFrom(other)
    return DateTime{ year = other.year,
                     year_tick = other.year_tick,
                     ticks_per_day = other.ticks_per_day,
                     calendar = other.calendar }
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

--------------
-- Duration --
--------------

-- A Duration can also represent a date and time.

Duration = defclass(Duration, DateTime)

-- returns ticks since year zero
function Duration:ticks()
    -- Lua supports numbers up to 2^1024
    -- so in practice, we don't have to worry about overflow
    return self.year * self.ticks_per_year + self.year_tick
end

-- returns an integer pair: (days since year zero), (day tick count)
function Duration:days()
    return self.year * self.calendar.daysPerYear() + self:dayOfYearTick(), self:dayTick()
end

-- returns an integer pair: (months since year zero), (month tick count)
function Duration:months()
    return self.year * self.calendar.monthsPerYear() + self:monthOfYearTick(), self:monthTick()
end

-- returns parts of an elapsed time:
-- years,
-- months,          - since start of year
-- days,            - since start of month
-- day tick count   - since start of day
function Duration:yearsMonthsDays()
    return self.year, self:monthOfYearTick(), self:dayOfMonthTick(), self:dayTick()
end

function Duration:__add(other)
    -- normalize() handles adjustments to year and year_tick
    return Duration{ year = (self.year + other.year),
                     year_tick = (self.year_tick + other.year_tick),
                     ticks_per_day = self.ticks_per_day,
                     calendar = self.calendar }
end

function Duration:__sub(other)
    -- normalize() handles adjustments to year and year_tick
    return Duration{ year = (self.year - other.year),
                     year_tick = (self.year_tick - other.year_tick),
                     ticks_per_day = self.ticks_per_day,
                     calendar = self.calendar }
end
function Duration:__eq(other)
    return self:_isEq(other)
end

function Duration:__lt(other)
    return self:_isLt(other)
end

function Duration:__le(other)
    return (self:_isLt(other) or self:_isEq(other))
end

return _ENV
