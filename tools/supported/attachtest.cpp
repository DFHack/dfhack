// Attach test
// attachtest - 1000x suspend/resume

#include <iostream>
#include <climits>
#include <vector>
#include <ctime>
using namespace std;
#include <DFHack.h>

int main (void)
{
    time_t start, end;
    double time_diff;
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context* DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
        DF->Detach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
/*
    // attach/detach test
    cout << "Testing attach/detach"  << endl;
    time(&start);
    bool all_ok = true;
    for (int i = 0; i < 100; i++)
    {
        cout << "Try " << i << endl;
        if(DF.Attach())
        {
            if(DF.Detach())
            {
                continue;
            }
            else
            {
                cout << "cycle " << i << ", detach failed" << endl;
                all_ok = false;
            }
        }
        else
        {
            cout << "cycle " << i << ", attach failed" << endl;
            all_ok = false;
        }
        cout << endl;
    }
    if(!all_ok)
    {
        cerr << "failed to attach or detach in cycle! exiting" << endl;
        return 1;
    }
    time(&end);
    
    time_diff = difftime(end, start);
    cout << "attach tests done in " << time_diff << " seconds." << endl;
    */
    cout << "Testing suspend/resume"  << endl;
    DF->Attach();
    time(&start);
    for (int i = 0; i < 1000; i++)
    {
        DF->Suspend();
        if(i%10 == 0)
            cout << i / 10 << "%" << endl;
        DF->Resume();
    }
    time(&end);
    DF->Detach();
    time_diff = difftime(end, start);
    cout << "suspend tests done in " << time_diff << " seconds." << endl;
    
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
