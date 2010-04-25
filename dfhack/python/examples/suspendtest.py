import pydfapi

if __name__ == "__main__":
    df = pydfapi.API("Memory.xml")
    
    if not df.Attach():
        print "Unable to attach!"
        return False
    
    print "Attached, DF should be suspended now"
    raw_input()
    
    df.Resume()
    
    print "Resumed, DF should be running"
    raw_input()
    
    df.Suspend()
    
    print "Suspended, DF should be suspended now"
    raw_input()
    
    df.Resume()
    
    print "Resumed, testing ForceResume.  Suspend using SysInternals Process Explorer"
    raw_input()
    
    df.Force_Resume()
    
    print "ForceResumed.  DF should be running."
    raw_input()
    
    if not df.Detach():
        print "Can't detach from DF"
        return False
    
    print "Detached, DF should be running again"
    raw_input()