-- Executes an autolabor command for each labor where skill level influences output quality.

--[[=begin

autolabor-artisans
==================
Runs `autolabor`, with settings tuned for small but highly skilled workforces.

=end]]
local artisan_labors = {
    "CARPENTER",
    "DETAIL",
    "MASON",
    "ARCHITECT",
    "ANIMALTRAIN",
    "LEATHER",
    "BREWER",
    "WEAVER",
    "CLOTHESMAKER",
    "COOK",
    "FORGE_WEAPON",
    "FORGE_ARMOR",
    "FORGE_FURNITURE",
    "METAL_CRAFT",
    "CUT_GEM",
    "ENCRUST_GEM",
    "WOOD_CRAFT",
    "STONE_CRAFT",
    "BONE_CARVE",
    "GLASSMAKER",
    "SIEGECRAFT",
    "BOWYER",
    "MECHANIC",
    "DYER",
    "POTTERY",
    "WAX_WORKING",
}

local args = {...}

function make_cmd(labor)
    local cmd = string.format("autolabor %s", labor)
    for i, arg in ipairs(args) do
        cmd = cmd .. " " .. arg
    end
    return cmd
end

function run()
    if #args == 0 or args[1] == "help" then
        print('Applies an autolabor command to all labors with quality-based output.')
        print('')
        print('Examples:')
        print('  autolabor-artisans 0 2 3')
        print('  autolabor-artisans disable')
        return false
    end

    dfhack.run_command("autolabor enable")

    -- Test with one to make sure the arguments are valid.
    local cmd = make_cmd(artisan_labors[1])
    local output, status = dfhack.run_command_silent(cmd)
    if status ~= CR_OK then
        qerror("Invalid arguments.", status)
        return false
    end

    for i, labor in ipairs(artisan_labors) do
        dfhack.run_command(make_cmd(labor))
    end

    return true
end

run()
