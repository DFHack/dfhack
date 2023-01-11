changevein
==========

.. dfhack-tool::
    :summary: Change the material of a mineral inclusion.
    :tags: untested fort armok map

You can change a vein to any inorganic material RAW id. Note that this command
only affects tiles within the current 16x16 block - for large veins and
clusters, you will need to use this command multiple times.

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
