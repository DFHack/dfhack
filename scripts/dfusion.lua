-- a collection of misc lua scripts
local dfu=require("plugins.dfusion")
local myos=dfhack.getOSType()
args={...}
mainmenu=dfu.SimpleMenu()
function runsave()
    local path=string.format("data/save/%s/dfhack.lua",df.global.world.cur_savegame.save_dir)
    print("doing file:"..path)
    loadfile(path)()
end
mainmenu:add("Run save script",runsave)
mainmenu:add("Adventurer tools",require("plugins.dfusion.adv_tools").menu)
mainmenu:add("Misc tools",require("plugins.dfusion.tools").menu)
mainmenu:display()