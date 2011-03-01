#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;
std::string teststr1;
    std::string * teststr2;
    std::string teststr3("test");
int main (int numargs, const char ** args)
{
    printf("std::string E : 0x%x\n", &teststr1);
    teststr1 = "This is a fairly long string, much longer than the one made by default constructor.";
    cin.ignore();
    printf("std::string L : 0x%x\n", &teststr1);
    teststr1 = "This one is shorter";
    cin.ignore();
    printf("std::string S : 0x%x\n", &teststr1);
    cin.ignore();
    teststr2 = new string();
    printf("std::string * : 0x%x\n", &teststr2);
    printf("std::string(\"test\") : 0x%x\n", &teststr3);
    cin.ignore();
    return 0;
}
