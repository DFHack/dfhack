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

You can make all your scripts in ``hack/scripts/``, but this is not recommended as it makes things much harder to maintain each update. It's recommended to make a folder with a name like "own-scripts" and add it to ``dfhack-config/script-paths.txt``. You should also make a folder for external installed scripts from the internet that are not in ``hack/scripts/``. You can prepend your script paths entries with a ``+`` so that they take precedence over other folders.

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

Getting used to gm-editor and DFStructs exploration
---------------------------------------------------

So how could you have known about the field and type we just used?

s

Detecting triggers
------------------

s

Setting up an environment for a more advanced modular mod
---------------------------------------------------------

s

Your first whole mod
--------------------

s
