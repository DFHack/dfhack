#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include <DFHack.h>
#include <dfhack/VersionInfoFactory.h>
using namespace DFHack;

int main (int numargs, const char ** args)
{
    /*
    DFHack::VersionInfoFactory * VIF = new DFHack::VersionInfoFactory("Memory.xml");
    for(int i = 0; i < VIF->versions.size(); i++)
    {
        cout << VIF->versions[i]->PrintOffsets();
    }
    */
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();
    try
    {
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    cout << DF->getMemoryInfo()->PrintOffsets();
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    //delete VIF;
    return 0;
}
