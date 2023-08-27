-- Original opening comment before lua adaptation
-- View or set cavern adaptation levels
-- based on removebadthoughts.rb
-- Rewritten by TBSTeun using OpenAI GPT from adaptation.rb

local args = {...}

local mode = args[1] or 'help'
local who = args[2]
local value = args[3]

local function print_color(color, s)
    dfhack.color(color)
    dfhack.print(s)
    dfhack.color(COLOR_RESET)
end

local function usage(s)
    if s then
        dfhack.printerr(s)
    end
    print(dfhack.script_help())
end

local function set_adaptation_value(unit, v)
    if not dfhack.units.isCitizen(unit) or not dfhack.units.isAlive(unit) then
        return 0
    end

    for _, t in ipairs(unit.status.misc_traits) do
        if t.id == df.misc_trait_type.CaveAdapt then
            if mode == 'show' then
                print_color(COLOR_RESET, ('Unit %s (%s) has an adaptation of '):format(unit.id, dfhack.TranslateName(dfhack.units.getVisibleName(unit))))
                if t.value <= 399999 then
                    print_color(COLOR_GREEN, ('%s\n'):format(t.value))
                elseif t.value <= 599999 then
                    print_color(COLOR_YELLOW, ('%s\n'):format(t.value))
                else
                    print_color(COLOR_RED, ('%s\n'):format(t.value))
                end

                return 0
            elseif mode == 'set' then
                print(('Unit %s (%s) changed from %s to %s'):format(unit.id, dfhack.TranslateName(dfhack.units.getVisibleName(unit)), t.value, v))
                t.value = v
                return 1
            end
        end
    end
    if mode == 'show' then
        print_color(COLOR_RESET, ('Unit %s (%s) has an adaptation of '):format(unit.id, dfhack.TranslateName(dfhack.units.getVisibleName(unit))))
        print_color(COLOR_GREEN, '0\n')
    elseif mode == 'set' then
        local new_trait = dfhack.units.getMiscTrait(unit, df.misc_trait_type.CaveAdapt, true)
        new_trait.id = df.misc_trait_type.CaveAdapt
        new_trait.value = v
        print(('Unit %s (%s) changed from 0 to %d'):format(unit.id, dfhack.TranslateName(dfhack.units.getVisibleName(unit)), v))
        return 1
    end

    return 0
end

if mode == 'help' then
    usage()
    return
elseif mode ~= 'show' and mode ~= 'set' then
    usage(('Invalid mode %s: must be either "show" or "set"'):format(mode))
    return
end

if not who then
    usage('Target not specified')
    return
elseif who ~= 'him' and who ~= 'all' then
    usage(('Invalid target %s'):format(who))
    return
end

if mode == 'set' then
    if not value then
        usage('Value not specified')
        return
    elseif not tonumber(value) then
        usage(('Invalid value %s'):format(value))
        return
    end

    value = tonumber(value)
    if value < 0 or value > 800000 then
        usage(('Value must be between 0 and 800000 (inclusive), input value was %s'):format(value))
    end
end

if who == 'him' then
    local u = dfhack.gui.getSelectedUnit(true)
    if u then
        set_adaptation_value(u, value)
    else
        dfhack.printerr('Please select a dwarf ingame')
    end
elseif who == 'all' then
    local num_set = 0

    for _, uu in ipairs(df.global.world.units.all) do
        num_set = num_set + set_adaptation_value(uu, value)
    end

    if num_set > 0 then
        print(('%s units changed'):format(num_set))
    end
end
