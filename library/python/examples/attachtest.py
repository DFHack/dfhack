import time
from pydfhack import ContextManager

df_cm = ContextManager("Memory.xml")
df = None

def test_attach():
    global df
    
    if not df:
        df = df_cm.get_single_context()
    
    if not df.attach():
        print "Unable to attach!"
        return False
    elif not df.detach():
        print "Unabled to detach!"
        return False
    else:
        return True

def suspend_test():
    global df
    
    if not df:
        df = df_cm.get_single_context()
    
    print "Testing suspend/resume"
    
    df.attach()
    
    t1 = time.time()
    
    for i in xrange(1000):
        df.suspend()
        
        if i % 10 == 0:
            print "%i%%" % (i / 10.0,)
        
        df.resume()
    
    t2 = time.time()
    
    df.detach()
    
    print "suspend test done in $0.9f seconds" % (t2 - t1)

if __name__ == "__main__":
    if test_attach():
        suspend_test()
    
    print "Done.  Press any key to continue"
    raw_input()