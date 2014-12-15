--prints current time and position

local months = {
    'Granite, in early Spring.', 
    'Slate, in mid Spring.',
    'Felsite, in late Spring.',
    'Hematite, in early Summer.',
    'Malachite, in mid Summer.',
    'Galena, in late Summer.',
    'Limestone, in early Autumn.',
    'Sandstone, in mid Autumn.',
    'Timber, in late Autumn.',
    'Moonstone, in early Winter.',
    'Opal, in mid Winter.',
    'Obsidian, in late Winter.',
}

--Fortress mode counts 1200 ticks per day and 403200 per year
--Adventurer mode counts 86400 ticks to a day and 29030400 ticks per year
--Twelve months per year, 28 days to every month, 336 days per year

local julian_day = math.floor(df.global.cur_year_tick / 1200) + 1
local month = math.floor(julian_day / 28) + 1 --days and months are 1-indexed
local day = julian_day % 28

local time_of_day = math.floor(df.global.cur_year_tick_advmode / 336)
local second = time_of_day % 60
local minute = (time_of_day / 60) % 60
local hour = math.floor(time_of_day / 3600) % 24

print('Time:')
print('    The time is '..string.format('%02d:%02d:%02d', hour, minute, second))
print('    The date is '..string.format('%05d-%02d-%02d', df.global.cur_year, month, day))
print('    It is the month of '..months[month])

print('Place:')
print('    Local window is at  x='..df.global.window_x
      ..' y='..df.global.window_y..' z='..df.global.window_z)

--TODO:  add cursor, regional/embark square co-ords, window size?
