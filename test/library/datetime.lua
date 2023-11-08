config.target = 'core'

local dateT = require('datetime')

local DWARF_TICKS_PER_DAY = dateT.DWARF_TICKS_PER_DAY
local DWARF_TICKS_PER_MONTH = DWARF_TICKS_PER_DAY * dateT.DAYS_PER_MONTH

function test.getOrdinalSuffix()
    expect.eq(dateT.getOrdinalSuffix(1), 'st')
    expect.eq(dateT.getOrdinalSuffix(2), 'nd')
    expect.eq(dateT.getOrdinalSuffix(3), 'rd')
    expect.eq(dateT.getOrdinalSuffix(4), 'th')
    expect.eq(dateT.getOrdinalSuffix(11), 'th')
    expect.eq(dateT.getOrdinalSuffix(12), 'th')
    expect.eq(dateT.getOrdinalSuffix(13), 'th')
    expect.eq(dateT.getOrdinalSuffix(23), 'rd')
    expect.eq(dateT.getOrdinalSuffix(111), 'th')
    expect.eq(dateT.getOrdinalSuffix(112), 'th')
    expect.eq(dateT.getOrdinalSuffix(113), 'th')
    expect.eq(dateT.getOrdinalSuffix(123), 'rd')
end

function test.normalize()
    local year_ticks = DWARF_TICKS_PER_DAY * dateT.DAYS_PER_YEAR
    local dt = dateT.DateTime{ year = 1, year_tick = year_ticks+1 }
    expect.eq(dt.year, 2)
    expect.eq(dt.year_tick, 1)
    dt = dateT.DateTime{ year = 2, year_tick = -1 }
    expect.eq(dt.year, 1)
    expect.eq(dt.year_tick, year_ticks-1)
end

function test.setTickRate()
    local dt = dateT.DateTime{}

    dt:setTickRate(0)
    expect.eq(dt.ticks_per_day, DWARF_TICKS_PER_DAY)
    expect.eq(dt.ticks_per_month, DWARF_TICKS_PER_MONTH)
    expect.eq(dt.ticks_per_year, DWARF_TICKS_PER_DAY * dateT.DAYS_PER_YEAR)

    dt:setTickRate(df.game_mode.DWARF)
    expect.eq(dt.ticks_per_day, DWARF_TICKS_PER_DAY)
    expect.eq(dt.ticks_per_month, DWARF_TICKS_PER_MONTH)
    expect.eq(dt.ticks_per_year, DWARF_TICKS_PER_DAY * dateT.DAYS_PER_YEAR)

    dt:setTickRate(df.game_mode.ADVENTURE)
    expect.eq(dt.ticks_per_day, dateT.ADVENTURE_TICKS_PER_DAY)
    expect.eq(dt.ticks_per_month, dateT.ADVENTURE_TICKS_PER_DAY * dateT.DAYS_PER_MONTH)
    expect.eq(dt.ticks_per_year, dateT.ADVENTURE_TICKS_PER_DAY * dateT.DAYS_PER_YEAR)

    dt:setTickRate(1000)
    expect.eq(dt.ticks_per_day, 1000)
    expect.eq(dt.ticks_per_month, 1000 * dateT.DAYS_PER_MONTH)
    expect.eq(dt.ticks_per_year, 1000 * dateT.DAYS_PER_YEAR)
end

function test.addTicks()
    local dt = dateT.DateTime{ year_tick = 0 }
    dt:addTicks(1)
    expect.eq(dt.year_tick, 1)
    dt:addTicks(-1)
    expect.eq(dt.year_tick, 0)
end

function test.addDays()
    local dt = dateT.DateTime{ year_tick = 0 }
    dt:addDays(1)
    expect.eq(dt.year_tick, dt.ticks_per_day)
    dt:addDays(-1)
    expect.eq(dt.year_tick, 0)
end

function test.addMonths()
    local dt = dateT.DateTime{ year_tick = 0 }
    dt:addMonths(1)
    expect.eq(dt.year_tick, dt.ticks_per_month)
    dt:addMonths(-1)
    expect.eq(dt.year_tick, 0)
end

function test.addYears()
    local dt = dateT.DateTime{ year = 0 }
    dt:addYears(1)
    expect.eq(dt.year, 1)
    dt:addYears(-1)
    expect.eq(dt.year, 0)
end

function test.getYears()
    local dt = dateT.DateTime{ year = 1, year_tick = 1 }
    local y, t = dt:getYears()
    expect.eq(y, 1)
    expect.eq(t, 1)
end

function test.setDayOfMonth()
    local dt = dateT.DateTime{ year_tick = 0 }
    dt:setDayOfMonth(2, 1)
    expect.eq(dt.year_tick, dt.ticks_per_day)
end

function test.getDayOfMonth()
    local dt = dateT.DateTime{ year_tick = DWARF_TICKS_PER_MONTH + 1,
                               ticks_per_day = DWARF_TICKS_PER_DAY }
    local day, ticks = dt:getDayOfMonth()
    expect.eq(day, 1)
    expect.eq(ticks, 1)
end

function test.getDayOfMonthWithSuffix()
    local dt = dateT.DateTime{ year_tick = DWARF_TICKS_PER_MONTH + 1,
                               ticks_per_day = DWARF_TICKS_PER_DAY }
    local day = dt:getDayOfMonthWithSuffix()
    expect.eq(day, '1st')
end

function test.getDayOfYear()
    local dt = dateT.DateTime{ year_tick = DWARF_TICKS_PER_MONTH + 1,
                               ticks_per_day = DWARF_TICKS_PER_DAY }
    local day, ticks = dt:getDayOfYear()
    expect.eq(day, dateT.DAYS_PER_MONTH+1)
    expect.eq(ticks, 1)
end

function test.getMonth()
    local dt = dateT.DateTime{ year_tick = DWARF_TICKS_PER_MONTH + 1,
                               ticks_per_day = DWARF_TICKS_PER_DAY }
    local month, ticks = dt:getMonth()
    expect.eq(month, 2)
    expect.eq(ticks, 1)
end

function test.getNameOfMonth()
    local dt = dateT.DateTime{ year_tick = DWARF_TICKS_PER_MONTH + 1,
                               ticks_per_day = DWARF_TICKS_PER_DAY }
    local name = dt:getNameOfMonth()
    expect.eq(name, 'Slate')
end

function test.ticksToDays()
    local dt = dateT.DateTime{ year_tick = DWARF_TICKS_PER_MONTH + 1,
                               ticks_per_day = DWARF_TICKS_PER_DAY }
    local days = dt:ticksToDays()
    expect.eq(days, dateT.DAYS_PER_MONTH)
end

function test.ticksToDayOfMonth()
    local dt = dateT.DateTime{ year_tick = DWARF_TICKS_PER_MONTH + 1,
                               ticks_per_day = DWARF_TICKS_PER_DAY }
    local days = dt:ticksToDayOfMonth()
    expect.eq(days, 0)
end

function test.ticksToMonths()
    local dt = dateT.DateTime{ year_tick = DWARF_TICKS_PER_MONTH + 1,
                               ticks_per_day = DWARF_TICKS_PER_DAY }
    local m = dt:ticksToMonths()
    expect.eq(m, 1)
end

function test.getDayTicks()
    local dt = dateT.DateTime{ year_tick = DWARF_TICKS_PER_MONTH + 1,
                               ticks_per_day = DWARF_TICKS_PER_DAY }
    local t = dt:getDayTicks()
    expect.eq(t, 1)
end

function test.getMonthTicks()
    local dt = dateT.DateTime{ year_tick = DWARF_TICKS_PER_MONTH + 1,
                               ticks_per_day = DWARF_TICKS_PER_DAY }
    local t = dt:getMonthTicks()
    expect.eq(t, 1)
end

function test.daysToTicks()
    local dt = dateT.DateTime{ ticks_per_day = DWARF_TICKS_PER_DAY }
    local t = dt:daysToTicks(1)
    expect.eq(t, DWARF_TICKS_PER_DAY)
end

function test.monthsToTicks()
    local dt = dateT.DateTime{ ticks_per_day = DWARF_TICKS_PER_DAY }
    local t = dt:monthsToTicks(1)
    expect.eq(t, DWARF_TICKS_PER_MONTH)
end

function test.isFullMoon()
    local dt = dateT.DateTime{}
    expect.false_(dt:setDayOfMonth(1, 1):isFullMoon())
    expect.true_(dt:setDayOfMonth(25, 1):isFullMoon())
end

function test.nextFullMoon()
    local dt = dateT.DateTime{}
    expect.eq(25, dt:setDayOfMonth(1, 1):nextFullMoon():getDayOfMonth())
    expect.eq(28, dt:setDayOfMonth(2, 12):nextFullMoon():getDayOfMonth())
end

function test.calendar_ops()
    local dt_a = dateT.DwarfCalendar{ year = 1,
                                      year_tick = DWARF_TICKS_PER_DAY,
                                      ticks_per_day = DWARF_TICKS_PER_DAY }
    local dt_b = dateT.DwarfCalendar{ year = 1,
                                      year_tick = DWARF_TICKS_PER_DAY + 1,
                                      ticks_per_day = DWARF_TICKS_PER_DAY }
    local dt = dt_b - dt_a
    expect.eq(dt.year, 0)
    expect.eq(dt.year_tick, 1)

    dt = dt_a + dt_b
    expect.eq(dt.year, 2)
    expect.eq(dt.year_tick, DWARF_TICKS_PER_DAY * 2 + 1)
end

function test.dateTime_ops()
    local dt_a = dateT.DateTime{ year = 1,
                                 year_tick = DWARF_TICKS_PER_DAY,
                                 ticks_per_day = DWARF_TICKS_PER_DAY }
    local dt_b = dateT.DateTime{ year = 1,
                                 year_tick = DWARF_TICKS_PER_DAY + 1,
                                 ticks_per_day = DWARF_TICKS_PER_DAY }
    local dt = dt_b - dt_a
    expect.eq(dt.year, 0)
    expect.eq(dt.year_tick, 1)

    dt = dt_a + dt_b
    expect.eq(dt.year, 2)
    expect.eq(dt.year_tick, DWARF_TICKS_PER_DAY * 2 + 1)
end

function test.dateTime_getTime()
    -- 1 hour = 7200 tick
    -- 1 min = 120 tick
    -- 1 sec = 2 tick
    -- 7324 is like 1.99999999999 secs in ADV time but truncated due to rounding
    local dt = dateT.DateTime{ year_tick = 7325,
                               ticks_per_day = dateT.ADVENTURE_TICKS_PER_DAY }
    local h, m, s = dt:getTime()
    expect.eq(1, h)
    expect.eq(1, m)
    expect.eq(2, s)
end

function test.dateTime_now()
    expect.eq('table', type(dateT.DateTime.now()))
end

function test.toDuration()
    local dt = dateT.DateTime{}
    expect.eq('table', type(dt:toDuration()))
end

function test.duration_getTicks()
    local d = dateT.Duration{ year_tick = 1 }
    expect.eq(1, d:getTicks())
end

function test.duration_getDays()
    local d = dateT.Duration{ year = 1,
                              year_tick = DWARF_TICKS_PER_DAY + 1,
                              ticks_per_day = DWARF_TICKS_PER_DAY }
    local days, ticks = d:getDays()
    expect.eq(days, dateT.DAYS_PER_YEAR + 1)
    expect.eq(ticks, 1)
end

function test.duration_getMonths()
    local d = dateT.Duration{ year = 1,
                              year_tick = DWARF_TICKS_PER_DAY + 1,
                              ticks_per_day = DWARF_TICKS_PER_DAY }
    local months, ticks = d:getMonths()
    expect.eq(months, dateT.MONTHS_PER_YEAR)
    expect.eq(ticks, DWARF_TICKS_PER_DAY + 1)
end

function test.duration_getYearsMonthsDays()
    local d = dateT.Duration{ year = 1,
                              year_tick = DWARF_TICKS_PER_MONTH + DWARF_TICKS_PER_DAY + 1,
                              ticks_per_day = DWARF_TICKS_PER_DAY }
    local y, m, d, t = d:getYearsMonthsDays()
    expect.eq(y, 1)
    expect.eq(m, 1)
    expect.eq(d, 1)
    expect.eq(t, 1)
end

function test.duration_ops()
    local dt_a = dateT.Duration{ year = 1,
                                 year_tick = DWARF_TICKS_PER_DAY,
                                 ticks_per_day = DWARF_TICKS_PER_DAY }
    local dt_b = dateT.Duration{ year = 1,
                                 year_tick = DWARF_TICKS_PER_DAY + 1,
                                 ticks_per_day = DWARF_TICKS_PER_DAY }
    local dt = dt_b - dt_a
    expect.eq(dt.year, 0)
    expect.eq(dt.year_tick, 1)

    dt = dt_a + dt_b
    expect.eq(dt.year, 2)
    expect.eq(dt.year_tick, DWARF_TICKS_PER_DAY * 2 + 1)
end
