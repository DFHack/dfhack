-- Run a command if the current entity matches a given ID.

--[[
Consider this public domain (CC0).
    - Milo Christiansen
]]

local usage = [====[
modtools/if-entity
==================

Run a command if the current entity matches a given ID.

To use this script effectively it needs to be called from "raw/onload.init".
Calling this from the main dfhack.init file will do nothing, as no world has
been loaded yet.

Usage:

- ``id``:
    Specify the entity ID to match
- ``cmd [ commandStrs ]``:
    Specify the command to be run if the current entity matches the entity
    given via -id

All arguments are required.

Example:

- Print a message if you load an elf fort, but not a dwarf, human, etc. fort::

    if-entity -id "FOREST" -cmd [ lua "print('Dirty hippies.')" ]
]====]

local utils = require 'utils'

local validArgs = utils.invert({
    'help',
    'id',
    'cmd',
})

local args = utils.processArgs({...}, validArgs)

if not args.id or not args.cmd or args.help then
    dfhack.print(usage)
    return
end

local entsrc = df.historical_entity.find(df.global.plotinfo.civ_id)
if entsrc == nil then
    dfhack.printerr("Could not find current entity. No world loaded?")
    return
end

if entsrc.entity_raw.code == args.id then
    dfhack.run_command(table.unpack(args.cmd))
end
