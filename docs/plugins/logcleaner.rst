logcleaner
==========
.. dfhack-tool::
    :summary: Automatically clear combat, sparring, and hunting reports.
    :tags: fort auto units

This plugin prevents spam from cluttering your announcement history and filling
the 3000-item reports buffer. It runs every 100 ticks and clears selected report
types from both the global reports buffer and per-unit logs.

Usage
-----

Basic commands
~~~~~~~~~~~~~~

``logcleaner``
    Show the current status of the plugin.
``logcleaner enable``
    Enable the plugin (persists per save).
``logcleaner disable``
    Disable the plugin.

Configuring filters
~~~~~~~~~~~~~~~~~~~

``logcleaner combat``
    Clear combat reports (also enables the plugin if disabled).
``logcleaner sparring``
    Clear sparring reports.
``logcleaner hunting``
    Clear hunting reports.
``logcleaner combat,sparring``
    Clear multiple report types (comma-separated).
``logcleaner all``
    Enable all three filter types.
``logcleaner none``
    Disable all filter types.

Examples
~~~~~~~~

Clear only sparring reports::

    logcleaner sparring

Clear combat and hunting, but not sparring::

    logcleaner combat,hunting

Overlay UI
----------

Run ``gui/logcleaner`` to open the settings overlay, or access it from the
control panel under the Gameplay tab.

The overlay provides:

- **Enable toggle**: Turn the plugin on or off (``Shift+E``)
- **Combat toggle**: Clear combat reports (``Shift+C``)
- **Sparring toggle**: Clear sparring reports (``Shift+S``)
- **Hunting toggle**: Clear hunting reports (``Shift+H``)
