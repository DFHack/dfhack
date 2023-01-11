steam-engine
============

.. dfhack-tool::
    :summary: Allow modded steam engine buildings to function.
    :tags: untested fort gameplay buildings
    :no-command:

The steam-engine plugin detects custom workshops with the string
``STEAM_ENGINE`` in their token, and turns them into real steam engines!

The plugin auto-enables itself when it detects the relevant tags in the world
raws. It does not need to be enabled with the `enable` command.

Rationale
---------
The vanilla game contains only water wheels and windmills as sources of power,
but windmills give relatively little power, and water wheels require flowing
water, which must either be a real river and thus immovable and
limited in supply, or actually flowing and thus laggy.

Compared to the
:wiki:`dwarven water reactor <Water_wheel#Dwarven_Water_Reactor>` exploit,
steam engines make a lot of sense!

Construction
------------
The workshop needs water as its input, which it takes via a passable floor tile
below it, like usual magma workshops do. The magma version also needs magma.

Due to DF game limits, the workshop will collapse over true open space. However,
down stairs are passable but support machines, so you can use them.

After constructing the building itself, machines can be connected to the edge
tiles that look like gear boxes. Their exact position is extracted from the
workshop raws.

Like with collapse above, due to DF game limits the workshop can only
immediately connect to machine components built AFTER it.  This also means that
engines cannot be chained without intermediate axles built after both engines.

Operation
---------
In order to operate the engine, queue the Stoke Boiler job (optionally on
repeat). A furnace operator will come, possibly bringing a bar of fuel, and
perform it. As a result, a "boiling water" item will appear in the :kbd:`t`
view of the workshop.

.. note::

    The completion of the job will actually consume one unit
    of the appropriate liquids from below the workshop. This means
    that you cannot just raise 7 units of magma with a piston and
    have infinite power. However, liquid consumption should be slow
    enough that water can be supplied by a pond zone bucket chain.

Every such item gives 100 power, up to a limit of 300 for coal, or 500 for a
magma engine. The building can host twice that amount of items to provide longer
autonomous running. When the boiler gets filled to capacity, all queued jobs are
suspended. Once it drops back to 3+1 or 5+1 items, they are re-enabled.

While the engine is providing power, steam is being consumed. The consumption
speed includes a fixed 10% waste rate, and the remaining 90% is applied
proportionally to the actual load in the machine. With the engine at nominal 300
power with 150 load in the system, it will consume steam for actual
300*(10% + 90%*150/300) = 165 power.

A masterpiece mechanism and chain will decrease the mechanical power drawn by
the engine itself from 10 to 5. A masterpiece barrel decreases waste rate by 4%.
A masterpiece piston and pipe decrease it by further 4%, and also decrease the
whole steam use rate by 10%.

Explosions
----------
The engine must be constructed using barrel, pipe, and piston from fire-safe,
or, in the magma version, magma-safe metals.

During operation, weak parts gradually wear out, and eventually the engine
explodes. It should also explode if toppled during operation by a building
destroyer or a tantruming dwarf.

Save files
----------
It should be safe to load and view engine-using fortresses from a DF version
without DFHack installed, except that in such case the engines, of course, won't
work. However actually making modifications to them or machines they connect to
(including by pulling levers) can easily result in inconsistent state once this
plugin is available again. The effects may be as weird as negative power being
generated.
