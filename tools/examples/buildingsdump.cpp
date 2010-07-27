// Creature dump

#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
#include <stdio.h>
//using namespace std;

#define DFHACK_WANT_MISCUTILS
#include <DFHack.h>

int main (int argc,const char* argv[])
{
    int lines = 16;
    int mode = 0;
    if (argc < 2 || argc > 3)
    {
        /*
        cout << "usage:" << endl;
        cout << argv[0] << " object_name [number of lines]" << endl;
        #ifndef LINUX_BUILD
            cout << "Done. Press any key to continue" << endl;
            cin.ignore();
        #endif
        return 0;
        */
    }
    else if(argc == 3)
    {
        std::string s = argv[2]; //blah. I don't care
        std::istringstream ins; // Declare an input string stream.
        ins.str(s);        // Specify string to read.
        ins >> lines;     // Reads the integers from the string.
        mode = 1;
    }
    
    std::map <uint32_t, std::string> custom_workshop_types;
    
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
    
    DFHack::memory_info * mem = DF->getMemoryInfo();
    DFHack::Buildings * Bld = DF->getBuildings();
    DFHack::Position * Pos = DF->getPosition();
    
    uint32_t numBuildings;
    if(Bld->Start(numBuildings))
    {
        Bld->ReadCustomWorkshopTypes(custom_workshop_types);
        if(mode)
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
                    if(typestr == argv[1])
                    {
                        //cout << buildingtypes[temp.type] << " 0x" << hex << temp.origin << endl;
                        //hexdump(DF, temp.origin, 16);
                        //addresses.push_back(temp.origin);
                    }
                                        addresses.push_back(temp.origin);
                }
                else
                {
                    // couldn't translate type, print out the vtable
                    std::cout << "unknown vtable: " << temp.vtable << std::endl;
                }
            }
            interleave_hex(DF,addresses,lines / 4);
        }
        else // mode
        {
            int32_t x,y,z;
            Pos->getCursorCoords(x,y,z);
            if(x != -30000)
            {
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
                        std::string typestr;
                        mem->resolveClassIDToClassname(temp.type, typestr);
                        printf("Address 0x%x, type %d (%s), %d/%d/%d\n",temp.origin, temp.type, typestr.c_str(), temp.x1,temp.y1,temp.z);
                        printf("Material %d %d\n", temp.material.type, temp.material.index);
                        int32_t custom;
                        if((custom = Bld->GetCustomWorkshopType(temp)) != -1)
                        {
                            printf("Custom workshop type %s (%d)\n",custom_workshop_types[custom].c_str(),custom);
                        }
                        hexdump(DF,temp.origin,120);
                    }
                }
            }
            else
            {
                std::cout << numBuildings << std::endl;
                for(uint32_t i = 0; i < numBuildings; i++)
                {
                    DFHack::t_building temp;
                    Bld->Read(i, temp);
                    std::string typestr;
                    mem->resolveClassIDToClassname(temp.type, typestr);
                    printf("Address 0x%x, type %d (%s), %d/%d/%d\n",temp.origin, temp.type, typestr.c_str(), temp.x1,temp.y1,temp.z);
                }
            }
        }
        Bld->Finish();
    }
    else
    {
        std::cerr << "buildings not supported for this DF version" << std::endl;
    }

    DF->Detach();
    #ifndef LINUX_BUILD
        std::cout << "Done. Press any key to continue" << std::endl;
        cin.ignore();
    #endif
    return 0;
}
