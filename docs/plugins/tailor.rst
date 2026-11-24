tailor
======

.. dfhack-tool::
    :summary: Automatically keep your dwarves in fresh clothing.
    :tags: fort auto workorders

Once a day, this plugin will scan the clothing situation in the fort. If there
are fresh cloths available, dwarves who are wearing tattered clothing will have
their rags confiscated (in the same manner as the `cleanowned` tool) so that
they'll reequip with replacement clothes.

If there are not enough clothes available, manager orders will be generated to
manufacture some more. ``tailor`` will intelligently create orders using raw
materials that you have on hand in the fort. For example, if you have lots of
silk, but no cloth, then ``tailor`` will order only silk clothing to be made.

``tailor`` determines what each unit needs based on the unit's actual body
plan rather than a fixed list of clothing types. Every body part that wants
clothing -- upper and lower body parts, the head, grasp parts (hands), and
stance parts (feet, or whatever else the unit stands on) -- gets its own
requirement, so a dwarf needs *two* shoes, a one-legged creature needs only
one, and a creature that stands on a tail needs a shoe for its tail. For each
such part, ``tailor`` chooses from the clothing types your civilization knows
how to make, preferring the ones that cover the most of the unit's body based
on the item definition's UBSTEP/LBSTEP/UPSTEP reach. Since gloves and shoes
are manufactured in pairs, orders for them account for pairs: one order makes
two identical shoes that can cover any two stance parts, or one left and one
right glove for the matching grasp parts.

If you are wondering whether you should enable `autoclothing` instead, see the
head-to-head comparison in the `autoclothing` docs.

Usage
-----

::

    enable tailor
    tailor [status]
    tailor now
    tailor materials <material> [<material> ...]
    tailor confiscate [true|false]
    tailor dye [true|false]

By default, ``tailor`` will prefer using materials in this order::

    silk cloth yarn leather

but you can use the ``tailor materials`` command to restrict which materials are
used, and in what order. By default, ``tailor`` will "confiscate" (i.e., remove
ownership and mark for dumping) equipped tattered clothing once replacements are
available. This can be changed using ``tailor confiscate``. The default behavior
minimizes the time you dwarves keep wearing worn-out clothing, minimizing the
number of negative thoughts incurred by this. However, it will also result in
tattered clothing being brought to your "garbage" zones and forbidden, which may
be undesirable.

``tailor`` supports adamantine cloth (using the ``materials`` keyword
``adamantine``) but does not use it by default, as most players find adamantine
too precious to routinely make into cloth. ``tailor`` does not support modded
"cloth" types which utilize custom reactions for making clothing out of those
cloth types.

If dye management is enabled, ``tailor`` will also issue orders to dye cloth
as needed to fulfill fortress clothing requirements, and will issue orders to
manufacture dyes required to fulfill the dye cloth orders. Dye management is
disabled by default.

Examples
--------

``enable tailor``
    Start replacing tattered clothes with default settings.

``tailor now``
    Run a scan and order cycle right now, regardless of whether the plugin is
    enabled.

``tailor materials silk cloth yarn``
    Restrict the materials used for automatically manufacturing clothing to
    silk, cloth, and yarn, preferred in that order. This saves leather for
    other uses, like making armor.

Caveats
-------

Modded cloth-like materials are not supported because custom reactions do not
support being sized for non-dwarf races. The game only supports sizing the
built-in default make-clothing or make-armor reactions.

Dye automation will not not issue orders to mill dyes that are made at a
millstone or quern because Dwarf Fortress does not currently support
"mill plant" orders for a specified material.

At present, dye automation uses any available dye and cloth will be dyed
with whatever color dye can be found or made. The ability to specify color
preferences may be added in the future.
