import time
from context import ContextManager

class HideBlock(object):
    __slots__ = [ "x", "y", "z", "hiddens" ]
    
    def __init__(self, *args, **kwargs):
        self.x = 0
        self.y = 0
        self.z = 0
        self.hiddens = [[0 for i in xrange(16)] for j in xrange(16)]

df_cm = ContextManager("Memory.xml")
df = df_cm.get_single_context()

df.attach()

m = df.maps
w = df.world

print "Pausing..."

w.start()

#this mimics the hack in the C++ reveal tool that attempts to ensure that DF isn't in the middle of
#a frame update
w.set_pause_state(True)
df.resume()
time.sleep(1)
df.suspend()

w.finish()

m.start()

print "Revealing, please wait..."

m_x, m_y, m_z = m.size
hide_blocks = []

for x in xrange(m_x):
    for y in xrange(m_y):
        for z in xrange(m_z):
            if m.is_valid_block(x, y, z):
                hb = HideBlock()
                
                hb.x = x
                hb.y = y
                hb.z = z
                
                d = m.read_designations(x, y, z)
                
                for k_i, i in enumerate(d):
                    for k_j, j in enumerate(i):
                        hb.hiddens[k_i][k_j] = j.bits.hidden
                        
                        j.bits.hidden = 0
                
                hide_blocks.append(hb)
                
                m.write_designations(x, y, z, d)

m.finish()
df.detach()

print "Map revealed.  The game has been paused for you."
print "Unpausing can unleash the forces of hell!"
print "Press any key to unreveal."
print "Close to keep the map revealed !!FOREVER!!"

raw_input()

print "Unrevealing...please wait"

df.attach()
m = df.maps
m.start()

for h in hide_blocks:
    d = m.read_designations(h.x, h.y, h.z)
    
    for k_i, i in enumerate(h.hiddens):
        for k_j, j in enumerate(i):            
            d[k_i][k_j].bits.hidden = j
    
    m.write_designations(h.x, h.y, h.z, d)

m.finish()

print "Done. Press any key to continue"
raw_input()

df.detach()