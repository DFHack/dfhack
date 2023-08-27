-- Set a dwarf's characteristics according to a predefined profile
--@ module = true

local help = [====[

assign-profile
==============
A script to change the characteristics of a unit
according to a profile loaded from a json file.

A profile can describe which attributes, skills, preferences, beliefs,
goals and facets a unit must have. The script relies on the presence
of the other ``assign-...`` modules in this collection: please refer
to the other modules documentation for more specific information.

For information about the json schema, please see the
the "/hack/scripts/dwarf_profiles.json" file.

Usage:

``-help``:
                    print the help page.

``-unit <UNIT_ID>``:
                    the target unit ID. If not present, the
                    target will be the currently selected unit.

``-file <filename>``:
                    the json file containing the profile to apply.
                    It's a relative path, starting from the DF
                    root directory and ending at the json file.
                    It must begin with a slash. Default value:
                    "/hack/scripts/dwarf_profiles.json".

``-profile <profile>``:
                    the profile to apply. It's the name of
                    the profile as stated in the json file.

``-reset [ <list of characteristics> ]``:
                    the characteristics to be reset/cleared. If not present,
                    it will not clear or reset any characteristic, and it will
                    simply add what is described in the profile. If it's a
                    valid list of characteristic, those characteristics will
                    be reset, and then what is described in the profile
                    will be applied. If set to ``PROFILE``, it will reset
                    only the characteristics directly modified by the profile
                    (and then the new values described will be applied).
                    If set to ``ALL``, it will reset EVERY characteristic and
                    then it will apply the profile.
                    Accepted values:
                    ``ALL``, ``PROFILE``, ``ATTRIBUTES``, ``SKILLS``,
                    ``PREFERENCES``, ``BELIEFS``, ``GOALS``, ``FACETS``.
                    There must be a space before and after each square
                    bracket. If only one value is provided, the square brackets
                    can be omitted.

Examples:

* Loads and applies the profile called "DOCTOR" in the default json file,
  resetting/clearing all the characteristics changed by the profile, leaving
  the others unchanged, and then applies the new values::

    assign-profile -profile DOCTOR -reset PROFILE

* Loads and applies the profile called "ARCHER" in the provided (fictional) json,
  keeping all the old characteristics but the attributes and the skills, which
  will be reset (and then, if the profile provides some attributes or skills values,
  those new values will be applied)::

    assign-profile -file /hack/scripts/military_profiles.json -profile ARCHER -reset [ ATTRIBUTES SKILLS ]

]====]

local json = require "json"
local utils = require("utils")

local valid_args = utils.invert({
                                    "help",
                                    "unit",
                                    "file",
                                    "profile",
                                    "reset",
                                })

-- add a script here to include it in the profile. The key must be the same as written in the json.
local scripts = {
    ATTRIBUTES = reqscript("assign-attributes"),
    SKILLS = reqscript("assign-skills"),
    PREFERENCES = reqscript("assign-preferences"),
    BELIEFS = reqscript("assign-beliefs"),
    GOALS = reqscript("assign-goals"),
    FACETS = reqscript("assign-facets"),
}

local default_filename = "/hack/scripts/dwarf_profiles.json"

-- ------------------------------------------------- APPLY PROFILE -------------------------------------------------- --
--- Apply the given profile to a unit, erasing or resetting the unit characteristics as requested.
---   :profile: a table. Each field has a characteristic name as key, and a table suitable to be passed as
---             an argument to the ``assign`` function of the script related to the characteristic.
---             See the called script documentation for more details.
---   :unit: a valid unit id, a df.unit object, or nil. The called script has the responsibility to validate this value.
---   :reset: nil, or a list of values. See this script documentation for accepted values.
--luacheck: in=string[],df.unit,bool[]
function apply_profile(profile, unit, reset)
    assert(type(profile) == "table")
    assert(not unit or type(unit) == "number" or df.unit:is_instance(unit))
    assert(not reset or type(reset) == "table")

    reset = reset or {}

    local function apply(characteristic_name, script)
        local reset_flag = reset.ALL ~= nil or
                reset[characteristic_name] ~= nil or
                (reset.PROFILE and profile[characteristic_name] ~= nil)
        script.assign(profile[characteristic_name], unit, reset_flag)
    end

    for characteristic_name, script in pairs(scripts) do
        apply(characteristic_name, script)
    end
end

-- --------------------------------------------------- LOAD PROFILE ------------------------------------------------- --
--- Load the given profile, searching it inside the given JSON file (if not nil) or inside the default JSON file.
--- The filename must begin with a slash and must be a relative path starting from the root DF directory and ending
--- with the desired filename.
--- Return the parsed profile as a table.
--luacheck: in=string,string
function load_profile(profile_name, filename)
    assert(profile_name ~= nil)

    local json_file = string.format("%s%s", dfhack.getDFPath(), filename or default_filename)
    local profiles = {} --as:string[][]
    if dfhack.filesystem.isfile(json_file) then
        profiles = json.decode_file(json_file)
    else
        qerror(string.format("File '%s' not found.", json_file))
    end
    if profiles[profile_name] then
        return profiles[profile_name]
    else
        qerror(string.format("Profile '%s' not found", profile_name))
    end
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

    local filename = args.file

    local profile_name
    if args.profile then
        profile_name = args.profile
    else
        qerror("Missing profile name.")
    end

    local reset
    if type(args.reset) == "string" then
        reset = {}
        reset[args.reset] = 1
    elseif type(args.reset) == "table" then
        reset = utils.invert(args.reset)
    end

    local profile = load_profile(profile_name, filename)
    apply_profile(profile, unit_id, reset)
end

if not dfhack_flags.module then
    main(...)
end
