forceequip
==========

.. dfhack-tool::
    :summary: Move items into a unit's inventory.
    :tags: untested adventure fort animals items military units

This tool is typically used to equip specific clothing/armor items onto a dwarf,
but can also be used to put armor onto a war animal or to add unusual items
(such as crowns) to any unit. Make sure the unit you want to equip is standing
on the target items, which must be on the ground and be unforbidden. If multiple
units are standing on the same tile, the first one will be equipped.

The most reliable way to set up the environment for this command is to pile
target items on a tile of floor with a garbage dump activity zone or the
`autodump` command, then walk/pasture a unit (or use `gui/teleport`) on top of
the items. Be sure to unforbid the items that you want to work with!

.. note::

    Weapons are not currently supported.

Usage
-----

::

    forceequip [<options>]

As mentioned above, this plugin can be used to equip items onto units (such as
animals) who cannot normally equip gear. There's an important caveat here: such
creatures will automatically drop inappropriate gear almost immediately (within
10 game ticks). If you want them to retain their equipment, you must forbid it
AFTER using forceequip to get it into their inventory. This technique can also
be used to clothe dwarven infants, but only if you're able to separate them from
their mothers.

By default, the ``forceequip`` command will attempt to abide by game rules as
closely as possible. For instance, it will skip any item which is flagged for
use in a job, and will not equip more than one piece of clothing/armor onto any
given body part. These restrictions can be overridden via options, but doing so
puts you at greater risk of unexpected consequences. For instance, a dwarf who
is wearing three breastplates will not be able to move very quickly.

Items equipped by this plugin DO NOT become owned by the recipient. Adult
dwarves are free to adjust their own wardrobe, and may promptly decide to doff
your gear in favour of their owned items. Animals, as described above, will tend
to discard ALL clothing immediately unless it is manually forbidden. Armor items
seem to be an exception: an animal will tend to retain an equipped suit of mail
even if you neglect to forbid it.

Please note that armored animals are quite vulnerable to ranged attacks. Unlike
dwarves, animals cannot block, dodge, or deflect arrows, and they are slowed by
the weight of their armor.

Examples
--------

``forceequip``
    Attempts to equip all of the clothing and armor under the cursor onto the
    unit under the cursor, following game rules regarding which item can be
    equipped on which body part and only equipping 1 item onto each body part.
    Items owned by other dwarves are ignored.
``forceequip v bp QQQ``
    List the bodyparts of the selected unit.
``forceequip bp LH``
    Equips an appropriate item onto the unit's left hand.
``forceequip m bp LH``
    Equips ALL appropriate items onto the unit's left hand. The unit may end up
    wearing a dozen left-handed mittens. Use with caution, and remember that
    dwarves tend to drop extra items ASAP.
``forceequip i bp NECK``
    Equips an item around the unit's neck, ignoring appropriateness
    restrictions. If there's a millstone or an albatross carcass sitting on the
    same square as the targeted unit, then there's a good chance that it will
    end up around his neck. For precise control, remember that you can
    selectively forbid some of the items that are piled on the ground.
``forceequip s``
    Equips the item currently selected in the k menu, if possible.
``forceequip s m i bp HD``
    Equips the selected item onto the unit's head. Ignores all restrictions and
    conflicts. If you know exactly what you want to equip, and exactly where you
    want it to go, then this is the most straightforward and reliable option.

Options
-------

``i``, ``ignore``
    Bypasses the usual item eligibility checks (such as "Never equip gear
    belonging to another dwarf" and "Nobody is allowed to equip a Hive".
``m``, ``multi``
    Bypasses the 1-item-per-bodypart limit. Useful for equipping both a mitten
    and a gauntlet on the same hand (or twelve breastplates on the upper body).
``m2``, ``m3``, ``m4``
    Modifies the 1-item-per-bodypart limit, allowing each part to receive 2, 3,
    or 4 pieces of gear.
``s``, ``selected``
    Equip only the item currently selected in the k menu and ignore all other
    items in the tile.
``bp``, ``bodypart <body part code>``
    Specify which body part should be equipped.
``v``, ``verbose``
    Provide detailed narration and error messages, including listing available
    body parts when an invalid ``bodypart`` code is specified.
