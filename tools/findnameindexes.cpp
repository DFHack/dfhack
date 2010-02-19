// Map cleaner. Removes all the snow, mud spills, blood and vomit from map tiles.

#include <iostream>
#include <iomanip>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

int main (void)
{
	DFHack::API DF ("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
	map< string, vector<string> > names;
	if(!DF.InitReadNameTables(names))
	{
		cerr << "Could not get Names" << endl;
		return 1;
	}
	string input;
	DF.ForceResume();
    cout << "\nSelect Name to search or q to Quit" << endl;
    getline (cin, input);
	while(input != "q"){
		for( map< string, vector<string> >::iterator it = names.begin();it != names.end(); it++){
			for(uint32_t i = 0; i < it->second.size(); i++){
				uint32_t found = input.find(it->second[i]);
				if(found != string::npos){
					cout << it->first << " " << it->second[i] << " " << setfill('0') << setw(8) << hex << i << endl;
				}
			}
		}
		DF.Resume();
		getline(cin,input);
	}
    DF.Detach();
	DF.FinishReadNameTables();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
