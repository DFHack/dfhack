-- Change the beliefs of a unit.
--@ module = true

local help = [====[

assign-beliefs
==============
A script to change the beliefs (values) of a unit.

Beliefs are defined with the belief token and a number from -3 to 3,
which describes the different levels of belief strength, as explained here:
https://dwarffortresswiki.org/index.php/DF2014:Personality_trait#Beliefs

========  =========
Strength  Effect
========  =========
3         Highest
2         Very High
1         High
0         Neutral
-1        Low
-2        Very Low
-3        Lowest
========  =========

Resetting a belief means setting it to a level that does not trigger a
report in the "Thoughts and preferences" screen.

Usage:

``-help``:
                    print the help page.

``-unit <UNIT_ID>``:
                    set the target unit ID. If not present, the
                    currently selected unit will be the target.

``-beliefs [ <BELIEF> <LEVEL> <BELIEF> <LEVEL> <...> ]``:
                    the beliefs to modify and their levels. The
                    valid belief tokens can be found in the wiki page
                    linked above; level values range from -3 to 3.
                    There must be a space before and after each square
                    bracket.

``-reset``:
                    reset all beliefs to a neutral level. If the script is
                    called with both this option and a list of beliefs/levels,
                    first all the unit beliefs will be reset and then those
                    beliefs listed after ``-beliefs`` will be modified.

Example::

    assign-beliefs -reset -beliefs [ TRADITION 2 CRAFTSMANSHIP 3 POWER 0 CUNNING -1 ]

Resets all the unit beliefs, then sets the listed beliefs to the following
values:

* Tradition: a random value between 26 and 40 (level 2);
* Craftsmanship: a random value between 41 and 50 (level 3);
* Power: a random value between -10 and 10 (level 0);
* Cunning: a random value between -25 and -11 (level -1).

The final result (for a dwarf) will be: "She personally is a firm believer in
the value of tradition and sees guile and cunning as indirect and somewhat
worthless."

Note that the beliefs aligned with the cultural values of the unit have not
triggered a report.

]====]

local utils = require("utils")
local setneed = reqscript("modtools/set-need")

local valid_args = utils.invert({
                                    'help',
                                    'unit',
                                    'beliefs',
                                    'reset',
                                })

-- ----------------------------------------------- UTILITY FUNCTIONS ------------------------------------------------ --
local function print_yellow(text)
    dfhack.color(COLOR_YELLOW)
    print(text)
    dfhack.color(-1)
end

-- ------------------------------------------------- ASSIGN BELIEFS ------------------------------------------------- --
-- NOTE: in the game data, beliefs are called values.

--- Assign the given beliefs to a unit, clearing all the other beliefs if requested.
---   :beliefs: nil, or a table. The fields have the belief name as key and its level as value.
---   :unit: a valid unit id, a df.unit object, or nil. If nil, the currently selected unit will be targeted.
---   :reset: boolean, or nil.
function assign(beliefs, unit, reset)
    assert(not beliefs or type(beliefs) == "table")
    assert(not unit or type(unit) == "number" or df.unit:is_instance(unit))
    assert(not reset or type(reset) == "boolean")

    beliefs = beliefs or {}
    reset = reset or false

    if type(unit) == "number" then
        unit = df.unit.find(tonumber(unit)) --luacheck:retype
    end
    unit = unit or dfhack.gui.getSelectedUnit(true)
    if not unit then
        qerror("No unit found.")
    end

    --- Calculate a random value within the desired belief level
    local function calculate_random_belief_value(belief_level)
        local beliefs_levels = {
            -- {minimum level value, maximum level value}
            { -50, -41 }, -- -3: lowest
            { -40, -26 }, -- -2: very low
            { -25, -11 }, -- -1: low
            { -10, 10 }, -- 0: neutral
            { 11, 25 }, -- 1: high
            { 26, 40 }, -- 2: very high
            { 41, 51 } -- 3: highest
        }
        local lvl = beliefs_levels[belief_level + 4]
        return math.random(lvl[1], lvl[2])
    end

    -- clear beliefs
    if reset then
        unit.status.current_soul.personality.values = {}
    end

    --assign beliefs
    for belief, level in pairs(beliefs) do
        assert(type(level) == "number")
        belief = belief:upper()
        -- there's a typo in the game data
        if belief == "PERSEVERANCE" then
            belief = "PERSEVERENCE"
        end
        if df.value_type[belief] then
            if level >= -3 and level <= 3 then
                local belief_value = calculate_random_belief_value(level)
                utils.insert_or_update(unit.status.current_soul.personality.values,
                                       { new = true, type = df.value_type[belief], strength = belief_value },
                                       "type")
            else
                print_yellow("WARNING: level out of range for belief '" .. belief .. "'. Skipping...")
            end
        else
            print_yellow("WARNING: '" .. belief .. "' is not a valid belief token. Skipping...")
        end
    end
    setneed.rebuildNeeds(unit)
end

-- ------------------------------------------------------ MAIN ------------------------------------------------------ --
local function main(...)
    local args = utils.processArgs({...}, valid_args)

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

    -- parse beliefs list
    local beliefs = {}
    if args.beliefs then
        local i = 1
        while i <= #args.beliefs do
            local v = args.beliefs[i]
            -- v can be a belief name but it can also be a level value, so we have to check
            if not tonumber(v) then
                -- assume it's a valid belief name, for now
                local belief_name = tostring(v):upper()
                -- then try to get the level value
                local level_str = args.beliefs[i + 1]
                if not level_str then
                    -- we reached the end of the beliefs list
                    qerror("Missing level value after '" .. v .. "'.")
                end
                local level_int = tonumber(level_str)
                if not level_int then
                    qerror("'" .. level_str .. "' is not a valid number.")
                end
                -- assume the level value is in range, for now
                beliefs[belief_name] = level_int
            end
            i = i + 1 -- skip next arg because we already consumed it
        end
    end

    assign(beliefs, unit, reset)
end

if not dfhack_flags.module then
    main(...)
end
