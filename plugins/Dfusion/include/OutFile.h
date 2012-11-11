#ifndef OUTFILE_H
#define OUTFILE_H
#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
namespace OutFile
{
struct Header
{
        unsigned short machinetype;
        unsigned short sectioncount;
        unsigned long time;
        unsigned long symbolptr;
        unsigned long symbolcount;
        unsigned short opthead;
        unsigned short flags;
     void PrintData()
    {
        std::cout<<"Symbol start:"<<symbolptr<<"\n";
    }
};
struct Section
{
    char name[8];
    unsigned long Vsize;
    unsigned long Vstart;
    unsigned long size;
    unsigned long start;
    unsigned long ptrRel;
    unsigned long ptrLine;
    unsigned short numRel;
    unsigned short numLine;
    unsigned long flags;
    void PrintData()
    {
        std::cout<<name<<" size:"<<size<<" start:"<<start<<"\n";
    }
};
struct Symbol
{

    std::string name;
    unsigned long pos;
    unsigned short sectnumb;
    unsigned short type;
    unsigned char storageclass;
    unsigned char auxsymbs;
    //char unk2[6];
    void Read(std::iostream &s,unsigned long strptr)
    {
        union
        {
            char buf[8];
            struct
            {
            unsigned long zeros;
            unsigned long strptr;
            };

        }data;

        s.read((char*)&data,8);
        s.read((char*)&pos,4);
        s.read((char*)&sectnumb,2);
        s.read((char*)&type,2);
        s.read((char*)&storageclass,1);
        s.read((char*)&auxsymbs,1);
        if(data.zeros!=0)
        {
            name=data.buf;
            name=name.substr(0,8);
        }
        else
        {
            //name="";
            //std::cout<<"Name in symbol table\n";
            char buf[256];
            s.seekg(strptr+data.strptr);
            s.get(buf,256,'\0');
            name=buf;
        }

        //s.seekp(6,std::ios::cur);
    }
    void PrintData()
    {
        std::cout<<name<<" section:"<<sectnumb<<" pos:"<<pos<<"\n";
    }
};
struct Relocation
{
    unsigned long  ptr;
    unsigned long  tblIndex;
    unsigned short type;
};
typedef std::vector<Symbol> vSymbol;
class File
{
public:
    File(std::string path);
    virtual ~File();

    void GetText(char *ptr);
    size_t GetTextSize();
    void LoadSymbols();
    const vSymbol &GetSymbols(){LoadSymbols();return symbols;};
    void PrintSymbols();
    void PrintRelocations();
protected:
private:
    typedef std::map<std::string,Section> secMap;

    secMap sections;
    vSymbol symbols;
    Section &GetSection(std::string name);

    std::fstream mystream;
    Header myhead;
   // Section Text;
    //Section Data;
   // Section Bss;
};
}
#endif // OUTFILE_H
