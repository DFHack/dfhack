import sys
from pydfhack import ContextManager

df_cm = ContextManager("Memory.xml")
df = df_cm.get_single_context()

if not df.attach():
    print "Unable to attach!"
    print "Press any key to continue"
    
    raw_input()
    sys.exit(1)

gui = df.gui

print "view coords:  %s" % (gui.view_coords,)
print "cursor coords:  %s" % (gui.cursor_coords,)
print "window size:  %s" % (gui.window_size,)

if not df.detach():
    print "Unable to detach!"

print "Done.  Press any key to continue"
raw_input()