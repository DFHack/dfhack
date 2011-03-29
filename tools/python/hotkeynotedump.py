from context import Context, ContextManager

cm = ContextManager("Memory.xml")
df = cm.get_single_context()

df.attach()

gui = df.gui

print "Hotkeys"

hotkeys = gui.read_hotkeys()

for key in hotkeys:
    print "x: %d\ny: %d\tz: %d\ttext: %s" % (key.x, key.y, key.z, key.name)

df.detach()

print "Done.  Press any key to continue"
raw_input()