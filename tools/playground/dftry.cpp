#include <iostream>
#include <fstream>
#include <sstream>
#include <DFHack.h>

using namespace std;

bool getNumber (string prompt, int & output, int def, bool pdef = true)
{
    cout << prompt;
    if(pdef)
        cout << ", default=" << def << endl;
    while (1)
    {
        string select;
        cout << ">>";
        std::getline(cin, select);
        if(select.empty())
        {
            output = def;
            break;
        }
        else if( sscanf(select.c_str(), "%d", &output) == 1 )
        {
            break;
        }
        else
        {
            continue;
        }
    }
    return true;
}

bool splitvector(const vector<uint64_t> & in, vector<uint64_t> & out1, vector<uint64_t> & out2)
{
    size_t length = in.size();
    if(length > 1)
    {
        size_t split = length / 2;
        out1.clear();
        out2.clear();
        out1.insert(out1.begin(),in.begin(),in.begin() + split);
        out2.insert(out2.begin(),in.begin() + split,in.end());
        return true;
    }
    return false;
}
void printvector (const vector<uint64_t> &in)
{
    cout << "[" << endl;
    for(size_t i = 0; i < in.size(); i++)
    {
        cout << hex << in[i] << endl;
    }
    cout << "]" << endl;
}

bool tryvals (DFHack::Context * DF, const vector<uint64_t> &in, uint8_t current, uint8_t testing)
{
    DF->Suspend();
    DFHack::Process * p = DF->getProcess();
    for(size_t i = 0; i < in.size(); i++)
    {
        p->writeByte(in[i],testing);
    }
    DF->Resume();
    int result;
    while (!getNumber("Is the change good? 0 for no, positive for yes.",result,0));
    DF->Suspend();
    for(size_t i = 0; i < in.size(); i++)
    {
        p->writeByte(in[i],current);
    }
    DF->Resume();
    if(result)
        return true;
    return false;
}

bool dotry (DFHack::Context * DF, const vector<uint64_t> &in, uint8_t current, uint8_t testing)
{
    vector <uint64_t> a, b;
    bool found = false;
    if(!tryvals(DF,in,current,testing))
        return false;
    if(splitvector(in, a,b))
    {
        if(dotry(DF, a, current, testing))
            return true;
        if(dotry(DF, b, current, testing))
            return true;
        return false;
    }
    return false;
}

int main()
{
    vector <uint64_t> addresses;
    vector <uint64_t> a1;
    vector <uint64_t> a2;
    vector <uint8_t> values;
    string line;

    DFHack::ContextManager CM("Memory.xml");
    DFHack::Context * DF = CM.getSingleContext();
    if(!DF)
        return 1;
    DF->Resume();

    ifstream fin("siege.txt");
    if(!fin.is_open())
        return 1;
    do
    {
        getline(fin,line);
        stringstream input (line);
        input << hex;
        uint64_t value;
        input >> value;
        if(value)
        {
            cout << hex << value << endl;
            addresses.push_back(value);
        }
    } while (!fin.eof());
    int val_current,val_testing;
    while (!getNumber("Insert current value",val_current,1));
    while (!getNumber("Insert testing value",val_testing,0));
    dotry(DF, addresses,val_current,val_testing);
    return 0;
}