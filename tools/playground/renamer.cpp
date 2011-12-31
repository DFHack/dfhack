// Creature dump

#include <iostream>
#include <sstream>
#include <climits>
#include <vector>
using namespace std;

#define DFHACK_WANT_MISCUTILS
#include <DFHack.h>
#include <modules/Materials.h>
#include <modules/Units.h>
#include <modules/Translation.h>

vector< vector<string> > englishWords;
vector< vector<string> > foreignWords;
uint32_t numCreatures;
vector<DFHack::t_matgloss> creaturestypes;

void printDwarves(DFHack::ContextManager & DF)
{
    int dwarfCounter = 0;
    DFHack::Creatures * c = DF.getCreatures();
    for (uint32_t i = 0; i < numCreatures; i++)
    {
        DFHack::t_creature temp;
        c->ReadCreature(i, temp);
        string type = creaturestypes[temp.type].id;
        if (type == "DWARF" && !temp.flags1.bits.dead && !temp.flags2.bits.killed)
        {
            cout << i << ":";
            if (temp.name.nickname[0])
            {
                cout << temp.name.nickname;
            }
            else
            {
                cout << temp.name.first_name;
            }
            string transName = DF.TranslateName(temp.name,englishWords,foreignWords, false);
            cout << " " << temp.custom_profession; //transName;
            if (dwarfCounter%3 != 2)
            {
                cout << '\t';
            }
            else
            {
                cout << endl;
            }
            dwarfCounter++;
        }
    }
}

bool getDwarfSelection(DFHack::ContextManager & DF, DFHack::t_creature & toChange,string & changeString, string & commandString,int & eraseAmount,int &dwarfNum,bool &isName)
{
    static string lastText;
    bool dwarfSuccess = false;

    while (!dwarfSuccess)
    {
        string input;
        cout << "\nSelect Dwarf to Change or q to Quit" << endl;
        DF.Resume();
        getline (cin, input);
        DF.Suspend();
        if (input == "q")
        {
            return false;
        }
        else if (input == "r")
        {
            printDwarves(DF);
        }
        else if (!input.empty())
        {
            int num;
            stringstream(input) >> num;//= atol(input.c_str());
            dwarfSuccess = DF.ReadCreature(num,toChange);
            string type = creaturestypes[toChange.type].id;
            if (type != "DWARF")
            {
                dwarfSuccess = false;
            }
            else
            {
                dwarfNum = num;
            }
        }
    }
    bool changeType = false;
    while (!changeType)
    {
        string input;
        cout << "\n(n)ickname or (p)rofession?" << endl;
        getline(cin, input);
        if (input == "q")
        {
            return false;
        }
        if (input == "n")
        {
            commandString = "pzyn";
            eraseAmount = string(toChange.name.nickname).length();
            changeType = true;
            isName = true;
        }
        else if (input == "p")
        {
            commandString = "pzyp";
            eraseAmount = string(toChange.custom_profession).length();
            changeType = true;
            isName = false;
        }
    }
    bool changeValue = false;
    while (!changeValue)
    {
        string input;
        cout << "value to change to?" << endl;
        getline(cin, input);
        if (input == "q")
        {
            return false;
        }
        if (!lastText.empty() && input.empty())
        {
            changeValue = true;
        }
        else if ( !input.empty())
        {
            lastText = input;
            changeValue = true;
        }
    }
    changeString = lastText;
    return true;
}

bool waitTillChanged(DFHack::ContextManager &DF, int creatureToCheck, string changeValue, bool isName)
{
    DFHack::DFWindow * w = DF.getWindow();
    DF.Suspend();
    DFHack::t_creature testCre;
    DF.ReadCreature(creatureToCheck,testCre);
    int tryCount = 0;
    if (isName)
    {
        while (testCre.name.nickname != changeValue && tryCount <50)
        {
            DF.Resume();
            w->TypeSpecial(DFHack::WAIT,1,100);
            DF.Suspend();
            DF.ReadCreature(creatureToCheck,testCre);
            tryCount++;
        }
    }
    else
    {
        while (testCre.custom_profession != changeValue && tryCount < 50)
        {
            DF.Resume();
            w->TypeSpecial(DFHack::WAIT,1,100);
            DF.Suspend();
            DF.ReadCreature(creatureToCheck,testCre);
            tryCount++;
        }
    }
    if (tryCount >= 50)
    {
        cerr << "Something went wrong, make sure that DF is at the correct screen";
        return false;
    }
    DF.Resume();
    return true;
}


bool waitTillScreenState(DFHack::ContextManager &DF, string screenState,bool EqualTo=true)
{
    DFHack::DFWindow * w = DF.getWindow();
    DFHack::t_viewscreen current;
    DF.Suspend();
    DF.ReadViewScreen(current);
    string nowScreenState;
    DF.getMemoryInfo()->resolveClassIDToClassname(current.type,nowScreenState);
    int tryCount = 0;
    while (((EqualTo && nowScreenState != screenState) || (!EqualTo && nowScreenState == screenState)) && tryCount < 50)
    {
        DF.Resume();
        w->TypeSpecial(DFHack::WAIT,1,100);
        DF.Suspend();
        DF.ReadViewScreen(current);
        tryCount++;
    }
    if (tryCount >= 50) {
        cerr << "Something went wrong, DF at " << nowScreenState << endl;
        return false;
    }
    DF.Resume();
    return true;
}


bool waitTillCursorState(DFHack::ContextManager &DF, bool On)
{
    DFHack::DFWindow * w = DF.getWindow();
    int32_t x,y,z;
    int tryCount = 0;
    DF.Suspend();
    bool cursorResult = DF.getCursorCoords(x,y,z);
    while (tryCount < 50 && On && !cursorResult || !On && cursorResult)
    {
        DF.Resume();
        w->TypeSpecial(DFHack::WAIT,1,100);
        tryCount++;
        DF.Suspend();
        cursorResult = DF.getCursorCoords(x,y,z);
    }
    if (tryCount >= 50)
    {
        cerr << "Something went wrong, cursor at x: " << x << " y: " << y << " z: " << z << endl;
        return false;
    }
    DF.Resume();
    return true;
}


bool waitTillMenuState(DFHack::ContextManager &DF, uint32_t menuState,bool EqualTo=true)
{
    int tryCount = 0;
    DFHack::DFWindow * w = DF.getWindow();
    DF.Suspend();
    uint32_t testState = DF.ReadMenuState();
    while (tryCount < 50 && ((EqualTo && menuState != testState) || (!EqualTo && menuState == testState)))
    {
        DF.Resume();
        w->TypeSpecial(DFHack::WAIT,1,100);
        tryCount++;
        DF.Suspend();
        testState = DF.ReadMenuState();
    }
    if (tryCount >= 50)
    {
        cerr << "Something went wrong, menuState: "<<testState << endl;
        return false;
    }
    DF.Resume();
    return true;
}


bool moveToBaseWindow(DFHack::ContextManager &DF)
{
    DFHack::DFWindow * w = DF.getWindow();
    DFHack::t_viewscreen current;
    DF.ReadViewScreen(current);
    string classname;
    DF.getMemoryInfo()->resolveClassIDToClassname(current.type,classname);
    while (classname != "viewscreen_dwarfmode")
    {
        w->TypeSpecial(DFHack::F9); // cancel out of text input in names
//        DF.TypeSpecial(DFHack::ENTER); // cancel out of text input in hotkeys
        w->TypeSpecial(DFHack::SPACE); // should move up a level
        if (!waitTillScreenState(DF,classname,false)) return false; // wait until screen changes from current
        DF.ReadViewScreen(current);
        DF.getMemoryInfo()->resolveClassIDToClassname(current.type,classname);
    }
    if (DF.ReadMenuState() != 0) {// if menu state != 0 then there is a menu, so escape it
        w->TypeSpecial(DFHack::F9);
        w->TypeSpecial(DFHack::ENTER); // exit out of any text prompts
        w->TypeSpecial(DFHack::SPACE); // go back to base state
        if (!waitTillMenuState(DF,0))return false;
    }
    DF.Resume();
    return true;
}


bool setCursorToCreature(DFHack::ContextManager &DF)
{
    DFHack::DFWindow * w = DF.getWindow();
    int32_t x,y,z;
    DF.Suspend();
    DF.getCursorCoords(x,y,z);
    DF.Resume();
    if (x == -30000) {
        w->TypeStr("v");
        if (!waitTillCursorState(DF,true)) return false;
    }
    else { // reset the cursor to be the creature cursor
        w->TypeSpecial(DFHack::SPACE);
        if (!waitTillCursorState(DF,false)) return false;
        w->TypeStr("v");
        if (!waitTillCursorState(DF,true)) return false;
    }
    return true;
}


int main (void)
{
    DFHack::ContextManager DF("Memory.xml");
    DFHack::Creatures *c;
    DFHack::Materials *m;
    try
    {
        DF.Attach();
        c = DF.getCreatures();
        
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    
    DFHack::memory_info * mem = DF.getMemoryInfo();

    
    if (!m->ReadCreatureTypes(creaturestypes))
    {
        cerr << "Can't get the creature types." << endl;
        return 1;
    }

    DF.InitReadNameTables(englishWords,foreignWords);
    c->Start(numCreatures);
    // DF.InitViewAndCursor();
    DFHack::Process * p = DF.getProcess();
    DFHack::DFWindow * w = DF.getWindow();

    DFHack::t_creature toChange;
    string changeString,commandString;
    int eraseAmount;
    int toChangeNum;
    bool isName;
    bool useKeys = true;
    string input2;

    // use key event emulation or direct writing?
    cout << "\nUse \n1:Key simulation\n2:Direct Writing" << endl;
    getline(cin,input2);
    if (input2 == "1")
    {
        useKeys = true;
    }
    else {
        useKeys = false;
    }
    printDwarves(DF);

    while (getDwarfSelection(DF,toChange,changeString,commandString,eraseAmount,toChangeNum,isName))
    {
        // limit length, DF doesn't accept input after this point
        if (changeString.size() > 39)
        {
            changeString.resize(39);
        }
start:
        bool completed = false;
        if (useKeys) {
            if (moveToBaseWindow(DF) && setCursorToCreature(DF))
            {
                DF.Suspend();
                DF.setCursorCoords(toChange.x, toChange.y,toChange.z);
                uint32_t underCursor;
                DF.getCurrentCursorCreature(underCursor);
                while (underCursor != toChangeNum)
                {
                    DF.Resume();
                    w->TypeStr("v",100);
                    DF.Suspend();
                    DF.setCursorCoords(toChange.x, toChange.y,toChange.z);
                    DF.ReadCreature(toChangeNum,toChange);
                    DF.getCurrentCursorCreature(underCursor);
                }
                /*//CurrentCursorCreatures gives the creatures in the order that you see them with the 'k' cursor.
                //The 'v' cursor displays them in the order of last, then first,second,third and so on
                //Pretty weird, but it works
                //The only place that seems to display which creature is currently selected is on the stack, whose location is likely not static, so not usable
                if (underCursor[underCursor.size()-1] != toChange.origin)
                {
                    for (int i = 0;i<underCursor.size()-1;i++)
                    {
                        DF.Resume();
                        w->TypeStr("v",100);
                        if (underCursor[i] == toChange.origin)
                        {
                            break;
                        }
                    }
                }*/
                DF.Resume();
                w->TypeStr(commandString.c_str());
                if (waitTillScreenState(DF,"viewscreen_customize_unit"))
                {
                    DF.Resume();
                    w->TypeSpecial(DFHack::BACK_SPACE,eraseAmount);
                    if (waitTillChanged(DF,toChangeNum,"",isName))
                    {
                        DF.Resume();
                        w->TypeStr(changeString.c_str());
                        if (waitTillChanged(DF,toChangeNum,changeString,isName))
                        {
                            DF.Resume();
                            w->TypeSpecial(DFHack::ENTER);
                            w->TypeSpecial(DFHack::SPACE); // should take you to unit screen if everything worked
                            if (waitTillScreenState(DF,"viewscreen_unit"))
                            {
                                DF.Resume();
                                w->TypeSpecial(DFHack::SPACE);
                                if (waitTillScreenState(DF,"viewscreen_dwarfmode"))
                                {
                                    DF.Resume();
                                    w->TypeSpecial(DFHack::SPACE);
                                    if (waitTillCursorState(DF,false))
                                    {
                                        completed = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (!completed) {
                cerr << "Something went wrong, please reset DF to its original state, then press any key to continue" << endl;
                goto start;
            }
        }
        else
        {
            // will only work with the shm probably should check for it, but I don't know how,
            // I have the writeString function do nothing for normal mode
            if (commandString == "pzyn") // change nickname
            {
                try
                {
                    uint32_t nickname = mem->getOffset("creature_name") + mem->getOffset("name_nickname");
                    p->writeSTLString(toChange.origin+nickname,changeString);
                }
                catch (DFHack::Error::AllMemdef&)
                {
                    cerr << "Writing creature nicknames unsupported in this version!" << endl;
                }
            }
            else
            {
                try
                {
                    uint32_t custom_prof = mem->getOffset("creature_custom_profession");
                    p->writeSTLString(toChange.origin+custom_prof,changeString);
                }
                catch (DFHack::Error::AllMemdef&)
                {
                    cerr << "Writing creature custom profession unsupported in this version!" << endl;
                }

            }
        }
        DF.Suspend();
        printDwarves(DF);
    }
    return 0;
}
