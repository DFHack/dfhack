export-world-map
================

.. dfhack-tool::
    :summary: Export world map data for GIS and other external tools.
    :tags: inspection embark map

This plugin exports world-map information to files in the DFHack configuration
folder under the ``map-export`` directory. It must be run from the
embark-selection screen. The exports are generated at the granularity of
individual region tiles (16 x 16 per world tile). Those tiles are only generated
by DF as you scroll around on the zoomed-in embark map, which thus needs to be
done prior to running this export tool.

The supported export targets are:

- ``regions``: writes world tile regions and their biome information to ``regions.csv``.
- ``sites``: writes site extents and ownership/civilization information to ``sites.csv``.
- ``rivers``: writes rivers to ``rivers.csv``.
- ``elevation``: writes elevation data to ``elevation.dat`` and ``elevation.vrt``.

Unless you have software such as ArcGIS or QGIS/GDAL available, these exports
will be of little use.

Usage
-----

::

    export-world-map
    export-world-map <topic>
    export-world-map <topic> <topic> ...
    export-world-map all

If no topic is given, or if ``all`` is supplied, every supported export target
is run.

Options
-------

``--group-by-world``
    Create a folder named after the world in the ``map-export`` directory.

``--group-by-date``
    Create a folder with name ``year-month`` inside the world folder. Implies
    ``--group-by-world``.

Examples
--------

``export-world-map``
    Export all supported datasets.

``export-world-map regions``
    Export only region boundary geometries.

``export-world-map sites rivers``
    Export site and river geometries only.


Notes on file formats
---------------------

To avoid depending on external libraries such as GDAL, the export formats are
kept as simple as possible to generate. This means raw bitmap data for the
elevation export and CSV files for the others. Before these files can be used
reasonably in software such as QGIS, they need to be assigned a coordinate
reference system (CRS) and converted into formats that can be accessed more
efficiently than CSV. While any CRS will do, WGS 84 / Pseudo-Mercator
(EPSG:3857) is the preferred choice.

For detailed map data (``regions`` and ``rivers``), Parquet is by far the
preferred format. The elevation export provides a VRT file to facilitate the
transformation into a GeoTIFF with the aforementioned CRS.

Assuming a recent version of GDAL built with Parquet support and with all
command-line tools in your PATH, this can be achieved with the following
commands:

::

    ogr2ogr -a_srs "EPSG:3857" -of Parquet regions.parquet -oo 'GEOM_POSSIBLE_NAMES=*wkt' -oo 'KEEP_GEOM_COLUMNS=NO' -oo 'AUTODETECT_TYPE=YES' regions.csv
    ogr2ogr -a_srs "EPSG:3857" -of Parquet rivers.parquet -oo 'GEOM_POSSIBLE_NAMES=*wkt' -oo 'KEEP_GEOM_COLUMNS=NO' -oo 'AUTODETECT_TYPE=YES' rivers.csv
    gdal raster convert elevation.vrt elevation.tif
