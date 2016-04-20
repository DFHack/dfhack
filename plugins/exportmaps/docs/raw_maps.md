# **Raw maps**

## What are raw maps?
Raw maps are binary files with native data from DF. A drainage raw map, for example, will contain only the
drainage value for each world position. They're not graphical images, so you can't open them with a image
viewer. To open them you need a hex viewer.

## What are they used for?
You can use them in external tools for reconstructing the DF world. If you combine elevation-raw maps
with biome-raw and vegetation-raw maps you've all the data needed for displaying the world in 3D in a basic way.

As the world is exported in embark coordinates, you get a x16 zoom respect to
the data that you can get from `exportlegends`, 
which exports it in world coordinates ( 1 world tile = 16 x 16 embark tiles)

## What is their extension?
They have a .raw extension, so a drainage raw map will be called `drainage.raw`

## What is their format?
A raw map has the following fields:

* One int32 (4 bytes) with the world witdh in embark coordinates
* One int32 (4 bytes) with the world height in embark coordinates
* A int16 (2 bytes) sequence of values. Each value corresponding to a embark coordinate,
  starting from [top,left] and ending at [bottom,right]

## Are they big?
For a 129x129 DF world, the generated raw file is about 8,5 Megabytes.

## How are they generated?
If you want to generate the drainage raw map, load a game and type the in the DFHack console:

`exportmaps -drainage-raw`

You can also chain options, so

`exportmaps -drainage-raw -elevation-raw`

will generate the drainage and the elevation raw maps.


The available maps are the following:

| Command | Map generated |
| --- | --- |
| -temperature-raw     | TEMPERATURE |
| -rainfall-raw        | RAINFALL |
| -drainage-raw        | DRAINAGE |
| -savagery-raw        | SAVAGERY |
| -volcanism-raw       | VOLCANISM |
| -vegetation-raw      | VEGETATION |
| -evilness-raw        | EVILNESS |
| -salinity-raw        | SALINITY |
| -elevation-raw       | ELEVATION |
| -elevation-water-raw | ELEVATION RESPECTING WATER |
| -biome-type-raw      | BIOME |
| -biome-region-raw    | BIOME |

As you can see with the biome-region-raw, you can create new maps different to the ones generated
by DF.

This map will have the region id of each world coordinate.



