-- Set the FPS cap at runtime.
--[[=begin

setfps
======
Run ``setfps <number>`` to set the FPS cap at runtime, in case you want to watch
combat in slow motion or something.

=end]]

local cap = ...
local capnum = tonumber(cap)

if not capnum or capnum < 1 then
    qerror('Invalid FPS cap value: '..cap)
end

df.global.enabler.fps = capnum
