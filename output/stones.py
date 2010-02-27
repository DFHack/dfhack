import dfhack
x = dfhack.API("Memory.xml")
y = dfhack.MatglossVector()

if x.Attach():
  if x.ReadStoneMatgloss(y):
    print "Dumping all stone"
    for matgloss in y:
      print "ID %s, name %s" % (matgloss.id, matgloss.name)
  x.Detach()
