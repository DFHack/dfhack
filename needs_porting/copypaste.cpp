// Console version of DF copy paste, proof of concept
// By belal

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <fstream>

#define DFHACK_WANT_MISCUTILS
#define DFHACK_WANT_TILETYPES
#include <DFHack.h>
#include "modules/WindowIO.h"

using namespace DFHack;
//bool waitTillCursorState(DFHack::Context *DF, bool On);
//bool waitTillCursorPositionState(DFHack::Context *DF, int32_t x,int32_t y, int32_t z);

//change this if you are having problems getting correct results, lower if you would like to go faster
//const int WAIT_AMT = 25;

void sort(uint32_t &a,uint32_t &b)
{
    if(a > b){
        uint32_t c = b;
        b = a;
        a = c;
    }
}
void sort(int32_t &a,int32_t &b)
{
    if(a > b){
        int16_t c = b;
        b = a;
        a = c;
    }
}
void printVecOfVec(ostream &out, vector<vector<vector<string> > >vec,char sep)
{
    for(size_t k=0;k<vec.size();k++)
	{
      for(size_t i =0;i<vec[k].size();i++)
	  {
        for(size_t j=0;j<vec[k][i].size();j++)
		{
            out << vec[k][i][j];
            if(j==vec[k][i].size()-1)
            {
                out << "\n";
            }
            else
            {
                out << sep;
            }
        }
      }
      out << "#<\n";
    }
}
int main (int numargs, const char ** args)
{
    map<string, string> buildCommands;
    buildCommands["building_stockpilest"]="";
    buildCommands["building_zonest"]="";
    buildCommands["building_construction_blueprintst"]="";
    buildCommands["building_wagonst"]="";
    buildCommands["building_armor_standst"]="a";
    buildCommands["building_bedst"]="b";
    buildCommands["building_seatst"]="c";
    buildCommands["building_burial_receptaclest"]="n";
    buildCommands["building_doorst"]="d";
    buildCommands["building_floodgatest"]="x";
    buildCommands["building_floor_hatchst"]="H";
    buildCommands["building_wall_gratest"]="W";
    buildCommands["building_floor_gratest"]="G";
    buildCommands["building_vertical_barsst"]="B";
    buildCommands["building_floor_barsst"]="alt-b";
    buildCommands["building_cabinetst"]="f";
    buildCommands["building_containerst"]="h";
    buildCommands["building_shopst"]="";
    buildCommands["building_workshopst"]="";
    buildCommands["building_alchemists_laboratoryst"]="wa";
    buildCommands["building_carpenters_workshopst"]="wc";
    buildCommands["building_farmers_workshopst"]="ww";
    buildCommands["building_masons_workshopst"]="wm";
    buildCommands["building_craftdwarfs_workshopst"]="wr";
    buildCommands["building_jewelers_workshopst"]="wj";
    buildCommands["building_metalsmiths_workshopst"]="wf";
    buildCommands["building_magma_forgest"]="";
    buildCommands["building_bowyers_workshopst"]="wb";
    buildCommands["building_mechanics_workshopst"]="wt";
    buildCommands["building_siege_workshopst"]="ws";
    buildCommands["building_butchers_shopst"]="wU";
    buildCommands["building_leather_worksst"]="we";
    buildCommands["building_tanners_shopst"]="wn";
    buildCommands["building_clothiers_shopst"]="wk";
    buildCommands["building_fisheryst"]="wh";
    buildCommands["building_stillst"]="wl";
    buildCommands["building_loomst"]="wo";
    buildCommands["building_quernst"]="wq";
    buildCommands["building_kennelsst"]="k";
    buildCommands["building_kitchenst"]="wz";
    buildCommands["building_asheryst"]="wy";
    buildCommands["building_dyers_shopst"]="wd";
    buildCommands["building_millstonest"]="wM";
    buildCommands["building_farm_plotst"]="p";
    buildCommands["building_weapon_rackst"]="r";
    buildCommands["building_statuest"]="s";
    buildCommands["building_tablest"]="t";
    buildCommands["building_paved_roadst"]="o";
    buildCommands["building_bridgest"]="g";
    buildCommands["building_wellst"]="l";
    buildCommands["building_siege enginest"]="i";
    buildCommands["building_catapultst"]="ic";
    buildCommands["building_ballistast"]="ib";
    buildCommands["building_furnacest"]="";
    buildCommands["building_wood_furnacest"]="ew";
    buildCommands["building_smelterst"]="es";
    buildCommands["building_glass_furnacest"]="ek";
    buildCommands["building_kilnst"]="ek";
    buildCommands["building_magma_smelterst"]="es";
    buildCommands["building_magma_glass_furnacest"]="ek";
    buildCommands["building_magma_kilnst"]="ek";
    buildCommands["building_glass_windowst"]="y";
    buildCommands["building_gem_windowst"]="Y";
    buildCommands["building_tradedepotst"]="D";
    buildCommands["building_mechanismst"]="";
    buildCommands["building_leverst"]="Tl";
    buildCommands["building_pressure_platest"]="Tp";
    buildCommands["building_cage_trapst"]="Tc";
    buildCommands["building_stonefall_trapst"]="Ts";
    buildCommands["building_weapon_trapst"]="Tw";
    buildCommands["building_spikest"]="";
    buildCommands["building_animal_trapst"]="m";
    buildCommands["building_screw_pumpst"]="Ms";
    buildCommands["building_water_wheelst"]="Mw";
    buildCommands["building_windmillst"]="Mm";
    buildCommands["building_gear_assemblyst"]="Mg";
    buildCommands["building_horizontal_axlest"]="Mh";
    buildCommands["building_vertical_axlest"]="Mv";
    buildCommands["building_supportst"]="S";
    buildCommands["building_cagest"]="j";
    buildCommands["building_archery_targetst"]="A";
    buildCommands["building_restraintst"]="v";

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();
    
    try
    {
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
    
    DFHack::Gui *Gui = DF->getGui();
    DFHack::VersionInfo* mem = DF->getMemoryInfo();
    DFHack::Process * p = DF->getProcess();
    OffsetGroup * OG_Maps = mem->getGroup("Maps");
    OffsetGroup * OG_MapBlock = OG_Maps->getGroup("block");
    OffsetGroup * OG_LocalFt = OG_Maps->getGroup("features")->getGroup("local");
    uint32_t designations = OG_MapBlock->getOffset("designation");
    uint32_t block_feature1 = OG_MapBlock->getOffset("feature_local");
    uint32_t block_feature2 = OG_MapBlock->getOffset("feature_global");
    uint32_t region_x_offset = OG_Maps->getAddress("region_x");
    uint32_t region_y_offset = OG_Maps->getAddress("region_y");
    uint32_t region_z_offset = OG_Maps->getAddress("region_z");
    uint32_t feature1_start_ptr = OG_LocalFt->getAddress("start_ptr");
    int32_t regionX, regionY, regionZ;
    
    // read position of the region inside DF world
    p->readDWord (region_x_offset, (uint32_t &)regionX);
    p->readDWord (region_y_offset, (uint32_t &)regionY);
    p->readDWord (region_z_offset, (uint32_t &)regionZ);
    while(1){
    int32_t cx1,cy1,cz1;
    cx1 = -30000;
    while(cx1 == -30000)
    {
        DF->ForceResume();
        cout << "Set cursor at first position, then press any key";
        cin.ignore();
        DF->Suspend();
        Gui->getCursorCoords(cx1,cy1,cz1);
    }

    uint32_t tx1,ty1,tz1;
    tx1 = cx1/16;
    ty1 = cy1/16;
    tz1 = cz1;

    int32_t cx2,cy2,cz2;
    cx2 = -30000;
    while(cx2 == -30000)
    {
        DF->Resume();
        cout << "Set cursor at second position, then press any key";
        cin.ignore();
        DF->Suspend();
        Gui->getCursorCoords(cx2,cy2,cz2);
    }
    uint32_t tx2,ty2,tz2;
    tx2 = cx2/16;
    ty2 = cy2/16;
    tz2 = cz2;
    sort(tx1,tx2);
    sort(ty1,ty2);
    sort(tz1,tz2);
    sort(cx1,cx2);
    sort(cy1,cy2);
    sort(cz1,cz2);

    vector <vector<vector<string> > >dig(cz2-cz1+1,vector<vector<string> >(cy2-cy1+1,vector<string>(cx2-cx1+1)));
    vector <vector<vector<string> > >build(cz2-cz1+1,vector<vector<string> >(cy2-cy1+1,vector<string>(cx2-cx1+1)));
    mapblock40d block;
    DFHack::Maps *Maps = DF->getMaps();
    Maps->Start();
    for(uint32_t y = ty1;y<=ty2;y++)
    {
        for(uint32_t x = tx1;x<=tx2;x++)
        {
            for(uint32_t z = tz1;z<=tz2;z++)
            {
                if(Maps->isValidBlock(x,y,z))
                {
                    if(Maps->ReadBlock40d(x,y,z,&block))
                    {
                        int ystart,yend,xstart,xend;
                        ystart=xstart=0;
                        yend=xend=15;
                        if(y == ty2)
                        {
                            yend = cy2 % 16;
                        }
                        if(y == ty1)
                        {
                            ystart = cy1 % 16;
                        }
                        if(x == tx2)
                        {
                            xend = cx2 % 16;
                        }
                        if(x == tx1)
                        {
                            xstart = cx1 % 16;
                        }
                        int zidx = z-tz1;
                        for(int yy = ystart; yy <= yend;yy++)
                        {
                            int yidx = yy+(16*(y-ty1)-(cy1%16));
                            for(int xx = xstart; xx <= xend;xx++)
                            {
                                int xidx = xx+(16*(x-tx1)-(cx1%16));
                                int16_t tt = block.tiletypes[xx][yy];
                                DFHack::TileShape ts = DFHack::tileShape(tt);
                                if(DFHack::isOpenTerrain(tt) || DFHack::isFloorTerrain(tt))
                                {
                                    dig[zidx][yidx][xidx] = "d";
                                }
                                else if(DFHack::STAIR_DOWN == ts)
                                {
                                    dig [zidx][yidx][xidx] = "j";
                                    build [zidx][yidx][xidx] = "Cd";
                                }
                                else if(DFHack::STAIR_UP == ts)
                                {
                                    dig [zidx][yidx][xidx] = "u";
                                    build [zidx][yidx][xidx] = "Cu";
                                }
                                else if(DFHack::STAIR_UPDOWN == ts)
                                {
                                    dig [zidx][yidx][xidx] = "i";
                                    build [zidx][yidx][xidx] = "Cx";
                                }
                                else if(DFHack::isRampTerrain(tt))
                                {
                                    dig [zidx][yidx][xidx] = "r";
                                    build [zidx][yidx][xidx] = "Cr";
                                }
                                else if(DFHack::isWallTerrain(tt))
                                {
                                    build [zidx][yidx][xidx] = "Cw";
                                }
                            }
                            yidx++;
                        }
                    }
                }
            }
        }
    }
    DFHack::Buildings * Bld = DF->getBuildings();
    std::map <uint32_t, std::string> custom_workshop_types;
    uint32_t numBuildings;
    if(Bld->Start(numBuildings))
    {
        Bld->ReadCustomWorkshopTypes(custom_workshop_types);
        for(uint32_t i = 0; i < numBuildings; i++)
        {
            DFHack::t_building temp;
            Bld->Read(i, temp);
            if(temp.type != 0xFFFFFFFF) // check if type isn't invalid
            {
                std::string typestr;
                mem->resolveClassIDToClassname(temp.type, typestr);
                if(temp.z == cz1 && cx1 <= temp.x1 && cx2 >= temp.x2 && cy1 <= temp.y1 && cy2 >= temp.y2)
                {
                    string currStr = build[temp.z-cz1][temp.y1-cy1][temp.x1-cx1];
                    stringstream stream;
                    string newStr = buildCommands[typestr];
                    if(temp.x1 != temp.x2)
                    {
                        stream << "(" << temp.x2-temp.x1+1 << "x" << temp.y2-temp.y1+1 << ")";
                        newStr += stream.str();
                    }
                    build[temp.z-cz1][temp.y1-cy1][temp.x1-cx1] = newStr + currStr;
                }
            }
        }
    }
// for testing purposes
    //ofstream outfile("test.txt");
//    printVecOfVec(outfile, dig,'\t');
//    outfile << endl;
//   printVecOfVec(outfile, build,'\t');
//    outfile << endl;
//    outfile.close();

    int32_t cx3,cy3,cz3,cx4,cy4,cz4;
    uint32_t tx3,ty3,tz3,tx4,ty4,tz4;
    char result;
    while(1){
        cx3 = -30000;
        while(cx3 == -30000){
            DF->Resume();
            cout << "Set cursor at new position, then press any key:";
            result = cin.get();
            DF->Suspend();
            Gui->getCursorCoords(cx3,cy3,cz3);
        }
        if(result == 'q'){
            break;
        }
        cx4 = cx3+cx2-cx1;
        cy4 = cy3+cy2-cy1;
        cz4 = cz3+cz2-cz1;
        tx3=cx3/16;
        ty3=cy3/16;
        tz3=cz3;
        tx4=cx4/16;
        ty4=cy4/16;
        tz4=cz4;
        DFHack::WindowIO * Win = DF->getWindowIO();
        designations40d designationBlock;
        for(uint32_t y = ty3;y<=ty4;y++)
        {
            for(uint32_t x = tx3;x<=tx4;x++)
            {
                for(uint32_t z = tz3;z<=tz4;z++)
                {
                    Maps->Start();
                    Maps->ReadBlock40d(x,y,z,&block);
                    Maps->ReadDesignations(x,y,z,&designationBlock);
                    int ystart,yend,xstart,xend;
                    ystart=xstart=0;
                    yend=xend=15;
                    if(y == ty4){
                        yend = cy4 % 16;
                    }
                    if(y == ty3){
                        ystart = cy3 % 16;
                    }
                    if(x == tx4){
                        xend = cx4 % 16;
                    }
                    if(x == tx3){
                        xstart = cx3 % 16;
                    }
                    int zidx = z-tz3;
                    for(int yy = ystart; yy <= yend;yy++){
                        int yidx = yy+(16*(y-ty3)-(cy3%16));
                        for(int xx = xstart; xx <= xend;xx++){
                            int xidx = xx+(16*(x-tx3)-(cx3%16));
                                if(dig[zidx][yidx][xidx] != ""){
                                    char test = dig[zidx][yidx][xidx].c_str()[0];
                                    switch (test){
                                        case 'd':
                                            designationBlock[xx][yy].bits.dig = DFHack::designation_default;
                                            break;
                                        case 'i':
                                            designationBlock[xx][yy].bits.dig = DFHack::designation_ud_stair;
                                            break;
                                        case 'u':
                                            designationBlock[xx][yy].bits.dig = DFHack::designation_u_stair;
                                            break;
                                        case 'j':
                                            designationBlock[xx][yy].bits.dig = DFHack::designation_d_stair;
                                            break;
                                        case 'r':
                                            designationBlock[xx][yy].bits.dig = DFHack::designation_ramp;
                                            break;
                                    }
                                
                                }
                        }
                        yidx++;
                    }
                    Maps->Start();
                    Maps->WriteDesignations(x,y,z,&designationBlock);
                }
            }
        }
    }
    }
    DF->Detach();
    #ifndef LINUX_BUILD
    std::cout << "Done. Press any key to continue" << std::endl;
    cin.ignore();
    #endif
    return 0;
}
/*
bool waitTillCursorState(DFHack::Context *DF, bool On)
{
    DFHack::WindowIO * w = DF->getWindowIO();
    DFHack::Position * p = DF->getPosition();
    int32_t x,y,z;
    int tryCount = 0;
    DF->Suspend();
    bool cursorResult = p->getCursorCoords(x,y,z);
    while(tryCount < 50 && On && !cursorResult || !On && cursorResult)
    {
        DF->Resume();
        w->TypeSpecial(DFHack::WAIT,1,WAIT_AMT);
        tryCount++;
        DF->Suspend();
        cursorResult = p->getCursorCoords(x,y,z);
    }
    if(tryCount >= 50)
    {
        cerr << "Something went wrong, cursor at x: " << x << " y: " << y << " z: " << z << endl;
        return false;
    }
    DF->Resume();
    return true;
}
bool waitTillCursorPositionState(DFHack::Context *DF, int32_t x,int32_t y, int32_t z)
{
    DFHack::WindowIO * w = DF->getWindowIO();
    DFHack::Position * p = DF->getPosition();
    int32_t x2,y2,z2;
    int tryCount = 0;
    DF->Suspend();
    bool cursorResult = p->getCursorCoords(x2,y2,z2);
    while(tryCount < 50 && (x != x2 || y != y2 || z != z2))
    {
        DF->Resume();
        w->TypeSpecial(DFHack::WAIT,1,WAIT_AMT);
        tryCount++;
        DF->Suspend();
        cursorResult = p->getCursorCoords(x2,y2,z2);
    }
    if(tryCount >= 50)
    {
        cerr << "Something went wrong, cursor at x: " << x2 << " y: " << y2 << " z: " << z2 << endl;
        return false;
    }
    DF->Resume();
    return true;
}*/