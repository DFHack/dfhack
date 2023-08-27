-- Run multiple dfhack commands separated by ';'
--[====[

multicmd
========
Run multiple dfhack commands. The argument is split around the character ";",
and all parts are run sequentially as independent dfhack commands. Useful for
hotkeys.

Example::

    multicmd locate-ore IRON; digv; digcircle 16
]====]

for _,cmd in ipairs(table.concat({...}, ' '):split(';+')) do
    dfhack.run_command(cmd)
end
