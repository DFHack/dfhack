// Hotkey and Note Dump
// Or Hot Keynote Dump? :P
#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFContextManager.h>
#include <DFContext.h>
#include <DFMemInfo.h>
#include <modules/Position.h>


int main (void)
{
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
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
    
    DFHack::memory_info * mem = DF->getMemoryInfo();
    DFHack::Position * Pos = DF->getPosition();
    // get stone matgloss mapping
    /*
    uint32_t numNotes;
    if(!DF.InitReadNotes(numNotes))
    {
        cerr << "Can't get notes" << endl;
        return 1;
    }
    */
    /*
    cout << "Notes" << endl;
    for(uint32_t i = 0; i < numNotes; i++)
    {
        DFHack::t_note temp;
        DF.ReadNote(i,temp);
        cout << "x: " << temp.x << "\ty: " << temp.y << "\tz: " << temp.z <<
            "\tsymbol: " << temp.symbol << "\tfg: " << temp.foreground << "\tbg: " << temp.background <<
            "\ttext: " << temp.name << endl;
    }
    */
    cout << "Hotkeys" << endl;
    DFHack::t_hotkey hotkeys[NUM_HOTKEYS];
    Pos->ReadHotkeys(hotkeys);
    for(uint32_t i =0;i< NUM_HOTKEYS;i++)
    {
        cout << "x: " << hotkeys[i].x << "\ty: " << hotkeys[i].y << "\tz: " << hotkeys[i].z <<
            "\ttext: " << hotkeys[i].name << endl;
    }
    //DF.FinishReadNotes();
    DF->Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}

