# -*- coding: utf-8 -*-
from pydfhack import API
DF = API("Memory.xml")

if DF.Attach():
  Mats = DF.materials
  stones = Mats.Read_Inorganic_Materials()
  print "Dumping all stone"
  for matgloss in stones:
    print "ID %s, name %s" % (matgloss.id, matgloss.name)
  DF.Detach()
