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

If you are wondering whether you should enable `autoclothing` instead, see the
head-to-head comparsion in the `autoclothing` docs.

Usage
-----

::

    enable tailor
    tailor [status]
    tailor now
    tailor materials <material> [<material> ...]
    tailor confiscate [true|false]

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
