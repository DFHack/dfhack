-- makes creatures [in]fertile, by modifying orientation
-- usage:  fix-ster [fert|ster] [all|animals|only:<creature>]
-- original author: Tacomagic
-- minor fixes by PeridexisErrant, Lethosor
--@ module = true
--[[=begin

fix-ster
========
Utilizes the orientation tag to either fix infertile creatures or inflict
infertility on creatures that you do not want to breed.  Usage::

    fix-ster [fert|ster] [all|animals|only:<creature>]

``fert`` or ``ster`` is a required argument; whether to make the target fertile
or sterile.  Optional arguments specify the target: no argument for the
selected unit, ``all`` for all units on the map, ``animals`` for all non-dwarf
creatures, or ``only:<creature>`` to only process matching creatures.

=end]]

function changeorient(unit, ori)
    --Sets the fertility flag based on gender.
    if not unit.status.current_soul then
        return
    end
    if unit.sex == 0 then
        unit.status.current_soul.orientation_flags.marry_male=ori
    else
        unit.status.current_soul.orientation_flags.marry_female=ori
    end
end

function changelots(creatures, ori, silent)
    local v, unit
    local c = 0
    --loops through indexes in creatures table and changes orientation flags
    for _, v in ipairs(creatures) do
        unit = df.global.world.units.active[v]
        changeorient(unit,ori)
        c = c + 1
    end
    if not silent then
        print("Changed " .. c .. " creatures.")
    end
end

function process_args(args)
    local n, v, ori, crename, crenum
    local creatures = {}
    --Checks for any arguments at all.
    if args == nil or #args == 0 then
        print("No arguments.  Usage is: fixster <fert|ster> [all|animals|only:<creature>]")
        return
    end
    for i, a in pairs(args) do
        args[i] = tostring(a):lower()
    end
    if args[1]:sub(1, 1) == "s" then  -- sterile
        ori = false
    elseif args[1]:sub(1, 1) == "f" then  -- fertile
        ori = true
    else
        qerror("Unrecognised first argument: " .. args[1] .. ".  Aborting.")
    end

    --Checks for the existence of the second argument.  If it's missing, uses selected unit (if any)
    if args[2] == nil then
        unit = dfhack.gui.getSelectedUnit()
        if not unit then return end
        changeorient(unit, ori)
        print('Changed selected creature.')

    --ALL arg processing
    elseif args[2] == "all" then
        --Create table of all current unit indexes
        for n,v in ipairs(df.global.world.units.active) do
            table.insert(creatures,n)
        end
        changelots(creatures,ori)

    --ANIMALS arg processing
    elseif args[2] == "animals" then
        --Create a table of all creature indexes except dwarves on the current map
        for n,v in ipairs(df.global.world.units.active) do
            if v.race ~= df.global.ui.race_id then
                table.insert(creatures,n)
            end
        end
        changelots(creatures,ori)

    -- ONLY:<creature> arg processing
    elseif args[2]:sub(1,4) == "only" then
        crename = args[2]:sub(6):upper()

        --Search raws for creature
        for k,v in pairs(df.global.world.raws.creatures.all) do
            if v.creature_id == crename then
                crenum = k
            end
        end
        --If no match, abort
        if crenum == nil then
            qerror("Creature not found.  Check spelling.")
        end

        --create a table of all the matching creature indexes on the map for processing
        for n,v in ipairs(df.global.world.units.active) do
            if v.race == crenum then
                table.insert(creatures,n)
            end
        end
        changelots(creatures,ori)
    else
        qerror("Unrecognised optional argument. Aborting")
    end
end

if not moduleMode then
    local args = table.pack(...)
    process_args(args)
end
