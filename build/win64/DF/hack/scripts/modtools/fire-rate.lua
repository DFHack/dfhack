-- Allows altering the fire rates of ranged weapons
--@ module = true

--[[ TODO / WISHLIST:
- Add in an alternative mode that behaves more like vanilla
- Account for units being burdened (apparently that's a thing that affects
  recovery rate according to my tests with throwing)
- Allow throw + item combinations so the fire rate of throwing weapons can
  be modified
]]

local help = [====[

modtools/fire-rate
==================

Allows altering the fire rates of ranged weapons. Each are defined on a per-item
basis. As this is done in an on-world basis, commands for this should be placed
in an ``onLoad*.init``. This also technically serves as a patch to any of the
weapons targeted in adventure mode, reducing the times down to their intended
speeds (the game applies an additional hardcoded recovery time to any ranged
attack you make in adventure mode).

Once run, all ranged attacks will use this script's systems for calculating
recovery speeds, even for items that haven't directly been modified using this
script's commands. One minor side effect is that it can't account for
interactions with the ``FREE_ACTION`` token; interactions with that tag which
launch projectiles will be subject to recovery times (though there aren't any
interaction in vanilla where this would happen, as far as I know).

Requires a Target and any number of Modifiers.

Targets:

:-item <item token>:
  The full token of the item to modify.
  Example: ``WEAPON:ITEM_WEAPON_BOW``
:-throw:
  Modify the fire rate for throwing.
  This is specifically for thrown attacks without a weapon - if you have a
  weapon that uses ``THROW`` as its skill, you need to use the ``-item``
  argument for it.

Modifiers:

:-material <material token>:
  Apply only to items made of the given material token. With the ``-item``
  argument, this will apply to the material that the weapon is made of, whereas
  with the ``-throw`` argument this will apply to the material being thrown (or
  fired, in the case of interactions). This is optional.
  Format examples: "CREATURE:COW:MILK", "PLANT:MUSHROOM_HELMET_PLUMP:DRINK",
  "INORGANIC:GOLD", "VOMIT"
:-fortBase <integer> -advBase <integer>:
  Set the base fire rate for the weapon in ticks to use in the respective mode
  (fortress/adventure). Means one shot per x ticks. Defaults to the game default
  of 80.
:-fortSkillFactor <float> -advSkillFactor <float>:
  Multiplier that modifies how effective a user's skill is at improving the fire
  rate in the respective modes. In basic mode, recovery time is reduced by this
  value * user's skill ticks. Defaults to 2.7. With that value and default
  settings, it will make a Legendary shooter fire at the speed cap.
:-fortCap <integer> -advCap <integer>:
  Sets a cap on the fastest fire rate that can be achieved in their respective
  mode. Due to game limitations, the cap can't be less than 10 in adventure
  mode. Defaults to half of the base fire rate defined by the ``-fort`` or
  ``-adv`` arguments.

Other:

:-mode <"basic" | "vanilla">:
  Sets what method is used to determine how skill affects fire rates. This is
  applied globally, rather than on a per-item basis. Basic uses a simplified
  method for working out fire rates - each point in a skill reduces the fire
  cooldown by a consistent, fixed amount. This method is the default.
  Vanilla mode attempts to replicate behaviour for fire rates - skill rolls
  determine which of 6 fixed increments of speeds is used, with a unit's skill
  affecting the range and averages. **NOT YET IMPLEMENTED!**

]====]

--[[ Here's my current research on how I think this stuff works (v 47.05)
Fort Mode uses a unit's think_counter. After firing a shot, it's set to 80 (not
accounting for skill stuff). This delays them from acting again until then.

If you make it so onProjItemCheckMovement sets their think_counter to 0 whenever
they fire something, they can fire 1 shot every tick.

Adventure mode uses think_counter + a fixed delay. After firing a shot, the game
advances by the fixed delay, then starts decrementing the think_counter. The
fixed delay is either 10 or 9 from testing. It probably exists to prevent units
walking into their own shots.

Example:
- Throw - think_counter 1, total time passed 10
- Spit - think_counter 1, total time passed 10

When in tactical mode, the fixed delay always occurs after making a ranged
attack before switching control to the next character.

Then, the think counter delay prevents switching back into that character until
it's passed.

Some figures might be off by 1 because of delays between firing and when
onProjItemCheckMovement triggers.
]]

---------------------------------------------------------------------
local utils = require("utils")
local eventful = require("plugins.eventful")

local validArgs = utils.invert({
  "help",
  "item",
  "throw",
  "material",
  "fortBase",
  "advBase",
  "advSkillFactor",
  "fortSkillFactor",
  "fortCap",
  "advCap",
  "mode",
})

-- The (hopefully unique) separator used to split a weapon token and material
-- token when they're combined to make a key
local separator = "|||"

-- The (hopefully unique) key used as a substitute for a weapon token when the
-- attack is coming from a throw.
local throw_key = "throw"

-- The hardcoded delay (in ticks) that the game adds when performing any ranged
-- attack in adventure mode.
local adventure_ranged_delay = 9
---------------------------------------------------------------------
weapon_data = weapon_data or {}
config = config or {
  skill_mode = "basic",
}
---------------------------------------------------------------------

function register_weapon(weapon_token, material_token, data)
  local key = weapon_token

  if ( material_token ~= nil ) then
    key = key ..  separator .. material_token
  end

  if ( data == nil ) then
    data = {}
  end

  local new_entry = {
    fort_rate = data.fort_rate or 80,
    adv_rate = data.adv_rate or 80,
    fort_skill_factor = data.fort_skill_factor or 2.7,
    adv_skill_factor = data.adv_skill_factor or 2.7,
  }

  -- Determine caps
  if ( data.fort_cap ) then
    new_entry.fort_cap = data.fort_cap
  else -- default to half main rate
    -- Half the standard rate, then round to nearest number
    new_entry.fort_cap = math.floor(new_entry.fort_rate / 2 + 0.5)
  end

  -- Accounting for the adventure hard-coded cap won't be done at this stage
  if ( data.adv_cap ) then
    new_entry.adv_cap = data.adv_cap
  else
    new_entry.adv_cap = math.floor(new_entry.adv_rate / 2 + 0.5)
  end

  weapon_data[key] = new_entry
  return new_entry
end

-- Get the data for the given weapon (or create it, if missing)
-- When searching, it'll check using the following priority:
-- > Weapon + material combination
-- > Weapon by itself
-- If neither is found, it will create an entry for that particular
-- weapon (NOT weapon + material combo) so that all ranged weapons
-- are consistently using this new system
function get_weapon_data(weapon_token, material_token)
  return weapon_data[weapon_token..separator..(material_token or '')] or
        weapon_data[weapon_token] or
        register_weapon(weapon_token)
end

function get_weapon_fire_rate(weapon_token, material_token, game_mode)
  local weapon_data = get_weapon_data(weapon_token, material_token)

  if ( game_mode == nil or game_mode == df.game_mode.DWARF ) then
    return weapon_data.fort_rate
  else -- assume df.game_mode.ADVENTURE
    return weapon_data.adv_rate
  end
end

function get_weapon_skill_factor(weapon_token, material_token, game_mode)
  local weapon_data = get_weapon_data(weapon_token, material_token)

  if ( game_mode == nil or game_mode == df.game_mode.DWARF ) then
    return weapon_data.fort_skill_factor
  else -- assume ADVENTURE
    return weapon_data.adv_skill_factor
  end
end


function get_weapon_fire_rate_cap(weapon_token, material_token, game_mode)
  local weapon_data = get_weapon_data(weapon_token, material_token)

  if ( game_mode == nil or game_mode == df.game_mode.DWARF ) then
    return weapon_data.fort_cap
  else -- assume ADVENTURE
    return weapon_data.adv_cap
  end
end

function set_config_skill_mode(value)
  if ( value ~= "basic" and value ~= "vanilla" ) then
    config.skill_mode = "basic"
    return
  end

  config.skill_mode = value
end

function get_config_skill_mode()
  return config.skill_mode
end

---------------------------------------------------------------------
-- Performs a skill roll based on configured mode
function get_fire_rate_for_unit(unit, weapon_token, material_token)
  local weapon_ranged_skill = get_weapon_skill_ranged(weapon_token)
  local unit_skill_level = dfhack.units.getNominalSkill(
                                                unit, weapon_ranged_skill, true)

  local current_game_mode

  -- Technically don't have to do this filtering, but it's good to do anyway
  if ( dfhack.world.isFortressMode() ) then
    current_game_mode = df.game_mode.DWARF
  elseif ( dfhack.world.isAdventureMode() ) then
    current_game_mode = df.game_mode.ADVENTURE
  elseif ( dfhack.world.isArena() ) then
    current_game_mode = df.global.gamemode
  end

  local fire_rate = get_weapon_fire_rate(weapon_token, material_token,
                                         current_game_mode)
  local speed_cap = get_weapon_fire_rate_cap(weapon_token, material_token,
                                             current_game_mode)
  local skill_factor = get_weapon_skill_factor(weapon_token, material_token,
                                               current_game_mode)

  -- Filter method based on current mode
  -- (TODO: Except not actually because only one is available)
  if ( get_config_skill_mode() == "basic" or true ) then
    local uncapped_rate = math.floor(
                            fire_rate - (unit_skill_level * skill_factor) + 0.5)

    return math.max(uncapped_rate, speed_cap)
  end
end

-- First rolls a skill check to determine how long it'll take for the unit to
-- recover, then updates the unit's recovery time accordingly
function do_shot_cooldown(unit, weapon_token, material_token)
  local fire_rate = get_fire_rate_for_unit(unit, weapon_token, material_token)

  apply_shot_cooldown(unit, fire_rate)
end

-- Causes the unit to have to wait the given length of time (in ticks) before
-- they can act again
-- This will account for the hard-coded delay present in adventure actions
function apply_shot_cooldown(unit, recovery_time)
  local time_to_use = recovery_time

  -- Account for the hard-coded recovery time added for adventure mode actions
  -- performed by the player (AI aren't subject to the delay)
  if ( df.global.gamemode == df.game_mode.ADVENTURE
            and unit.id == get_adventurer_unit().id ) then
    -- The hardcoded delay is still active even in tactical mode, so no need to
    -- try and account for it
    time_to_use = math.max(recovery_time - adventure_ranged_delay, 0)
  end

  unit.counters.think_counter = time_to_use
end

---------------------------------------------------------------------
function get_weapon_skill_ranged(weapon_token)
  if ( weapon_token == throw_key ) then
    return df.job_skill.THROW
  end

  local item_def = get_item_def_by_token(weapon_token)
  -- Only tools and weapons have a `skill_ranged` value
  if ( df.itemdef_toolst:is_instance(item_def)
            or df.itemdef_weaponst:is_instance(item_def) ) then
    return item_def.skill_ranged
  else
    return df.job_skill.NONE -- NONE, or default to throw?
  end
end

-- Returns the item def of the provided item
-- (will technically clash if two items in different categories use the same
-- token, but I don't know if that's allowed anyway)
function get_item_def_by_token(item_token)
  local subtype_token = utils.split_string(item_token, ":")[2]

  for _, item_def in pairs(df.global.world.raws.itemdefs.all) do
    if ( item_def.id == subtype_token ) then
      return item_def
    end
  end
end

-- Returns the currently active adventurer
function get_adventurer_unit()
  local nemesis = df.nemesis_record.find(df.global.adventure.player_id)
  local unit = df.unit.find(nemesis.unit_id)

  return unit
end

---------------------------------------------------------------------
initialized = initialized or false
handled_projectiles = {}

function init()
  weapon_data = {}
  handled_projectiles = {}
  config = {
    skill_mode = "basic",
  }

  -- Register a default value for throwing
  register_weapon(throw_key, nil, {
    adv_rate = 10, -- Default game speed
    adv_cap = 10, -- Throwing skill basically does nothing
  })

  eventful.onProjItemCheckMovement["fire-rate"] = on_projectile_move

  initialized = true
end

function reset()
  weapon_data = nil
  handled_projectiles = nil
  config = nil

  eventful.onProjItemCheckMovement["fire-rate"] = nil

  initialized = false
end

function on_projectile_move(projectile)
  if ( handled_projectiles[projectile.id] == nil ) then
    -- Only handle the projectile once
    handled_projectiles[projectile.id] = true

    -- only handle item projectiles
    if ( not df.is_instance(df.proj_itemst, projectile) ) then return end

    -- Get the shooter
    local shooter
    if ( projectile.firer ~= nil ) then
      shooter = projectile.firer
    else -- Don't need to bother modifying recovery time if there's no shooter!
      return
    end

    -- Determine the weapon
    local weapon_token
    local weapon_item
    local was_thrown = false
    if ( projectile.bow_id ~= -1 ) then -- The projectile was fired
      weapon_item = df.item.find(projectile.bow_id)
      weapon_token = df.item_type[weapon_item:getType()] .. ":"
                        .. weapon_item.subtype.id

      was_thrown = false
    else -- Projectile was thrown
      weapon_token = throw_key
      was_thrown = true
    end

    -- Determine the material (note that where we get the material from varies
    -- based on if the item was thrown or not)
    local material_token
    local matinfo
    if ( not was_thrown ) then -- Want to get the weapon's material
      matinfo = dfhack.matinfo.decode(weapon_item)
      material_token = matinfo:getToken()
    else -- Want to get the projectile's material
      matinfo = dfhack.matinfo.decode(projectile.item)
      material_token = matinfo:getToken()
    end

    do_shot_cooldown(shooter, weapon_token, material_token)
  end
end

dfhack.onStateChange["fire-rate"] = function(code)
  -- Wipe / reset data on whenever loaded state changes
  if code == SC_WORLD_UNLOADED then
    reset()
  elseif code == SC_WORLD_LOADED then
-- Disable until this script is updated for v50
--    if ( not initialized ) then
--      init()
--    end
  end
end

function main(...)
  local args = utils.processArgs({...}, validArgs)

  if args.help then
    print(help)
    return
  end

  -- Ensure world is actually loaded
  if not dfhack.isWorldLoaded() then
    qerror("The world needs to be loaded to use this.")
  end

  -- Initialize if not already
  if not initialized then init() end

  -- Handle arguments...
  if (args.mode) then
    if ( args.mode ~= "basic" and args.mode ~= "vanilla" ) then
      qerror("Please provide a valid mode: basic or vanilla.")
    end

    set_config_skill_mode(args.mode)

    -- If this is the only argument given, quit out now
    if ( args.item == nil and args.throw == nil ) then
      return
    end
  end

  if ( args.item == nil and args.throw == nil ) then
    qerror("Please provide a valid target.")
  end

  local weapon_token
  local material_token
  local data = {}

  if ( args.item ) then
    weapon_token = args.item
  else -- Assume throw
    weapon_token = throw_key
  end

  if ( args.material ) then
    -- Check material is valid
    if ( dfhack.matinfo.find(args.material) == nil ) then
      qerror("Couldn't find material: " .. args.material)
    end

    material_token = args.material
  end

  -- Bases
  if ( args.fortBase and tonumber(args.fortBase) ~= nil ) then
    data.fort_rate = math.floor(tonumber(args.fortBase))
  end
  if ( args.advBase and tonumber(args.advBase) ~= nil ) then
    data.adv_rate = math.floor(tonumber(args.advBase))
  end
  -- Skil factos
  if ( args.fortSkillFactor and tonumber(args.fortSkillFactor) ~= nil ) then
    data.fort_skill_factor = tonumber(args.fortSkillFactor)
  end
  if ( args.advSkillFactor and tonumber(args.advSkillFactor) ~= nil ) then
    data.adv_skill_factor = tonumber(args.advSkillFactor)
  end
  -- Caps
  if ( args.fortCap and tonumber(args.fortCap) ~= nil ) then
    data.fort_cap = math.floor(tonumber(args.fortCap))
  end
  if ( args.advCap and tonumber(args.advCap) ~= nil ) then
    data.adv_cap = math.floor(tonumber(args.advCap))
  end

  register_weapon(weapon_token, material_token, data)
end

if not dfhack_flags.module then
  main(...)
end
