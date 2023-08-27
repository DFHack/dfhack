-- Change the duration of the "Updating World" timeskip preceding the start of a new game.
-- Author: Atomic Chicken

local usage = [====[

set-timeskip-duration
=====================
Starting a new fortress/adventurer session is preceded by
an "Updating World" process which is normally 2 weeks long.
This script allows you to modify the duration of this timeskip,
enabling you to jump into the game earlier or later than usual.

You can use this at any point before the timeskip begins
(for example, while still at the "Start Playing" menu).

It is also possible to run the script while the world is updating,
which can be useful if you decide to end the process earlier
or later than initially planned.

Note that the change in timeskip duration will persist until either

- the game is closed,
- the "-clear" argument is used (see below),
- or it is overwritten by setting a new duration.

The timeskip duration can be specified using any combination of
the following arguments.

Usage::

    -ticks X
        Replace "X" with a positive integer (or 0)
        Adds the specified number of ticks to the timeskip duration
        The following conversions may help you calculate this:
            1 tick = 72 seconds = 1 minute 12 seconds
            50 ticks = 60 minutes = 1 hour
            1200 ticks = 24 hours = 1 day
            8400 ticks = 7 days = 1 week
            33600 ticks = 4 weeks = 1 month
            403200 ticks = 12 months = 1 year

    -years X
        Replace "X" with a positive integer (or 0)
        Adds (403200 multiplied by X) ticks to the timeskip duration

    -months X
        Replace "X" with a positive integer (or 0)
        Adds (33600 multiplied by X) ticks to the timeskip duration

    -days X
        Replace "X" with a positive integer (or 0)
        Adds (1200 multiplied by X) ticks to the timeskip duration

    -hours X
        Replace "X" with a positive integer (or 0)
        Adds (50 multiplied by X) ticks to the timeskip duration

    -clear
        This resets the timeskip duration to its default value.
        Note that it won't affect timeskips which have already begun.

Example::

    set-timeskip-duration -ticks 851249
        Sets the end of the timeskip to
        2 years, 1 month, 9 days, 8 hours, 58 minutes, 48 seconds
        from the current date.

    set-timeskip-duration -years 2 -months 1 -days 9 -hours 8 -ticks 49
        Does the same thing as the previous example

]====]

local utils = require 'utils'

function printTimeskipCalendarDuration(ticks)
  print("Timeskip duration set to " .. ticks .. " tick" .. (ticks == 1 and "." or "s."))
  if ticks > 0 then
    print("This is equivalent to:")
    local years = math.floor(ticks/403200)
    ticks = ticks%403200
    if years > 0 then
      print("  " .. tostring(years) .. " year" .. (years == 1 and "" or "s"))
    end
    local months = math.floor(ticks/33600)
    ticks = ticks%33600
    if months > 0 then
      print("  " .. tostring(months) .. " month" .. (months == 1 and "" or "s"))
    end
    local days = math.floor(ticks/1200)
    ticks = ticks%1200
    if days > 0  then
      print("  " .. tostring(days) .. " day" .. (days == 1 and "" or "s"))
    end
    local hours = math.floor(ticks/50)
    ticks = ticks%50
    if hours > 0 then
      print("  " .. tostring(hours) .. " hour" .. (hours == 1 and "" or "s"))
    end
    local minutes = math.floor(6*ticks/5)
    ticks = ticks - (minutes*5)/6
    if minutes > 0 then
      print("  " .. tostring(minutes) .. " minute" .. (minutes == 1 and "" or "s"))
    end
    local seconds = math.ceil(72*ticks)
    if seconds > 0 then
      print("  " .. tostring(seconds) .. " second" .. (seconds == 1 and "" or "s"))
    end
  end
end

function getTargetDate(ticks)
  local targetYear = tonumber(df.global.cur_year)
  local targetTick = tonumber(df.global.cur_year_tick)
  if targetTick + ticks >= 403200 then
    ticks = ticks - (403200 - targetTick)
    targetTick = 0
    targetYear = targetYear + 1
  end
  targetYear = targetYear + math.floor(ticks/403200)
  targetTick = targetTick + (ticks%403200)
  return targetYear, targetTick
end

function setTargetDate(scr, ticks) -- df.viewscreen_update_regionst
  local targetYear, targetTick = getTargetDate(ticks)
  scr.year = targetYear
  scr.year_tick = targetTick
end

local validArgs = utils.invert({
  'ticks',
  'years',
  'months',
  'days',
  'hours',
  'clear',
  'help'
})
local args = utils.processArgs({...}, validArgs)

if args.help then
  print(usage)
  return
end

if args.clear then
  dfhack.onStateChange.TimeskipMonitor = nil
  print("Timeskip duration reset to default.")
  return
end

local ticks

if args.ticks then
  if tonumber(args.ticks) and tonumber(args.ticks) >= 0 then
    ticks = (ticks or 0) + tonumber(args.ticks)
  else
    qerror("Invalid -ticks duration: " .. tostring(args.ticks))
  end
end

if args.years then
  if tonumber(args.years) and tonumber(args.years) >= 0 then
    ticks = (ticks or 0) + tonumber(args.years)*403200
  else
    qerror("Invalid -years duration: " .. tostring(args.years))
  end
end

if args.months then
  if tonumber(args.months) and tonumber(args.months) >= 0 then
    ticks = (ticks or 0) + tonumber(args.months)*33600
  else
    qerror("Invalid -months duration: " .. tostring(args.months))
  end
end

if args.days then
  if tonumber(args.days) and tonumber(args.days) >= 0 then
    ticks = (ticks or 0) + tonumber(args.days)*1200
  else
    qerror("Invalid -days duration: " .. tostring(args.days))
  end
end

if args.hours then
  if tonumber(args.hours) and tonumber(args.hours) >= 0 then
    ticks = (ticks or 0) + tonumber(args.hours)*50
  else
    qerror("Invalid -hours duration: " .. tostring(args.hours))
  end
end

if not ticks then
  qerror("Duration not specified! Enter \"set-timeskip-duration -help\" for more information.")
end

ticks = math.floor(ticks) -- get rid of decimals the user may have inputted

printTimeskipCalendarDuration(ticks)

local scr = dfhack.gui.getCurViewscreen()
if scr._type == df.viewscreen_update_regionst then
  setTargetDate(scr, ticks)
end

dfhack.onStateChange.TimeskipMonitor = function(event)
  if event == SC_VIEWSCREEN_CHANGED then
    local scr = dfhack.gui.getCurViewscreen()
    if scr._type == df.viewscreen_update_regionst then
      setTargetDate(scr, ticks)
    end
  end
end
