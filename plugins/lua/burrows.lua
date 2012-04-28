local _ENV = mkmodule('plugins.burrows')

--[[

 Native events:

 * onBurrowRename(burrow)
 * onDigComplete(job_type,pos,old_tiletype,new_tiletype)

 Native functions:

 * findByName(name) -> burrow
 * copyUnits(dest,src,enable)
 * copyTiles(dest,src,enable)
 * setTilesByKeyword(dest,kwd,enable) -> success

 'enable' selects between add and remove modes

--]]

rawset_default(_ENV, dfhack.burrows)

return _ENV