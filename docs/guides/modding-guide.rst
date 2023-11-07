.. _modding-guide:

DFHack modding guide
====================

.. highlight:: lua

What is the difference between a script and a mod?
--------------------------------------------------

Well, sometimes there is no difference. A mod is anything you add to the game,
which can be graphics overrides, content in the raws, DFHack scripts, any, or
all. There are already resources out there for
`raws modding <https://dwarffortresswiki.org/index.php/Modding>`__, so this
guide will focus more on scripts, both standalone and as an extension to
raws-based mods.

A DFHack script is a Lua file that can be run as a command in
DFHack. Scripts can do pretty much anything, from displaying information to
enforcing new game mechanics. If you don't already know Lua, there's a great
primer at `lua.org <https://www.lua.org/pil/contents.html>`__.

Why not just mod the raws?
--------------------------

It depends on what you want to do. Some mods *are* better to do in just the
raws. You don't need DFHack to add a new race or modify attributes. However,
DFHack scripts can do many things that you just can't do in the raws, like make
a creature that trails smoke or launch a unit into the air when they are hit
with a certain type of projectile. Some things *could* be done in the raws, but
a script is better (e.g. easier to maintain, easier to extend, and/or not prone
to side-effects). A great example is adding a syndrome when a reaction
is performed. If done in the raws, you have to create an exploding boulder as
an intermediary to apply the syndrome. DFHack scripts can add the syndrome
directly and with much more flexibility. In the end, complex mods will likely
require a mix of raw modding and DFHack scripting.

The structure of a mod
----------------------

In the example below, we'll use a mod name of ``example-mod``. I'm sure your
mods will have more creative names! Mods have a basic structure that looks like
this::

    info.txt
    graphics/...
    objects/...
    blueprints/...
    scripts_modactive/example-mod.lua
    scripts_modactive/internal/example-mod/...
    scripts_modinstalled/...
    README.md (optional)

Let's go through that line by line.

- The :file:`info.txt` file contains metadata about your mod that DF will
    display in-game. You can read more about this file in the
    `Official DF Modding Guide <https://bay12games.com/dwarves/modding_guide.html>`__.
- Modifications to the game raws (potentially with custom raw tokens) go in
    the :file:`graphics/` and :file:`objects/` folders. You can read more about
    the files that go in these directories on the :wiki:`Modding` wiki page.
- Any `quickfort` blueprints included with your mod go in the
    :file:`blueprints` folder. Note that your mod can *just* be blueprints and
    nothing else if you like.
- A control script in :file:`scripts_modactive/` directory that handles
    system-level event hooks (e.g. reloading state when a world is loaded),
    registering `overlays <overlay-dev-guide>`, and
    `enabling/disabling <script-enable-api>` your mod. You can put other
    scripts in this directory as well if you want them to appear as runnable
    DFHack commands when your mod is active for the current world. Lua modules
    that your main scripts use, but which don't need to be directly runnable by
    the player, should go in a subdirectory under
    :file:`scripts_modactive/internal/` so they don't show up in the DFHack
    `launcher <gui/launcher>` command autocomplete lists.
- Scripts that you want to be available before a world is loaded (i.e. on the
    DF title screen) or that you want to be runnable in any world, regardless
    of whether your mod is active, should go in the
    :file:`scripts_modinstalled/` folder. You can also have an :file:`internal/`
    subfolder in here for private modules if you like.
- Finally, a :file:`README.md` file that has more information about your mod.
    If you develop your mod using version control (recommended!), that
    :file:`README.md` file can also serve as your git repository documentation.

These files end up in a subdirectory under :file:`mods/` when players copy them
in or install them from the
`Steam Workshop <https://steamcommunity.com/app/975370/workshop/>`__, and in
:file:`data/installed_mods/` when the mod is selected as "active" for the first
time.

What if I just want to distribute a simple script?
--------------------------------------------------

If your mod is just a script with no raws modifications, things get a bit
simpler. All you need is::

    info.txt
    scripts_modinstalled/yourscript.lua
    README.md (optional)

Adding your script to the :file:`scripts_modinstalled/` folder will allow
DFHack to find it and add your mod to the `script-paths`. Your script will be
runnable from the title screen and in any loaded world, regardless of whether
your mod is explicitly "active".

A mod-maker's development environment
-------------------------------------

Create a folder for development somewhere outside your Dwarf Fortress
installation directory (e.g. ``/path/to/mymods/``). If you work on multiple
mods, you might want to make a subdirectory for each mod.

If you have changes to the raws, you'll have to copy them into DF's
``data/installed_mods/`` folder to have them take effect, but you can set
things up so that scripts are run directly from your dev directory. This way,
you can edit your scripts and have the changes available in the game
immediately: no copying, no restarting.

How does this magic work? Just add a line like this to your
``dfhack-config/script-paths.txt`` file::

    +/path/to/mymods/example-mod/scripts_modinstalled

Then that directory will be searched when you run DFHack commands from inside
the game. The ``+`` at the front of the path means to search that directory
first, before any other script directory (like :file:`hack/scripts` or other
versions of your mod in the DF mod folders).

The structure of the game
-------------------------

"The game" is in the global variable `df <lua-df>`. Most of the information
relevant to a script is found in ``df.global.world``, which contains things
like the list of all items, whether to reindex pathfinding, et cetera. Also
relevant to us are the various data types found in the game, e.g.
``df.pronoun_type`` which we will be using in this guide. We'll explore more of
the game structures below.

Your first script
-----------------

So! It's time to write your first script. This section will walk you through how
to make a script that will get the pronoun type of the currently selected unit.

First line, we get the unit::

    local unit = dfhack.gui.getSelectedUnit()

If no unit is selected, ``unit`` will be ``nil`` and an error message will be
printed (which can be silenced by passing ``true`` to ``getSelectedUnit``).

If ``unit`` is ``nil``, we don't want the script to run anymore::

    if not unit then
        return
    end

Now, the field ``sex`` in a unit is an integer, but each integer corresponds to
a string value ("it", "she", or "he"). We get this value by indexing the
bidirectional map ``df.pronoun_type``. Indexing the other way, with one of the
strings, will yield its corresponding number. So::

    local pronounTypeString = df.pronoun_type[unit.sex]
    print(pronounTypeString)

Simple. Save this as a Lua file in your own scripts directory and run it from
`gui/launcher` when a unit is selected in the Dwarf Fortress UI.

Exploring DF state
------------------

So how could you have known about the field and type we just used? Well, there
are two main tools for discovering the various fields in the game's data
structures. The first is the ``df-structures``
`repository <https://github.com/DFHack/df-structures>`__ that contains XML files
describing the layouts of the game's structures. These are complete, but
difficult to read (for a human). The second option is the `gui/gm-editor`
interface, an interactive data explorer. You can run the script while objects
like units are selected to view the data within them. Press :kbd:`?` while the
script is active to view help.

Familiarising yourself with the many structs of the game will help with ideas
immensely, and you can always ask for help in the `right places <support>`.

Reacting to events
------------------

The common method for injecting new behaviour into the game is to define a
callback function and get it called when something interesting happens. DFHack
provides two libraries for this, ``repeat-util`` and `eventful <eventful-api>`.
``repeat-util`` is used to run a function once per a configurable number of
frames (paused or unpaused), ticks (unpaused), in-game days, months, or years.
If you need to be aware the instant something happens, you'll need to run a
check once a tick. Be careful not to do this gratuitously, though, since
running callbacks too often can slow down the game!

``eventful``, on the other hand, is much more performance-friendly since it will
only call your callback when a relevant event happens, like a reaction or job
being completed or a projectile moving.

To get something to run once per tick, we can call
``repeat-util.scheduleEvery()``. First, we load the module::

    local repeatUtil = require('repeat-util')

Both ``repeat-util`` and ``eventful`` require keys for registered callbacks. You
should use something unique, like your mod name::

    local modId = "callback-example-mod"

Then, we pass the key, amount of time units between function calls, what the
time units are, and finally the callback function itself::

    repeatUtil.scheduleEvery(modId, 1, "ticks", function()
        -- Do something like iterating over all active units and
        -- check for something interesting
        for _, unit in ipairs(df.global.world.units.active) do
            ...
        end
    end)

``eventful`` is slightly more involved. First get the module::

    local eventful = require('plugins.eventful')

``eventful`` contains a table for each event which you populate with functions.
Each function in the table is then called with the appropriate arguments when
the event occurs. So, for example, to print the position of a moving (item)
projectile::

    eventful.onProjItemCheckMovement[modId] = function(projectile)
        print(projectile.cur_pos.x, projectile.cur_pos.y,
              projectile.cur_pos.z)
    end

Check out the `full list of supported events <eventful-api>` to see what else
you can react to with ``eventful``.

Now, you may have noticed that you won't be able to register multiple callbacks
with a single key named after your mod. You can, of course, call all the
functions you want from a single registered callback. Alternately, you can
create multiple callbacks using different keys, using your mod ID as a key name
prefix. If you do register multiple callbacks, though, there are no guarantees
about the call order.

Custom raw tokens
-----------------

.. highlight:: none

In this section, we are going to use `custom raw tokens <custom-raw-tokens>`
applied to a reaction to transfer the material of a reagent to a product as a
handle improvement (like on artifact buckets), and then we are going to see how
you could make boots that make units go faster when worn.

First, let's define a custom crossbow with its own custom reaction. The
crossbow::

    [ITEM_WEAPON:ITEM_WEAPON_CROSSBOW_SIEGE]
        [NAME:crossbow:crossbows]
        [SIZE:600]
        [SKILL:HAMMER]
        [RANGED:CROSSBOW:BOLT]
        [SHOOT_FORCE:4000]
        [SHOOT_MAXVEL:800]
        [TWO_HANDED:0]
        [MINIMUM_SIZE:17500]
        [MATERIAL_SIZE:4]
        [ATTACK:BLUNT:10000:4000:bash:bashes:NO_SUB:1250]
            [ATTACK_PREPARE_AND_RECOVER:3:3]
        [SIEGE_CROSSBOW_MOD_FIRE_RATE_MULTIPLIER:2] custom token (you'll see)

The reaction to make it (you would add the reaction and not the weapon to an
entity raw)::

    [REACTION:MAKE_SIEGE_CROSSBOW]
        [NAME:make siege crossbow]
        [BUILDING:BOWYER:NONE]
        [SKILL:BOWYER]
        [REAGENT:mechanism 1:2:TRAPPARTS:NONE:NONE:NONE]
        [REAGENT:bar:150:BAR:NONE:NONE:NONE]
            [METAL_ITEM_MATERIAL]
        [REAGENT:handle 1:1:BLOCKS:NONE:NONE:NONE] wooden handles
            [ANY_PLANT_MATERIAL]
        [REAGENT:handle 2:1:BLOCKS:NONE:NONE:NONE]
            [ANY_PLANT_MATERIAL]
        [SIEGE_CROSSBOW_MOD_TRANSFER_HANDLE_MATERIAL_TO_PRODUCT_IMPROVEMENT:1]
            another custom token
        [PRODUCT:100:1:WEAPON:ITEM_WEAPON_CROSSBOW_SIEGE:GET_MATERIAL_FROM_REAGENT:bar:NONE]

So, we are going to use the ``eventful`` module to make it so that (after the
script is run) when this crossbow is crafted, it will have two handles, each
with the material given by the block reagents.

.. highlight:: lua

First, require the modules we are going to use::

    local eventful = require("plugins.eventful")
    local customRawTokens = require("custom-raw-tokens")

Now, let's make a callback (we'll be defining the body of this function soon)::

    local modId = "siege-crossbow-mod"
    eventful.onReactionComplete[modId] = function(reaction,
            reactionProduct, unit, inputItems, inputReagents,
            outputItems)

First, we check to see if it the reaction that just happened is relevant to this
callback::

    if not customRawTokens.getToken(reaction,
        "SIEGE_CROSSBOW_MOD_TRANSFER_HANDLE_MATERIAL_TO_PRODUCT_IMPROVEMENT")
    then
        return
    end

Then, we get the product number listed. Next, for every reagent, if the reagent
name starts with "handle" then we get the corresponding item, and...

::

    for i, reagent in ipairs(inputReagents) do
        if reagent.code:startswith('handle') then
            -- Found handle reagent
            local item = inputItems[i]

...We then add a handle improvement to the listed product within our loop::

    local new = df.itemimprovement_itemspecificst:new()
    new.mat_type, new.mat_index = item.mat_type, item.mat_index
    new.type = df.itemimprovement_specific_type.HANDLE
    outputItems[productNumber - 1].improvements:insert('#', new)

This works well as long as you don't have multiple stacks filling up one
reagent.

Let's also make some code to modify the fire rate of our siege crossbow::

    eventful.onProjItemCheckMovement[modId] = function(projectile)
        if projectile.distance_flown > 0 then
            -- don't make this adjustment more than once
            return
        end

        local firer = projectile.firer
        if not firer then
            return
        end

        local weapon = df.item.find(projectile.bow_id)
        if not weapon then
            return
        end

        local multiplier = tonumber(customRawTokens.getToken(
                weapon.subtype,
                "SIEGE_CROSSBOW_MOD_FIRE_RATE_MULTIPLIER")) or 1
        firer.counters.think_counter = math.floor(
                firer.counters.think_counter * multiplier)
    end

.. highlight:: none

Now, let's see how we could make some "pegasus boots". First, let's define the
item in the raws::

    [ITEM_SHOES:ITEM_SHOES_BOOTS_PEGASUS]
        [NAME:pegasus boot:pegasus boots]
        [ARMORLEVEL:1]
        [UPSTEP:1]
        [METAL_ARMOR_LEVELS]
        [LAYER:OVER]
        [COVERAGE:100]
        [LAYER_SIZE:25]
        [LAYER_PERMIT:15]
        [MATERIAL_SIZE:2]
        [METAL]
        [LEATHER]
        [HARD]
        [PEGASUS_BOOTS_MOD_FOOT_MOVEMENT_TIMER_REDUCTION_PER_TICK:2] custom raw token
            (you don't have to comment the custom token every time,
            but it does clarify what it is)

.. highlight:: lua

Then, let's make a ``repeat-util`` callback for once a tick::

    repeatUtil.scheduleEvery(modId, 1, "ticks", function()

Let's iterate over every active unit, and for every unit, iterate over their
worn items to calculate how much we are going to take from their on-foot
movement timers::

    for _, unit in ipairs(df.global.world.units.active) do
        local amount = 0
        for _, entry in ipairs(unit.inventory) do
            if entry.mode == df.unit_inventory_item.T_mode.Worn then
                local reduction = customRawTokens.getToken(
                        entry.item,
                        'PEGASUS_BOOTS_MOD_FOOT_MOVEMENT_TIMER_REDUCTION_PER_TICK')
                amount = amount + (tonumber(reduction) or 0)
            end
        end
        -- Subtract amount from on-foot movement timers if not on ground
        if not unit.flags1.on_ground then
            dfhack.units.subtractActionTimers(unit, amount,
                    df.unit_action_type_group.MovementFeet)
        end
    end

Putting it all together
-----------------------

Ok, you're all set up! Now, let's take a look at an example
``scripts_modinstalled/example-mod.lua`` file::

    -- main file for example-mod

    -- these lines indicate that the script supports the "enable"
    -- API so you can start it by running "enable example-mod" and
    -- stop it by running "disable example-mod"
    --@module = true
    --@enable = true

    -- this is the help text that will appear in `help` and
    -- `gui/launcher`. see possible tags here:
    -- https://docs.dfhack.org/en/latest/docs/Tags.html
    --[====[
    example-mod
    ===========

    Tags: fort | gameplay

    Short one-sentence description ...

    Longer description ...

    Usage
    -----

        enable example-mod
        disable example-mod
    ]====]

    local repeatUtil = require('repeat-util')
    local eventful = require('plugins.eventful')

    -- you can reference global values or functions declared in any of
    -- your internal scripts
    local moduleA = reqscript('internal/example-mod/module-a')
    local moduleB = reqscript('internal/example-mod/module-b')
    local moduleC = reqscript('internal/example-mod/module-c')
    local moduleD = reqscript('internal/example-mod/module-d')

    local GLOBAL_KEY = 'example-mod'

    enabled = enabled or false

    function isEnabled()
        -- this function is for the enabled API, the script won't show up on the
        -- control panel without it
        return enabled
    end

    dfhack.onStateChange[GLOBAL_KEY] = function(sc)
        if sc == SC_MAP_UNLOADED then
            dfhack.run_command('disable', 'example-mod')

            -- ensure our mod doesn't try to enable itself when a different
            -- world is loaded where we are *not* active
            dfhack.onStateChange[GLOBAL_KEY] = nil

            return
        end

        if sc ~= SC_MAP_LOADED or df.global.gamemode ~= df.game_mode.DWARF then
            return
        end

        dfhack.run_command('enable', 'example-mod')
    end

    if dfhack_flags.module then
        return
    end

    if not dfhack_flags.enable then
        print(dfhack.script_help())
        print()
        print(('Example mod is currently '):format(
                enabled and 'enabled' or 'disabled'))
        return
    end

    if dfhack_flags.enable_state then
        -- do any initialization your internal scripts might require
        moduleA.onLoad()
        moduleB.onLoad()

        -- multiple functions in the same repeat callback
        repeatUtil.scheduleEvery(modId .. ' every tick', 1, 'ticks', function()
            moduleA.every1Tick()
            moduleB.every1Tick()
        end)

        -- one function per repeat callback (you can put them in the
        -- above format if you prefer)
        repeatUtil.scheduleEvery(modId .. ' 100 frames', 1, 'frames',
                                 moduleD.every100Frames)

        -- multiple functions in the same eventful callback
        eventful.onReactionComplete[modId] = function(reaction,
                reaction_product, unit, input_items, input_reagents,
                output_items)
            -- pass the event's parameters to the listeners
            moduleB.onReactionComplete(reaction, reaction_product,
                    unit, input_items, input_reagents, output_items)
            moduleC.onReactionComplete(reaction, reaction_product,
                    unit, input_items, input_reagents, output_items)
        end

        -- one function per eventful callback (you can put them in the
        -- above format if you prefer)
        eventful.onProjItemCheckMovement[modId] = moduleD.onProjItemCheckMovement
        eventful.onProjUnitCheckMovement[modId] = moduleD.onProjUnitCheckMovement

        print('Example mod enabled')
        enabled = true
    else
        -- call any shutdown functions your internal scripts might require
        moduleA.onUnload()

        repeatUtil.cancel(modId .. ' every ticks')
        repeatUtil.cancel(modId .. ' 100 frames')

        eventful.onReactionComplete[modId] = nil
        eventful.onProjItemCheckMovement[modId] = nil
        eventful.onProjUnitCheckMovement[modId] = nil

        print('Example mod disabled')
        enabled = false
    end

Inside ``scripts_modinstalled/internal/example-mod/module-a.lua`` you could
have code like this::

    --@ module = true

    function onLoad() -- global variables are exported
        -- do initialization here
    end

    -- this is a local function: local functions/variables
    -- are not accessible to other scripts.
    local function usedByOnTick(unit)
        -- ...
    end

    function onTick() -- exported
        for _,unit in ipairs(df.global.world.units.all) do
            usedByOnTick(unit)
        end
    end

The `reqscript <reqscript>` function reloads scripts that have changed, so you
can modify your scripts while DF is running and just disable/enable your mod to
load the changes into your ongoing game!
