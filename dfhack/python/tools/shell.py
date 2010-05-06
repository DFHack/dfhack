#!/usr/bin/python
import sys
import pydfhack
DF = pydfhack.API("Memory.xml")

DF.Attach()

pos = DF.position
maps = DF.maps
refc = dict(pydfhack=pydfhack, API=pydfhack.API, DF=DF, pos=pos, maps=maps)
cursor = pos.get_cursor()
msize = maps.get_size()
block = None
tile  = None
if cursor:
    block  = maps.get_block(point=cursor)
    if block:
        tile = block.get_tile(point=cursor)
DF.Resume()

locs = dict(pydfhack=pydfhack, API=pydfhack.API, DF=DF, pos=pos, maps=maps, msize=msize, cursor=cursor, block=block, tile=tile)

banner = """DFHack Shell\n\n"""\
         """\tpydfhack = {pydfhack}\n"""\
         """\tAPI      = {API}\n"""\
         """\tDF       = {DF}\n"""\
         """\tpos      = {pos}\n"""\
         """\tmaps     = {maps}\n"""\
         """\tmsize    = {msize}\n"""\
         """\tcursor   = {cursor}\n"""\
         """\tblock    = {block}\n"""\
         """\ttile     = {tile}\n""".format(**locs)

from IPython.Shell import IPShellEmbed
shell = IPShellEmbed()
shell.set_banner(shell.IP.BANNER + '\n\n' + banner)
shell(local_ns=locs, global_ns={})
DF.Detach()
