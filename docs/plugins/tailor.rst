tailor
======

Whenever the bookkeeper updates stockpile records, this plugin will scan every unit in the fort,
count up the number that are worn, and then order enough more made to replace all worn items.
If there are enough replacement items in inventory to replace all worn items, the units wearing them
will have the worn items confiscated (in the same manner as the `cleanowned` plugin) so that they'll
reeequip with replacement items.

Use the `enable` and `disable <disable>` commands to toggle this plugin's status, or run
``tailor status`` to check its current status.
