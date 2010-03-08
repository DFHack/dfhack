#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>


void printSettlement(DFHack::API & DF, const DFHack::t_settlement & settlement, const vector< vector<string> > &englishWords, const vector< vector<string> > &foreignWords)
{
	string genericName = DF.TranslateName(settlement.name,englishWords,foreignWords,true);
	string dwarfName = DF.TranslateName(settlement.name,englishWords,foreignWords,false);
	cout << dwarfName << " " << genericName << " " << "world x: " << settlement.world_x << " world y: " << settlement.world_y 
		<< " local_x: " << settlement.local_x1 << " local_y: " << settlement.local_y1 << " size: " << settlement.local_x2 - settlement.local_x1 << " by " << settlement.local_y2 - settlement.local_y1 << "\n";
}

int main (int argc,const char* argv[])
{
    DFHack::API DF ("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    DFHack::t_settlement current;
    uint32_t numSettlements;
    if(!DF.InitReadSettlements(numSettlements))
    {
		cerr << "Could not read Settlements" << endl;
		return 1;
	}
	vector< vector<string> > englishWords;
	vector< vector<string> > foreignWords;
    if(!DF.InitReadNameTables(englishWords,foreignWords))
    {
        cerr << "Can't get name tables" << endl;
        return 1;
    }
	cout << "Settlements\n";
	/*for(uint32_t i =0;i<numSettlements;i++){
		DFHack::t_settlement temp;
		DF.ReadSettlement(i,temp);
		printSettlement(DF,temp,englishWords,foreignWords);
	}*/
	// MSVC claims this is causing the heap to be corrupted, I think it is because the currentSettlement vector only has 1 item in it
	cout << "Current Settlement\n";
	if(DF.ReadCurrentSettlement(current))
		printSettlement(DF,current,englishWords,foreignWords);

	DF.FinishReadNameTables();
	DF.FinishReadSettlements();
	DF.Detach();
	#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
	return 0;
}