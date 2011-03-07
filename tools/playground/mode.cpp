#include <iostream>
using namespace std;

#include <DFHack.h>
using namespace DFHack;

void printCurrentModes(t_gamemodes gm)
{
	cout << "Current game mode:\t" << gm.game_mode << " (";
	switch(gm.game_mode)
	{
	case GM_Fort:
		cout << "Fortress)" << endl;
		break;
	case GM_Adventurer:
		cout << "Adventurer)" << endl;
		break;
	case GM_Menu:
		cout << "Menu)" << endl;
		break;
	case GM_Arena:
		cout << "Arena)" << endl;
		break;
	case GM_Arena_Assumed:
		cout << "Arena - Assumed)" << endl;
		break;
	case GM_Kittens:
		cout << "Kittens!)" << endl;
		break;
	case GM_Worldgen:
		cout << "Worldgen)" << endl;
		break;
	}
	cout << "Current control mode:\t" << gm.control_mode << " (";
	switch (gm.control_mode)
	{
	case CM_Managing:
		cout << "Managing)" << endl;
		break;
	case CM_DirectControl:
		cout << "Direct Control)" << endl;
		break;
	case CM_Kittens:
		cout << "Kittens!)" << endl;
		break;
	case CM_Menu:
		cout << "Menu)" << endl;
		break;
	}
}

int main(int argc, char **argv)
{
	string command = "";
	DFHack::ContextManager DFMgr("Memory.xml");
	DFHack::Context *DF = DFMgr.getSingleContext();
	try
	{
		DF->Attach();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		#ifndef LINUX_BUILD
			cin.ignore();
		#endif
		return 1;
	}
	World *world = DF->getWorld();
	world->Start();
	bool end = false;
	t_gamemodes gm;
	while(!end)
	{
		DF->Suspend();
		world->ReadGameMode(gm);
		DF->Resume();
		printCurrentModes(gm);
		
		cout << "\nOptions:" << endl 
			 << "'c' to change modes" << endl
			 << "'q' to quit" << endl
			 << "anything else to refresh" << endl
			 << ">";
		getline(cin, command);
		if (command=="c")
		{
			cout << "\n\tPossible choices:" << endl
				 << "Game Modes:\t\tControl Modes:" << endl
				 << "0 = Fortress\t\t0 = Managing" << endl
				 << "1 = Adventurer\t\t1 = Direct Control" << endl
				 << "2 = Legends\t\t2 = Kittens!" << endl
				 << "3 = Menu\t\t3 = Menu" << endl
				 << "4 = Arena" << endl
				 << "5 = Arena - Assumed" << endl
				 << "6 = Kittens!" << endl
				 << "7 = Worldgen" << endl << endl;
			uint32_t newgm=99, newcm=99;
			while (newgm>GM_MAX)
			{
				cout << "Enter new game mode: ";
				cin >> newgm;
			}
			while (newcm>CM_MAX)
			{
				cout << "Enter new control mode: ";
				cin >> newcm;
			}
			gm.game_mode = (GameMode)newgm;
			gm.control_mode = (ControlMode)newcm;
			DF->Suspend();
			world->WriteGameMode(gm);
			DF->Resume();
			cout << endl;
		}
		else if (command == "q")
		{
			end = true;
		}
	}
}