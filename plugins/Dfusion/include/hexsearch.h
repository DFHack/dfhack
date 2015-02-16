#ifndef HEXSEARCH_H
#define HEXSEARCH_H
#include <vector>
#include "Core.h" //for some reason process.h needs core
#include "MemAccess.h"

//(not yet)implemented using Boyer-Moore algorithm

class Hexsearch
{
public:
    typedef std::vector<int> SearchArgType;
    enum SearchConst  //TODO add more
    {
        ANYBYTE=0x101,DWORD_,ANYDWORD,ADDRESS
    };

    Hexsearch(const SearchArgType &args,char * startpos,char * endpos);
    ~Hexsearch();

    void Reset(){pos_=startpos_;};
    void SetStart(char * pos){pos_=pos;};

    void * FindNext();
    std::vector<void *> FindAll();

private:
    bool Compare(int a,int b);
    void ReparseArgs();
    SearchArgType args_;
    char * pos_,* startpos_,* endpos_;
    std::vector<int> BadCharShifts,GoodSuffixShift;
    void PrepareGoodSuffixTable();
    void PrepareBadCharShift();
};

#endif