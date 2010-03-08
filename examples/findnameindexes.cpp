// Map cleaner. Removes all the snow, mud spills, blood and vomit from map tiles.

#include <iostream>
#include <iomanip>
#include <integers.h>
#include <vector>
#include <algorithm>
#include <sstream>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

// returns a lower case version of the string 
string tolower (const string & s)
{
	string d (s);

	transform (d.begin (), d.end (), d.begin (), (int(*)(int)) tolower);
	return d;
}
string groupBy2(const string & s)
{
	string d;
	for(int i =2;i<s.length();i++){
		if(i%2==0)
		{
			d+= s.substr(i-2,2) + " ";
		}
	}
	d+=s.substr(s.length()-2,2);
	return(d);
}

uint32_t endian_swap(uint32_t x)
{
    x = (x>>24) | 
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
	return x;
}


int main (void)
{
	DFHack::API DF ("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
	vector< vector<string> > englishWords;
	vector< vector<string> > foreignWords;
    if(!DF.InitReadNameTables(englishWords,foreignWords))
	{
		cerr << "Could not get Names" << endl;
		return 1;
	}
	string input;
	DF.ForceResume();
    cout << "\nSelect Name to search or q to Quit" << endl;
    getline (cin, input);
	while(input != "q"){
		/*for( map< string, vector<string> >::iterator it = names.begin();it != names.end(); it++){
			for(uint32_t i = 0; i < it->second.size(); i++){
				uint32_t found = tolower(input).find(tolower(it->second[i]));
				if(found != string::npos){
					stringstream value;
					value << setfill('0') << setw(8) << hex << endian_swap(i);
					cout << it->first << " " << it->second[i] << " "  << groupBy2(value.str()) << endl;
				}
			}
		}*/
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
