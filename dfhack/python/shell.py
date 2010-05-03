#!/usr/bin/python
import sys
import pydfhack
from smarthttp.lib.containers import SmartDict
DF = pydfhack.API("Memory.xml")

DF.Attach()

pos = DF.position
maps = DF.maps
maps.Start()
cursor = pos.cursor_coords
msize = maps.size
block = SmartDict()
if cursor:
    block.coords = (cursor[0]/16, cursor[1]/16, cursor[2])
    block.tiles  = maps.Read_Tile_Types(block.coords[0], block.coords[1], block.coords[2])
maps.Finish()
DF.Resume()

locs = dict(pydfhack=pydfhack, API=pydfhack.API, DF=DF, pos=pos, maps=maps, msize=msize, cursor=cursor, block=block)

banner = """DFHack Shell\n\n"""\
         """\tpydfhack = {pydfhack}\n"""\
         """\tAPI      = {API}\n"""\
         """\tDF       = {DF}\n"""\
         """\tpos      = {pos}\n"""\
         """\tmaps     = {maps}\n"""\
         """\tmsize    = {msize}\n"""\
         """\tcursor   = {cursor}\n"""\
         """\tblock    = {block}\n""".format(**locs)

from IPython.Shell import IPShellEmbed
shell = IPShellEmbed()
shell.set_banner(shell.IP.BANNER + '\n\n' + banner)
shell(local_ns=locs, global_ns={})

