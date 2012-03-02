from context import Context, ContextManager

cm = ContextManager("Memory.xml")
df = cm.get_single_context()

df.attach()

gui = df.gui
veg = df.vegetation
mps = df.maps
mat = df.materials

x, y, z = gui.get_cursor_coords()

num_veg = veg.start()

if x == -30000:
    print "----==== Trees ====----"
    
    for i in xrange(num_veg):
        t = veg.read(i)
        
        print "%d/%d/%d, %d:%d" % (t.x, t.y, t.z, t.type, t.material)
else:
    #new method, gets the list of trees in a block.  can show farm plants
    if mps.start():
        pos_tuple = (x, y, z)
        trees = mps.read_vegetation(x / 16, y / 16, z)
        
        if trees is not None:
            for t in trees:
                if (t.x, t.y, t.z) == pos_tuple:
                    print "----==== Tree at %d/%d/%d ====----" % pos_tuple
                    print str(t)
                    break
        mps.finish()
    
    #old method, get the tree from the global vegetation vector.  can't show farm plants
    for i in xrange(num_veg):
        t = veg.read(i)
        
        if (t.x, t.y, t.z) == pos_tuple:
            print "----==== Tree at %d/%d/%d ====----" % pos_tuple
            print str(t)
            break

veg.finish()

df.detach()

print "Done.  Press any key to continue"
raw_input()