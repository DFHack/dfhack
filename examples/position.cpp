// Just show some position data

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <ctime>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

int main (void)
{
    DFHack::API DF("Memory.xml");
    try
    {
        DF.Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    
    if (DF.InitViewAndCursor())
    {
       int32_t x,y,z;
       if(DF.getViewCoords(x,y,z))
            cout << "view coords: " << x << "/" << y << "/" << z << endl;
       if(DF.getCursorCoords(x,y,z))
            cout << "cursor coords: " << x << "/" << y << "/" << z << endl;
    }
    else
    {
        cerr << "cursor and window parameters are unsupported on your version of DF" << endl;
    }
    
    if(DF.InitViewSize())
    {
        int32_t width,height;
        if(DF.getWindowSize(width,height))
            cout << "window size : " << width << " " << height << endl;
    }
    else
    {
        cerr << "view size is unsupported on your version of DF" << endl;
    }
    
    if(!DF.Detach())
    {
        cerr << "Can't detach from DF" << endl;
    }
    
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
