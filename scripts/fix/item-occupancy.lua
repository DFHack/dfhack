-- Verify item occupancy and block item list integrity.
--
-- Checks:
-- 1) Item has flags.on_ground <=> it is in the correct block item list
-- 2) A tile has items in block item list <=> it has occupancy.item
-- 3) The block item lists are sorted.

local utils = require 'utils'

function check_block_items(fix)
    local cnt = 0
    local icnt = 0
    local found = {}
    local found_somewhere = {}

    local should_fix = false
    local can_fix = true

    for _,block in ipairs(df.global.world.map.map_blocks) do
        local itable = {}
        local bx,by,bz = pos2xyz(block.map_pos)

        -- Scan the block item vector
        local last_id = nil
        local resort = false

        for _,id in ipairs(block.items) do
            local item = df.item.find(id)
            local ix,iy,iz = pos2xyz(item.pos)
            local dx,dy,dz = ix-bx,iy-by,iz-bz

            -- Check sorted order
            if last_id and last_id >= id then
                print(bx,by,bz,last_id,id,'block items not sorted')
                resort = true
            else
                last_id = id
            end

            -- Check valid coordinates and flags
            if not item.flags.on_ground then
                print(bx,by,bz,id,dx,dy,'in block & not on ground')
            elseif dx < 0 or dx >= 16 or dy < 0 or dy >= 16 or dz ~= 0 then
                found_somewhere[id] = true
                print(bx,by,bz,id,dx,dy,dz,'invalid pos')
                can_fix = false
            else
                found[id] = true
                itable[dx + dy*16] = true;

                -- Check missing occupancy
                if not block.occupancy[dx][dy].item then
                    print(bx,by,bz,dx,dy,'item & not occupied')
                    if fix then
                        block.occupancy[dx][dy].item = true
                    else
                        should_fix = true
                    end
                end
            end
        end

        -- Sort the vector if needed
        if resort then
            if fix then
                utils.sort_vector(block.items)
            else
                should_fix = true
            end
        end

        icnt = icnt + #block.items

        -- Scan occupancy for spurious marks
        for x=0,15 do
            local ocx = block.occupancy[x]
            for y=0,15 do
                if ocx[y].item and not itable[x + y*16] then
                    print(bx,by,bz,x,y,'occupied & no item')
                    if fix then
                        ocx[y].item = false
                    else
                        should_fix = true
                    end
                end
            end
        end

        cnt = cnt + 256
    end

    -- Check if any items are missing from blocks
    for _,item in ipairs(df.global.world.items.all) do
        if item.flags.on_ground and not found[item.id] then
            can_fix = false
            if not found_somewhere[item.id] then
                print(id,item.pos.x,item.pos.y,item.pos.z,'on ground & not in block')
            end
        end
    end

    -- Report
    print(cnt.." tiles and "..icnt.." items checked.")

    if should_fix and can_fix then
        print("Use 'fix/item-occupancy --fix' to fix the listed problems.")
    elseif should_fix then
        print("The problems are too severe to be fixed by this script.")
    end
end

local opt = ...
local fix = false

if opt then
    if opt == '--fix' then
        fix = true
    else
        qerror('Invalid option: '..opt)
    end
end

print("Checking item occupancy - this will take a few seconds.")
check_block_items(fix)
