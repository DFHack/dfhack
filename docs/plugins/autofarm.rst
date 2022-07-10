autofarm
========

Automatically handles crop selection in farm plots based on current plant
stocks, and selects crops for planting if current stock is below a threshold.
Selected crops are dispatched on all farmplots. (Note that this plugin replaces
an older Ruby script of the same name.)

Use the `enable` or `disable <disable>` commands to change whether this plugin is
enabled.

Usage:

* ``autofarm runonce``:
    Updates all farm plots once, without enabling the plugin
* ``autofarm status``:
    Prints status information, including any applied limits
* ``autofarm default 30``:
    Sets the default threshold
* ``autofarm threshold 150 helmet_plump tail_pig``:
    Sets thresholds of individual plants
