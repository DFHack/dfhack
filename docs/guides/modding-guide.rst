.. _modding-guide:

DFHack modding guide
====================

What is a mod/script?
---------------------

A script is a single file that can be run as a command in DFHack, like something that modifies or displays game data on request. A mod is something you install to get persistent behavioural changes in the game and/or add new content. DFHack mods contain and use scripts as well as often having a raw mod component.

DFHack scripts are written in Lua. If you don't already know Lua, there's a great primer at https://www.lua.org/pil/1.html.

Why not just use raw modding?
-----------------------------

For many things it's either completely and only (sensibly) doable in raws or completely and only doable with DFHack. For mods where DFHack is an alternative and not the only option, it's much less hacky, easier to maintain, and easier to extend, and is not prone to side-effects. A great example is adding a syndrome when a reaction is performed requiring an exploding boulder in raws but having dedicated tools for it if you use DFHack. Many things will require a mix of raw modding and DFHack.

A mod-maker's development environment
-------------------------------------

Scripts can be run from a world's ``raw/scripts/`` directory, and (configurably) are run by default from ``hack/scripts/``. Scripts in ``raw/init.d/`` are automatically run on world load. Scripts within the raws are a component for more advanced mods.

A script is run by writing its path and name from a script path folder without the file extension into a DFHack command prompt (in-game or the external one). E.g. ``gui/gm-editor`` for ``hack/scripts/gui/gm-editor.lua``.

You can make all your scripts in ``hack/scripts/``, but this is not recommended as it makes things much harder to maintain each update. It's recommended to make a folder with a name like ``own-scripts`` and add it to ``dfhack-config/script-paths.txt``. You could also make a folder for external installed scripts from the internet that are not in ``hack/scripts/``. You can prepend your script paths entries with a ``+`` so that they take precedence over other folders.

If your mod is installed into ``raw/scripts/`` be aware that the copies of the scripts in ``data/save/*/raw/`` are checked first and will run instead of any changes you make to an in-development copy outside of a raw folder.

The structure of the game
-------------------------

"The game" is in the global variable `df <lua-df>`. The game's memory can be found in ``df.global``, containing things like the list of all items, whether to reindex pathfinding, et cetera. Also relevant to us in ``df`` are the various types found in the game, e.g. ``df.pronoun_type`` which we will be using.

Your first script
-----------------

So! It's time to write your first script. We are going to make a script that will get the pronoun type of the currently selected unit (there are many contexts where the function that gets the currently selected unit works).

First line, we get the unit. ::

    local unit = dfhack.gui.getSelectedUnit()

If no unit is selected, an error message will be printed (which can be silenced by passing ``true`` to ``getSelectedUnit``) and ``unit`` will be ``nil``.

If ``unit`` is ``nil``, we don't want the script to run anymore. ::

    if not unit then
        return
    end

Now, the field ``sex`` in a unit is an integer, but each integer corresponds to a string value (it, she, or he). We get this value by indexing the bidirectional map ``df.pronoun_type`` with an integer from the unit. Indexing the other way, with one of the strings, will yield its corresponding number. So: ::

    local pronounTypeString = df.pronoun_type[unit.sex]
    print(pronounTypeString)

Simple. Save this as a Lua file in your own scripts directory and run it as shown before when focused on a unit one way or another.

Getting used to gm-editor and df-structures exploration
-------------------------------------------------------

So how could you have known about the field and type we just used? Well, there are two main tools for discovering the various fields in the game's data structures. The first is the ``df-structures`` repository (at https://github.com/DFHack/df-structures) that contains XML files denoting the contents of the game's structures. The second is the script ``gui/gm-editor`` which is an interactive data explorer. You can run the script while objects like units are selected to view the data within them. You can also pass ``scr`` as an argument to the script to view the data for the current screen. Press ? while the script is active to view help.

Familiarising yourself with the many structs of the game will help with ideas immensely, and you can always ask for help in the right places (e.g. DFHack's Discord).

Detecting triggers
------------------

One main method for getting new behaviour into the game is callback functions. There are two main libraries for this, ``repeat-util`` and ``eventful``. ``repeat-util`` is used to run a function once per configurable number of frames (paused or unpaused), ticks (unpaused), in-game days, months, or years. For adding behaviour you will most often want something to run once a tick. ``eventful`` is used to get code to run (with special parameters!) when something happens in the game, like a reaction or job being completed or a projectile moving.

To get something to run once per tick, we would want to call ``repeat-util``'s ``scheduleEvery`` function.

First, we load the module: ::

    local repeatUtil = require("repeat-util")

Both ``repeat-util`` and ``eventful`` require keys for registered callbacks. It's recommended to use something like a mod id. ::

    local modId = "callback-example-mod"

Then, we pass the key, amount of time units between function calls, what the time units are, and finally the callback function itself: ::

    repeatUtil.scheduleEvery(modId, 1, "ticks", function()
        -- Do something like iterating over all active units
        for _, unit in ipairs(df.global.world.units.active) do
            print(unit.id)
        end
    end)

``eventful`` is slightly more involved. First get the module: ::

    local eventful = require("plugins.eventful")

``eventful`` contains a table for each event which you populate with functions. Each function in the table is then called with the appropriate arguments when the event occurs. So, for example, to print the position of a moving (item) projectile: ::

    eventful.onProjItemCheckMovement[modId] = function(projectile)
        print(projectile.cur_pos.x, projectile.cur_pos.y, projectile.cur_pos.z)
    end

Check the full list of events at https://docs.dfhack.org/en/stable/docs/Lua%20API.html#list-of-events.

Custom raw tokens
-----------------

In this section, we are going to use `custom raw tokens <custom-raw-tokens>` applied to a reaction to transfer the material of a reagent to a product as a handle improvement (like on artifact buckets), and then we are going to see how you could make boots that make units go faster when worn. Both of these involve custom raw tokens.

First, let's define a custom crossbow with its own custom reaction. The crossbow: ::

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

The reaction to make it (you would add the reaction and not the weapon to an entity raw): ::

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
        [SIEGE_CROSSBOW_MOD_TRANSFER_HANDLE_MATERIAL_TO_PRODUCT_IMPROVEMENT:1] another custom token
        [PRODUCT:100:1:WEAPON:ITEM_WEAPON_CROSSBOW_SIEGE:GET_MATERIAL_FROM_REAGENT:bar:NONE]

So, we are going to use the ``eventful`` module to make it so that (after the script is run) when this crossbow is crafted, it will have two handles, each with the material given by the block reagents.

First, require the modules we are going to use. ::

    local eventful = require("plugins.eventful")
    local customRawTokens = require("custom-raw-tokens")

Now, let's make a callback: ::

    local modId = "siege-crossbow-mod"
    eventful.onReactionComplete[modId] = function(reaction, reactionProduct, unit, inputItems, inputReagents, outputItems)

First, we check to see if it the reaction that just happened is relevant to this callback: ::

    if not customRawTokens.getToken(reaction, "SIEGE_CROSSBOW_MOD_TRANSFER_HANDLE_MATERIAL_TO_PRODUCT_IMPROVEMENT") then
        return
    end

Then, we get the product number listed. Next, for every reagent, if the reagent name starts with "handle" then we get the corresponding item, and... ::

    for i, reagent in ipairs(inputReagents) do
        if reagent.code:sub(1, #"handle") == "handle" then
            -- Found handle reagent
            local item = inputItems[i] -- hopefully found handle item

...We then add a handle improvement to the listed product within our loop. ::

    local new = df.itemimprovement_itemspecificst:new()
    new.mat_type, new.mat_index = item.mat_type, item.mat_index
    -- new.maker = outputItems[0].maker -- not a typical improvement
    new.type = df.itemimprovement_specific_type.HANDLE
    outputItems[productNumber - 1].improvements:insert("#", new)
    -- break -- multiple handles, multiple "the handle is made from"s, so no break

It's all a bit loose and hacky but it works, at least if you don't have multiple stacks filling up one reagent.

Let's also make some code to modify the fire rate of the siege crossbow. ::

    eventful.onProjItemCheckMovement[modId] = function(projectile)
        if projectile.distance_flown > 0 then -- don't repeat this
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

        local multiplier = tonumber(customRawTokens.getToken(weapon.subtype, "SIEGE_CROSSBOW_MOD_FIRE_RATE_MULTIPLIER")) or 1
        firer.counters.think_counter = math.floor(firer.counters.think_counter * multiplier)
    end

Now, let's see how we could make some "pegasus boots". First, let's define the item in the raws: ::

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
        [PEGASUS_BOOTS_MOD_MOVEMENT_TIMER_REDUCTION_PER_TICK:5] custom raw token (you don't have to comment this every time)

Then, let's make a ``repeat-util`` callback for once a tick: ::

    repeatUtil.scheduleEvery(modId, 1, "ticks", function()

Let's iterate over every active unit, and for every unit, initialise a variable for how much we are going to take from their movement timer and iterate over all their worn items: ::

    for _, unit in ipairs(df.global.world.units.active) do
        local amount = 0
        for _, entry in ipairs(unit.inventory) do

Now, we will add up the effect of all speed-increasing gear and apply it: ::

        if entry.mode == df.unit_inventory_item.T_mode.Worn then
            amount = amount + tonumber((customRawTokens.getToken(entry.item, "PEGASUS_BOOTS_MOD_MOVEMENT_TIMER_REDUCTION_PER_TICK")) or 0)
        end
    end
    dfhack.units.addMoveTimer(-amount) -- Subtract amount from movement timer if currently moving

The structure of a full mod
---------------------------

Now, you may have noticed that you won't be able to run multiple functions on tick/as event callbacks with that ``modId`` idea alone. To solve that we can just define all the functions we want and call them from a single function. Alternatively you can create multiple callbacks with your mod ID being a prefix, though this way there is no guarantee about the order if that is important. You will have to use your mod ID as a prefix if you register multiple ``repeat-util`` callbacks, though.

Create a folder for mod projects somewhere (e.g. ``hack/my-scripts/mods/``, or maybe somewhere outside your Dwarf Fortress installation) and use your mod ID (in hyphen-case) as the name for the mod folders within it. The structure of and environment for fully-functioning modular mods are as follows:

* The main content of the mod would be in the ``raw`` folder:

  * A Lua file in ``raw/init.d/`` to initialise the mod by calling ``your-mod-id/main/ enable``.
  * Raw content (potentially with custom raw tokens) in ``raw/objects/``.
  * A subfolder for your mod in ``raw/scripts/`` containing a ``main.lua`` file (an example of which we will see) and all the modules containing the functions used in callbacks to ``repeat-util`` and ``eventful``. Potentially a file containing constant definitions used by your mod (perhaps defined by the game, like the acceleration of parabolic projectiles due to gravity (``4900``)) too.

* Using git within each mod folder is recommended, but not required.
* A ``readme.md`` markdown file is also recommended.
* An ``addToEntity.txt`` file containing lines to add to entity definitions for access to mod content would be needed if applicable.
* Unless you want to merge your ``raw`` folder with your worlds every time you make a change to your scripts, you should add ``path/to/your-mod/raw/scripts/`` to your script paths.

Now, let's take a look at an example ``main.lua`` file. ::

    local repeatUtil = require("repeat-util")
    local eventful = require("plugins.eventful")

    local modId = "example-mod"
    local args = {...}

    if args[1] == "enable" then
        -- The modules and what they link into the environment with
        -- Each module exports functions named the way they are to be used
        local moduleA = dfhack.reqscript("example-mod/module-a") -- on load, every tick
        local moduleB = dfhack.reqscript("example-mod/module-b") -- on load, on unload, onReactionComplete
        local moduleC = dfhack.reqscript("example-mod/module-c") -- onReactionComplete
        local moduleD = dfhack.reqscript("example-mod/module-d") -- every 100 frames, onProjItemCheckMovement, onProjUnitCheckMovement

        -- Set up the modules
        -- Order: on load, repeat-util ticks (from smallest interval to largest), days, months, years, and frames, then eventful callbacks in the same order as the first modules to use them

        moduleA.onLoad()
        moduleB.onLoad()

        repeatUtil.scheduleEvery(modId .. " 1 ticks", 1, "ticks", function()
            moduleA.every1Tick()
        end)

        repeatUtil.scheduleEvery(modID .. " 100 frames", 1, "frames", function()
            moduleD.every100Frames()
        end

        eventful.onReactionComplete[modId] = function(...)
            -- Pass the event's parameters to the listeners, whatever they are
            moduleB.onReactionComplete(...)
            moduleC.onReactionComplete(...)
        end

        eventful.onProjItemCheckMovement[modId] = function(...)
            moduleD.onProjItemCheckMovement(...)
        end

        eventful.onProjUnitCheckMovement[modId] = function(...)
            moduleD.onProjUnitCheckMovement(...)
        end

        print("Example mod enabled")
    elseif args[1] == "disable" then
        -- Order: on unload, then cancel the callbacks in the same order as above

        moduleA.onUnload()

        repeatUtil.cancel(modId .. " 1 ticks")
        repeatUtil.cancel(modId .. " 100 frames")

        eventful.onReactionComplete[modId] = nil
        eventful.onProjItemCheckMovement[modId] = nil
        eventful.onProjUnitCheckMovement[modId] = nil

        print("Example mod disabled")
    elseif not args[1] then
        dfhack.printerr("No argument given to example-mod/main")
    else
        dfhack.printerr("Unknown argument \"" .. args[1] .. "\" to example-mod/main")
    end

You can see there are four cases depending on arguments. Set up the callbacks and call on-load functions if enabled, dismantle the callbacks and call on-unload functions if disabled, no arguments given, and invalid argument(s) given.

Here is an example of an ``raw/init.d/`` file: ::

    dfhack.run_command("example-mod/main enable") -- Very simple. Could be called "init-example-mod.lua"

Here is what ``raw/scripts/module-a.lua`` would look like: ::

    --@ module = true
    -- The above line is required for dfhack.reqscript to work

    function onLoad() -- global variables are exported
        -- blah
    end

    local function usedByOnTick() -- local variables are not exported
        -- blah
    end

    function onTick() -- exported
        for blah in ipairs(blah) do
            usedByOnTick()
        end
    end

It is recommended to check `reqscript <reqscript>`'s documentation. ``reqscript`` caches scripts but will reload scripts that have changed (it checks the file's last modification date) so you can do live editing *and* have common tables et cetera between scripts that require the same module.
