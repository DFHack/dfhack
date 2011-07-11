// Building dump

#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
#include <stdio.h>
//using namespace std;

#define DFHACK_WANT_MISCUTILS
#include <DFHack.h>

void doWordPerLine(DFHack::Context *DF, DFHack::VersionInfo * mem,
                   DFHack::Buildings * Bld, uint32_t numBuildings,
                   const char* type, int lines)
{
    std::cout << numBuildings << std::endl;
    std::vector < uint32_t > addresses;
    for(uint32_t i = 0; i < numBuildings; i++)
    {
        DFHack::t_building temp;
        Bld->Read(i, temp);
        if(temp.type != 0xFFFFFFFF) // check if type isn't invalid
        {
            std::string typestr;
            mem->resolveClassIDToClassname(temp.type, typestr);
            std::cout << typestr << std::endl;
            if(typestr == type)
                addresses.push_back(temp.origin);
        }
        else
        {
            // couldn't translate type, print out the vtable
            std::cout << "unknown vtable: " << temp.vtable << std::endl;
        }
    }

    if (addresses.empty())
    {
        std::cout << "No buildings matching '" << type << "'" << endl;
        return;
    }

    interleave_hex(DF,addresses,lines / 4);
}

void doUnderCursor(DFHack::Context *DF, DFHack::VersionInfo * mem,
                   DFHack::Buildings * Bld, uint32_t numBuildings,
                   int32_t x, int32_t y, int32_t z)
{
    std::map <uint32_t, std::string> custom_workshop_types;
    Bld->ReadCustomWorkshopTypes(custom_workshop_types);
    
    uint32_t num_under_cursor = 0;

    for(uint32_t i = 0; i < numBuildings; i++)
    {
        DFHack::t_building temp;
        Bld->Read(i, temp);
        if(    (uint32_t)x >= temp.x1
               && (uint32_t)x <= temp.x2
               && (uint32_t)y >= temp.y1
               && (uint32_t)y <= temp.y2
               && (uint32_t)z == temp.z
          )
        {
            num_under_cursor++;
            std::string typestr;
            mem->resolveClassIDToClassname(temp.type, typestr);
            printf("Address 0x%x, type %d (%s), %d/%d/%d\n", temp.origin,
                   temp.type, typestr.c_str(), temp.x1, temp.y1, temp.z);
            printf("Material %d %d\n", temp.material.type,
                   temp.material.index);
            int32_t custom;
            if((custom = Bld->GetCustomWorkshopType(temp)) != -1)
            {
                printf("Custom workshop type %s (%d)\n",
                       custom_workshop_types[custom].c_str(), custom);
            }
            hexdump(DF, temp.origin, 34*16);
        }
    }

    if (num_under_cursor == 0)
        std::cout << "No buildings present under cursor." << endl;
}

void doListAll(DFHack::VersionInfo * mem, DFHack::Buildings * Bld,
               uint32_t numBuildings)
{
    std::cout << "Num buildings present: " << numBuildings << std::endl;
    for(uint32_t i = 0; i < numBuildings; i++)
    {
        DFHack::t_building temp;
        Bld->Read(i, temp);
        std::string typestr;
        mem->resolveClassIDToClassname(temp.type, typestr);
        printf("Address 0x%x, type %d (%s), Coord %d/%d/%d\n", temp.origin,
               temp.type, typestr.c_str(), temp.x1,temp.y1,temp.z);
    }
}

void doFinish(DFHack::Context *DF)
{
    DF->Detach();
    #ifndef LINUX_BUILD
        std::cout << "Done. Press any key to continue" << std::endl;
        cin.ignore();
    #endif
}

int main (int argc,const char* argv[])
{
    int lines = 16;
    bool word_per_line = false;
    if (argc == 3)
    {
        std::string num = argv[2]; //blah. I don't care
        std::istringstream ins; // Declare an input string stream.
        ins.str(num);        // Specify string to read.
        ins >> lines;     // Reads the integers from the string.
        word_per_line = true;
    }
    
    DFHack::ContextManager DFMgr ("Memory.xml");
    DFHack::Context *DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    
    DFHack::VersionInfo * mem = DF->getMemoryInfo();
    DFHack::Buildings * Bld = DF->getBuildings();
    DFHack::Gui * Gui = DF->getGui();
    
    uint32_t numBuildings;
    if(!Bld->Start(numBuildings))
    {
        std::cerr << "buildings not supported for this DF version" << std::endl;
        doFinish(DF);
        return 1;
    }

    if (numBuildings == 0)
    {
        cout << "No buildings on site." << endl;
        doFinish(DF);
        return 0;
    }

    if (word_per_line)
        doWordPerLine(DF, mem, Bld, numBuildings, argv[1], lines);
    else
    {
        int32_t x,y,z;
        Gui->getCursorCoords(x,y,z);

        if(x != -30000)
            doUnderCursor(DF, mem, Bld, numBuildings, x, y, z);
        else
            doListAll(mem, Bld, numBuildings);
    }

    Bld->Finish();

    doFinish(DF);

    return 0;
}
