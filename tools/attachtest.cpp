// Attach test
// attachtest - 100x attach/detach, 100x reads, 100x writes

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <ctime>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

int main (void)
{
    time_t start, end;
    double time_diff;
    
    
    DFHackAPI *pDF = CreateDFHackAPI("Memory.xml");
    DFHackAPI &DF = *pDF;
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    if(!DF.Detach())
    {
        cerr << "Can't detach from DF" << endl;
        return 1;
    }
    
    // attach/detach test
    cout << "Testing attach/detach"  << endl;
    time(&start);
    bool all_ok = true;
    for (int i = 0; i < 1000; i++)
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
    DF.Detach();
    cout << "attach tests done in " << time_diff << " seconds." << endl;
    cout << "Press any key to continue" << endl;
    cin.ignore();
    
    delete pDF;
    return 0;
}