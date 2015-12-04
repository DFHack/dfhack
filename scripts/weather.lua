-- Print the weather map or change weather.
local helpstr = [[=begin

weather
=======
Prints a map of the local weather, or with arguments ``clear``,
``rain``, and ``snow`` changes the weather.

=end]]

local args = {...}
if args[1] == "help" or args[1] == "?" then
    print("The current weather is "..df.weather_type[dfhack.world.ReadCurrentWeather()])
    print((helpstr:gsub('=[a-z]+', '')))
elseif args[1] == "clear" then
    dfhack.world.SetCurrentWeather(df.weather_type["None"])
    print("The weather has cleared.")
elseif args[1] == "rain" then
    dfhack.world.SetCurrentWeather(df.weather_type["Rain"])
    print("It is now raining.")
elseif args[1] == "snow" then
    dfhack.world.SetCurrentWeather(df.weather_type["Snow"])
    print("It is now snowing.")
else
    -- df.global.current_weather is arranged in columns, not rows
    kind = {[0]="C ", "R ", "S "}
    print("Weather map (C = clear, R = rain, S = snow):")
    for y=0, 4 do
        s = ""
        for x=0, 4 do
            s = s..kind[df.global.current_weather[x][y]]
        end
        print(s)
    end
end
