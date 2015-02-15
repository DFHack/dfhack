#include "OutFile.h"
#include <stdexcept>
using namespace OutFile;
File::File(std::string path)
{
    //mystream.exceptions ( std::fstream::eofbit | std::fstream::failbit | std::fstream::badbit );
    mystream.open(path.c_str(),std::fstream::binary|std::ios::in|std::ios::out);


    if(mystream)
    {
        mystream.read((char*)&myhead,sizeof(myhead));
        for(unsigned i=0;i<myhead.sectioncount;i++)
        {
            Section x;
            mystream.read((char*)&x,sizeof(Section));
            sections[x.name]=x;
        }
        //std::cout<<"Sizeof:"<<sizeof(Section)<<"\n";
      /*myhead.PrintData();
      for(auto it=sections.begin();it!=sections.end();it++)
      {
          it->second.PrintData();
      }*/

    }
    else
    {
        throw std::runtime_error("Error opening file!");
    }
}
Section &File::GetSection(std::string name)
{
    return sections[name];
}
void File::GetText(char *ptr)
{
    Section &s=GetSection(".text");
    mystream.seekg(s.start);

    mystream.read(ptr,s.size);
}
size_t File::GetTextSize()
{
    Section &s=GetSection(".text");
    return s.size;
}
void File::PrintRelocations()
{
    for(auto it=sections.begin();it!=sections.end();it++)
      {
          std::cout<<it->first<<":\n";
          for(unsigned i=0;i<it->second.numRel;i++)
          {
              Relocation r;
              mystream.seekg(it->second.ptrRel+10*i);
              mystream.read((char*)&r,10);
              std::cout<<r.ptr<<" -- "<<r.tblIndex<<":"<</*symbols[r.tblIndex].name<<*/" type:"<<r.type<<"\n";
          }
      }
}
void File::PrintSymbols()
{

    std::cout<<"Sizeof symbol:"<<sizeof(Symbol)<<std::endl;
    std::cout<<"Symbol count:"<<myhead.symbolcount<<std::endl;
    for(unsigned i=0;i<myhead.symbolcount;i++)
    {
        mystream.seekg(myhead.symbolptr+i*18);
        Symbol s;
        std::cout<<i<<"\t";
        s.Read(mystream,myhead.symbolptr+18*myhead.symbolcount);

        //mystream.read((char*)&s,sizeof(Symbol));
        s.PrintData();
        symbols.push_back(s);
        if(s.auxsymbs>0)
        {
            i+=s.auxsymbs;
        }
    }

}
void File::LoadSymbols()
{
    symbols.clear();
    for(unsigned i=0;i<myhead.symbolcount;i++)
    {
        mystream.seekg(myhead.symbolptr+i*18);
        Symbol s;
        s.Read(mystream,myhead.symbolptr+18*myhead.symbolcount);
        symbols.push_back(s);
        if(s.auxsymbs>0)
        {
            i+=s.auxsymbs;
        }
    }
}
File::~File()
{

}
