// This is a reveal program. It reveals the map.

#include <iostream>
#include <vector>
#include <map>
using namespace std;

#include <DFHack.h>
#include <dfhack/modules/Gui.h>

#ifdef LINUX_BUILD
#include <unistd.h>
void waitmsec (int delay)
{
    usleep(delay);
}
#else
#include <windows.h>
void waitmsec (int delay)
{
    Sleep(delay);
}
#endif

struct hideblock
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint8_t hiddens [16][16];
};

int main (void)
{
    uint32_t x_max,y_max,z_max;
    DFHack::designations40d designations;
    
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
    
    DFHack::Maps *Maps =DF->getMaps();
    DFHack::Gui *Gui =DF->getGui();
    // walk the map, save the hide bits, reveal.
    cout << "Pausing..." << endl;

    // horrible hack to make sure the pause is really set
    // preblem here is that we could be 'arriving' at the wrong time and DF could be in the middle of a frame.
    // that could mean that revealing, even with suspending DF's thread, would mean unleashing hell *in the same frame* 
    // this here hack sets the pause state, resumes DF, waits a second for it to enter the pause (I know, BS value.) and suspends.
    Gui->SetPauseState(true);
    DF->Resume();
    waitmsec(1000);
    DF->Suspend();

    // init the map
    if(!Maps->Start())
    {
        cerr << "Can't init map." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    cout << "Revealing, please wait..." << endl;

    Maps->getSize(x_max,y_max,z_max);
    vector <hideblock> hidesaved;

    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(Maps->isValidBlock(x,y,z))
                {
                    hideblock hb;
                    hb.x = x;
                    hb.y = y;
                    hb.z = z;
                    // read block designations
                    Maps->ReadDesignations(x,y,z, &designations);
                    // change the hidden flag to 0
                    for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
                    {
                        hb.hiddens[i][j] = designations[i][j].bits.hidden;
                        designations[i][j].bits.hidden = 0;
                    }
                    hidesaved.push_back(hb);
                    // write the designations back
                    Maps->WriteDesignations(x,y,z, &designations);
                }
            }
        }
    }
    // FIXME: force game pause here!
    DF->Detach();
    cout << "Map revealed. The game has been paused for you." << endl;
    cout << "Unpausing can unleash the forces of hell!" << endl << endl;
    cout << "Press any key to unreveal." << endl;
    cout << "Close to keep the map revealed." << endl;
    cin.ignore();
    cout << "Unrevealing... please wait." << endl;
    // FIXME: do some consistency checks here!
    DF->Attach();
    Maps = DF->getMaps();
    Maps->Start();
    for(int i = 0; i < hidesaved.size();i++)
    {
        hideblock & hb = hidesaved[i];
        Maps->ReadDesignations(hb.x,hb.y,hb.z, &designations);
        for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
        {
            designations[i][j].bits.hidden = hb.hiddens[i][j];
        }
        Maps->WriteDesignations(hb.x,hb.y,hb.z, &designations);
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
