-- tame and train animals {set animals as Wild through to Masterfully Trained}

local help=[====[
tame
====
Tame and train animals.

Usage: ``tame -read`` OR ``tame -set <level>``

:0: semi-wild
:1: trained
:2: well-trained
:3: skillfully trained
:4: expertly trained
:5: exceptionally trained
:6: masterfully trained
:7: tame
:8: wild
:9: wild
]====]

local utils = require('utils')
local selected = dfhack.gui.getSelectedUnit()
local validArgs = utils.invert({'set','read','help'})
local args = utils.processArgs({...}, validArgs)

if args.help then
    print(help)
    return
end

if selected ~= nil then
    if args.set and tonumber(args.set) then
        local level = tonumber(args.set)
        if level < 0 or level > 9 then
            qerror("range must be 0 to 9")
            print(help)
        end
        selected.flags1.tame = level ~= 8 and level ~= 9
        if selected.flags1.tame then
            selected.training_level = level
            selected.civ_id = df.global.plotinfo.civ_id
        else
            selected.training_level = 9
        end
    elseif args.read then
        print("tame:",selected.flags1.tame)
        print("training level:",selected.training_level)
    else
        print(help)
    end
else
    qerror("select a valid unit")
end
