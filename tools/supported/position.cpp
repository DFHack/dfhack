// Just show some position data

#include <iostream>
#include <climits>
#include <vector>
#include <ctime>
using namespace std;

#include <DFHack.h>

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
	
    DFHack::Position * Position = 0;
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
        Position = DF->getPosition();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    if (Position)
    {
       int32_t x,y,z;
       int32_t width,height;
       
       if(Position->getViewCoords(x,y,z))
            cout << "view coords: " << x << "/" << y << "/" << z << endl;
       if(Position->getCursorCoords(x,y,z))
            cout << "cursor coords: " << x << "/" << y << "/" << z << endl;
       if(Position->getWindowSize(width,height))
           cout << "window size : " << width << " " << height << endl;
    }
    else
    {
        cerr << "cursor and window parameters are unsupported on your version of DF" << endl;
    }
    
    if(!DF->Detach())
    {
        cerr << "Can't detach from DF" << endl;
    }
    
    #ifndef LINUX_BUILD
	if(!quiet)
	{
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
	}
    #endif
    return 0;
}
