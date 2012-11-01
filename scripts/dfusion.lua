-- a collection of misc lua scripts
local dfu=require("plugins.dfusion")
local myos=dfhack.getOSType()
args={...}
mainmenu=dfu.SimpleMenu()
function runsave()
    print("doing file:"..df.global.world.cur_savegame.save_dir)
end
mainmenu:add("Run save script",runsave)
mainmenu:add("Adventurer tools",require("plugins.dfusion.adv_tools").menu)
mainmenu:display()