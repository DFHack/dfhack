-- spawns flows at locations
--author expwnent
local usage = [====[

modtools/spawn-flow
===================
Creates flows at the specified location.

Arguments::

    -material mat
        specify the material of the flow, if applicable
        examples:
            INORGANIC:IRON
            CREATURE_MAT:DWARF:BRAIN
            PLANT_MAT:MUSHROOM_HELMET_PLUMP:DRINK
    -location [ x y z]
        the location to spawn the flow
    -flowType type
        specify the flow type
        examples:
            Miasma
            Steam
            Mist
            MaterialDust
            MagmaMist
            Smoke
            Dragonfire
            Fire
            Web
            MaterialGas
            MaterialVapor
            OceanWave
            SeaFoam
    -flowSize size
        specify how big the flow is

]====]
local utils = require 'utils'

local validArgs = utils.invert({
 'help',
 'material',
 'flowType',
 'location',
 'flowSize',
})
local args = utils.processArgs({...}, validArgs)

if args.help then
 print(usage)
 return
end

local mat_index = -1;
local mat_type = -1;
if args.material then
 local mat = dfhack.matinfo.find(args.material)
 if not mat then
  error ('Invalid material: ' .. mat)
 end
 mat_index = mat.index
 mat_type = mat['type']
end

if args.flowSize and not tonumber(args.flowSize) then
 error ('Invalid flow size: ' .. args.flowSize)
end

local flowSize = tonumber(args.flowSize or 'z') or 100

if not args.flowType or not df.flow_type[args.flowType] then
 error ('Invalid flow type: ' .. (args.flowType or 'none specified'))
end
local flowType = df.flow_type[args.flowType]

if not args.location then
 error 'Specify a location.'
end

local pos = df.coord:new();
pos.x = tonumber(args.location[1] or 'a')
pos.y = tonumber(args.location[2] or 'a')
pos.z = tonumber(args.location[3] or 'a')
if not pos.x or not pos.y or not pos.z then
 error ('Invalid pos.')
end

dfhack.maps.spawnFlow(pos, flowType, mat_type, mat_index, flowSize)
