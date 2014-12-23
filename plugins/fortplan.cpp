#include "buildingplan-lib.h"
#include <fstream>
#include <vector>

DFHACK_PLUGIN("fortplan");
#define PLUGIN_VERSION 0.10

command_result fortplan(color_ostream &out, vector<string> & params);

struct BuildingInfo {
	std::string code;
	df::building_type type;
	std::string name;
	
	BuildingInfo(std::string theCode, df::building_type theType, std::string theName) {
		code = theCode;
		type = theType;
		name = theName;
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
    commands.push_back(PluginCommand("fortplan","Lay out buildings in your fortress based on a Quickfort-style CSV input file.",fortplan,false,
        "Lay out buildings in your fortress based on a Quickfort-style CSV input file.\n"
        "Usage: fortplan [filename]\n"));
        
    buildings.push_back(BuildingInfo("a",df::building_type::Armorstand,"Armor Stand"));
	buildings.push_back(BuildingInfo("r",df::building_type::Weaponrack,"Weapon Rack"));
	buildings.push_back(BuildingInfo("b",df::building_type::Bed,"Bed"));
	buildings.push_back(BuildingInfo("f",df::building_type::Cabinet,"Cabinet"));
	buildings.push_back(BuildingInfo("h",df::building_type::Box,"Box"));
	buildings.push_back(BuildingInfo("d",df::building_type::Door,"Door"));
	buildings.push_back(BuildingInfo("n",df::building_type::Coffin,"Coffin"));
	buildings.push_back(BuildingInfo("c",df::building_type::Chair,"Chair"));
	buildings.push_back(BuildingInfo("t",df::building_type::Table,"Table"));
    
    out << "Loaded fortplan version " << PLUGIN_VERSION << endl;
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
std::string get_working_path()
{
   char temp[MAXPATHLEN];
   return ( getcwd(temp, MAXPATHLEN) ? std::string( temp ) : std::string("") );
}

command_result fortplan(color_ostream &out, vector<string> & params) {

    auto & con = out;
	if (params.size()) {
		coord32_t cursor;
		coord32_t startCursor;
		if (!DFHack::Gui::getCursorCoords(cursor.x, cursor.y, cursor.z)) {
			con.print("You must have an active in-game cursor.\n");
			return CR_FAILURE;
		}
		DFHack::Gui::getCursorCoords(startCursor.x, startCursor.y, startCursor.z);

		std::string cwd = get_working_path();
		std::string filename = cwd+"/"+params[0];
		con.print("Loading file '%s'...\n",filename.c_str());
		std::ifstream infile(filename.c_str());
		if (!infile.good()) {
			con.print("Could not open the file.\n");
			return CR_FAILURE;
		}
		std::string line;
		int lineNum = 0;
		while (std::getline(infile, line)) {
			lineNum++;
			if (lineNum==1) {
				auto hashBuild = line.find("#build");
				if (hashBuild >= 0) {
					auto startLoc = line.find("start(");
					if (startLoc != line.npos) {
						startLoc += 6;
						auto nextDelimiter = line.find(";",startLoc);
						std::string startXStr = line.substr(startLoc,nextDelimiter-startLoc);
						int startXOffset = std::stoi(startXStr);
						startLoc = nextDelimiter+1;
						nextDelimiter = line.find(";",startLoc);
						std::string startYStr = line.substr(startLoc,nextDelimiter-startLoc);
						int startYOffset = std::stoi(startYStr);
						startCursor.x -= startXOffset;
						startCursor.y -= startYOffset;
						DFHack::Gui::setCursorCoords(startCursor.x,startCursor.y,startCursor.z);
						
						auto startEnd = line.find(")",nextDelimiter);
						
						con.print("Starting at (%d,%d,%d) which is described as: %s\n",startCursor.x,startCursor.y,startCursor.z,line.substr(nextDelimiter+1,startEnd-nextDelimiter).c_str());
						std::string desc = line.substr(startEnd+1);
						if (desc.size()>0) {
							con.print("Description of this plan: %s\n",desc.c_str());
						}
						continue;
					} else {
						con.print("No start location found for this block\n");
					}
				} else {
					con.print("Not a build file: %s\n",line.c_str());
					break;
				}
			}
			
			int start = 0;
			auto nextInd = line.find(',');
			std::string curCell = line.substr(start,nextInd-start);
			if (strcmp(curCell.substr(0,1).c_str(),"#")==0) {
				continue;
			}
			do {
				
				if (strcmp(curCell.c_str(),"`")!=0) {
					con.print("Found a cell with '%s' in it (line %d:%d-%d)\n",curCell.c_str(),lineNum,start,nextInd);
					auto buildingIndex = std::find_if(buildings.begin(), buildings.end(), MatchesCode(curCell.c_str()));

					// = std::find(validInstructions.begin(), validInstructions.end(), curCell);
					if(buildingIndex == buildings.end()) {
						con.print("That is not a valid code.\n");
					} else {
						//con.print("I can build that!\n");
						BuildingInfo buildingInfo = *buildingIndex;
						con.print("Building a(n) %s.\n",buildingInfo.name.c_str());
						planner.allocatePlannedBuilding(buildingInfo.type);
					}
				}
				cursor.x++;
				DFHack::Gui::setCursorCoords(cursor.x, cursor.y, cursor.z);
				start = nextInd+1;
				nextInd = line.find(',',start);
				curCell = line.substr(start,nextInd-start);
			} while (nextInd != line.npos);
			
			cursor.y++;
			cursor.x = startCursor.x;
			
		}
		con.print("Done with file.\n");
	} else {
		con.print("You must supply a filename to read.\n");
	}
	
	return CR_OK;
}