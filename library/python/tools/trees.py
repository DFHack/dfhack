#!/usr/bin/python
import sys
import pydfhack
from pydfhack.blocks import Point
DF = pydfhack.API("Memory.xml")

DF.Attach()

pos = DF.position
maps = DF.maps
cursor = pos.get_cursor()
refc = dict(pydfhack=pydfhack, API=pydfhack.API, DF=DF, pos=pos, maps=maps)
msize = maps.get_size()
print msize
locs = dict(pydfhack=pydfhack, API=pydfhack.API, DF=DF, pos=pos, maps=maps, msize=msize, cursor=cursor)
for z in range(msize[2]):
    trees = 0
    saplings = 0
    dead_saplings = 0
    for x in range(msize[0]):
        for y in range(msize[1]):
            p = Point(x, y, z, True)
            block = maps.get_block(point=p)
            changed = False
            if not block.tiles:
                break
            for tx in range(16):
                for ty in range(16):
                    tile = block.tiles[tx][ty]
                    if tile.ttype == 24:
                        trees += 1
                    elif tile.ttype == 231:
                        saplings += 1
                        tile.ttype = 24
                        changed = True
                    elif tile.ttype == 392:
                        dead_saplings += 1
                        tile.ttype = 24
                        changed = True
            if changed:
                block.save()
                print "Saved block {0}".format(block)
    print "Z-Level {0}: Trees {1}, Saplings {2}, Dead saplings {3}".format(z, trees, saplings, dead_saplings)
DF.Resume()
DF.Detach()
