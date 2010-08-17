import sys
from cStringIO import StringIO
import pydfapi

df = pydfapi.API("Memory.xml")

if not df.Attach():
    print "Unable to attach!\nPress any key to continue"
    raw_input()
    sys.exit(1)

pos = df.position
veg = df.vegetation
mat = df.materials

organics = mat.Read_Organic_Materials()

x, y, z = pos.cursor_coords

num_vegs = veg.Start()

if x == -30000:
    print "----==== Trees ====----"

    for i in xrange(num_vegs):
        tree = veg.Read(i)

        t_x, t_y, t_z = tree["position"]

        print "%f/%f/%f, %f:%f" % (t_x, t_y, t_z, tree["type"], tree["material"])
else:
    print "----==== Tree at %i/%i/%i" % (x, y, z)

    for i in xrange(num_vegs):
        tree = veg.Read(i)

        t_x, t_y, t_z = tree["position"]
        t_type = tree["address"]

        if t_x == x and t_y == y and t_z == z:
            s = StringIO()
            
            s.write("%f:%f = " % (tree["type"], tree["material"]))

            if t_type in (1, 3):
                s.write("near-water ")

            s.write("%i " % (organics[tree["material"]]["id"]),)

            if t_type in (0, 1):
                s.write("tree\n")
            elif t_type in (2, 3):
                s.write("shrub\n")

            print s.getvalue()
            print "Address: 0x%x" % (tree["address"],)

            break

veg.Finish()

if not df.Detach():
    print "Unable to detach!"

print "Done.  Press any key to continue"
raw_input()
