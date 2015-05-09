-- makes creatures [in]fertile, by modifying orientation
-- usage:  fix-ster [fert|ster] [all|animals|only:<creature>]
-- by Tacomagic (minor fixes by PeridexisErrant)

--[[
This script utilizes the orientation tag to either fix infertile creatures
or inflict infertility on creatures that you do not want to breed

Required arg:
   fert: sets the flag to assure creature(s) are fertile
   ster: sets the flag to assure creature(s) are sterile
Optional args:
   <no arg>         the script will only process the currently selected creature
   all:             process all creatures currently on the map
   animals:         processes everything but dwarves on the map
   only:<creature>: processes all of the creatures matching the specified creature on the map
]]--

function getunit()
    local unit
    unit=dfhack.gui.getSelectedUnit()
    if unit==nil then
        print("No unit under cursor.")
        return
    end
    return unit
end

function changeorient(unit,ori)
    --Sets the fertility flag based on gender.
    if unit.sex == 0 then
        unit.status.current_soul.orientation_flags.marry_male=ori
    else
        unit.status.current_soul.orientation_flags.marry_female=ori
    end
    return
end

function changelots(creatures,ori)
    local v
    local unit
    local c = 0
    --loops through indexes in creatures table and changes orientation flags
    for _,v in ipairs(creatures) do
        unit = df.global.world.units.active[v]
        changeorient(unit,ori)
        c = c + 1
    end
    print("Changed "..c.. " creatures.")
    return
end

function process_args(arg)
    local n,v
    local ori
    local creatures = {}
    local crenum
    --Checks for any arguments at all.
    if arg==nil then
        print("No arguments.  Usage is: fixster <fert|ster> [all|animals|only:<creature>]")
        return
    end
    if string.lower(arg[1])=="ster" then
        ori = false
    elseif string.lower(arg[1])=="fert" then
        ori = true
    else
        print("Unrecognised first argument.  Aborting.")
        return
    end

    --Checks for the existence of the second argument.  If it's missing, uses selected unit (if any)
    if arg[2]==nil then
        unit = getunit()
        if unit == nil then return end
        changeorient(unit,ori)
        print('Changed selected creature.')

    --ALL arg processing
    elseif string.lower(arg[2])=="all" then
        --Create table of all current unit indexes
        for n,v in ipairs(df.global.world.units.active) do
            table.insert(creatures,n)
        end
        changelots(creatures,ori)

    --ANIMALS arg processing
    elseif string.lower(arg[2])=="animals" then
        --Create a table of all creature indexes except dwarves on the current map
        for n,v in ipairs(df.global.world.units.active) do
            if v.race~=df.global.ui.race_id then
                table.insert(creatures,n)
            end
        end
        changelots(creatures,ori)

    -- ONLY:<creature> arg processing
    elseif string.lower(string.sub(arg[2],1,4))=="only" then
        creidx = string.upper(string.sub(arg[2],6))

        print(string.upper(string.sub(arg[2],6)))

        --Search raws for creature
        for k,v in pairs(df.global.world.raws.creatures.all) do
            if v.creature_id==creidx then
                crenum = k
            end
        end
        --If no match, abort
        if crenum == nil then
            print("Creature not found.  Check spelling.")
            return
        end

        --create a table of all the matching creature indexes on the map for processing
        for n,v in ipairs(df.global.world.units.active) do
            if v.race == crenum then
                table.insert(creatures,n)
            end
        end
        changelots(creatures,ori)
    else
        print("Unrecognised optional argument.  Aborting")
        return
    end
    return
end

local arg = table.pack(...)
process_args(arg)
