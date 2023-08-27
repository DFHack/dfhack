-- Displays announcements in the DFHack console
--@ enable = true
--[====[

devel/annc-monitor
==================
Displays announcements and reports in the console.

:enable|start:      Begins monitoring
:disable|stop:      Stops monitoring
:report enable:     Show combat reports
:report disable:    Only show announcements

]====]

VERSION = '0.3'

local eventful = require 'plugins.eventful'

enabled = enabled or false
filter = filter or false

function usage()
    print [[
Usage:
    annc-monitor start|enable: Begin monitoring
    annc-monitor stop|disable: End monitoring
    annc-monitor report enable: Show combat reports
    annc-monitor report disable: Only show announcements
]]
end

function log(s, color)
    dfhack.color(color)
    print(dfhack.df2console(s))
    dfhack.color(COLOR_RESET)
end

local args = {...}
if dfhack_flags and dfhack_flags.enable then
    table.insert(args, dfhack_flags.enable_state and 'enable' or 'disable')
end
if #args >= 1 then
    if args[1] == 'start' or args[1] == 'enable' then
        enabled = true
    elseif args[1] == 'stop' or args[1] == 'disable' then
        enabled = false
    elseif args[1] == 'report' and #args >= 2 then
        if args[2] == 'enable' then
            filter = false
        elseif args[2] == 'disable' then
            filter = true
        else
            usage()
        end
    else
        usage()
    end
else
    usage()
end

eventful.enableEvent(eventful.eventType.REPORT, 1)
function eventful.onReport.annc_monitor(id)
    if enabled and dfhack.isWorldLoaded() then
        local annc = df.report.find(id)
        if filter and not annc.flags.announcement then
            return
        end
        local color = annc.color + (annc.bright and 8 or 0)
        log(annc.text, color)
    end
end
