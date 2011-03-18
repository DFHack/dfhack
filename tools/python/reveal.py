from pydfhack import ContextManager, Maps

df_cm = ContextManager("Memory.xml")
df = df_cm.get_single_context()

df.attach()

m = df.maps()

m.start()

m_x, m_y, m_z = m.size

for x in xrange(m_x):
    for y in xrange(m_y):
        for z in xrange(m_z):
            if m.is_valid_block(x, y, z):
                d = m.read_designations(x, y, z)
                
                for i in d:
                    for j in i:
                        j.bits.hidden = 0
                
                m.write_designations(x, y, z, d)

m.finish()
df.detach()