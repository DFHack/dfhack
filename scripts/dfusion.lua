-- a binary hack/plugin collection for df
local dfu=require("plugins.dfusion")
local myos=dfhack.getOSType()

--some imports go here
local plugins={
    require("plugins.dfusion.embark").CustomEmbark
}
--show a table of all the statuses
function status(plug)
    if dfu.plugins[plug.name]==nil then
        return plug.class_status
    else
        return dfu.plugins[plug.name]:status()
    end
end
function printPlugs()
    local endthis=false
    print("current:")
    while(not endthis) do
        for k,v in pairs(plugins) do
            if v then
                print(string.format("%2d. %15s-%s",k,v.name,status(v)))
            end
        end
        print("e-edit and load, u-unload,c-cancel and then number to manipulate:")
        local choice=io.stdin:read()
        local num=tonumber(io.stdin:read())
        if num then
            local plg=dfu.plugins[plugins[num].name] or plugins[num]()
            if choice=='e' then
                plg:edit()
            elseif choice=='u' then
                plg:uninstall()
            elseif choice=='c' then
                endthis=true
            end
        end
    end
end

printPlugs()