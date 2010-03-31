# -*- coding: utf-8 -*-
import pydfhack
DF = pydfhack.API("Memory.xml")

if DF.Attach():
  success,stones = DF.ReadStoneMatgloss()
  if success:
    print "Dumping all stone"
    for matgloss in stones:
      print "ID %s, name %s" % (matgloss.id, matgloss.name)
  DF.Detach()
