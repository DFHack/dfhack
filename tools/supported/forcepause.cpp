// This forces the game to pause.

#include <iostream>
#include <vector>
#include <map>
using namespace std;

#include <DFHack.h>
#include <dfhack/modules/Gui.h>

int main (void)
{
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
            cin.ignore();
        #endif
        return 1;
    }

    DFHack::Gui *Gui =DF->getGui();
    cout << "Pausing..." << endl;

    Gui->SetPauseState(true);
    DF->Resume();
    #ifndef LINUX_BUILD
    cout << "Done. The current game frame will have to finish first. This can take some time on bugged maps." << endl;
    cin.ignore();
    #endif
    return 0;
}
