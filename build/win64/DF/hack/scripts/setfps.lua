-- Set the FPS cap at runtime.
--[====[

setfps
======
Sets the FPS cap at runtime. Useful in case you want to speed up the game or
watch combat in slow motion.

Usage::

    setfps <number>

]====]

local cap = ...
local capnum = tonumber(cap)

if not cap or cap:match('help') then
    print('Usage:\n\tsetfps <number>')
    return
end

if not capnum or capnum < 1 then
    qerror('Invalid FPS cap value: '..cap)
end

df.global.enabler.fps = capnum
df.global.enabler.fps_per_gfps = df.global.enabler.fps / df.global.enabler.gfps
