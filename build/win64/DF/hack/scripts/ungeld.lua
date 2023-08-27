-- Ungelds animals
-- Written by Josh Cooper(cppcooper) on 2019-12-10, last modified: 2020-02-23
utils = require('utils')
local validArgs = utils.invert({
    'unit',
    'help',
})
local args = utils.processArgs({...}, validArgs)
local help = [====[

ungeld
======
A wrapper around `geld` that ungelds the specified animal.

Valid options:

``-unit <id>``: Ungelds the unit with the specified ID.
                This is optional; if not specified, the selected unit is used instead.

``-help``:      Shows this help information

]====]

if args.help then
    print(help)
    return
end

local geld_args = {'-ungeld'}

if args.unit then
    table.insert(geld_args, '-unit')
    table.insert(geld_args, args.unit)
end

dfhack.run_script('geld', table.unpack(geld_args))
