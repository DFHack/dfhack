#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <cstdlib>
#include <limits>
using namespace std;

#include <conio.h>

#include <DFHack.h>
#include <DFTileTypes.h>
#include <extra/MapExtras.h>


//Globals
DFHack::Context *DF;
DFHack::Maps *maps;
DFHack::Gui *gui;
DFHack::Materials *mats;



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int main (void)
{
    int32_t 
		cx,cy,z,	//cursor coords
		tx,ty,		//tile coords within block
		bx,by;		//block coords
    //DFHack::designations40d designations;
    //DFHack::tiletypes40d tiles;
    //DFHack::t_temperatures temp1,temp2;
    uint32_t x_max,y_max,z_max;

    DFHack::ContextManager DFMgr("Memory.xml");
    try
    {

        DF=DFMgr.getSingleContext();
        DF->Attach();
        maps = DF->getMaps();
        maps->Start();
        maps->getSize(x_max,y_max,z_max);
        gui = DF->getGui();
		mats = DF->getMaterials();
		mats->ReadAllMaterials();
		if(!mats->ReadInorganicMaterials())
		{
			printf("Error: Could not load materials!\n");
			#ifndef LINUX_BUILD
				cin.ignore();
			#endif
			return 1;
		}
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }



    bool end = false;
    cout << "Welcome to the Vein Swap tool.\nType 'help' or ? for a list of available commands, 'q' to quit" << endl;
    string mode = "";
    string command = "";

	while(!end)
	{
        DF->Resume();

		cout << endl << ":";
		getline(cin, command);
		int ch = command[0];
		if(command.length()<=0) ch=0;
		if( ((int)command.find("help")) >=0 ) ch='?';  //under windows, find was casting unsigned!

		//Process user command.
		switch(ch)
        {
		case 'h':
		case '?':
            cout << "" << endl
                 << "Commands:" << endl
                 << "p                       - print vein at cursor" << endl
                 << "m                       - print all inorganic material types" << endl
                 << "r MatTo                 - replace the vein at cursor with MatTo" << endl
                 << "R Percent MatFrom MatTo - replace Percent of MatFrom veins with MatTo veins" << endl
                 << "help OR ?               - print this list of commands" << endl
                 << "q                       - quit" << endl
                 << endl
                 << "Usage:\n\t" << endl;
			break;
		case 'p':
		case 10:
		case 13:
		case 0:
			{
			//Print current cursor
			mats->ReadAllMaterials();
			
			if(!maps->Start())
            {
                cout << "Can't see any DF map loaded." << endl;
                break;
            }
			if(!gui->getCursorCoords(cx,cy,z))
            {
                cout << "Can't get cursor coords! Make sure you have a cursor active in DF." << endl;
                break;
            }
            //cout << "cursor coords: " << x << "/" << y << "/" << z << endl;
			tx=cx%16; ty=cy%16;
			bx=cx/16; by=cy/16;
			DFHack::DFCoord xyz(cx,cy,z);

			printf("Cursor[%d,%d,%d] Block(%d,%d) Tile(%d,%d)\n", cx,cy,z, bx,by, tx,ty );

            if(!maps->isValidBlock(bx,by,z))
            {
                cout << "Invalid block." << endl;
                break;
            }

			vector<DFHack::t_vein> veinVector;
			int found=0;
            maps->ReadVeins(bx,by,z,&veinVector);
			printf("Veins in block (%d):\n",veinVector.size());
			for(unsigned long i=0;i<veinVector.size();++i){
				found = veinVector[i].getassignment(tx,ty);
				printf("\t%c %4d %s\n", 
					(found ? '*' : ' '), 
					veinVector[i].type, 
					mats->inorganic[veinVector[i].type].id
					);
			}
			printf("Cursor is%s in vein.\n", (found?"":" not") );

			maps->Finish();
			DF->Resume();
			}
			break;
		case 'v':
			break;
		case 'm':
			break;
		case 'R':
			break;
		case 'q':
            end = true;
            cout << "Bye!" << endl;
			break;
		case 'r':
            DF->Suspend();
			do{
			//Process parameters
			long matto = atol( command.c_str()+1 );
			if( matto < 0 || matto >= (long)mats->inorganic.size() ){
				cout << "Invalid material: " << matto << endl;
				break;
			}
			
			if(!maps->Start())
            {
                cout << "Can't see any DF map loaded." << endl;
                break;
            }
            if(!gui->getCursorCoords(cx,cy,z))
            {
                cout << "Can't get cursor coords! Make sure you have a cursor active in DF." << endl;
                break;
            }
			tx=cx%16; ty=cy%16;
			bx=cx/16; by=cy/16;
			printf("Cursor[%d,%d,%d] Block(%d,%d) Tile(%d,%d)\n", cx,cy,z, bx,by, tx,ty );

            if(!maps->isValidBlock(bx,by,z))
            {
                cout << "Invalid block." << endl;
                break;
            }

			//MapExtras::MapCache MC(maps);
			//MapExtras::Block B(maps,DFHack::DFCoord(bx,by,z));

			vector<DFHack::t_vein> veinVector;
			int v=-1; //the vector pointed to by the cursor

			mats->ReadAllMaterials();

            maps->ReadVeins(bx,by,z,&veinVector);
			for(unsigned long i=0 ; v<0 && i<veinVector.size() ; ++i){
				if( veinVector[i].getassignment(tx,ty) )
					v=i;
			}
			printf("Replacing %d %s with %d %s...\n", veinVector[v].type, mats->inorganic[veinVector[v].type].id, matto, mats->inorganic[matto].id );
printf("%X\n",veinVector[v].vtable);
			vector<DFHack::t_vein> veinTable;

			veinVector[v].type = matto;
			maps->WriteVein( &veinVector[v] );


            maps->Finish();

			cout << endl << "Finished." << endl;
			}while(0);
			DF->Resume();
			break;
		default:
			cout << "Unknown command: " << command << endl;
		}//end switch

    }//end while

    DF->Detach();
    //#ifndef LINUX_BUILD
    //cout << "Done. Press any key to continue" << endl;
    //cin.ignore();
    //#endif
    return 0;
}
