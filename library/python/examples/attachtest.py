import time
import math
import pydfapi

df = pydfapi.API("Memory.xml")

def test_attach():
    if not df.Attach():
        print "Unable to attach!"
        return False
    elif not df.Detach():
        print "Unable to detach!"
        return False
    else:
        return True

def suspend_test():
    print "Testing suspend/resume"
    
    df.Attach()
    
    t1 = time.time()
    
    for i in xrange(1000):
        df.Suspend()
        
        if i % 10 == 0:
            print "%i%%" % (i / 10,)
        
        df.Resume()
    
    t2 = time.time()
    
    df.Detach()
    
    print "suspend tests done in %0.9f seconds" % (t2 - t1,)

if __name__ == "__main__":
    if test_attach():
        suspend_test()
    
    print "Done.  Press any key to continue"
    raw_input()