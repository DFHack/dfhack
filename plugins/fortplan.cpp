#include "buildingplan-lib.h"
#include <fstream>
#include <vector>
#include "modules/Filesystem.h"

DFHACK_PLUGIN("fortplan");
#define PLUGIN_VERSION 0.15

command_result fortplan(color_ostream &out, vector<string> & params);

struct BuildingInfo {
    std::string code;
    df::building_type type;
    df::furnace_type furnaceType;
    df::workshop_type workshopType;
    df::trap_type trapType;
    std::string name;
    bool variableSize;
    int defaultHeight;
    int defaultWidth;
    bool hasCustomOptions;

    BuildingInfo(std::string theCode, df::building_type theType, std::string theName, int height, int width) {
        code = theCode;
        type = theType;
        name = theName;
        variableSize = false;
        defaultHeight = height;
        defaultWidth = width;
        hasCustomOptions = false;
    }

    bool allocate() {
        return planner.allocatePlannedBuilding(type);
    }
};

class MatchesCode
{
    std::string _code;

public:
    MatchesCode(const std::string &code) : _code(code) {}

    bool operator()(const BuildingInfo &check) const
    {
        return check.code == _code;
    }
};

std::vector<BuildingInfo> buildings;

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand("fortplan","Lay out buildings from a Quickfort-style CSV file.",fortplan,false,
        "Lay out buildings in your fortress based on a Quickfort-style CSV input file.\n"
        "Usage: fortplan [filename]\n"));

    buildings.push_back(BuildingInfo("c",df::building_type::Chair,"Chair",1,1));
    buildings.push_back(BuildingInfo("b",df::building_type::Bed,"Bed",1,1));
    buildings.push_back(BuildingInfo("t",df::building_type::Table,"Table",1,1));
    buildings.push_back(BuildingInfo("n",df::building_type::Coffin,"Coffin",1,1));
    buildings.push_back(BuildingInfo("d",df::building_type::Door,"Door",1,1));
    buildings.push_back(BuildingInfo("x",df::building_type::Floodgate,"Floodgate",1,1));
    buildings.push_back(BuildingInfo("h",df::building_type::Box,"Box",1,1));
    buildings.push_back(BuildingInfo("r",df::building_type::Weaponrack,"Weapon Rack",1,1));
    buildings.push_back(BuildingInfo("a",df::building_type::Armorstand,"Armor Stand",1,1));
    buildings.push_back(BuildingInfo("f",df::building_type::Cabinet,"Cabinet",1,1));
    buildings.push_back(BuildingInfo("s",df::building_type::Statue,"Statue",1,1));
    buildings.push_back(BuildingInfo("y",df::building_type::WindowGlass,"Glass Window",1,1));
    buildings.push_back(BuildingInfo("m",df::building_type::AnimalTrap,"Animal Trap",1,1));
    buildings.push_back(BuildingInfo("v",df::building_type::Chain,"Chain",1,1));
    buildings.push_back(BuildingInfo("j",df::building_type::Cage,"Cage",1,1));
    buildings.push_back(BuildingInfo("H",df::building_type::Hatch,"Floor Hatch",1,1));
    buildings.push_back(BuildingInfo("W",df::building_type::GrateWall,"Wall Grate",1,1));
    buildings.push_back(BuildingInfo("G",df::building_type::GrateFloor,"Floor Grate",1,1));
    buildings.push_back(BuildingInfo("B",df::building_type::BarsVertical,"Vertical Bars",1,1));
    buildings.push_back(BuildingInfo("~b",df::building_type::BarsFloor,"Floor Bars",1,1));
    buildings.push_back(BuildingInfo("R",df::building_type::TractionBench,"Traction Bench",1,1));
    buildings.push_back(BuildingInfo("~s",df::building_type::Slab,"Slab",1,1));
    buildings.push_back(BuildingInfo("N",df::building_type::NestBox,"Nest Box",1,1));
    buildings.push_back(BuildingInfo("~h",df::building_type::Hive,"Hive",1,1));

    planner.initialize();

    return CR_OK;
}

#define DAY_TICKS 1200
DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    static decltype(world->frame_counter) last_frame_count = 0;
    if ((world->frame_counter - last_frame_count) >= DAY_TICKS/2)
    {
        last_frame_count = world->frame_counter;
        planner.doCycle();
    }

    return CR_OK;
}

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        planner.reset(out);

        is_enabled = enable;
    }

    return CR_OK;
}

std::vector<std::vector<std::string>> tokenizeFile(std::string filename) {
    std::ifstream infile(filename.c_str());
    std::vector<std::vector<std::string>> fileTokens(128, std::vector<std::string>(128));
    std::vector<std::vector<std::string>>::size_type x, y;
    if (!infile.good()) {
        throw -1;
    }
    std::string line;
    y = 0;
    while (std::getline(infile, line)) {
        x = 0;
        if (strcmp(line.substr(0,1).c_str(),"#")==0) {
            fileTokens[y++][0] = line;
            continue;
        }
        int start = 0;
        auto nextInd = line.find(',');
        std::string curCell = line.substr(start,nextInd-start);
        do {
            fileTokens[y][x] = curCell;
            start = nextInd+1;
            nextInd = line.find(',',start);
            curCell = line.substr(start,nextInd-start);
            x++;
        } while (nextInd != line.npos);
        y++;
    }
    return fileTokens;
}

command_result fortplan(color_ostream &out, vector<string> & params) {

    auto & con = out;
    std::vector<std::vector<std::string>> layout(128, std::vector<std::string>(128));
    if (params.size()) {
        coord32_t cursor;
        coord32_t userCursor;
        coord32_t startCursor;
        if (!DFHack::Gui::getCursorCoords(cursor.x, cursor.y, cursor.z)) {
            con.print("You must have an active in-game cursor.\n");
            return CR_FAILURE;
        }
        DFHack::Gui::getCursorCoords(startCursor.x, startCursor.y, startCursor.z);
        userCursor = startCursor;

        std::string cwd = Filesystem::getcwd();
        std::string filename = cwd+"/"+params[0];
        con.print("Loading file '%s'...\n",filename.c_str());
        try {
            layout = tokenizeFile(filename);
        } catch (int e) {
            con.print("Could not open the file.\n");
            return CR_FAILURE;
        }
        if (!is_enabled) {
            plugin_enable(out, true);
        }
        con.print("Loaded.\n");
        std::vector<std::vector<std::string>>::size_type x, y;
        bool started = false;
        for (y = 0; y < layout.size(); y++) {
            x = 0;
            auto hashBuild = layout[y][x].find("#build");
            if (hashBuild != layout[y][x].npos) {
                auto startLoc = layout[y][x].find("start(");
                if (startLoc != layout[y][x].npos) {
                    startLoc += 6;
                    auto nextDelimiter = layout[y][x].find(";",startLoc);
                    std::string startXStr = layout[y][x].substr(startLoc,nextDelimiter-startLoc);
                    int startXOffset = std::stoi(startXStr);
                    startLoc = nextDelimiter+1;
                    nextDelimiter = layout[y][x].find(";",startLoc);
                    std::string startYStr = layout[y][x].substr(startLoc,nextDelimiter-startLoc);
                    int startYOffset = std::stoi(startYStr);
                    startCursor.x -= startXOffset;
                    startCursor.y -= startYOffset;
                    DFHack::Gui::setCursorCoords(startCursor.x,startCursor.y,startCursor.z);
                    started = true;

                    auto startEnd = layout[y][x].find(")",nextDelimiter);

                    con.print("Starting at (%d,%d,%d) which is described as: %s\n",startCursor.x,startCursor.y,startCursor.z,layout[y][x].substr(nextDelimiter+1,startEnd-nextDelimiter).c_str());
                    std::string desc = layout[y][x].substr(startEnd+1);
                    if (desc.size()>0) {
                        con.print("Description of this plan: %s\n",desc.c_str());
                    }
                    continue;
                } else {
                    con.print("No start location found for this block\n");
                }
            } else if (!started) {
                con.print("Not a build file: %s\n",layout[y][x].c_str());
                break;
            }

            for (x = 0; x < layout[y].size(); x++) {
                if (strcmp(layout[y][x].substr(0,1).c_str(),"#")==0) {
                    continue;
                }

                if (strcmp(layout[y][x].c_str(),"`")!=0) {
                    auto dataIndex = layout[y][x].find("(");
                    std::string curCode;
                    std::vector<std::string> curData;
                    if (dataIndex != layout[y][x].npos) {
                        curCode = layout[y][x].substr(0,dataIndex);
                        int dataStart = dataIndex+1;
                        auto nextDataStart = layout[y][x].find(",",dataStart);
                        while (nextDataStart!=layout[y][x].npos) {
                            std::string nextData = layout[y][x].substr(dataStart,nextDataStart);
                            if (strcmp(nextData.substr(nextData.size()-1,1).c_str(),")")==0) {
                                nextData = nextData.substr(0,nextData.size()-1);
                            }
                            curData.push_back(nextData);
                            dataStart = nextDataStart+1;
                            nextDataStart = layout[y][x].find(",",dataStart);
                        }
                    } else {
                        curCode = layout[y][x];
                    }
                    //con.print("Found a cell with '%s' in it (layout[y][x] %d:%d-%d)\n",layout[y][x].c_str(),lineNum,start,nextInd);
                    auto buildingIndex = std::find_if(buildings.begin(), buildings.end(), MatchesCode(curCode.c_str()));

                    // = std::find(validInstructions.begin(), validInstructions.end(), layout[y][x]);
                    if(buildingIndex == buildings.end()) {
                        //con.print("That is not a valid code.\n");
                    } else {
                        //con.print("I can build that!\n");
                        BuildingInfo buildingInfo = *buildingIndex;
                        if (buildingInfo.variableSize || buildingInfo.defaultHeight > 1 || buildingInfo.defaultWidth > 1) {
                            //con.print("Found a building at (%d,%d) called %s, which has a size %dx%d or variable size\n",x,y,buildingInfo.name.c_str(),buildingInfo.defaultWidth, buildingInfo.defaultHeight);
                            // TODO: Make this function smarter, able to determine the exact shape
                            //        and location of the building
                            // For now, we just assume that we are always looking at the top left
                            //        corner of where it is located in the input file
                            coord32_t buildingSize;
                            if (!buildingInfo.variableSize) {
                                bool single = true;
                                bool block = true;
                                std::vector<std::vector<std::string>>::size_type checkX, checkY;
                                int yOffset = ((buildingInfo.defaultHeight-1)/2);
                                int xOffset = ((buildingInfo.defaultWidth-1)/2);
                                //con.print(" - Checking to see if it's a single, with %d<=x<%d and %d<=y<%d...\n",x-xOffset,x+xOffset,y-yOffset,y+yOffset);
                                // First, check to see if this is in the center of an empty square of its default size
                                for (checkY = y-yOffset; checkY <= (y+yOffset); checkY++) {
                                    for (checkX = x-xOffset; checkX <= (x+xOffset); checkX++) {
                                        if (checkX==x && checkY==y) {
                                            continue;
                                        }
                                        auto checkDataIndex = layout[checkY][checkX].find("(");
                                        std::string checkCode;
                                        if (checkDataIndex != layout[checkY][checkX].npos) {
                                            checkCode = layout[checkY][checkX].substr(0,checkDataIndex);
                                        } else {
                                            checkCode = layout[checkY][checkX];
                                        }

                                        con.print(" - Code at (%d,%d) is '%s': ",checkX,checkY,checkCode.c_str());
                                        auto checkIndex = std::find_if(buildings.begin(), buildings.end(), MatchesCode(checkCode.c_str()));
                                        //if (checkIndex == buildings.end()) {
                                        //    con.print("this is not a valid code, so we keep going.\n");
                                        //    continue;
                                        //}
                                        //if (curCode.compare(layout[checkY][checkX]) != 0) {
                                        if (checkIndex != buildings.end()) {
                                            //con.print("this is a building, so we break.\n");
                                            single = false;
                                            break;
                                        } else {
                                            //con.print("this is not a building, so we keep going.\n");
                                        }
                                    }
                                    if (!single) {
                                        //con.print("Not a single. Moving forward.\n");
                                        break;
                                    }
                                }
                                if (!single) {
                                    // If that's not the case, check to see if this is the top-left corner of
                                    //    a square of its default size
                                    //con.print(" - It's not single; checking to see if it's a block...\n");
                                    for (checkY = y; checkY < (y+buildingInfo.defaultHeight); checkY++) {
                                        for (checkX = x; checkX < (x+buildingInfo.defaultWidth); checkX++) {
                                            if (checkX==x && checkY==y) {
                                                continue;
                                            }
                                            auto checkDataIndex = layout[checkY][checkX].find("(");
                                            std::string checkCode;
                                            if (checkDataIndex != layout[checkY][checkX].npos) {
                                                checkCode = layout[checkY][checkX].substr(0,checkDataIndex);
                                            } else {
                                                checkCode = layout[checkY][checkX];
                                            }

                                            //con.print(" - Code at (%d,%d) is '%s': ",checkX,checkY,checkCode.c_str());
                                            if (curCode.compare(checkCode) != 0) {
                                                //con.print("this is not the same as '%s', so we break.\n",curCode.c_str());
                                                block = false;
                                                break;
                                            } else {
                                                //con.print("this is the same as '%s', so we erase it and move on.\n",curCode.c_str());
                                                layout[checkY][checkX] = "``";
                                            }
                                        }
                                        if (!block) {
                                            //con.print("Not a block. Moving forward.\n");
                                            break;
                                        }
                                    }
                                }

                                if (single) {
                                    //con.print("Placing a building with code '%s' centered at (%d,%d) and default size %dx%d.\n",curCode.c_str(),x,y,buildingInfo.defaultWidth,buildingInfo.defaultHeight);
                                    coord32_t offsetCursor = cursor;
                                    offsetCursor.x -= xOffset;
                                    offsetCursor.y -= yOffset;
                                    DFHack::Gui::setCursorCoords(offsetCursor.x, offsetCursor.y, offsetCursor.z);
                                    if (!buildingInfo.allocate()) {
                                        con.print("*** There was an error placing building with code '%s' centered at (%d,%d).\n",curCode.c_str(),x,y);
                                    }
                                    DFHack::Gui::setCursorCoords(cursor.x, cursor.y, cursor.z);
                                } else if (block) {
                                    //con.print("Placing a building with code '%s' with corner at (%d,%d) and default size %dx%d.\n",curCode.c_str(),x,y,buildingInfo.defaultWidth,buildingInfo.defaultHeight);
                                    if (!buildingInfo.allocate()) {
                                        con.print("*** There was an error placing building with code '%s' with corner at (%d,%d).\n",curCode.c_str(),x,y);
                                    }
                                } else {
                                    con.print("*** Found a code '%s' at (%d,%d) for a building with default size %dx%d with an invalid size designation.\n",curCode.c_str(),x,y,buildingInfo.defaultWidth,buildingInfo.defaultHeight);
                                }
                            } else {
                                //buildingSize = findBuildingExtent(layout, x, y, -1, -1, out);
                                //con.print(" - The building has variable size %dx%d\n",buildingSize.x, buildingSize.y);
                                //con.print(" - The building has a variable size, which is not yet handled.\n");
                            }
                        } else {
                            //con.print("Building a(n) %s.\n",buildingInfo.name.c_str());
                            if (!buildingInfo.allocate()) {
                                con.print("*** There was an error placing the %s at (%d,%d).\n",buildingInfo.name.c_str(),x,y);
                            }
                        }
                    }
                }
                cursor.x++;
                DFHack::Gui::setCursorCoords(cursor.x, cursor.y, cursor.z);
            }

            cursor.y++;
            cursor.x = startCursor.x;

        }
        DFHack::Gui::setCursorCoords(userCursor.x, userCursor.y, userCursor.z);
        con.print("Done with file.\n");
    } else {
        con.print("You must supply a filename to read.\n");
    }

    return CR_OK;
}
