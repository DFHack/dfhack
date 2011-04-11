/*********************************************
 * tools/supported/skillmodify.cpp
 *
 * Purpose:
 *
 * - Display creatures
 * - Modify skills and labors of creatures
 *
 * Version: 0.1
 * Date: 2011-04-07
 *
 * Todo:
 * - Option to add/remove single skills
 * - Ghosts should not be displayed
 * - Override forbidden mass-designation with -f
 * - Filter by nickname with -n
 * - Filter by first name with -fn
 * - Filter by last name with -ln
 * - Kill/revive creature(s) with --kill/--revive
 * Done:
 * - Option to add/remove single labors
 * - Switches -ras and rl should only be possible with -nn or -i
 * - Switch -rh removes hauler jobs
 * - Dead creatures should not be displayed
 * - Childs should not get labors assigned to
 * - Babies should not get labors assigned to
 * - Switch -al <n> adds labor number n
 * - Switch -rl <n> removes labor number n
 * - Switch -ral removes all labors
 * - Switch -ll lists all available labors
 *********************************************
*/

#include <iostream>
#include <climits>
#include <string.h>
#include <vector>
#include <stdio.h>
using namespace std;

#define DFHACK_WANT_MISCUTILS
#include <DFHack.h>
#include <dfhack/modules/Creatures.h>

void usage(int argc, const char * argv[])
{
    cout
        << "Usage:" << endl
        << argv[0] << " [option 1] [option 2] [...]" << endl
        << "-q            : Suppress \"Press any key to continue\" at program termination" << endl
        << "-c creature   : Only show/modify this creature type instead of dwarfes" << endl
        << "-i id         : Only show/modify creature with this id" << endl
        << "-nn           : Only show/modify creatures with no custom nickname" << endl
        << "-ll           : List available labors" << endl
        << "-al <n>       : Add labor <n> to creature" << endl
        << "-rl <n>       : Remove labor <n> from creature" << endl
        << "-ras          : Remove all skills from creature" << endl
        << "-ral          : Remove all labors from creature" << endl
        << "-ah           : Add hauler labors (stone hauling, etc.) to creature" << endl
        << "-rh           : Remove hauler labors (stone hauling, etc.) from creature" << endl
        << "-me           : Make creature an engraver" << endl
        << "-mm           : Make creature a mason" << endl
        << "--setmood <n> : Set mood to n (-1 = no mood, max=4)" << endl
        // Doesn't work, because hapiness is recalculated
        //<< "--sethappiness <n> : Set happiness to n" << endl
        << "-f            : Force an action" << endl
        << endl
        << "Example 1: Show all creatures" << endl
        << argv[0] << endl
        << endl
        << "Example 2: Just show dwarfs" << endl
        << argv[0] << " -c DWARF" << endl
        << endl
        << "Example 3: Remove skill from dwarf 32" << endl
        << argv[0] << " -i 32 -ras" << endl
        << endl
        << "Example 4: Remove all skills and labors from dwarfs with no custom nickname" << endl
        << argv[0] << " -c DWARF -nn -ras -ral" << endl
        << endl
        << "Example 5: Additionally assign the hauling labors and engraving" << endl
        << argv[0] << " -c DWARF -nn -ras -ral -me -ah" << endl
        ;
}

enum likeType
{
    FAIL = 0,
    MATERIAL = 1,
    ITEM = 2,
    FOOD = 3
};

DFHack::Materials * Materials;
DFHack::VersionInfo *mem;
vector< vector<string> > englishWords;
vector< vector<string> > foreignWords;
DFHack::Creatures * Creatures = NULL;


// Note that toCaps() changes the string itself and I'm using it a few times in
// an unsafe way below. Didn't crash yet however.
std::string toCaps(std::string s)
{
    const int length = s.length();
    if (length == 0) {
        return s;
    }
    s[0] = std::toupper(s[0]);
    for(int i=1; i!=length ; ++i)
    {
        s[i] = std::tolower(s[i]);
    }
    return s;
}

int strtoint(const string &str)
{
    stringstream ss(str);
    int result;
    return ss >> result ? result : -1;
}

void printCreature(DFHack::Context * DF, const DFHack::t_creature & creature, int index)
{
    cout << "Creature[" << index << "]: " << toCaps(Materials->raceEx[creature.race].rawname);

    DFHack::Translation *Tran = DF->getTranslation();
    DFHack::VersionInfo *mem = DF->getMemoryInfo();

    if(creature.name.nickname[0])
    {
        cout << ", " << creature.name.nickname;
    }
    else
    {
        if(creature.name.first_name[0])
        {
            cout << ", " << toCaps(creature.name.first_name);
        }

        string transName = Tran->TranslateName(creature.name,false);
        if(!transName.empty())
        {
            cout << " " << toCaps(transName);
        }
    }

    string prof_string="";
    try {
        prof_string = mem->getProfession(creature.profession);
    }
    catch (exception& e)
    {
        cout << "Error retrieving creature profession: " << e.what() << endl;
    }
    cout << ", " << toCaps(prof_string) << "(" << int(creature.profession) << ")";

    if(creature.custom_profession[0])
    {
        cout << "/" << creature.custom_profession;
    }

    if(creature.current_job.active)
    {
        cout << ", current job: " << mem->getJob(creature.current_job.jobId);
    }

    cout << ", Happy = " << creature.happiness;
    cout << endl;

    if((creature.mood != -1) && (creature.mood<5))
    {
        cout << "Creature is in a strange mood (mood=" << creature.mood << "), skill: " << mem->getSkill(creature.mood_skill) << endl;
        vector<DFHack::t_material> mymat;
        if(Creatures->ReadJob(&creature, mymat))
        {
            for(unsigned int i = 0; i < mymat.size(); i++)
            {
                printf("\t%s(%d)\t%d %d %d - %.8x\n", Materials->getDescription(mymat[i]).c_str(), mymat[i].itemType, mymat[i].subType, mymat[i].subIndex, mymat[i].index, mymat[i].flags);
            }
        }
    }

    if(creature.has_default_soul)
    {
        // Print out skills
        int skillid;
        int skillrating;
        int skillexperience;
        string skillname;

        cout << setiosflags(ios::left);

        for(unsigned int i = 0; i < creature.defaultSoul.numSkills;i++)
        {
            skillid = creature.defaultSoul.skills[i].id;
            if ( // leave out some skill that are not interesting and clutter the screen
                    skillid != 72
                    && skillid != 73
                    && skillid != 74
                    && skillid != 79
                    && skillid != 80
                    && skillid != 81
                    && skillid != 82
                    && skillid != 83
                    && skillid != 84
               )
            {
                skillrating = creature.defaultSoul.skills[i].rating;
                skillexperience = creature.defaultSoul.skills[i].experience;
                try
                {
                    skillname = mem->getSkill(skillid);
                }
                catch(DFHack::Error::AllMemdef &e)
                {
                    skillname = "Unknown skill";
                    cout << e.what() << endl;
                }
                cout << "(Skill " << int(skillid) << ") " << setw(16) << skillname << ": " 
                    << skillrating << "/" << skillexperience << endl;
            }
        }

        for(unsigned int i = 0; i < NUM_CREATURE_LABORS;i++)
        {
            if(!creature.labors[i])
                continue;
            string laborname;
            try
            {
                laborname = mem->getLabor(i);
            }
            catch(exception &e)
            {
                //cout << "Found undefined labor (" << i << ")" << endl;
                //cout << e.what() << endl;
                laborname = "(Undefined)";
                //continue;
            }
            cout << "(Labor " << i << ") " << setw(16) << laborname << endl;
        }
    }
    /* FLAGS 1 */
    if(creature.flags1.bits.dead)       { cout << "Flag: Dead" << endl; }
    if(creature.flags1.bits.on_ground)  { cout << "Flag: On the ground" << endl; }
    if(creature.flags1.bits.skeleton)   { cout << "Flag: Skeletal" << endl; }
    if(creature.flags1.bits.zombie)     { cout << "Flag: Zombie" << endl; }
    if(creature.flags1.bits.tame)       { cout << "Flag: Tame" << endl; }
    if(creature.flags1.bits.royal_guard){ cout << "Flag: Royal_guard" << endl; }
    if(creature.flags1.bits.fortress_guard){cout<<"Flag: Fortress_guard" << endl; }
    /* FLAGS 2 */
    if(creature.flags2.bits.killed)     { cout << "Flag: Killed by kill function" << endl; }
    if(creature.flags2.bits.resident)   { cout << "Flag: Resident" << endl; }
    if(creature.flags2.bits.gutted)     { cout << "Flag: Gutted" << endl; }
    if(creature.flags2.bits.slaughter)  { cout << "Flag: Marked for slaughter" << endl; }
    if(creature.flags2.bits.underworld) { cout << "Flag: From the underworld" << endl; }

    if(creature.flags1.bits.had_mood && (creature.mood == -1 || creature.mood == 8 ) )
    {
        string artifact_name = Tran->TranslateName(creature.artifact_name,false);
        cout << "Artifact: " << artifact_name << endl;
    }
}

int main (int argc, const char* argv[])
{
    bool quiet = true;
    // let's be more useful when double-clicked on windows
#ifndef LINUX_BUILD
    quiet = false;
#endif

    string creature_type = "Dwarf";
    string creature_id = "";
    int creature_id_int = 0;
    bool find_nonicks = false;
    bool find_nicks = false;
    bool remove_skills = false;
    bool remove_labors = false;
    bool make_hauler = false;
    bool remove_hauler = false;
    bool make_engraver = false;
    bool make_mason = false;
    bool add_labor = false;
    int add_labor_n = -99999;
    bool remove_labor = false;
    int remove_labor_n = -99999;
    bool set_happiness = false;
    int set_happiness_n = -99999;
    bool set_mood = false;
    int set_mood_n = -99999;
    bool list_labors = false;
    bool force_massdesignation = false;

    if (argc == 1) {
        usage(argc, argv);
        return 1;
    }

    for(int i = 1; i < argc; i++)
    {
        string arg_cur = argv[i];
        string arg_next = "";
        int arg_next_int = -99999;
        /* Check if argv[i+1] is a number >= 0 */
        if (i < argc-1) {
            arg_next = argv[i+1];
            arg_next_int = strtoint(arg_next);
            if (arg_next != "0" && arg_next_int == 0) {
                arg_next_int = -99999;
            }
        }

        if(arg_cur == "-q")
        {
            quiet = true;
        }
        else if(arg_cur == "-ras")
        {
            remove_skills = true;
        }
        else if(arg_cur == "-f")
        {
            force_massdesignation = true;
        }
        // list labors
        else if(arg_cur == "-ll")
        {
            list_labors = true;
        }
        // add single labor
        else if(arg_cur == "-al" && i < argc-1)
        {
            if (arg_next_int == -99999 || arg_next_int >= NUM_CREATURE_LABORS) {
                usage(argc, argv);
                return 1;
            }
            add_labor = true;
            add_labor_n = arg_next_int;
            i++;
        }
        // remove single labor
        else if(arg_cur == "-rl" && i < argc-1)
        {
            if (arg_next_int == -99999 || arg_next_int >= NUM_CREATURE_LABORS) {
                usage(argc, argv);
                return 1;
            }
            remove_labor = true;
            remove_labor_n = arg_next_int;
            i++;
        }
        else if(arg_cur == "--setmood" && i < argc-1)
        {
            if (arg_next_int < -1 || arg_next_int >= 5) {
                usage(argc, argv);
                return 1;
            }
            set_mood = true;
            set_mood_n = arg_next_int;
            i++;
        }
        else if(arg_cur == "--sethappiness" && i < argc-1)
        {
            if (arg_next_int < 1 || arg_next_int >= 2000) {
                usage(argc, argv);
                return 1;
            }
            set_happiness = true;
            set_happiness_n = arg_next_int;
            i++;
        }
        else if(arg_cur == "-ral")
        {
            remove_labors = true;
        }
        else if(arg_cur == "-ah")
        {
            make_hauler = true;
        }
        else if(arg_cur == "-rh")
        {
            remove_hauler = true;
        }
        else if(arg_cur == "-mm")
        {
            make_mason = true;
        }
        else if(arg_cur == "-me")
        {
            make_engraver = true;
        }
        else if(arg_cur == "-nn")
        {
            find_nonicks = true;
        }
        else if(arg_cur == "--nicks")
        {
            find_nicks = true;
        }
        else if(arg_cur == "-c" && i < argc-1)
        {
            creature_type = argv[i+1];
            i++;
        }
        else if(arg_cur == "-i" && i < argc-1)
        {
            creature_id = argv[i+1];
            sscanf(argv[i+1], "%d", &creature_id_int);
            i++;
        }
        else
        {
            if (arg_cur != "-h") {
                cout << "Unknown option '" << arg_cur << "'" << endl;
                cout << endl;
            }
            usage(argc, argv);
            return 1;
        }
    }

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context* DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        if (quiet == false) 
        {
            cin.ignore();
        }
        return 1;
    }

    Creatures = DF->getCreatures();
    Materials = DF->getMaterials();
    DFHack::Translation * Tran = DF->getTranslation();

    uint32_t numCreatures;
    if(!Creatures->Start(numCreatures))
    {
        cerr << "Can't get creatures" << endl;
        if (quiet == false) 
        {
            cin.ignore();
        }
        return 1;
    }
    if(!numCreatures)
    {
        cerr << "No creatures to print" << endl;
        if (quiet == false) 
        {
            cin.ignore();
        }
        return 1;
    }

    mem = DF->getMemoryInfo();
    Materials->ReadInorganicMaterials();
    Materials->ReadOrganicMaterials();
    Materials->ReadWoodMaterials();
    Materials->ReadPlantMaterials();
    Materials->ReadCreatureTypes();
    Materials->ReadCreatureTypesEx();
    Materials->ReadDescriptorColors();

    if(!Tran->Start())
    {
        cerr << "Can't get name tables" << endl;
        return 1;
    }

    // List all available labors (reproduces contents of Memory.xml)
    if (list_labors == true) {
        string laborname;
        for (int i=0; i < NUM_CREATURE_LABORS; i++) {
            try {
                laborname = mem->getLabor(i);
            }
            catch (exception& e) {
                laborname = "Unknown";
            }
            cout << "Labor " << int(i) << ": " << laborname << endl;
        }
    }
    else
    {
        vector<uint32_t> addrs;
        for(uint32_t creature_idx = 0; creature_idx < numCreatures; creature_idx++)
        {
            DFHack::t_creature creature;
            Creatures->ReadCreature(creature_idx,creature);
            /* Check if we want to display/change this creature or skip it */
            bool hasnick = (creature.name.nickname[0] != '\0');

            if (
                    // Check for -i <num>
                    (creature_id.empty() || creature_idx == creature_id_int) 
                    // Check for -c <type>
                    && (creature_type.empty() || toCaps(string(Materials->raceEx[creature.race].rawname)) == toCaps(creature_type))
                    // Check for -nn
                    && ((find_nonicks == true && hasnick == false)
                        || (find_nicks == true && hasnick == true)
                        || (find_nicks == false && find_nonicks == false))
                    && (find_nonicks == false || creature.name.nickname[0] == '\0')
                    && (!creature.flags1.bits.dead)
               )
            {
                printCreature(DF,creature,creature_idx);
                addrs.push_back(creature.origin);

                bool dochange = (
                        remove_skills
                        || remove_labors || add_labor || remove_labor
                        || make_hauler || remove_hauler 
                        || make_engraver || make_mason
                        || set_happiness
                        || set_mood
                        );

                // 96=Child, 97=Baby
                if (creature.profession == 96 || creature.profession == 97) 
                {
                    dochange = false;
                }

                bool allow_massdesignation =
                    !creature_id.empty() || toCaps(creature_type) != "Dwarf" || find_nonicks == true || force_massdesignation;
                if (dochange == true && allow_massdesignation == false)
                {
                    cout
                        << "Not changing creature because none of -c (other than dwarf), -i or -nn was" << endl
                        << "selected. Add -f to still do mass designation." << endl;
                    dochange = false;
                }

                // cout << "dochange = " << int(dochange) << endl;
                // cout << "remove_skills = " << int(remove_skills) << endl;
                // cout << "remove_labors = " << int(remove_labors) << endl;
                // cout << "add_labor = " << int(add_labor) << endl;
                // cout << "remove_labor = " << int(remove_labor) << endl;
                // cout << "make_hauler = " << int(make_hauler) << endl;
                // cout << "remove_hauler = " << int(remove_hauler) << endl;
                // cout << "make_engraver = " << int(make_engraver) << endl;
                // cout << "make_mason = " << int(make_mason) << endl;
                // cout << "allow_massdesignation = " << int(allow_massdesignation) << endl;
                // cout << "force_massdesignation = " << int(force_massdesignation) << endl;
                // cout << "creature.profession = " << int(creature.profession) << endl;

                if (dochange) 
                {
                    if(creature.has_default_soul)
                    {
                        if (set_mood) 
                        {
                            cout << "Setting mood to " << set_mood_n << "..." << endl;
                            Creatures->WriteMood(creature_idx, set_mood_n);
                        }

                        if (set_happiness) 
                        {
                            cout << "Setting happiness to " << set_happiness_n << "..." << endl;
                            Creatures->WriteHappiness(creature_idx, set_happiness_n);
                        }

                        if (remove_skills) 
                        {
                            DFHack::t_soul & soul = creature.defaultSoul;

                            cout << "Removing skills..." << endl;
                            for(unsigned int sk = 0; sk < soul.numSkills;sk++)
                            {
                                soul.skills[sk].rating=0;
                                soul.skills[sk].experience=0;
                            }
                            // Doesn't work anyways, so better leave it alone
                            //soul.numSkills=0;
                            if (Creatures->WriteSkills(creature_idx, soul) == true) {
                                cout << "Success writing skills." << endl;
                            } else {
                                cout << "Error writing skills." << endl;
                            }
                        }

                        if (add_labor || remove_labor || remove_labors || make_hauler || remove_hauler || make_engraver || make_mason)
                        {
                            if (add_labor) {
                                cout << "Adding labor " << add_labor_n << "..." << endl;
                                creature.labors[add_labor_n] = 1;
                            }

                            if (remove_labor) {
                                cout << "Removing labor " << remove_labor_n << "..." << endl;
                                creature.labors[remove_labor_n] = 0;
                            }

                            if (remove_labors) {
                                cout << "Removing labors..." << endl;
                                for(unsigned int lab = 0; lab < NUM_CREATURE_LABORS; lab++) {
                                    creature.labors[lab] = 0;
                                }
                            }
                            if (remove_hauler) {
                                cout << "Removing hauler labors..." << endl;
                                creature.labors[1] = 0;
                                creature.labors[2] = 0;
                                creature.labors[3] = 0;
                                creature.labors[4] = 0;
                                creature.labors[5] = 0;
                                creature.labors[6] = 0;
                                creature.labors[7] = 0;
                                creature.labors[8] = 0;
                                creature.labors[9] = 0;
                                creature.labors[22] = 0;
                                creature.labors[23] = 0;
                            }
                            if (make_hauler) {
                                cout << "Setting hauler labors..." << endl;
                                creature.labors[1] = 1;
                                creature.labors[2] = 1;
                                creature.labors[3] = 1;
                                creature.labors[4] = 1;
                                creature.labors[5] = 1;
                                creature.labors[6] = 1;
                                creature.labors[7] = 1;
                                creature.labors[8] = 1;
                                creature.labors[9] = 1;
                                creature.labors[22] = 1;
                                creature.labors[23] = 1;
                            }
                            if (make_engraver) {
                                cout << "Setting 'Stone Detailing'..." << endl;
                                creature.labors[12] = 1;
                            }
                            if (make_mason) {
                                cout << "Setting 'Masonry'..." << endl;
                                creature.labors[13] = 1;
                            }
                            if (Creatures->WriteLabors(creature_idx, creature.labors) == true) {
                                cout << "Success writing labors." << endl;
                            } else {
                                cout << "Error writing labors." << endl;
                            }
                        }
                    }
                    else
                    {
                        cout << "Error removing skills: Creature has no default soul." << endl;
                    }
                    printCreature(DF,creature,creature_idx);
                } /* End remove skills/labors */
                cout << endl;
            } /* if (print creature) */
        } /* End for(all creatures) */
    } /* End if (we need to walk creatures) */

    Creatures->Finish();
    DF->Detach();
    if (quiet == false) 
    {
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    }
    return 0;
}
