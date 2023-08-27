-- Change the goals of a unit.
--@ module = true

local help = [====[

assign-goals
============
A script to change the goals (dreams) of a unit.

Goals are defined with the goal token and a true/false value
that describes whether or not the goal has been accomplished. Be
advised that this last feature has not been properly tested and
might be potentially destructive: I suggest leaving it at false.

For a list of possible goals:
https://dwarffortresswiki.org/index.php/DF2014:Personality_trait#Goals

Bear in mind that nothing will stop you from assigning zero or
more than one goal, but it's not clear how it will affect the game.

Usage:

``-help``:
                    print the help page.

``-unit <UNIT_ID>``:
                    set the target unit ID. If not present, the
                    currently selected unit will be the target.

``-goals [ <GOAL> <REALIZED_FLAG> <GOAL> <REALIZED_FLAG> <...> ]``:
                    the goals to modify/add and whether they have
                    been realized or not. The valid goal tokens
                    can be found in the wiki page linked above.
                    The flag must be a true/false value.
                    There must be a space before and after each square
                    bracket.

``-reset``:
                    clear all goals. If the script is called with
                    both this option and a list of goals, first all
                    the unit goals will be erased and then those
                    goals listed after ``-goals`` will be added.

Example::

    assign-goals -reset -goals [ MASTER_A_SKILL false ]

Clears all the unit goals, then sets the "master a skill" goal. The final result
will be: "dreams of mastering a skill".

]====]

local utils = require("utils")

local valid_args = utils.invert({
                                    'help',
                                    'unit',
                                    'goals',
                                    'reset',
                                })

-- ----------------------------------------------- UTILITY FUNCTIONS ------------------------------------------------ --
local function print_yellow(text)
    dfhack.color(COLOR_YELLOW)
    print(text)
    dfhack.color(-1)
end

-- ------------------------------------------------- ASSIGN GOALS ------------------------------------------------- --
-- NOTE: in the game data, goals are called both dreams and goals.

--- Assign the given goals to a unit, clearing all the other goals if requested.
---   :goals: nil, or a table. The fields have the goal name as key and true/false as value.
---   :unit: a valid unit id, a df.unit object, or nil. If nil, the currently selected unit will be targeted.
---   :reset: boolean, or nil.
function assign(goals, unit, reset)
    assert(not goals or type(goals) == "table")
    assert(not unit or type(unit) == "number" or df.unit:is_instance(unit))
    assert(not reset or type(reset) == "boolean")

    goals = goals or {}
    reset = reset or false

    if type(unit) == "number" then
        unit = df.unit.find(tonumber(unit)) --luacheck:retype
    end
    unit = unit or dfhack.gui.getSelectedUnit(true)
    if not unit then
        qerror("No unit found.")
    end

    -- erase goals
    if reset then
        unit.status.current_soul.personality.dreams = {}
    end

    --assign goals
    for goal, realized in pairs(goals) do
        assert(type(realized) == "boolean")
        goal = goal:upper()
        if df.goal_type[goal] then
            utils.insert_or_update(unit.status.current_soul.personality.dreams,
                                   { new = true, type = df.goal_type[goal], flags = {accomplished = realized} },
                                   "type")
        else
            print_yellow("WARNING: '" .. goal .. "' is not a valid goal token. Skipping...")
        end
    end
end

-- ------------------------------------------------------ MAIN ------------------------------------------------------ --
local function main(...)
    local function is_flag(string)
        return string:upper() == "TRUE" or string:upper() == "FALSE"
    end

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

    -- parse goals list
    local goals = {}
    if args.goals then
        local i = 1
        while i <= #args.goals do
            local v = args.goals[i]
            -- v can be a goal name but it can also be a boolean flag, so we have to check
            if not is_flag(v) then
                -- assume it's a valid goal name, for now
                local goal_name = tostring(v):upper()
                -- then try to get the boolean flag
                local flag_str = args.goals[i + 1]
                if not flag_str then
                    -- we reached the end of the goals list
                    qerror("Missing realized flag after '" .. v .. "'.")
                end
                local flag_bool
                if flag_str:upper() == "TRUE" then
                    flag_bool = true
                elseif flag_str:upper() == "FALSE" then
                    flag_bool = false
                end
                if flag_bool == nil then
                    qerror("'" .. flag_str .. "' is not a true or false value.")
                end
                goals[goal_name] = flag_bool
            end
            i = i + 1 -- skip next arg because we already consumed it
        end
    end

    assign(goals, unit, reset)
end

if not dfhack_flags.module then
    main(...)
end
