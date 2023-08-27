--@ module = true
local repeatUtil = require('repeat-util')

liquidSources = liquidSources or {}

local sourceId = 'liquidSources'

local function formatPos(pos)
    return ('[%d, %d, %d]'):format(pos.x, pos.y, pos.z)
end

function IsFlowPassable(pos)
    local tiletype = dfhack.maps.getTileType(pos)
    local titletypeAttrs = df.tiletype.attrs[tiletype]
    local shape = titletypeAttrs.shape
    local tiletypeShapeAttrs = df.tiletype_shape.attrs[shape]
    return tiletypeShapeAttrs.passable_flow
end

function AddLiquidSource(pos, liquid, amount)
    table.insert(liquidSources, {
        liquid = liquid,
        amount = amount,
        pos = copyall(pos),
    })

    repeatUtil.scheduleEvery(sourceId, 12, 'ticks', function()
        if next(liquidSources) == nil then
            repeatUtil.cancel(sourceId)
        else
            for _, v in pairs(liquidSources) do
                local block = dfhack.maps.getTileBlock(v.pos)
                local x = v.pos.x
                local y = v.pos.y
                if block and IsFlowPassable(v.pos) then
                    local isMagma = v.liquid == 'magma'

                    local flags = dfhack.maps.getTileFlags(v.pos)
                    local flow = flags.flow_size

                    if flow ~= v.amount then
                        local target = flow + 1
                        if flow > v.amount then
                            target = flow - 1
                        end

                        flags.liquid_type = isMagma
                        flags.flow_size = target
                        flags.flow_forbid = (isMagma or target >= 4)

                        dfhack.maps.enableBlockUpdates(block, true)
                    end
                end
            end
        end
    end)
end

function DeleteLiquidSource(pos)
    for k, v in pairs(liquidSources) do
        if same_xyz(pos, v.pos) then liquidSources[k] = nil end
        return
    end
end

function ClearLiquidSources()
    for k, _ in pairs(liquidSources) do
        liquidSources[k] = nil
    end
end

function ListLiquidSources()
    print('Current Liquid Sources:')
    for _,v in pairs(liquidSources) do
        print(('%s %s %d'):format(formatPos(v.pos), v.liquid, v.amount))
    end
end

function FindLiquidSourceAtPos(pos)
    for k,v in pairs(liquidSources) do
        if same_xyz(v.pos, pos) then
            return k
        end
    end
    return nil
end

function main(args)
    local command = args[1]

    if command == 'list' then
        ListLiquidSources()
        return
    end

    if command == 'clear' then
        ClearLiquidSources()
        print("Cleared sources")
        return
    end

    local targetPos = copyall(df.global.cursor)
    local index = FindLiquidSourceAtPos(targetPos)

    if command == 'delete' then
        if targetPos.x < 0 then
            qerror("Please place the cursor where there is a source to delete")
        end
        if not index then
            DeleteLiquidSource(targetPos)
            print(('Deleted source at %s'):format(formatPos(targetPos)))
        else
            qerror(('%s Does not contain a liquid source'):format(formatPos(targetPos)))
        end
        return
    end

    if command == 'add' then
        if targetPos.x < 0 then
            qerror('Please place the cursor where you would like a source')
        end
        local liquidArg = args[2]
        if not liquidArg then
            qerror('You must specify a liquid to add a source for')
        end
        liquidArg = liquidArg:lower()
        if not (liquidArg == 'magma' or liquidArg == 'water') then
            qerror('Liquid must be either "water" or "magma"')
        end
        if not IsFlowPassable(targetPos) then
            qerror("Tile not flow passable: I'm afraid I can't let you do that, Dave.")
        end
        local amountArg = tonumber(args[3]) or 7
        AddLiquidSource(targetPos, liquidArg, amountArg)
        print(('Added %s %d at %s'):format(liquidArg, amountArg, formatPos(targetPos)))
        return
    end

    print(dfhack.script_help())
end

if not dfhack_flags.module then
    main({...})
end
