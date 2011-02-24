#include <iostream>
using namespace std;

#include <DFHack.h>
using namespace DFHack;

int main ()
{
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
    DFHack::Process * Process = DF->getProcess();
    DFHack::Gui * gui = DF->getGui();
    cout << Process->getPath() << endl;
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}