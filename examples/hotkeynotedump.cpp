// Hotkey and Note Dump
// Or Hot Keynote Dump? :P
#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFMemInfo.h>


int main (void)
{
    vector<DFHack::t_matgloss> creaturestypes;
    DFHack::API DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    
    DFHack::memory_info * mem = DF.getMemoryInfo();
    // get stone matgloss mapping
    uint32_t numNotes;
    if(!DF.InitReadNotes(numNotes))
    {
        cerr << "Can't get notes" << endl;
        return 1;
    }
    if(!DF.InitReadHotkeys())
    {
        cerr << "Can't get hotkeys" << endl;
        return 1;
    }
    cout << "Notes" << endl;
    for(uint32_t i = 0; i < numNotes; i++)
    {
        DFHack::t_note temp;
        DF.ReadNote(i,temp);
        cout << "x: " << temp.x << "\ty: " << temp.y << "\tz: " << temp.z <<
            "\tsymbol: " << temp.symbol << "\tfg: " << temp.foreground << "\tbg: " << temp.background <<
            "\ttext: " << temp.name << endl;
    }
    cout << "Hotkeys" << endl;
    DFHack::t_hotkey hotkeys[NUM_HOTKEYS];
    DF.ReadHotkeys(hotkeys);
    for(uint32_t i =0;i< NUM_HOTKEYS;i++)
    {
        cout << "x: " << hotkeys[i].x << "\ty: " << hotkeys[i].y << "\tz: " << hotkeys[i].z <<
            "\ttext: " << hotkeys[i].name << endl;
    }
    DF.FinishReadNotes();
    DF.Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}