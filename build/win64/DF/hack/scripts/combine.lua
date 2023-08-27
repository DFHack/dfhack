-- Combines items in a stockpile that could be stacked together

local argparse = require('argparse')
local utils = require('utils')

local opts, args = {
    help = false,
    all = nil,
    here = nil,
    dry_run = false,
    types = nil,
    quiet = false,
    verbose = 0,
  }, {...}

-- default max stack size of 30
local MAX_ITEM_STACK=30
local MAX_AMMO_STACK=25
local MAX_CONT_ITEMS=30
local MAX_MAT_AMT=30

-- list of types that use race and caste
local typesThatUseCreatures = utils.invert{'REMAINS', 'FISH', 'FISH_RAW', 'VERMIN', 'PET', 'EGG', 'CORPSE', 'CORPSEPIECE'}
local typesThatUseMaterial=utils.invert{'CORPSEPIECE'}

-- list of valid item types for merging
-- Notes: 1. mergeable stacks are ones with the same type_id+race+caste or type_id+mat_type+mat_index
--        2. the maximum stack size is calcuated at run time: the highest value of MAX_ITEM_STACK or largest current stack size.
--        3. even though powders are specified, sand and plaster types items are excluded from merging.
--        4. seeds cannot be combined in stacks > 1.
local valid_types_map = {
    ['all']    = { },
    ['ammo']   = {[df.item_type.AMMO]        ={type_id=df.item_type.AMMO,         max_size=MAX_AMMO_STACK}},
    ['parts']  = {[df.item_type.CORPSEPIECE] ={type_id=df.item_type.CORPSEPIECE,  max_size=1}},
    ['drink']  = {[df.item_type.DRINK]       ={type_id=df.item_type.DRINK,        max_size=MAX_ITEM_STACK}},
    ['fat']    = {[df.item_type.GLOB]        ={type_id=df.item_type.GLOB,         max_size=MAX_ITEM_STACK},
                  [df.item_type.CHEESE]      ={type_id=df.item_type.CHEESE,       max_size=MAX_ITEM_STACK}},
    ['fish']   = {[df.item_type.FISH]        ={type_id=df.item_type.FISH,         max_size=MAX_ITEM_STACK},
                  [df.item_type.FISH_RAW]    ={type_id=df.item_type.FISH_RAW,     max_size=MAX_ITEM_STACK},
                  [df.item_type.EGG]         ={type_id=df.item_type.EGG,          max_size=MAX_ITEM_STACK}},
    ['food']   = {[df.item_type.FOOD]        ={type_id=df.item_type.FOOD,         max_size=MAX_ITEM_STACK}},
    ['meat']   = {[df.item_type.MEAT]        ={type_id=df.item_type.MEAT,         max_size=MAX_ITEM_STACK}},
    ['plant']  = {[df.item_type.PLANT]       ={type_id=df.item_type.PLANT,        max_size=MAX_ITEM_STACK},
                  [df.item_type.PLANT_GROWTH]={type_id=df.item_type.PLANT_GROWTH, max_size=MAX_ITEM_STACK}},
    ['powder'] = {[df.item_type.POWDER_MISC] ={type_id=df.item_type.POWDER_MISC,  max_size=MAX_ITEM_STACK}},
    ['seed']   = {[df.item_type.SEEDS]       ={type_id=df.item_type.SEEDS,        max_size=1}},
}

-- populate all types entry
for k1,v1 in pairs(valid_types_map) do
    if k1 ~= 'all' then
        for k2,v2 in pairs(v1) do
            valid_types_map['all'][k2]={}
            for k3,v3 in pairs (v2) do
                valid_types_map['all'][k2][k3]=v3
            end
        end
    end
end

local function log(level, ...)
    -- if verbose is specified, then print the arguments, or don't.
    if not opts.quiet and opts.verbose >= level then dfhack.print(string.format(...)) end
end

-- CList class
-- generic list class used for key value pairs.
local CList = { }

function CList:new(o)
    -- key, value pair table structure. __len allows # to be used for table count.
    o = o or { }
    setmetatable(o, self)
    self.__index = self
    self.__len = function (t) local n = 0 for _, __ in pairs(t) do n = n + 1 end return n end
    return o
end

local function comp_item_new(comp_key, max_size)
    -- create a new comp_item entry to be added to a comp_items table.
    local comp_item = {}
    if not comp_key then qerror('new_comp_item: comp_key is nil') end
    comp_item.comp_key = comp_key                       -- key used to index comparable items for merging
    comp_item.description = ''                          -- description of the comp item for output
    comp_item.max_size = max_size or 0                  -- how many of a comp item can be in one stack
    -- item info
    comp_item.items = CList:new(nil)                    -- key:item.id,
                                                        -- val:{item,
                                                        --      before_size, after_size, before_cont_id, after_cont_id,
                                                        --      stockpile_id, stockpile_name,
                                                        --      before_mat_amt {Leather, Bone, Shell, Tooth, Horn, HairWool, Yarn}
                                                        --      after_mat_amt {Leather, Bone, Shell, Tooth, Horn, HairWool, Yarn}
                                                        --  }
    comp_item.item_qty = 0                              -- total quantity of items
    comp_item.material_amt = 0                          -- total amount of materials
    comp_item.max_mat_amt = MAX_MAT_AMT                 -- max amount of materials in one stack

    comp_item.before_stacks = 0                         -- the number of stacks of the items before...
    comp_item.after_stacks = 0                          -- ...and after the merge
    --container info
    comp_item.before_cont_ids = CList:new(nil)          -- key:container.id, val:container.id
    comp_item.after_cont_ids = CList:new(nil)           -- key:container.id, val:container.id
    return comp_item
end

local function comp_item_add_item(stockpile, stack_type, comp_item, item, container)
    -- add an item into the comp_items table, setting the comp_item attributes.
    if not comp_item.items[item.id] then
        comp_item.item_qty = comp_item.item_qty + item.stack_size
        comp_item.before_stacks = comp_item.before_stacks + 1
        comp_item.description = utils.getItemDescription(item, 1)

        if item.stack_size > comp_item.max_size then
            comp_item.max_size = item.stack_size
        end

        local new_item = {}
        new_item.item = item
        new_item.before_size = item.stack_size

        new_item.stockpile_id = stockpile.id
        new_item.stockpile_name = stockpile.name

        -- material amount info
        new_item.before_mat_amt = {}
        new_item.before_mat_amt.Qty = 0
        new_item.after_mat_amt = {}
        new_item.after_mat_amt.Qty = 0

        -- material amount used?
        if typesThatUseMaterial[df.item_type[stack_type.type_id]] then
            new_item.before_mat_amt.Leather  = item.material_amount.Leather
            new_item.before_mat_amt.Bone     = item.material_amount.Bone
            new_item.before_mat_amt.Shell    = item.material_amount.Shell
            new_item.before_mat_amt.Tooth    = item.material_amount.Tooth
            new_item.before_mat_amt.Horn     = item.material_amount.Horn
            new_item.before_mat_amt.HairWool = item.material_amount.HairWool
            new_item.before_mat_amt.Yarn     = item.material_amount.Yarn
            for _, v in pairs(new_item.before_mat_amt) do if new_item.before_mat_amt.Qty < v then new_item.before_mat_amt.Qty = v end end

            comp_item.material_amt = comp_item.material_amt + new_item.before_mat_amt.Qty
            if new_item.before_mat_amt.Qty > comp_item.max_mat_amt then comp_item.max_mat_amt = new_item.before_mat_amt.Qty end
        end

        -- item is in a container
        if container then
            new_item.before_cont_id = container.id
            comp_item.before_cont_ids[container.id] = container.id
        end

        comp_item.items[item.id] = new_item
        return comp_item.items[item.id]
    else
        -- this case should not happen, unless an item id is duplicated.
        -- in which case, only allow one instance for the merge.
        return nil
    end
end

local function stack_type_new(type_vals)
    -- create a new stack type entry to be added to the stacks table.
    local stack_type = {}

    -- attributes from the type val table
    for k,v in pairs(type_vals) do
        stack_type[k] = v
    end

    -- item info
    stack_type.comp_items = CList:new(nil)              -- key:comp_key, val:comp_item
    stack_type.item_qty = 0                             -- total quantity of items types
    stack_type.material_amt = 0                         -- total amount of materials
    stack_type.before_stacks = 0                        -- the number of stacks of the item types before ...
    stack_type.after_stacks = 0                         -- ...and after the merge

    --container info
    stack_type.before_cont_ids = CList:new(nil)         -- key:container.id, val:container.id
    stack_type.after_cont_ids = CList:new(nil)          -- key:container.id, val:container.id
    return stack_type
end

local function stacks_add_item(stockpile, stacks, stack_type, item, container)
    -- add an item to the matching comp_items table; based on comp_key.
    local comp_key = ''

    if typesThatUseCreatures[df.item_type[stack_type.type_id]] then
        if not typesThatUseMaterial[df.item_type[stack_type.type_id]] then
            comp_key = tostring(stack_type.type_id) .. "+" .. tostring(item.race) .. "+" .. tostring(item.caste)
        else
            comp_key = tostring(stack_type.type_id) .. "+" .. tostring(item.race) .. "+" .. tostring(item.caste) .. "+" .. tostring(item:getActualMaterial()) .. "+" .. tostring(item:getActualMaterialIndex())
        end
    elseif item:isAmmo() then
        if item:getQuality() == 5 then
            comp_key = tostring(stack_type.type_id) .. "+" .. tostring(item.mat_type) .. "+" .. tostring(item.mat_index) .. "+" .. tostring(item:getQuality() .. "+" .. item:getMaker())
        else
            comp_key = tostring(stack_type.type_id) .. "+" .. tostring(item.mat_type) .. "+" .. tostring(item.mat_index) .. "+" .. tostring(item:getQuality())
        end
    else
        comp_key = tostring(stack_type.type_id) .. "+" .. tostring(item.mat_type) .. "+" .. tostring(item.mat_index)
    end

    if not stack_type.comp_items[comp_key] then
        stack_type.comp_items[comp_key] = comp_item_new(comp_key, stack_type.max_size)
    end

    local new_comp_item_item = comp_item_add_item(stockpile, stack_type, stack_type.comp_items[comp_key], item, container)
    if new_comp_item_item then
        stack_type.before_stacks = stack_type.before_stacks + 1
        stack_type.item_qty = stack_type.item_qty + item.stack_size
        stack_type.material_amt = stack_type.material_amt + new_comp_item_item.before_mat_amt.Qty

        stacks.before_stacks = stacks.before_stacks + 1
        stacks.item_qty = stacks.item_qty + item.stack_size
        stacks.material_amt = stacks.material_amt + new_comp_item_item.before_mat_amt.Qty

        if item.stack_size > stack_type.max_size then
            stack_type.max_size = item.stack_size
        end

        -- item is in a container
        if container then

            -- add it to the stack type list
            stack_type.before_cont_ids[container.id] = container.id

            -- add it to the before stacks container list
            stacks.before_cont_ids[container.id] = container.id
        end
    end
end

local function sorted_items_qty(tab)
    -- used to sort the comp_items by contained, then size. Important for combining containers.
    local tmp = {}
    for id, val in pairs(tab) do
        local val = {id=id, before_cont_id=val.before_cont_id, before_size=val.before_size}
        table.insert(tmp, val)
    end

    table.sort(tmp,
        function(a, b)
            if not a.before_cont_id and not b.before_cont_id or a.before_cont_id and b.before_cont_id then
                return a.before_size > b.before_size
            else
                return a.before_cont_id and not b.before_cont_id
            end
        end
        )

    local i = 0
    local iter =
        function()
            i = i + 1
            if tmp[i] == nil then
                return nil
            else
                return tmp[i].id, tab[tmp[i].id]
            end
        end
    return iter
end

local function sorted_items_mat(tab)
    -- used to sort the comp_items by mat amt.
    local tmp = {}
    for id, val in pairs(tab) do
        local val = {id=id, before_qty=val.before_mat_amt.Qty}
        table.insert(tmp, val)
    end

    table.sort(tmp,
        function(a, b)
                return a.before_qty > b.before_qty
        end
        )

    local i = 0
    local iter =
        function()
            i = i + 1
            if tmp[i] == nil then
                return nil
            else
                return tmp[i].id, tab[tmp[i].id]
            end
        end
    return iter
end

local function sorted_desc(tab, ids)
    -- used to sort the lists by description
    local tmp = {}
    for id, val in pairs(tab) do
        if ids[id] then
            local val = {id=id, description=val.description}
            table.insert(tmp, val)
        end
    end

    table.sort(tmp, function(a, b) return a.description < b.description end)

    local i = 0
    local iter =
        function()
            i = i + 1
            if tmp[i] == nil then
                return nil
            else
                return tmp[i].id, tab[tmp[i].id]
            end
        end
    return iter
end

local function print_stacks_details(stacks, quiet)
    -- print stacks details
    if quiet then return end
    if #stacks.containers > 0 then
        log(1, 'Summary:\nContainers:%5d before:%5d  after:%5d\n', #stacks.containers, #stacks.before_cont_ids, #stacks.after_cont_ids)
        for cont_id, cont in sorted_desc(stacks.containers, stacks.before_cont_ids) do
            log(2, ('   Cont: %50s <%6d>   bef:%5d aft:%5d\n'):format(cont.description, cont_id, cont.before_size, cont.after_size))
        end
    end
    if stacks.item_qty > 0 then
        log(1, ('Items: #Qty: %6d sizes: bef:%5d aft:%5d Mat amt:%6d\n'):format(stacks.item_qty, stacks.before_stacks, stacks.after_stacks, stacks.material_amt))
        for key, stack_type in pairs(stacks.stack_types) do
            if stack_type.item_qty > 0 then
                log(1, ('   Type: %12s <%d>   #Qty:%6d sizes: max:%5d bef:%6d aft:%6d Cont: bef:%5d aft:%5d Mat amt:%6d\n'):format(df.item_type[stack_type.type_id], stack_type.type_id,  stack_type.item_qty, stack_type.max_size, stack_type.before_stacks, stack_type.after_stacks, #stack_type.before_cont_ids, #stack_type.after_cont_ids, stack_type.material_amt))
                for _, comp_item in sorted_desc(stack_type.comp_items, stack_type.comp_items) do
                    if comp_item.item_qty > 0 then
                        log(2, ('      Comp item:%40s <%12s>  #Qty:%6d #stacks:%5d max:%5d bef:%6d aft:%6d Cont: bef:%5d aft:%5d Mat amt:%6d\n'):format(comp_item.description, comp_item.comp_key, comp_item.item_qty, #comp_item.items, comp_item.max_size, comp_item.before_stacks, comp_item.after_stacks, #comp_item.before_cont_ids, #comp_item.after_cont_ids, comp_item.material_amt))
                        for _, item in sorted_items_qty(comp_item.items) do
                            log(3, ('           Item:%40s <%6d> Qty: bef:%6d aft:%6d Cont: bef:<%5d> aft:<%5d> Mat Amt: bef: %6d aft:%6d stockpile:%s'):format(utils.getItemDescription(item.item), item.item.id, item.before_size or 0, item.after_size or 0, item.before_cont_id or 0, item.after_cont_id or 0, item.before_mat_amt.Qty or 0, item.after_mat_amt.Qty or 0, item.stockpile_name))
                            log(4, (' stackable: %s'):format(df.item_type.attrs[stack_type.type_id].is_stackable))
                            log(3, ('\n'))
                        end
                    end
                end
            end
        end
    end
end

local function print_stacks_summary(stacks, quiet, dry_run)
    -- print stacks summary to the console
    local printed = 0
    for _, s in pairs(stacks.stack_types) do
        if s.before_stacks ~= s.after_stacks then
            printed = printed + 1
            local str = ''
            if dry_run then str = 'will combine' else str ='combined' end
            print(('%s %d %s items from %d stacks into %d')
                    :format(str, s.item_qty, df.item_type[s.type_id], s.before_stacks, s.after_stacks))
        end
    end
    if printed == 0 and not quiet then
        print('All stacks already optimally combined.')
    end
end

local function stacks_new()
    local stacks = {}

    stacks.stack_types = CList:new(nil)         -- key=type_id, val=stack_type
    stacks.containers = CList:new(nil)          -- key=container.id, val={container, description, before_size, after_size}
    stacks.before_cont_ids = CList:new(nil)     -- key=container.id, val=container.id
    stacks.after_cont_ids = CList:new(nil)      -- key=container.id, val=container.id
    stacks.item_qty = 0
    stacks.material_amt = 0                     -- total amount of materials - used for CORPSEPIECEs
    stacks.before_stacks = 0
    stacks.after_stacks = 0

    return stacks
end

local function isRestrictedItem(item)
    -- is the item restricted from merging?
    local flags = item.flags
    return flags.rotten or flags.trader or flags.hostile or flags.forbid
        or flags.dump or flags.on_fire or flags.garbage_collect or flags.owned
        or flags.removed or flags.encased or flags.spider_web or flags.melt
        or flags.hidden or #item.specific_refs > 0
end

local function isValidPart(item)
    return item:getMaterial() >= 0 or
        (not item.corpse_flags.unbutchered and (
            item.material_amount.Leather > 0 or
            item.material_amount.Bone > 0 or
            item.material_amount.Shell > 0 or
            item.material_amount.Tooth > 0 or
            item.material_amount.Horn > 0 or
            item.material_amount.HairWool > 0 or
            item.material_amount.Yarn > 0))
end

local function stacks_add_items(stockpile, stacks, items, container, ind)
-- loop through each item and add it to the matching stack[type_id].comp_items table
-- recursively calls itself to add contained items
    if not ind then ind = '' end

    for _, item in pairs(items) do
        local type_id = item:getType()
        local subtype_id = item:getSubtype()
        local stack_type = stacks.stack_types[type_id]

        -- item type in list of included types?
        if stack_type and not item:isSand() and not item:isPlaster() and isValidPart(item) then
            if not isRestrictedItem(item) then

                stacks_add_item(stockpile, stacks, stack_type, item, container)

                if typesThatUseCreatures[df.item_type[type_id]] then
                    local raceRaw = df.global.world.raws.creatures.all[item.race]
                    local casteRaw = raceRaw.caste[item.caste]
                    log(4, ('      %sitem:%40s <%6d> is incl, type:%d, race:%s, caste:%s\n'):format(ind, utils.getItemDescription(item), item.id, type_id,  raceRaw.creature_id,  casteRaw.caste_id))
                elseif item:isAmmo() then
                    local mat_info = dfhack.matinfo.decode(item.mat_type, item.mat_index)
                    log(4, ('      %sitem:%40s <%6d> is incl, type:%d, info:%s, quality:%d, maker:%d\n'):format(ind, utils.getItemDescription(item), item.id, type_id, mat_info:toString(), item:getQuality(), item:getMaker()))
                else
                    local mat_info = dfhack.matinfo.decode(item.mat_type, item.mat_index)
                    log(4, ('      %sitem:%40s <%6d> is incl, type:%d, info:%s, sand:%s, plasterplaster:%s quality:%d ovl quality:%d\n'):format(ind, utils.getItemDescription(item), item.id, type_id, mat_info:toString(), item:isSand(), item:isPlaster(), item:getQuality(), item:getOverallQuality()))
                end

            else
                -- restricted; such as marked for action or dump.
                log(4, ('      %sitem:%40s <%6d> is restricted\n'):format(ind, utils.getItemDescription(item), item.id))
            end

        -- add contained items
        elseif dfhack.items.getGeneralRef(item, df.general_ref_type.CONTAINS_ITEM) then
            local contained_items = dfhack.items.getContainedItems(item)
            local count = #contained_items
            stacks.containers[item.id] = {}
            stacks.containers[item.id].container = item
            stacks.containers[item.id].before_size = #contained_items
            stacks.containers[item.id].description = utils.getItemDescription(item, 1)
            log(4, ('      %sContainer:%s <%6d> #items:%5d\n'):format(ind, utils.getItemDescription(item), item.id, count, item:isSandBearing()))
            stacks_add_items(stockpile, stacks, contained_items, item, ind .. '   ')

        -- excluded item types
        else
            log(5, ('      %sitem:%40s <%6d> is excl, type %d, sand:%s plaster:%s\n'):format(ind, utils.getItemDescription(item), item.id, type_id, item:isSand(), item:isPlaster()))
        end
    end
end

local function populate_stacks(stacks, stockpiles, types)
    -- 1. loop through the specified types and add them to the stacks table. stacks[type_id]
    -- 2. loop through the table of stockpiles, get each item in the stockpile, then add them to stacks if the type_id matches
    -- an item is stored at the bottom of the structure: stacks[type_id].comp_items[comp_key].item
    -- comp_key is a compound key comprised of type_id+race+caste or type_id+mat_type+mat_index
    log(4, 'Populating phase\n')

    -- iterate across the types
    log(4, 'stack types\n')
    for type_id, type_vals in pairs(types) do
        if not stacks.stack_types[type_id] then
            stacks.stack_types[type_id] = stack_type_new(type_vals)
            local stack_type = stacks.stack_types[type_id]
            log(4, ('   type: <%12s> <%d>   #item_qty:%5d  stack sizes:  max: %5d  bef:%5d  aft:%5d\n'):format(df.item_type[stack_type.type_id], stack_type.type_id,  stack_type.item_qty, stack_type.max_size, stack_type.before_stacks, stack_type.after_stacks))
        end
    end

    -- iterate across the stockpiles, get the list of items and call the add function to check/add as needed
    log(4, ('stockpiles\n'))
    for _, stockpile in pairs(stockpiles) do

        local items = dfhack.buildings.getStockpileContents(stockpile)
        log(4, ('   stockpile:%30s <%6d> pos:(%3d,%3d,%3d) #items:%5d\n'):format(stockpile.name, stockpile.id, stockpile.centerx, stockpile.centery, stockpile.z,  #items))

        if #items > 0 then
            stacks_add_items(stockpile, stacks, items)
        else
            log(4, '      skipping stockpile: no items\n')
        end
    end
end

local function preview_stacks(stacks)
    -- calculate the stacks sizes and store in after_item_stack_size
    -- the max stack size for each comp item is determined as the maximum stack size for it's type
    log(4, '\nPreview phase\n')

    for _, stack_type in pairs(stacks.stack_types) do
        log(4, ('   type: <%12s> <%d>   #item_qty:%5d  stack sizes:  max: %5d  bef:%5d  aft:%5d\n'):format(df.item_type[stack_type.type_id], stack_type.type_id,  stack_type.item_qty, stack_type.max_size, stack_type.before_stacks, stack_type.after_stacks))

        for comp_key, comp_item in pairs(stack_type.comp_items) do
            log(4, ('      comp item:%40s <%12s>  #qty:%5d #stacks:%5d sizes: max:%5d bef:%5d aft:%5d Cont: bef:%5d aft:%5d\n'):format(comp_item.description, comp_item.comp_key, comp_item.item_qty, #comp_item.items, comp_item.max_size, comp_item.before_stacks, comp_item.after_stacks, #comp_item.before_cont_ids, #comp_item.after_cont_ids))

            -- item qty used?
            if not typesThatUseMaterial[df.item_type[stack_type.type_id]] then

                -- max size comparison
                if stack_type.max_size > comp_item.max_size then
                    comp_item.max_size = stack_type.max_size
                end

                -- how many stacks are needed?
                local stacks_needed = math.floor(comp_item.item_qty / comp_item.max_size)

                -- how many items are left over after the max stacks are allocated?
                local stack_remainder = comp_item.item_qty - stacks_needed * comp_item.max_size

                if stack_remainder > 0 then
                    comp_item.after_stacks = stacks_needed + 1
                else
                    comp_item.after_stacks = stacks_needed
                end

                stack_type.after_stacks = stack_type.after_stacks + comp_item.after_stacks
                stacks.after_stacks = stacks.after_stacks + comp_item.after_stacks

                -- Update the after stack sizes.
                for _, item in sorted_items_qty(comp_item.items) do
                    if stacks_needed > 0 then
                        stacks_needed = stacks_needed - 1
                        item.after_size = comp_item.max_size
                    elseif stack_remainder > 0 then
                        item.after_size = stack_remainder
                        stack_remainder = 0
                    else
                        item.after_size = 0
                    end
                end

            -- material amount used.
            else
                local stacks_needed = math.floor(comp_item.material_amt / comp_item.max_mat_amt)
                local stack_remainder = comp_item.material_amt - stacks_needed * comp_item.max_mat_amt

                if stack_remainder > 0 then
                    comp_item.after_stacks = stacks_needed + 1
                else
                    comp_item.after_stacks = stacks_needed
                end

                stack_type.after_stacks = stack_type.after_stacks + comp_item.after_stacks
                stacks.after_stacks = stacks.after_stacks + comp_item.after_stacks

                for k1, item in sorted_items_mat(comp_item.items) do
                    item.after_mat_amt = {}
                    if stacks_needed > 0 then
                        stacks_needed = stacks_needed - 1
                        item.after_size = item.before_size
                        for k2, v in pairs(item.before_mat_amt) do
                            if v > 0 then
                                item.after_mat_amt[k2] = comp_item.max_mat_amt
                            else
                                item.after_mat_amt[k2] = 0
                            end
                        end
                    elseif stack_remainder > 0 then
                        item.after_size = item.before_size
                        for k2, v in pairs(item.before_mat_amt) do
                            if v > 0 then
                                item.after_mat_amt[k2] = stack_remainder
                            else
                                item.after_mat_amt[k2] = 0
                            end
                        end
                        stack_remainder = 0
                    else
                        for k2, v in pairs(item.before_mat_amt) do
                            item.after_mat_amt[k2] = 0
                        end
                        item.after_size = 0
                    end
                end
            end

            -- Container loop; combine item stacks in containers.
            local curr_cont = nil
            local curr_size = 0

            for item_id, item in sorted_items_qty(comp_item.items) do

                -- non-zero quantity?
                if item.after_size > 0 then

                    -- in a container before merge?
                    if item.before_cont_id then

                        local before_cont = stacks.containers[item.before_cont_id]

                        -- first contained item or current container full?
                        if not curr_cont or curr_size >= MAX_CONT_ITEMS then

                            curr_cont = before_cont
                            curr_size = curr_cont.before_size
                            stacks.after_cont_ids[item.before_cont_id] = item.before_cont_id
                            stack_type.after_cont_ids[item.before_cont_id] = item.before_cont_id
                            comp_item.after_cont_ids[item.before_cont_id] = item.before_cont_id

                        -- enough room in current container
                        else
                            curr_size = curr_size + 1
                            before_cont.after_size = (before_cont.after_size or before_cont.before_size) - 1
                        end

                        curr_cont.after_size = curr_size
                        item.after_cont_id = curr_cont.container.id

                    -- not in a container before merge, container exists, and has space
                    elseif curr_cont and curr_size < MAX_CONT_ITEMS then

                        curr_size = curr_size + 1
                        curr_cont.after_size = curr_size
                        item.after_cont_id = curr_cont.container.id

                    -- not in a container, no container exists or no space in container
                    else
                        -- do nothing
                    end

                -- zero after size, reduce the number of stacks in the container
                elseif item.before_cont_id then
                    local before_cont = stacks.containers[item.before_cont_id]
                    before_cont.after_size = (before_cont.after_size or before_cont.before_size) - 1
                end
            end
            log(4, ('      comp item:%40s <%12s>  #qty:%5d #stacks:%5d sizes: max:%5d bef:%5d aft:%5d cont: bef:%5d aft:%5d\n'):format(comp_item.description, comp_item.comp_key, comp_item.item_qty, #comp_item.items, comp_item.max_size, comp_item.before_stacks, comp_item.after_stacks, #comp_item.before_cont_ids, #comp_item.after_cont_ids))
        end
        log(4, ('   type: <%12s> <%d>   #item_qty:%5d  stack sizes:  max: %5d  bef:%5d  aft:%5d\n'):format(df.item_type[stack_type.type_id], stack_type.type_id,  stack_type.item_qty, stack_type.max_size, stack_type.before_stacks, stack_type.after_stacks))
    end
end

local function merge_stacks(stacks)
    -- apply the stack size changes in the after_item_stack_size
    -- if the after_item_stack_size is zero, then remove the item
    log(4, 'Merge phase\n')
    for _, stack_type in pairs(stacks.stack_types) do
        for comp_key, comp_item in pairs(stack_type.comp_items) do

            for item_id, item in pairs(comp_item.items) do
                log(4, ('  item amt:%40s <%6d> bef:%5d aft:%5d cont: bef:<%5d> aft:<%5d> mat: bef:%5d aft:%5d '):format(comp_item.description, item.item.id, item.before_size or 0, item.after_size or 0, item.before_cont_id or 0, item.after_cont_id or 0, item.before_mat_amt.Qty or 0, item.after_mat_amt.Qty or 0))

                -- no items left in stack?
                if item.after_size == 0 then
                    log(4, '  removing\n')
                    dfhack.items.remove(item.item)

                -- some items left in stack
                elseif not typesThatUseMaterial[df.item_type[stack_type.type_id]] and item.before_size ~= item.after_size then
                    log(4, '  updating qty\n')
                    item.item.stack_size = item.after_size

                elseif typesThatUseMaterial[df.item_type[stack_type.type_id]] and item.before_mat_amt.Qty ~= item.after_mat_amt.Qty then
                    log(4, '  updating material\n')
                    item.item.material_amount.Leather  = item.after_mat_amt.Leather
                    item.item.material_amount.Bone     = item.after_mat_amt.Bone
                    item.item.material_amount.Shell    = item.after_mat_amt.Shell
                    item.item.material_amount.Tooth    = item.after_mat_amt.Tooth
                    item.item.material_amount.Horn     = item.after_mat_amt.Horn
                    item.item.material_amount.HairWool = item.after_mat_amt.HairWool
                    item.item.material_amount.Yarn     = item.after_mat_amt.Yarn
                else
                    log(4, '  no change\n')
                end

                -- move to a container?
                if item.after_cont_id then
                    if (item.before_cont_id or 0) ~= item.after_cont_id then
                        log(4, ('  moving   item:%40s <%6d> bef:%5d aft:%5d cont: bef:<%5d> aft:<%5d>\n'):format(comp_item.description, item.item.id, item.before_size or 0, item.after_size or 0, item.before_cont_id or 0, item.after_cont_id or 0))
                        dfhack.items.moveToContainer(item.item, stacks.containers[item.after_cont_id].container)
                    end
                end
            end
        end
    end
end

local function get_stockpile_all()
    -- attempt to get all the stockpiles for the fort, or exit with error
    -- return the stockpiles as a table
    local stockpiles = {}
    for _, building in pairs(df.global.world.buildings.all) do
        if building:getType() == df.building_type.Stockpile then
            table.insert(stockpiles, building)
        end
    end
    if opts.verbose > 0 then
        print(('Stockpile(all): %d found'):format(#stockpiles))
    end
    return stockpiles
end

local function get_stockpile_here()
    -- attempt to get the selected stockpile, or exit with error
    -- return the stockpile as a table
    local stockpiles = {}
    local building = dfhack.gui.getSelectedStockpile()
    if not building then qerror('Please select a stockpile.') end
    table.insert(stockpiles, building)
    local items = dfhack.buildings.getStockpileContents(building)
    if opts.verbose > 0 then
        print(('Stockpile(here): %s <%d> #items:%d'):format(building.name, building.id, #items))
    end
    return stockpiles
end

local function parse_types_opts(arg)
    -- check the types specified on the command line, or exit with error
    -- return the selected types as a table
    local types = {}
    local div = ''
    local types_output = ''

    if not arg then
        qerror('Expected: comma separated list of types')
    end

    for _, t in pairs(argparse.stringList(arg)) do
        if not valid_types_map[t] then
            qerror(('Unknown type: %s'):format(t))
        end

        for k2, v2 in pairs(valid_types_map[t]) do
            if not types[k2] then
                types[k2]={}
                for k3, v3 in pairs(v2) do
                    types[k2][k3]=v3
                end
                types_output = types_output .. div .. df.item_type[types[k2].type_id]
                div=', '
            else
                qerror(('Expected: only one value for %s'):format(t))
            end
        end
    end
    return types
end

local function parse_commandline(opts, args)
    -- check the command line/exit on error, and set the defaults
    local positionals = argparse.processArgsGetopt(args, {
            {'h', 'help', handler=function() opts.help = true end},
            {'t', 'types', hasArg=true, handler=function(optarg) opts.types=parse_types_opts(optarg) end},
            {'d', 'dry-run', handler=function() opts.dry_run = true end},
            {'q', 'quiet', handler=function() opts.quiet = true end},
            {'v', 'verbose', hasArg=true, handler=function(optarg) opts.verbose = math.tointeger(optarg) or 0 end},
    })

    -- if stockpile option is not specificed, then default to all
    if args[1] == 'all' then
        opts.all=get_stockpile_all()
    elseif args[1] == 'here' then
        opts.here=get_stockpile_here()
    else
        opts.help = true
    end

    -- if types option is not specified, then default to all
    if not opts.types then
        opts.types = valid_types_map['all']
    end
end

-- main program starts here
local function main()

    if df.global.gamemode ~= df.game_mode.DWARF or not dfhack.isMapLoaded() then
        qerror('combine needs a loaded fortress map to work\n')
    end

    parse_commandline(opts, args)

    if opts.help then
        print(dfhack.script_help())
        return
    end

    local stacks = stacks_new()

    populate_stacks(stacks,  opts.all or opts.here, opts.types)

    preview_stacks(stacks)

    if not opts.dry_run then
        merge_stacks(stacks)
    end

    print_stacks_details(stacks)
    print_stacks_summary(stacks, opts.quiet, opts.dry_run)

end

if not dfhack_flags.module then
    main()
end
