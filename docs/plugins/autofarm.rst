autofarm
========

.. dfhack-tool::
    :summary: Automatically manage farm crop selection.
    :tags: fort auto plants

Periodically scan your plant stocks and assign crops to your farm plots based on
which plant stocks are low (as long as you have the appropriate seeds). The
target threshold for each crop type is configurable.

Usage:

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
following command::

    lua "for _,plant in ipairs(df.global.world.raws.plants.all) do if plant.flags.SEED then print(plant.id) end end"

Examples
--------

``autofarm default 30``
    Set the default threshold to 30.
``autofarm threshold 150 MUSHROOM_HELMET_PLUMP GRASS_TAIL_PIG``
    Set the threshold for Plump Helmets and Pig Tails to 150
