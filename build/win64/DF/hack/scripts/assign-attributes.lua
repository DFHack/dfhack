-- Change the attributes of a unit.
--@ module = true

local help = [====[

assign-attributes
=================
A script to change the physical and mental attributes of a unit.

Attributes are divided into tiers from -4 to 4. Tier 0 is the
standard level and represents the average values for that attribute,
tier 4 is the maximum level, and tier -4 is the minimum level.

An example of the attribute "Strength":

====  ===================
Tier  Description
====  ===================
4     unbelievably strong
3     mighty
2     very strong
1     strong
0     (no description)
-1    weak
-2    very weak
-3    unquestionably weak
-4    unfathomably weak
====  ===================

For more information:
https://dwarffortresswiki.org/index.php/DF2014:Attribute

Usage:

``-help``:
                    print the help page.

``-unit <UNIT_ID>``:
                    the target unit ID. If not present, the
                    currently selected unit will be the target.

``-attributes [ <ATTRIBUTE> <TIER> <ATTRIBUTE> <TIER> <...> ]``:
                    the list of the attributes to modify and their tiers.
                    The valid attribute names can be found in the wiki:
                    https://dwarffortresswiki.org/index.php/DF2014:Attribute
                    (substitute any space with underscores); tiers range from -4
                    to 4. There must be a space before and after each square
                    bracket.

``-reset``:
                    reset all attributes to the average level (tier 0).
                    If both this option and a list of attributes/tiers
                    are present, the unit attributes will be reset
                    and then the listed attributes will be modified.

Example::

    assign-attributes -reset -attributes [ STRENGTH 2 AGILITY -1 SPATIAL_SENSE -1 ]

This will reset all attributes to a neutral value and will set the following
values (if the currently selected unit is a dwarf):

 * Strength: a random value between 1750 and 1999 (tier 2);
 * Agility: a random value between 401 and 650 (tier -1);
 * Spatial sense: a random value between 1043 and 1292 (tier -1).

The final result will be: "She is very strong, but she is clumsy.
She has a questionable spatial sense."
]====]

local utils = require("utils")

local valid_args = utils.invert({
                                    'help',
                                    'unit',
                                    'attributes',
                                    'reset',
                                })


-- ----------------------------------------------- UTILITY FUNCTIONS ------------------------------------------------ --
function print_yellow(text)
    dfhack.color(COLOR_YELLOW)
    print(text)
    dfhack.color(-1)
end

-- ----------------------------------------------- GENERATE MEDIANS ------------------------------------------------- --
--- Retrieve and store the medians for the given race. Default race is DWARF.
local function generate_medians_table(race)
    assert(not race or type(race) == "number" or type(race) == "string")

    race = race or "DWARF"

    local medians = {
        PHYSICAL = {},
        MENTAL = {}
    }
    -- populate the medians table with the attributes and their standard "human" value
    do
        for _, physical_attribute in ipairs(df.physical_attribute_type) do
            medians.PHYSICAL[physical_attribute] = 1000
        end
        for _, mental_attribute in ipairs(df.mental_attribute_type) do
            medians.MENTAL[mental_attribute] = 1000
        end
    end
    -- now update those medians that are different from the standard median (i.e. the ones listed in the creature raws)
    do
        local creatures = df.global.world.raws.creatures.all
        local creature
        if type(race) == "number" then
            creature = creatures[race]
        else
            _, creature = utils.linear_index(creatures, race, "creature_id")
        end
        assert(creature ~= nil)
        for _, raw in ipairs(creature.raws) do
            if string.match(raw.value, "PHYS_ATT_RANGE") then
                local token_parts = raw.value:split(":")
                --     part 1         *2*    3   4   5   *6*   7    8    9
                -- [PHYS_ATT_RANGE:STRENGTH:450:950:1150:1250:1350:1550:2250]
                medians.PHYSICAL[token_parts[2]] = tonumber(token_parts[6])
            elseif string.match(raw.value, "MENT_ATT_RANGE") then
                local token_parts = raw.value:split(":")
                --     part 1         *2*    3   4   5   *6*   7    8    9
                -- [MENT_ATT_RANGE:PATIENCE:450:950:1150:1250:1350:1550:2250]
                medians.MENTAL[token_parts[2]] = tonumber(token_parts[6])
            end
        end
    end
    return medians
end

-- ----------------------------------------------- ASSIGN ATTRIBUTES ------------------------------------------------ --
--- Assign the given attributes to a unit, resetting all the other attributes if requested.
---   :attributes: nil, or a table. The fields have the attribute token as key and the tier as value.
---   :unit: a valid unit id, a df.unit object, or nil. If nil, the currently selected unit will be targeted.
---   :reset: boolean, or nil.
function assign(attributes, unit, reset)
    assert(not attributes or type(attributes) == "table")
    assert(not unit or type(unit) == "number" or df.unit:is_instance(unit))
    assert(not reset or type(reset) == "boolean")

    attributes = attributes or {}
    reset = reset or false

    if type(unit) == "number" then
        unit = df.unit.find(tonumber(unit)) --luacheck:retype
    else
        unit = unit or dfhack.gui.getSelectedUnit(true)
    end
    if not unit then
        qerror("No unit found.")
    end

    local medians = generate_medians_table(unit.race)

    --- Given an attribute and a tier, calculate a random value within the limits of the tier and return it.
    --- Tiers are normally 250 points apart, so they are 249 points wide, with the exception of tier 0 and tier 4:
    --- tier 0 minimum value is 249 points below the median, and its maximum value is 249 points above the median;
    --- tier 4 maximum value is theoretically 5000, but each unit has a maximum value equal to:
    ---                                   starting_value + max(starting_value, median)
    --- where starting_value is the value of the attribute before any training.
    --- The formula to calculate tier boundaries is:
    ---                                           median + (tier * 250).
    --- For tiers 1, 2, 3 and 4, the formula returns the minimum value of the tier; for tiers -1, -2, -3, -4, the
    --- formula returns the maximum value of the tier. Given that all tiers but tier 0 and tier 4 are 250 points
    --- apart, for tiers -4, -3, -2, -1, 1, 2 and 3 we can add (for positive tiers) or subtract (for negative tiers)
    --- up to 249 points to the formula result and still remain in the same tier. For tier 0, we can add or
    --- subtract 249 points to the median value. For tier 4, we could add points up to the maximum value, but we won't
    --- do it for simplicity's sake and we will treat tier 4 as any other tier.
    local function convert_tier_to_value(attribute, tier)
        assert(type(attribute) == "string")
        assert(type(tier) == "number" and tier >= -4 or tier <= 4)

        -- decide if we need to add or subtract
        local sign = 0
        if tier > 0 then
            sign = 1
        elseif tier < 0 then
            sign = -1
        else
            -- tier 0: decide randomly if we'll add or subtract
            local pos_or_neg = { 1, -1 }
            sign = pos_or_neg[math.random(2)]
        end
        local random_offset = math.random(0, 249) * sign
        local median = medians.PHYSICAL[attribute] or medians.MENTAL[attribute]
        local value = math.max(0, median + tier * 250 + random_offset)
        -- calculate the new max_value, using the new value as the starting value
        local max_value = value + math.max(value, median)
        return value, max_value
    end

    -- reset attributes around the median value
    if reset then
        for attribute_token, attribute_obj in pairs(unit.body.physical_attrs) do
            local v, max_v = convert_tier_to_value(attribute_token, 0)
            attribute_obj.value = v
            attribute_obj.max_value = max_v
        end
        for attribute_token, attribute_obj in pairs(unit.status.current_soul.mental_attrs) do
            local v, max_v = convert_tier_to_value(attribute_token, 0)
            attribute_obj.value = v
            attribute_obj.max_value = max_v
        end
    end

    -- assign new attributes
    for attribute, tier in pairs(attributes) do
        attribute = attribute:upper()

        if medians.PHYSICAL[attribute] then
            if tier >= -4 and tier <= 4 then
                local v, max_v = convert_tier_to_value(attribute, tier)
                unit.body.physical_attrs[attribute].value = v
                unit.body.physical_attrs[attribute].max_value = max_v
            else
                print_yellow("WARNING: tier out of range for attribute '" .. attribute .. "'. Skipping...")
            end
        elseif medians.MENTAL[attribute] then
            if tier >= -4 and tier <= 4 then -- code repetition, but I think the warning message it's clearer this way
                local v, max_v = convert_tier_to_value(attribute, tier)
                unit.status.current_soul.mental_attrs[attribute].value = v
                unit.status.current_soul.mental_attrs[attribute].max_value = max_v
            else
                print_yellow("WARNING: tier out of range for attribute '" .. attribute .. "'. Skipping...")
            end
        else
            print_yellow("WARNING: '" .. attribute .. "' is not a valid attribute. Skipping...")
        end
    end
end

-- ------------------------------------------------------ MAIN ------------------------------------------------------ --
local function main(...)
    local args = utils.processArgs({ ... }, valid_args)

    if args.help then
        print(help)
        return
    end

    local unit
    if args.unit then
        unit = tonumber(args.unit)
        if not unit then
            qerror("'" .. args.unit .. "' is not a valid unit ID.")
        end
    end

    local reset = false
    if args.reset then
        reset = true
    end

    -- parse attributes list
    local attributes = {}
    if args.attributes then
        local i = 1
        while i <= #args.attributes do
            local v = args.attributes[i]
            -- v can be an attribute name but it can also be a tier value, so we have to check
            if not tonumber(v) then
                -- assume it's a valid attribute name, for now
                local attribute_name = tostring(v):upper()
                -- then try to get the tier value
                local tier_str = args.attributes[i + 1]
                if not tier_str then
                    -- we reached the end of the attributes list
                    qerror("Missing tier value after '" .. v .. "'.")
                end
                local tier_int = tonumber(tier_str)
                if not tier_int then
                    qerror("'" .. tier_str .. "' is not a valid number.")
                end
                -- assume the tier value is in range, for now
                attributes[attribute_name] = tier_int
            end
            i = i + 1 -- skip next arg because we already consumed it
        end
    end

    assign(attributes, unit, reset)
end

if not dfhack_flags.module then
    main(...)
end
