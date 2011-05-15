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
#include "termutil.h"

int main (int numargs, const char ** args)
{
    bool temporary_terminal = TemporaryTerminal();
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
        if(temporary_terminal)
            cin.ignore();
        return 1;
    }
    cout << DF->getMemoryInfo()->PrintOffsets();
    if(temporary_terminal)
    {
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    }
    //delete VIF;
    return 0;
}
