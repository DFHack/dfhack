local argparse = require('argparse')
local opts = {}
local positionals = argparse.processArgsGetopt({...}, {
    {'h', 'help', handler = function() opts.help = true end},
    {nil, 'include-vermin', handler = function() opts.include_vermin = true end},
    {nil, 'include-pets', handler = function() opts.include_pets = true end},
    {'f', 'skip-forbidden', handler = function() opts.skip_forbidden = true end},
})

local function plural(nr, name)
    -- '1 cage' / '4 cages'
    return string.format('%s %s%s', nr, name, nr ~= 1 and 's' or '')
end

-- Checks item against opts to see if it's a vermin type that we ignore
local function isexcludedvermin(item)
    if df.item_verminst:is_instance(item) then
        return not opts.include_vermin
    elseif df.item_petst:is_instance(item) then
        return not opts.include_pets
    end
    return false
end

local function dump_item(item)
    if item.flags.dump then return 0 end
    if not item.flags.forbid or not opts.skip_forbidden then
        item.flags.dump = true
        item.flags.forbid = false
        return 1
    end
    return 0
end

local function cage_dump_items(list)
    local count = 0
    local count_cage = 0
    for _, cage in ipairs(list) do
        local pre_count = count
        for _, ref in ipairs(cage.general_refs) do
            if df.general_ref_contains_itemst:is_instance(ref) then
                local item = df.item.find(ref.item_id)
                if not isexcludedvermin(item) then
                    count = count + dump_item(item)
                end
            end
        end
        if pre_count ~= count then count_cage = count_cage + 1 end
    end
    print(string.format('Dumped %s in %s', plural(count, 'item'),
        plural(count_cage, 'cage')))
end

local function cage_dump_armor(list)
    local count = 0
    local count_cage = 0
    for _, cage in ipairs(list) do
        local pre_count = count
        for _, ref in ipairs(cage.general_refs) do
            if df.general_ref_contains_unitst:is_instance(ref) then
                local inventory = df.unit.find(ref.unit_id).inventory
                for _, it in ipairs(inventory) do
                    if it.mode == df.unit_inventory_item.T_mode.Worn then
                        count = count + dump_item(it.item)
                    end
                end
            end
        end
        if pre_count ~= count then count_cage = count_cage + 1 end
    end
    print(string.format('Dumped %s in %s', plural(count, 'armor piece'),
        plural(count_cage, 'cage')))
end

local function cage_dump_weapons(list)
    local count = 0
    local count_cage = 0
    for _, cage in ipairs(list) do
        local pre_count = count
        for _, ref in ipairs(cage.general_refs) do
            if df.general_ref_contains_unitst:is_instance(ref) then
                local inventory = df.unit.find(ref.unit_id).inventory
                for _, it in ipairs(inventory) do
                    if it.mode == df.unit_inventory_item.T_mode.Weapon then
                        count = count + dump_item(it.item)
                    end
                end
            end
        end
        if pre_count ~= count then count_cage = count_cage + 1 end
    end
    print(string.format('Dumped %s in %s', plural(count, 'weapon'),
        plural(count_cage, 'cage')))
end

local function cage_dump_all(list)
    local count = 0
    local count_cage = 0
    for _, cage in ipairs(list) do
        local pre_count = count
        for _, ref in ipairs(cage.general_refs) do
            if df.general_ref_contains_itemst:is_instance(ref) then
                local item = df.item.find(ref.item_id)
                if not isexcludedvermin(item) then
                    count = count + dump_item(item)
                end
            elseif df.general_ref_contains_unitst:is_instance(ref) then
                local inventory = df.unit.find(ref.unit_id).inventory
                for _, it in ipairs(inventory) do
                    if not isexcludedvermin(it.item) then
                        count = count + dump_item(it.item)
                    end
                end
            end

        end
        if pre_count ~= count then count_cage = count_cage + 1 end
    end
    print(string.format('Dumped %s in %s', plural(count, 'item'),
        plural(count_cage, 'cage')))
end

local function cage_dump_list(list)
    local count_total = {}
    local empty_cages = 0
    for _, cage in ipairs(list) do
        local count = {}
        for _, ref in ipairs(cage.general_refs) do
            if df.general_ref_contains_itemst:is_instance(ref) then
                local item = df.item.find(ref.item_id)
                if not isexcludedvermin(item) then
                    local classname = df.item_type.attrs[item:getType()].caption
                    count[classname] = (count[classname] or 0) + 1
                end
            elseif df.general_ref_contains_unitst:is_instance(ref) then
                local inventory = df.unit.find(ref.unit_id).inventory
                for _, it in ipairs(inventory) do
                    if not isexcludedvermin(it.item) then
                        local classname = df.item_type.attrs[it.item:getType()].caption
                        count[classname] = (count[classname] or 0) + 1
                    end
                end
            end
        end

        local type = df.item_type.attrs[cage:getType()].caption -- Default case
        if df.item_cagest:is_instance(cage) then
            type = 'Cage'
        elseif df.item_animaltrapst:is_instance(cage) then
            type = 'Animal trap'
        end

        -- If count is empty
        if not next(count) then
            empty_cages = empty_cages + 1
        else
            local sortedlist = {}
            for classname, n in pairs(count) do
                table.insert(sortedlist, {classname = classname, count = n})
            end
            table.sort(sortedlist, (function(i, j) return i.count < j.count end))
            print(('%s %d: '):format(type, cage.id))
            for _, t in ipairs(sortedlist) do
                print(' ' .. t.count .. ' ' .. t.classname)
            end
        end
        for k, v in pairs(count) do count_total[k] = (count_total[k] or 0) + v end
    end

    if list[1] then
        print('\nTotal:')
        local sortedlist = {}
        for classname, n in pairs(count_total) do
            sortedlist[#sortedlist + 1] = {classname = classname, count = n}
        end
        table.sort(sortedlist, (function(i, j) return i.count < j.count end))
        for _, t in ipairs(sortedlist) do
            print(' ' .. t.count .. ' ' .. t.classname)
        end
        print('\n' .. plural(empty_cages, 'empty cage'))
    end
end

-- handle magic script arguments

local list = {}
if positionals[2] == 'here' then
    local built = dfhack.gui.getSelectedBuilding(true)
    local item = dfhack.gui.getSelectedItem(true)
    local unit = dfhack.gui.getSelectedUnit(true)

    -- Is the selected cage a built cage?
    if built and (df.building_cagest:is_instance(built) or
            df.building_animaltrapst:is_instance(built)) then
        table.insert(list, built.contained_items[0].item)
    -- Is it an item in a stockpile or somewhere?
    elseif item and (df.item_cagest:is_instance(item) or
            df.item_animaltrapst:is_instance(item)) then
        table.insert(list, item)
    -- Is it a cage being hauled/carried by a unit?
    elseif unit then
        for _, it in ipairs(unit.inventory) do
            if df.item_cagest:is_instance(it.item) or
                    df.item_animaltrapst:is_instance(it.item) then
                table.insert(list, it.item)
            end
        end
    -- Is the player trying to select a cage using the keyboard cursor?
    elseif df.global.cursor.z > -10000 then -- cursor has values around -30000 if not valid/active.
        local cursor = df.global.cursor
        for _, cage in ipairs(df.global.world.items.other.ANY_CAGE_OR_TRAP) do
            if same_xyz(cursor, cage.pos) then
                table.insert(list, cage)
            end
        end
    end
    if not list[1] then
        print 'Please select a cage'
        return
    end
elseif tonumber(positionals[2]) then -- Check if user provided ids
    for i = 2, #positionals do
        local id = positionals[i]
        local it = df.item.find(tonumber(id))
        if not it then
            print('Invalid item id ' .. id)
        elseif not df.item_cagest:is_instance(it) and
            not df.item_animaltrapst:is_instance(it) then
            print('Item ' .. id .. ' is not a cage')
        else
            list[#list + 1] = it
        end
    end
    if not list[1] then
        print 'Please use a valid cage id'
        return
    end
else
    -- Default to vector of cages, but first convert it to a native Lua
    -- list like how 'list' is used elsewhere.
    for _, cage in ipairs(df.global.world.items.other.ANY_CAGE_OR_TRAP) do
        table.insert(list, cage)
    end
end

-- Print help if requested
if opts.help then
    print(dfhack.script_help())
    return
end

-- act
local choice = positionals[1] or ''

if choice:match '^it' then
    cage_dump_items(list)
elseif choice:match '^arm' then
    cage_dump_armor(list)
elseif choice:match '^wea' then
    cage_dump_weapons(list)
elseif choice == 'all' then
    cage_dump_all(list)
elseif choice == 'list' then
    cage_dump_list(list)
else
    print(dfhack.script_help())
end
