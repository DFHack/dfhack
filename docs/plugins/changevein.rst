changevein
==========

.. dfhack-tool::
    :summary: Change the material of a mineral inclusion.
    :tags: fort armok map

You can change a vein to any inorganic material RAW id.

You can use the `probe` command to discover the material RAW ids for existing
veins that you want to duplicate.

Usage
-----

::

    changevein <material RAW id>

Example
-------

``changevein NATIVE_PLATINUM``
    Convert vein at cursor position into platinum ore.
