import dfhack
x = dfhack.API("Memory.xml")
y = dfhack.MatglossVector()

if x.Attach():
  success,stones = x.ReadStoneMatgloss()
  if success:
    print "Dumping all stone"
    for matgloss in stones:
      print "ID %s, name %s" % (matgloss.id, matgloss.name)
  x.Detach()
