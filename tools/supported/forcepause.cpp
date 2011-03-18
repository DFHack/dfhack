// This forces the game to pause.

#include <iostream>
#include <vector>
#include <map>
using namespace std;

#include <DFHack.h>
#include <dfhack/modules/Gui.h>

int main (int argc, char** argv)
{
    bool quiet = false;
    for(int i = 1; i < argc; i++)
    {
        string test = argv[i];
        if(test == "-q")
        {
            quiet = true;
        }
    }

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
        if(!quiet) cin.ignore();
        #endif
        return 1;
    }

    DFHack::World *World =DF->getWorld();
    cout << "Pausing..." << endl;

    World->SetPauseState(true);
    DF->Resume();
    #ifndef LINUX_BUILD
        cout << "Done. The current game frame will have to finish first. This can take some time on bugged maps." << endl;
        if (!quiet) cin.ignore();
    #endif
    return 0;
}
