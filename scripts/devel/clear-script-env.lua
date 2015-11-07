-- Clear script environment
--[[=begin

devel/clear-script-env
======================
Clears the environment of the specified lua script(s).

=end]]
args = {...}
if #args < 1 then qerror("script name(s) required") end
for _, name in pairs(args) do
    local file = dfhack.findScript(name)
    if file then
        local script = dfhack.internal.scripts[file]
        if script then
            local env = script.env
            while next(env) do
                env[next(env)] = nil
            end
        else
            dfhack.printerr("Script not loaded: " .. name)
        end
    else
        dfhack.printerr("Can't find script: " .. name)
    end
end
