import sys
import pydfapi

df = pydfapi.API("Memory.xml")

if not df.Attach():
    print "Unable to attach!"
    print "Press any key to continue"
    raw_input()
    sys.exit(1)

pos = df.position

print "view coords:  %s" % (pos.view_coords,)
print "cursor coords:  %s" % (pos.cursor_coords,)
print "window size:  %s" % (pos.window_size,)

if not df.Detach():
    print "Unable to detach!"

print "Done.  Press any key to continue"
raw_input()
