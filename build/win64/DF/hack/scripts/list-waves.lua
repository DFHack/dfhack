-- displays migration wave information for citizens/units
-- Written by Josh Cooper(cppcooper) sometime around Decement 2019, last modified: 2020-02-19
utils ={}
utils = require('utils')
local validArgs = utils.invert({
    'unit',
    'all',
    'granularity',
    'showarrival',
    'help'
})
local args = utils.processArgs({...}, validArgs)
local help = [====[

list-waves
==========
This script displays information about migration waves of the specified citizen(s).

Examples::

  list-waves -all -showarrival -granularity days
  list-waves -all -showarrival
  list-waves -unit -granularity days
  list-waves -unit
  list-waves -unit -all -showarrival -granularity days

**Selection options:**

These options are used to specify what wave information to display

``-unit``:
    Displays the highlighted unit's arrival wave information

``-all``:
    Displays all citizens' arrival wave information

**Other options:**

``-granularity <value>``:
    Specifies the granularity of wave enumeration: ``years``, ``seasons``, ``months``, ``days``
    If omitted, the default granularity is ``seasons``, the same as Dwarf Therapist

``-showarrival``:
    Shows the arrival information for the selected unit.
    If ``-all`` is specified the info displayed will be relative to the
    granularity used. Note: this option is always used when ``-unit`` is used.

]====]

--[[
The script must loop through all active units in df.global.world.units.active and build
each wave one dwarf at a time. This requires calculating arrival information for each
dwarf and combining this information into a sort of unique wave ID number. After this
is finished these wave UIDs are looped through and normalized so they start at zero and
incremented by one for each new wave UID.

As you might surmise, this is a relatively dumb script and every execution has the
worst case big O execution. I've taken the liberty of only counting citizen units,
this should only include dwarves as far as I know.

]]

selected = dfhack.gui.getSelectedUnit()
local ticks_per_day = 1200
local ticks_per_month = 28 * ticks_per_day
local ticks_per_season = 3 * ticks_per_month
local ticks_per_year = 12 * ticks_per_month
local current_tick = df.global.cur_year_tick
local seasons = {
    'spring',
    'summer',
    'autumn',
    'winter',
}
function TableLength(table)
    local count = 0
    for i,k in pairs(table) do
        count = count + 1
    end
    return count
end
--sorted pairs
function spairs(t, cmp)
    -- collect the keys
    local keys = {}
    for k,v in pairs(t) do
        table.insert(keys,k)
    end

    utils.sort_vector(keys, nil, cmp)

    -- return the iterator function
    local i = 0
    return function()
        i = i + 1
        if keys[i] then
            return keys[i], t[keys[i]]
        end
    end
end
function isDwarfCitizen(dwf)
    return dfhack.units.isCitizen(dwf)
end



waves={}
function getWave(dwf)
    arrival_time = current_tick - dwf.curse.time_on_site
    --print(string.format("Current year %s, arrival_time = %s, ticks_per_year = %s", df.global.cur_year, arrival_time, ticks_per_year))
    arrival_year = df.global.cur_year + (arrival_time // ticks_per_year)
    arrival_season = 1 + (arrival_time % ticks_per_year) // ticks_per_season
    arrival_month = 1 + (arrival_time % ticks_per_year) // ticks_per_month
    arrival_day = 1 + ((arrival_time % ticks_per_year) % ticks_per_month) // ticks_per_day
    if args.granularity then
        if args.granularity == "days" then
            wave = arrival_day + (100 * arrival_month) + (10000 * arrival_year)
        elseif args.granularity == "months" then
            wave = arrival_month + (100 * arrival_year)
        elseif args.granularity == "seasons" then
            wave = arrival_season + (10 * arrival_year)
        elseif args.granularity == "years" then
            wave = arrival_year
        else
            error("Invalid granularity value. Omit the option if you want 'seasons'. Note: plurals only.")
        end
    else
        wave = 10 * arrival_year + arrival_season
    end
    if waves[wave] == nil then
        waves[wave] = {}
    end
    table.insert(waves[wave],dwf)
    if args.unit and dwf == selected then
        print(string.format("  Selected citizen arrived in the %s of year %d, month %d, day %d.",seasons[arrival_season],arrival_year,arrival_month,arrival_day))
    end
end

for _,v in ipairs(df.global.world.units.active) do
    if isDwarfCitizen(v) then
        getWave(v)
    end
end

if args.help or (not args.all and not args.unit) then
    print(help)
else
    zwaves = {}
    i = 0
    for k,v in spairs(waves, utils.compare) do
        if args.showarrival then
            season = nil
            month = nil
            day = nil
            if args.granularity then
                if args.granularity == "days" then
                    year = k // 10000
                    month = (k - (10000 * year)) // 100
                    season = 1 + (month // 3)
                    day = k - ((100 * month) + (10000 * year))
                elseif args.granularity == "months" then
                    year = k // 100
                    month = k - (100 * year)
                    season = 1 + (month // 3)
                elseif args.granularity == "seasons" then
                    year = k // 10
                    season = k - (10 * year)
                elseif args.granularity == "years" then
                    year = k
                end
            else
                year = k // 10
                season = k - (10 * year)
            end

            if season ~= nil then
                season = string.format("the %s of",seasons[season])
            else
                season = ""
            end
            if month ~= nil then
                month = string.format(", month %d", month)
            else
                month = ""
            end
            if day ~= nil then
                day = string.format(", day %d", day)
            else
                day = ""
            end
            if args.all then
                print(string.format("  Wave %d of citizens arrived in %s year %d%s%s.",i,season,year,month,day))
            end
        end
        zwaves[i] = waves[k]
        for _,dwf in spairs(v, utils.compare) do
            if args.unit and dwf == selected then
                print(string.format("  Selected citizen belongs to wave %d",i))
            end
        end
        i = i + 1
    end

    if args.all then
        for i = 0, TableLength(zwaves)-1 do
            print(string.format("  Wave %2s has %2d dwarf citizens.", i, #zwaves[i]))
        end
    end
end
