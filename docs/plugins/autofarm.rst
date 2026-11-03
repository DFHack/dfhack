autofarm
========

.. dfhack-tool::
    :summary: Automatically manage farm crop selection.
    :tags: fort auto plants

Periodically scan your plant stocks and assign crops to your farm plots based on
which plant stocks are low (as long as you have the appropriate seeds). The
target threshold for each crop type is configurable.

Farms that span multiple biomes are handled correctly: any crop that can grow
in any of the biomes covered by the farm's tiles may be assigned to that farm.

Individual farm plots can be excluded from autofarm management via the controls
that appear on the farm plot information sheet. That sheet also provides a
shortcut to `gui/autofarm`, a convenient UI for setting crop thresholds and
enabling or disabling autofarm.

Usage
-----

``enable autofarm``
    Enable the plugin and start managing crop assignment.
``autofarm runonce``
    Updates all farm plots once, without enabling the plugin.
``autofarm status``
    Prints status information, including any defined thresholds.
``autofarm default <number>``
    Sets the default threshold.
``autofarm threshold <number> <type> [<type> ...]``
    Sets thresholds of individual plant types.

You can find the identifiers for the crop types in your world by running the
following command: ``getplants -f``

Some plants produce usable growth items (such as fruits, leaves, or bulbs)
when processed or gathered. These products normally count toward the plant's
threshold together with the plant items themselves, but you can also assign a
threshold to a specific growth product with the ``<plant>:<growth>`` syntax.
A plant is grown while either its pooled stock or any of its assigned growth
products is below the corresponding threshold.

Examples
--------

``autofarm default 30``
    Set the default threshold to 30.
``autofarm threshold 150 MUSHROOM_HELMET_PLUMP GRASS_TAIL_PIG``
    Set the threshold for Plump Helmets and Pig Tails to 150
``autofarm threshold 30 BUSH_QUARRY:LEAVES``
    Keep at least 30 Quarry Bush Leaves in stock, growing Quarry Bushes until
    you have enough of them to process
