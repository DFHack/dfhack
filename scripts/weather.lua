-- Print the weather map or change weather.
local helpstr = [[=begin

weather
=======
Prints a map of the local weather, or with arguments ``clear``,
``rain``, and ``snow`` changes the weather.

=end]]

local args = {...}
local cmd
local val_override = tonumber(args[1])
if args[1] then
    cmd = args[1]:sub(1, 1)
end
if cmd == "h" or cmd == "?" then
    print("The current weather is "..df.weather_type[dfhack.world.ReadCurrentWeather()])
    print((helpstr:gsub('=[a-z]+', '')))
elseif cmd == "c" then
    dfhack.world.SetCurrentWeather(df.weather_type.None)
    print("The weather has cleared.")
elseif cmd == "r" then
    dfhack.world.SetCurrentWeather(df.weather_type.Rain)
    print("It is now raining.")
elseif cmd == "s" then
    dfhack.world.SetCurrentWeather(df.weather_type.Snow)
    print("It is now snowing.")
elseif val_override then
    dfhack.world.SetCurrentWeather(val_override)
    print("Set weather to " .. val_override)
elseif args[1] then
    qerror("Unrecognized argument: " .. args[1])
else
    -- df.global.current_weather is arranged in columns, not rows
    kind = {[0]="C", "R", "S"}
    print("Weather map (C = clear, R = rain, S = snow):")
    for y=0, 4 do
        s = ""
        for x=0, 4 do
            local cur = df.global.current_weather[x][y]
            s = s .. (kind[cur] or cur) .. ' '
        end
        print(s)
    end
end
