local _ENV = mkmodule('plugins.reactionhooks')

--[[

 Native events:

 * onReactionComplete(burrow)

--]]

rawset_default(_ENV, dfhack.reactionhooks)

return _ENV