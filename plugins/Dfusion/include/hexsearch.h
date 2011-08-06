#ifndef HEXSEARCH_H
#define HEXSEARCH_H
#include <vector>
#include "dfhack/Core.h" //for some reason process.h needs core
#include "dfhack/Process.h"

//(not yet)implemented using Boyer-Moore algorithm

class Hexsearch
{
public:
	typedef std::vector<int> SearchArgType;
	enum SearchConst  //TODO add more
	{
		ANYBYTE=0x101,DWORD_,ANYDWORD,ADDRESS
	};

	Hexsearch(const SearchArgType &args,uint64_t startpos,uint64_t endpos);
	~Hexsearch();

	void Reset(){pos_=startpos_;};
	void SetStart(uint64_t pos){pos_=pos;};

	uint64_t FindNext();
	std::vector<uint64_t> FindAll();
	
private:
	bool Compare(int a,int b);
	void ReparseArgs();
	SearchArgType args_;
	uint64_t pos_,startpos_,endpos_;
	std::vector<int> BadCharShifts,GoodSuffixShift;
	void PrepareGoodSuffixTable();
	void PrepareBadCharShift();
};

#endif