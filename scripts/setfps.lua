-- Set the FPS cap at runtime.

local cap = ...
local capnum = tonumber(cap)

if not capnum or capnum < 1 then
    qerror('Invalid FPS cap value: '..cap)
end

df.global.enabler.fps = capnum
