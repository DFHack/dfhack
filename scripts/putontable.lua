-- Makes item appear on the table (just like in shops). '-a' or '--all' for all items.
local pos=df.global.cursor
local args={...}
local doall
if args[1]=="-a" or args[1]=="--all" then
    doall=true
end
local build,items
items={}
build=dfhack.buildings.findAtTile(pos.x,pos.y,pos.z)
if not df.building_tablest:is_instance(build) then
    error("No table found at cursor")
end
for k,v in pairs(df.global.world.items.all) do
    if pos.x==v.pos.x and pos.y==v.pos.y and pos.z==v.pos.z and v.flags.on_ground then
        table.insert(items,v)
        if not doall then
            break
        end
    end
end
if #items==0 then
    error("No items found!")
end
for k,v in pairs(items) do
    dfhack.items.moveToBuilding(v,build,0)
end
