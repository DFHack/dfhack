# -*- coding: utf-8 -*-
import pydfhack
DF = pydfhack._API("Memory.xml")

if DF.Attach():
  Mats = DF.materials
  stones = Mats.Read_Inorganic_Materials()
  print "Dumping all stone"
  for matgloss in stones:
    print "ID %s, name %s" % (matgloss.id, matgloss.name)
  DF.Detach()
