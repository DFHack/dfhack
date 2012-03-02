/*
 * dfstatus.cpp
*/

#include <curses.h>

#ifndef LINUX_BUILD
    #include <windows.h>
#endif

#include <time.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
#include <cstring>
#include <string>
//#include <conio.h> //to break on keyboard input
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
using namespace std;
#include <DFHack.h>
#include <DFVector.h>
#include <extra/stopwatch.h>

WINDOW *create_newwin(int height, int width, int starty, int startx);

    int32_t drinkCount = 0;
    int32_t mealsCount = 0;
    int32_t plantCount = 0;
    int32_t fishCount = 0;
    int32_t meatCount = 0;
    int32_t logsCount = 0;
    int32_t barCount = 0;
    int32_t clothCount = 0;
    int32_t ironBars = 0;
    int32_t pigIronBars = 0;
    int32_t goldBars = 0;
    int32_t silverBars = 0;
    int32_t copperBars = 0;
    int32_t steelBars = 0;
    int32_t fuel = 0;
    uint64_t start_time = 0;
    uint64_t end_time = 0;
    uint64_t total_time = 0;

WINDOW *create_newwin(int height, int width, int starty, int startx){
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0);       /* 0, 0 gives default characters
                     * for the vertical and horizontal
                     * lines            */
    //first row
    mvwprintw(local_win,2 ,2,"Drinks     : %d", drinkCount);
    mvwprintw(local_win,4 ,2,"Meals      : %d", mealsCount);
    mvwprintw(local_win,6 ,2,"Plants     : %d", plantCount);
    mvwprintw(local_win,7 ,2,"Fish       : %d", fishCount);
    mvwprintw(local_win,8 ,2,"Meat       : %d", meatCount);
    mvwprintw(local_win,10,2,"Logs       : %d", logsCount);
    mvwprintw(local_win,12,2,"Cloth      : %d", clothCount);
    //second row
    mvwprintw(local_win,2,22,"Iron Bars     : %d", ironBars);
    mvwprintw(local_win,3,22,"Gold Bars     : %d", goldBars);
    mvwprintw(local_win,4,22,"Silver Bars   : %d", silverBars);
    mvwprintw(local_win,5,22,"Copper Bars   : %d", copperBars);
    mvwprintw(local_win,6,22,"Steel Bars    : %d", steelBars);
    mvwprintw(local_win,7,22,"Pig iron Bars : %d", pigIronBars);
    mvwprintw(local_win,9,22,"Fuel          : %d", fuel);
    total_time += end_time - start_time;
    mvwprintw(local_win,14,2,"Time: %d ms last update, %d ms total", end_time - start_time, total_time);

    wrefresh(local_win); // paint the screen and all components.

    return local_win;
}

int main()
{
    WINDOW *my_win;
    int startx, starty, width, height;

    DFHack::Process * p;
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
    DFHack::Materials * Materials;
    try{ //is DF running?
        DF = DFMgr.getSingleContext();
        DF->Attach();
        Materials = DF->getMaterials();
        Materials->ReadAllMaterials();
        DF->Resume();
    }
    catch (exception& e){
        cerr << e.what() << endl;
        return 1;
    }
    //init and Attach
    ofstream file("dfstatus_errors.txt");
    streambuf* strm_buffer = cerr.rdbuf(); // save cerr's output buffer
    cerr.rdbuf (file.rdbuf()); // redirect output into the file

    initscr(); //start curses.
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
    do{
        drinkCount = 0;
        mealsCount = 0;
        plantCount = 0;
        fishCount = 0;
        meatCount = 0;
        logsCount = 0;
        barCount = 0;
        clothCount = 0;
        ironBars = 0;
        pigIronBars = 0;
        goldBars = 0;
        silverBars = 0;
        copperBars = 0;
        steelBars = 0;
        fuel = 0;

        //FILE * pFile;
        //pFile = fopen("dump.txt","w");
        start_time = GetTimeMs64();
        if(!DF->Suspend())
        {
            break;
        }

        //DFHack::Gui * Gui = DF->getGui();

        DFHack::Items * Items = DF->getItems();
        Items->Start();

        DFHack::VersionInfo * mem = DF->getMemoryInfo();
        p = DF->getProcess();

        DFHack::OffsetGroup* itemGroup = mem->getGroup("Items");
        DFHack::DfVector <uint32_t> p_items (p, itemGroup->getAddress("items_vector"));
        uint32_t size = p_items.size();

        DFHack::dfh_item itm; //declare itm
        //memset(&itm, 0, sizeof(DFHack::dfh_item)); //seems to set every value in itm to 0

        for(unsigned int idx = 0; idx < size; idx++) //fill our item variables with this loop
        {
            Items->readItem(p_items[idx], itm);

            if (itm.base.flags.owned) //only count what we actually own.
                continue;

            string s0 = Items->getItemClass(itm.matdesc.itemType).c_str();
            string s1 = Items->getItemDescription(itm, Materials).c_str();

            if( s0 == "drink" ) {drinkCount += itm.quantity;}
            else if(s0 == "food"){mealsCount += itm.quantity;}
            else if(s0 == "plant"){plantCount += itm.quantity;}
            else if(s0 == "fish"){fishCount += itm.quantity;}
            else if(s0 == "meat"){meatCount += itm.quantity;}
            else if(s0 == "wood"){logsCount += itm.quantity;}
            else if(s0 == "cloth"){clothCount += itm.quantity;}
            else if(s0 == "bar") //need to break it down by ItemDescription to get the different types of bars.
            {
                barCount = barCount + itm.quantity;
                if(s1.find("PIG_IRON")!=string::npos){pigIronBars++;}
                else if(s1.find("IRON")!=string::npos){ironBars++;}
                else if(s1.find("GOLD")!=string::npos){goldBars++;}
                else if(s1.find("SILVER")!=string::npos){silverBars++;}
                else if(s1.find("COPPER")!=string::npos){copperBars++;}
                else if(s1.find("STEEL")!=string::npos){steelBars++;}
                else if(s1.find("COAL")!=string::npos){fuel++;}
            }
            /*if(s0 != "boulder" && s0 != "thread"){
                fprintf(pFile,"%5d: %12s - %64s - [%d]\n", idx, Items->getItemClass(itm.matdesc.itemType).c_str(), Items->getItemDescription(itm, Materials).c_str(), itm.quantity);
            }*/
        }
        DF->Resume();
        end_time = GetTimeMs64();
        //printf("%d - %d\n", (clock()/CLOCKS_PER_SEC),(clock()/CLOCKS_PER_SEC)%60);
        height = LINES;
        width  = COLS;
        starty = (LINES - height) / 2;
        startx = (COLS - width) / 2;

        my_win = create_newwin(height, width, starty, startx);

#ifdef LINUX_BUILD
        sleep(10);
#else
        Sleep(10000);
#endif

    } while(true);

    endwin();           /* End curses mode        */
    cerr.rdbuf (strm_buffer); // restore old output buffer
    file.close();

    return 0;
}