#include <idc.idc>

static main(void)
{
  auto SearchString;
  auto searchStart;
  auto searchTest;
  auto occurances;
  auto szFilePath,hFile;
  auto strSize;
  auto vTablePtr;
  auto vTableLoc;
  auto myString;
  auto nextAddress;
  auto byteVal;
  occurances = 0;
  searchStart = 0;

  SearchString = AskStr("", "What vtable binary to search?");
  szFilePath = AskFile(1, "*.txt", "Select output dump file:");
  hFile = fopen(szFilePath, "wb");
    Message("Scanning...");
    searchStart = FindBinary(searchStart, SEARCH_DOWN, SearchString);
    while(searchStart != BADADDR){
        MakeStr(searchStart-2, BADADDR);
        myString = GetString(searchStart-2,-1,GetStringType(searchStart-2));
        strSize = strlen(myString);
        nextAddress = searchStart-2+strSize+1;
        byteVal = Byte(nextAddress);
        while(byteVal == 0){
            nextAddress++;
            byteVal = Byte(nextAddress);
        }
        MakeDword(nextAddress);
        vTableLoc = FindBinary(141301056,SEARCH_DOWN, form("%X", nextAddress));
        MakeDword(nextAddress+4);
        fprintf(hFile,"%a\t%s\n",vTableLoc,myString);
        searchStart = FindBinary(searchStart+1, SEARCH_DOWN, SearchString);
        occurances++;
    }
    fclose(hFile);
    Message("Found %i Occurances",occurances);
}