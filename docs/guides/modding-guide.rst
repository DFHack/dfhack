.. _modding-guide:

DFHack modding guide
====================

Preface
-------

Why not just use raw modding?

It's much less hacky for many things, easier to maintain, and easier to extend. Of course if you're adding a new creature or whatever do use raw modding, but for things like adding syndromes when certain actions are performed, absolutely try out DFHack. Many things will require a mix of raw modding and DFHack.

What kind of things can we make?

Lots of things, and the list grows as more and more hooks and tools are developed within DFHack. You can modify behaviours by cleverly watching and modifying certain areas of the game with functions that run on every tick or by using hooks. Familiarising yourself with the many structs oof the game will help with ideas immensely, and you can always ask for help in the right places (e.g. DFHack's Discord).

DFHack scripts are written in Lua. If you don't already know Lua, there's a great primer at <link>.

A script-maker's environment
----------------------------

A script is run by writing its path and name from a script path folder without the file extension. E.g. gui/gm-editor for hack/scripts/gui/gm-editor.lua. You can make all your scripts in hack/scripts/, but this is not recommended as it makes things much harder to maintain each update. It's recommended to make a folder with a name like "own-scripts" and add it to dfhack-config/script-paths.txt. You should also make a folder for external installed scripts from the internet that are not in hack/scripts/.

The structure of the game
-------------------------

"The game" is in the global variable `df <lua-df>`. The game's memory can be found in ``df.global``, containing things like the list of all items, whether to reindex pathfinding, et cetera. Also relevant to us in ``df`` are the various types found in the game, e.g. ``df.pronoun_type`` which we will be using.

Your first script
-------------------------------------

So! It's time to write your first script. We are going to make a script that will get the pronoun type of the currently selected unit (there are many contexts where the function that gets the currently selected unit works).

First line, we get the unit.::

    local unit = dfhack.gui.getSelectedUnit()

If no unit is selected, an error message will be printed (which can be silenced by passing ``true`` to ``getSelectedUnit``) and ``unit`` will be ``nil``.

If ``unit`` is ``nil``, we don't want the script to run anymore.::

    if not unit then
        return
    end

Now, the field ``sex`` in a unit is an integer, but each integer corresponds to a string value (it, she, or he). We get this value by indexing the bidirectional map ``df.pronoun_type`` with an integer from the unit. Indexing the other way, with one of the strings, will yield its corresponding number. So:::

    local pronounTypeString = df.pronoun_type[unit.sex]
    print(pronounTypeString)

Simple.

Getting used to gm-editor and DFStructs exploration
---------------------------------------------------

So how could you have known about the field and type we just used?

A script that prints more complex data from a unit
--------------------------------------------------

s

Something that uses eventful and/or repeat-util
-----------------------------------------------

s

Setting up an environment for a more advanced modular mod
---------------------------------------------------------

s

A whole mod with multiple hooks and multiple functions that happen on tick
--------------------------------------------------------------------------

s
