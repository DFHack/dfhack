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

-- returns months since the beginning of a year, starting from zero
local function monthOfYearTick(obj)
    return obj.year_tick // obj.ticks_per_month
end

-- returns ticks since the beginning of a month
local function monthTick(obj)
    return obj.year_tick % obj.ticks_per_month
end

-- returns days since beginning of a year, starting from zero
local function dayOfYearTick(obj)
    return obj.year_tick // obj.ticks_per_day
end

-- returns days since the beginning of a month, starting from zero
local function dayOfMonthTick(obj)
    return dayOfYearTick(obj) % obj.calendar.daysPerMonth()
end

local function daysToTicks(obj, days)
    return days * obj.ticks_per_day
end

local function monthsToTicks(obj, months)
    return months * obj.ticks_per_month
end

-- returns ticks since the beginning of a day
local function dayTick(obj)
    return obj.year_tick % obj.ticks_per_day
end

local function normalize(obj)
    if (obj.year_tick > obj.ticks_per_year) then
        obj.year = obj.year + (obj.year_tick // obj.ticks_per_year)
        obj.year_tick = obj.year_tick % obj.ticks_per_year
    elseif (obj.year_tick < 0) then
        -- going backwards in time, subtract year by at least one.
        obj.year = obj.year - math.max(1, math.abs(obj.year_tick) // obj.ticks_per_year)
        -- porting note: Lua's modulo operator applies floor division,
        -- hence year_tick will always be positive after assignment
        -- equivalent to: year_tick - (TICKS_PER_YEAR * (year_tick // TICKS_PER_YEAR))
        obj.year_tick = obj.year_tick % obj.ticks_per_year
    end
    return obj
end

-- sets rate at which time passes
local function setTickRate(obj)
    if (obj.mode == df.game_mode.ADVENTURE) then
        obj.ticks_per_day = ADVENTURE_TICKS_PER_DAY
    else
        obj.ticks_per_day = DWARF_TICKS_PER_DAY
    end

    obj.ticks_per_month = obj.ticks_per_day * DwarfCalendar.daysPerMonth()
    obj.ticks_per_year  = obj.ticks_per_day * DwarfCalendar.daysPerYear()
    return obj
end

local function addTicks(obj, ticks)
    ticks = tonumber(ticks)
    if not ticks then qerror('ticks must be a number') end
    obj.year_tick = obj.year_tick + ticks
    return normalize(obj)
end

local function addDays(obj, days)
    days = tonumber(days)
    if not days then qerror('days must be a number') end
    obj.year_tick = obj.year_tick + daysToTicks(obj, days)
    return normalize(obj)
end

local function addMonths(obj, months)
    months = tonumber(months)
    if not months then qerror('months must be a number') end
    obj.year_tick = obj.year_tick + monthsToTicks(obj, months)
    return normalize(obj)
end

local function addYears(obj, years)
    years = tonumber(years)
    if not years then qerror('years must be a number') end
    obj.year = obj.year + years
    return obj
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


local DATETIME_NAME = "datetime"
local DURATION_NAME = "duration"

--------------
-- DateTime --
--------------

-- Supports calendars with the property that
-- each month has the same number of days

DateTime = defclass(DateTime)

DateTime.ATTRS{
    year=0,
    year_tick=0,
    mode=df.game_mode.DWARF
}

function DateTime:init()
    self._name = DATETIME_NAME
    setTickRate(self)
    normalize(self)
end

-- returns an integer pair: (year), (year tick count)
function DateTime:getYear()
    return self.year, self.year_tick
end

function DateTime:setYear(year)
    year = tonumber(year)
    if not year then qerror('year must be a number') end
    self.year = year
    return self
end

function DateTime:setYearTick(ticks)
    ticks = tonumber(ticks)
    if not ticks then qerror('ticks must be a number') end
    self.year_tick = ticks
    return normalize(self)
end

-- returns an integer pair: (current day of year, from 1), (day tick count)
function DateTime:dayOfYear()
    return dayOfYearTick(self) + 1, dayTick(self)
end

function DateTime:setDayOfYear(day)
    day = tonumber(day)
    if not day or (day < 1 or day > DwarfCalendar.daysPerYear())
        then qerror('day must be between 1 and %d, inclusive'):format(DwarfCalendar.daysPerYear())
    end
    self.year_tick = daysToTicks(self, day-1)
    return self
end

-- returns an integer pair: (day of month starting from 1), (day tick count)
function DateTime:dayOfMonth()
    return dayOfMonthTick(self) + 1, dayTick(self)
end

function DateTime:setDayOfMonth(day, month)
    day = tonumber(day)
    if not day or (day < 1 or day > self.calendar.daysPerMonth())
        then qerror('day must be between 1 and %d, inclusive'):format(self.calendar.daysPerMonth())
    end

    month = tonumber(month)
    if not month then month = self:month()
    elseif (month < 1 or month > self.calendar.monthsPerYear()) then
        qerror('month must be between 1 and %d, inclusive'):format(self.calendar.monthsPerYear())
    end
    -- zero based when converting to ticks
    self.year_tick = monthsToTicks(self, month-1) + daysToTicks(self, day-1)
    return self
end

-- returns a string in ordinal form (e.g. 1st, 12th, 22nd, 101st, 111th, 133rd)
function DateTime:dayOfMonthWithSuffix()
    local d = self:dayOfMonth()
    return tostring(d)..getOrdinalSuffix(d)
end

-- returns an integer pair: (current month of the year, from 1), (month tick count)
function DateTime:month()
    return monthOfYearTick(self) + 1, monthTick(self)
end

function DateTime:setMonth(month)
    month = tonumber(month)
    if not month or (month < 1 or month > self.calendar.monthsPerYear()) then
        qerror('month must be between 1 and %d, inclusive'):format(self.calendar.monthsPerYear())
    end
    self.year_tick = monthsToTicks(self, month-1)
    return self
end

-- returns a string of the current month of the year
function DateTime:nameOfMonth()
    return DwarfCalendar.months()[self:month()] or error("bad index?")
end

-- returns ticks since the beginning of a day
function DateTime:dayTick()
    return dayTick(self)
end

-- sets the current day tick count
function DateTime:setDayTick(ticks)
    ticks = tonumber(ticks)
    -- prevent rollover to next day
    if not ticks or (ticks < 0 or ticks > self.ticks_per_day - 1) then
        qerror('ticks must be between 0 and %d, inclusive'):format(self.ticks_per_day - 1)
    end
    self.year_tick = (self.year_tick - dayTick(self)) + ticks
    return self
end


function DateTime:addTicks(ticks)
    return addTicks(self, ticks)
end

function DateTime:addDays(days)
    return addDays(self, days)
end

function DateTime:addMonths(months)
    return addMonths(self, months)
end

function DateTime:addYears(years)
    return addYears(self, years)
end

-- returns hours (24 hour format), minutes, seconds
function DateTime:time()
    -- probably only useful in adv mode where a day is 144x longer
    local h = dayTick(self) / (self.ticks_per_day / 24)
    local m = (h * 60) % 60
    local s = (m * 60) % 60
    -- return as integers, rounded down
    return h//1, m//1, s//1
end

function DateTime:setTime(hour, min, sec)
    -- we're setting time of the current day, so we'll have to
    -- ensure we're within bounds
    hour = tonumber(hour)
    if not hour or (hour < 0 or hour > 23) then
        qerror('hour must be a number between 0 and 23, inclusive') end

    min = tonumber(min)
    if not min or (min < 0 or min > 59) then
        qerror('min must be a number between 0 and 59, inclusive') end

    sec = tonumber(sec)
    if not sec or (sec < 0 or sec > 59) then
        qerror('sec must be a number between 0 and 59, inclusive') end

    local h = hour * (self.ticks_per_day / 24)
    local m = min * (h / 60)
    local s = sec * (m / 60)
    return self:setDayTick((h + m + s)//1)
end

function DateTime:__add(other)
    -- normalize() handles adjustments to year and year_tick
    return DateTime{ year = (self.year + other.year),
                     year_tick = (self.year_tick + other.year_tick),
                     mode = self.mode }
end

function DateTime:__sub(other)
    -- normalize() handles adjustments to year and year_tick
    if other._name == DURATION_NAME then
        return DateTime{ year = (self.year - other.year),
                         year_tick = (self.year_tick - other.year_tick),
                         mode = self.mode }
    end

    return Duration{ year = (self.year - other.year),
                     year_tick = (self.year_tick - other.year_tick),
                     mode = self.mode }
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
                     mode = other.mode }
end

function DateTime.now(game_mode)
    game_mode = game_mode or df.global.gamemode
    -- if game_mode is not given or not ADVENTURE then default to DWARF mode
    local ticks = (game_mode == df.game_mode.ADVENTURE) and
            (df.global.cur_year_tick_advmode) or (df.global.cur_year_tick)

    return DateTime{ year = df.global.cur_year,
                     year_tick = ticks,
                     mode = game_mode }
end

--------------
-- Duration --
--------------

Duration = defclass(Duration)

Duration.ATTRS{
    year=0,
    year_tick=0,
    mode=df.game_mode.DWARF
}

function Duration:init()
    self._name = DURATION_NAME
    setTickRate(self)
    normalize(self)
end

-- returns ticks since year zero
function Duration:ticks()
    -- Lua supports numbers up to 2^1024
    -- so in practice, we don't have to worry about overflow
    return self.year * self.ticks_per_year + self.year_tick
end

-- returns an integer pair: (days since year zero), (day tick count)
function Duration:days()
    return self.year * DwarfCalendar.daysPerYear() + dayOfYearTick(self), dayTick(self)
end

-- returns an integer pair: (months since year zero), (month tick count)
function Duration:months()
    return self.year * DwarfCalendar.monthsPerYear() + monthOfYearTick(self), monthTick(self)
end

-- returns parts of an elapsed time:
-- years,
-- months,          - since start of year
-- days,            - since start of month
-- day tick count   - since start of day
function Duration:yearsMonthsDays()
    return self.year, monthOfYearTick(self), dayOfMonthTick(self), dayTick(self)
end

function Duration:addTicks(ticks)
    return addTicks(self, ticks)
end

function Duration:addDays(days)
    return addDays(self, days)
end

function Duration:addMonths(months)
    return addMonths(self, months)
end

function Duration:addYears(years)
    return addYears(self, years)
end

function Duration.newFrom(other)
    return Duration{ year = other.year,
                     year_tick = other.year_tick,
                     mode = other.mode }
end

function Duration:__add(other)
    if other._name == DATETIME_NAME then
        return DateTime{ year = (self.year - other.year),
                         year_tick = (self.year_tick - other.year_tick),
                         mode = self.mode }
    end

    return Duration{ year = (self.year + other.year),
                     year_tick = (self.year_tick + other.year_tick),
                     mode = self.mode }
end

function Duration:__sub(other)
    if other._name == DATETIME_NAME then
        return DateTime{ year = (self.year - other.year),
                         year_tick = (self.year_tick - other.year_tick),
                         mode = self.mode }
    end

    return Duration{ year = (self.year - other.year),
                     year_tick = (self.year_tick - other.year_tick),
                     mode = self.mode }
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
