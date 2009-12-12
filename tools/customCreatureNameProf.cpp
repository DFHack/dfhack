// Creature dump

#include <iostream>
#include <sstream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFMemInfo.h>

template <typename T>
void print_bits ( T val, std::ostream& out )
{
    T n_bits = sizeof ( val ) * CHAR_BIT;
    
    for ( unsigned i = 0; i < n_bits; ++i ) {
        out<< !!( val & 1 ) << " ";
        val >>= 1;
    }
}
vector <string> buildingtypes;
map<string, vector<string> > names;
uint32_t numCreatures;
vector<t_matgloss> creaturestypes;
void printDwarves(DFHack::API & DF)
{
    int dwarfCounter = 0;
    for(uint32_t i = 0; i < numCreatures; i++)
    {
        t_creature temp;
        DF.ReadCreature(i, temp);
        string type = creaturestypes[temp.type].id;
        if(type == "DWARF" && !temp.flags1.bits.dead && !temp.flags2.bits.killed){
            cout << i << ":";
            if(temp.nick_name[0])
            {
                cout << temp.nick_name;
            }
            else{
                cout << temp.first_name;
            }
            string transName = DF.TranslateName(temp.last_name,names,creaturestypes[temp.type].id);
            cout << " " << temp.custom_profession; //transName;
            if(dwarfCounter%3 != 2){
                cout << '\t';
            }
            else{
                cout << endl;
            }
            dwarfCounter++;
        }
    }
}

bool getDwarfSelection(DFHack::API & DF, t_creature & toChange,string & changeString, string & commandString,int & eraseAmount,int &dwarfNum,bool &isName)
{
    DF.ForceResume();
    static string lastText;
    bool dwarfSuccess = false;
    
    while(!dwarfSuccess)
    {
        string input;
        cout << "\nSelect Dwarf to Change or q to Quit" << endl;
        getline (cin, input);
        if(input == "q")
        {
            return false;
        }
        else if(input == "r")
        {
            DF.Suspend();
            printDwarves(DF);
            DF.Resume();
        }
        else if(!input.empty())
        {
            int num;
            stringstream(input) >> num;//= atol(input.c_str());
            DF.Suspend();
            dwarfSuccess = DF.ReadCreature(num,toChange);
            DF.Resume();
            string type = creaturestypes[toChange.type].id;
            if(type != "DWARF"){
                dwarfSuccess = false;
            }
            else
            {
                dwarfNum = num;
            }
        }
    }
    bool changeType = false;
    while(!changeType)
    {
        string input;
        cout << "\n(n)ickname or (p)rofession?" << endl;
        getline(cin, input);
        if(input == "q"){
            return false;
        }
        if(input == "n"){
            commandString = "pzyn";
            eraseAmount = string(toChange.nick_name).length();
            changeType = true;
            isName = true;
        }
        else if(input == "p"){
            commandString = "pzyp";
            eraseAmount = string(toChange.custom_profession).length();
            changeType = true;
            isName = false;
        }
    }
    bool changeValue = false;
    while(!changeValue)
    {
        string input;
        cout << "value to change to?" << endl;
        getline(cin, input);
        if(input == "q"){
            return false;
        }
        if(!lastText.empty() && input.empty()){
            changeValue = true;
        }
        else if( !input.empty()){
            lastText = input;
            changeValue = true;
        }
    }
    changeString = lastText;
    return true;
}
bool waitTillChanged(DFHack::API &DF, int creatureToCheck, string changeValue, bool isName)
{
    DF.Suspend();
    t_creature testCre;
    DF.ReadCreature(creatureToCheck,testCre);
    int tryCount = 0;
    if(isName)
    {
        while(testCre.nick_name != changeValue && tryCount <50)
        {
            DF.TypeSpecial(WAIT,1,100);
            DF.Suspend();
            DF.ReadCreature(creatureToCheck,testCre);
            tryCount++;
        }
    }
    else
    {
        while(testCre.custom_profession != changeValue && tryCount < 50)
        {
            DF.TypeSpecial(WAIT,1,100);
            DF.Suspend();
            DF.ReadCreature(creatureToCheck,testCre);
            tryCount++;
        }
    }
    if(tryCount >= 50){
        cerr << "Something went wrong, make sure that DF is at the correct screen";
        return false;
    }
    return true;
}
bool waitTillScreenState(DFHack::API &DF, string screenState)
{
    t_viewscreen current;
    DF.Suspend();
    DF.ReadViewScreen(current);
    int tryCount = 0;
    while(buildingtypes[current.type] != screenState && tryCount < 50){
        DF.TypeSpecial(WAIT,1,100);
        DF.Suspend();
        DF.ReadViewScreen(current);
        tryCount++;
    }
    if(tryCount >= 50){
        cerr << "Something went wrong, DF at " << buildingtypes[current.type] << endl;
        return false;
    }
    return true;
}

bool waitTillCursorState(DFHack::API &DF, bool On)
{
    int32_t x,y,z;
    int tryCount = 0;
    DF.Suspend();
    bool cursorResult = DF.getCursorCoords(x,y,z);
    while(tryCount < 50 && On && !cursorResult || !On && cursorResult)
    {
        DF.TypeSpecial(WAIT,1,100);
        tryCount++;
        DF.Suspend();
        cursorResult = DF.getCursorCoords(x,y,z);
    }
    if(tryCount >= 50)
    {
        cerr << "Something went wrong, cursor at x: " << x << " y: " << y << " z: " << z << endl;
        return false;
    }
    return true;
}
int main (void)
{   
    DFHack::API DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    //Need buildingtypes for the viewscreen vtables, this really should not be just buildings, as viewscreen and items both use the same interface
    uint32_t numBuildings = DF.InitReadBuildings(buildingtypes);

    DFHack::memory_info mem = DF.getMemoryInfo();
    // get stone matgloss mapping
    if(!DF.ReadCreatureMatgloss(creaturestypes))
    {
        cerr << "Can't get the creature types." << endl;
        return 1; 
    }
   
    DF.InitReadNameTables(names);
    numCreatures = DF.InitReadCreatures();
    DF.InitViewAndCursor();
    DF.Suspend();
    printDwarves(DF);
    t_creature toChange;
    string changeString,commandString;
    int eraseAmount;
    int toChangeNum;
    bool isName;
    while(getDwarfSelection(DF,toChange,changeString,commandString,eraseAmount,toChangeNum,isName))
    {
        bool completed = false;
        int32_t x,y,z;
        DF.Suspend();
        DF.getCursorCoords(x,y,z);
        if(x == -30000)// cursor not displayed
        { 
            DF.TypeStr("v");
        }
        if(waitTillCursorState(DF,true))
        {
            DF.Suspend();
            DF.setCursorCoords(toChange.x, toChange.y,toChange.z);
            vector<uint32_t> underCursor;
            while(!DF.getCurrentCursorCreatures(underCursor))
            {
                DF.TypeSpecial(WAIT,1,100);
                DF.Suspend();
                DF.setCursorCoords(toChange.x, toChange.y,toChange.z);
            }
            //CurrentCursorCreatures gives the creatures in the order that you see them with the 'k' cursor.  
            //The 'v' cursor displays them in the order of last, then first,second,third and so on
            //Pretty weird, but it works
            //The only place that seems to display which creature is currently selected is on the stack, whose location is likely not static, so not usable
            if(underCursor[underCursor.size()-1] != toChange.origin)
            {
                for(int i = 0;i<underCursor.size()-1;i++)
                {
                    DF.TypeStr("v",100);
                    if(underCursor[i] == toChange.origin)
                    {
                        break;
                    }
                }
            }
            DF.Resume();
            DF.TypeStr(commandString.c_str());
            if(waitTillScreenState(DF,"viewscreen_customize_unit"))
            {
                DF.TypeSpecial(BACK_SPACE,eraseAmount);
                if(waitTillChanged(DF,toChangeNum,"",isName))
                {
                    DF.TypeStr(changeString.c_str());
                    if(waitTillChanged(DF,toChangeNum,changeString,isName))
                    {
                        DF.TypeSpecial(ENTER);
                        DF.TypeSpecial(SPACE); // should take you to unit screen if everything worked
                        if(waitTillScreenState(DF,"viewscreen_unit"))
                        {
                            DF.TypeSpecial(SPACE);
                            if(waitTillScreenState(DF,"viewscreen_dwarfmode"))
                            {
                                DF.TypeSpecial(SPACE);
                                if(waitTillCursorState(DF,false))
                                {
                                    completed = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        if(!completed){
            cerr << "Something went wrong, please reset DF to its original state, then press any key to continue" << endl;
            string line;
            DF.Resume();
            getline(cin, line);
        }
        DF.Suspend();
        printDwarves(DF);
    }
    return 0;
}
