# -*- coding: utf-8 -*-
import pydfhack
x = pydfhack.API("Memory.xml")
y = pydfhack.MatglossVector()

if x.Attach():
  success,stones = x.ReadStoneMatgloss()
  if success:
    print "Dumping all stone"
    for matgloss in stones:
      print "ID %s, name %s" % (matgloss.id, matgloss.name)
  x.Detach()
