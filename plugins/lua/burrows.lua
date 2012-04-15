local _ENV = mkmodule('plugins.burrows')

--[[

 Native functions:

 * findByName(name) -> burrow
 * copyUnits(dest,src,enable)
 * copyTiles(dest,src,enable)
 * setTilesByKeyword(dest,kwd,enable) -> success

 'enable' selects between add and remove modes

--]]

clearUnits = dfhack.units.clearBurrowMembers

function isBurrowUnit(burrow,unit)
    return dfhack.units.isInBurrow(unit,burrow)
end
function setBurrowUnit(burrow,unit,enable)
    return dfhack.units.setInBurrow(unit,burrow,enable)
end

clearTiles = dfhack.maps.clearBurrowTiles
listBlocks = dfhack.maps.listBurrowBlocks

isBurrowTile = dfhack.maps.isBurrowTile
setBurrowTile = dfhack.maps.setBurrowTile
isBlockBurrowTile = dfhack.maps.isBlockBurrowTile
setBlockBurrowTile = dfhack.maps.setBlockBurrowTile

return _ENV