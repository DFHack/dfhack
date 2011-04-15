/*********************************************
 * Purpose:
 *
 * - Display creatures
 * - Modify skills and labors of creatures
 * - Kill creatures
 * - Etc.
 *
 * Version: 0.1.1
 * Date: 2011-04-07
 * Author: raoulxq (based on creaturedump.cpp from peterix)

 * Todo:
 * - Option to add/remove single skills
 * - Ghosts/Merchants/etc. should be tagged as not own creatures
 * - Filter by nickname with -n
 * - Filter by first name with -fn
 * - Filter by last name with -ln
 * - Add pattern matching (or at least matching) to -n/-fn/-ln
 * - Set nickname with --setnick (only if -i is given)
 * - Revive creature(s) with --revive
 * - Show skills/labors only when -ss/-sl/-v is given or a skill/labor is changed
 * - Make -1 the default for everything but -i
 * - Imply -i if first argument is a number
 * - Search for nick/profession if first argument is a string without - (i.e. no switch)
 * - Switch --showhappy (show dwarf's experiences which make her un-/happy)
 * - Switch --makefriendly
 * - Switch --listskills, showing first 3 important skills

 * Done:
 * - Allow comma separated list of IDs for -i
 * - '-c all' shows all creatures
 * - Rename from skillmodify.cpp to creature.cpp
 * - Kill creature(s) with --kill
 * - Hide skills with level 0 and 0 experience points
 * - Add --showallflags flag to display all flags (default: display a few important ones)
 * - Add --showdead flag to also display dead creatures
 * - Display more creature flags
 * - Show creature type (again)
 * - Add switch -1/--summary to only display one line for every creature. Good for an overview.
 * - Display current job (has been there all the time, but not shown in Windows due to missing memory offsets)
 * - Remove magic numbers
 * - Show social skills only when -ss is given
 * - Hide hauler labors when +sh is given
 * - Add -v for verbose
 * - Override forbidden mass-designation with -f
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

/* Note about magic numbers:
 * If you have an idea how to better solve this, tell me. Currently I'd be
 * either dependent on Toady One's implementation (#defining numbers) or
 * Memory.xml (#defining text). I voted for Toady One's numbers to be more
 * stable, but could be wrong.
 */
#define SKILL_PERSUASION     72
#define SKILL_NEGOTIATION    73
#define SKILL_JUDGING_INTENT 74
#define SKILL_INTIMIDATION   79
#define SKILL_CONVERSATION   80
#define SKILL_COMEDY         81
#define SKILL_FLATTERY       82
#define SKILL_CONSOLING      83
#define SKILL_PACIFICATION   84

#define LABOR_STONE_HAULING   1 
#define LABOR_WOOD_HAULING    2 
#define LABOR_BURIAL          3
#define LABOR_FOOD_HAULING    4 
#define LABOR_REFUSE_HAULING  5 
#define LABOR_ITEM_HAULING    6 
#define LABOR_FURNITURE_HAULING 7 
#define LABOR_ANIMAL_HAULING  8 
#define LABOR_CLEANING        9
#define LABOR_FEED_PATIENTS_PRISONERS   22
#define LABOR_RECOVERING_WOUNDED        23
#define NOT_SET              INT_MIN
#define MAX_MOOD             4
#define NO_MOOD             -1

#define PROFESSION_CHILD    96
#define PROFESSION_BABY     97

bool quiet=true;
bool verbose = false;
bool showhauler = true;
bool showsocial = false;
bool showfirstlineonly = false;
bool showdead = false;
bool showallflags = false;

int hauler_labors[] = {
    LABOR_STONE_HAULING
        ,LABOR_WOOD_HAULING
        ,LABOR_BURIAL
        ,LABOR_FOOD_HAULING
        ,LABOR_REFUSE_HAULING
        ,LABOR_ITEM_HAULING
        ,LABOR_FURNITURE_HAULING
        ,LABOR_ANIMAL_HAULING
        ,LABOR_CLEANING
        ,LABOR_FEED_PATIENTS_PRISONERS
        ,LABOR_RECOVERING_WOUNDED
};
int social_skills[] = 
{
    SKILL_PERSUASION
        ,SKILL_NEGOTIATION
        ,SKILL_JUDGING_INTENT
        ,SKILL_INTIMIDATION
        ,SKILL_CONVERSATION
        ,SKILL_COMEDY
        ,SKILL_FLATTERY
        ,SKILL_CONSOLING
        ,SKILL_PACIFICATION
};

void usage(int argc, const char * argv[])
{
    cout
        << "Usage:" << endl
        << argv[0] << " [option 1] [option 2] [...]" << endl
        << endl
        << "Display options:" << endl
        << "-q              : Suppress \"Press any key to continue\" at program termination" << endl
        << "-v              : Increase verbosity" << endl
        << "-c creature     : Show/modify this creature type instead of dwarfes ('all' to show all creatures)" << endl
        << "-1/--summary    : Only display one line per creature" << endl
        << "-i id1[,id2,...]: Only show/modify creature with this id" << endl
        << "-nn/--nonicks   : Only show/modify creatures with no custom nickname (migrants)" << endl
        << "--nicks         : Only show/modify creatures with custom nickname" << endl
        << "-ll/--listlabors: List available labors" << endl
        << "--showdead      : Also show/modify dead creatures" << endl
        << "--showallflags  : Show all flags of a creature" << endl
        << endl
        << "Modifying options:" << endl
        << "-al <n>         : Add labor <n> to creature" << endl
        << "-rl <n>         : Remove labor <n> from creature" << endl
        << "-ras            : Remove all skills from creature" << endl
        << "-ral            : Remove all labors from creature" << endl
        << "-ah             : Add hauler labors (stone hauling, etc.) to creature" << endl
        << "-rh             : Remove hauler labors (stone hauling, etc.) from creature" << endl
        // Disabling mood doesn't work as intented
        << "--setmood <n>   : Set mood to n (-1 = no mood, max=4, buggy!)" << endl
        << "--kill          : Kill creature(s) (may need to be called multiple times)" << endl
        // Setting happiness doesn't work really, because hapiness is recalculated
        //<< "--sethappiness <n> : Set happiness to n" << endl
        << "-f              : Force an action" << endl
        << endl
        << "Examples:" << endl
        << endl
        << "Show all dwarfs:" << endl
        << argv[0] << " -c Dwarf" << endl
        << endl
        << "Show summary of all creatures (spoiler: includes unknown creatures):" << endl
        << argv[0] << " -1 -c all" << endl
        << endl
        << "Kill that nasty ogre" << endl
        << argv[0] << " -i 52 --kill" << endl
        << endl
        << "Check that the ogre is really dead" << endl
        << argv[0] << " -c ogre --showdead" << endl
        << endl
        << "Remove all skills from dwarfs 15 and 32:" << endl
        << argv[0] << " -i 15,32 -ras" << endl
        << endl
        << "Remove all skills and labors from dwarfs with no custom nickname:" << endl
        << argv[0] << " -c DWARF -nn -ras -ral" << endl
        << endl
        << "Add hauling labors to all dwarfs without nickname (e.g. migrants):" << endl
        << argv[0] << " -c DWARF -nn -ah" << endl
        << endl
        << "Show list of labor ids:" << endl
        << argv[0] << " -c DWARF -ll" << endl
        << endl
        << "Add engraving labor to all dwarfs without nickname (get the labor id from the list above):" << endl
        << argv[0] << " -c DWARF -nn -al 13" << endl
        << endl
        << "Make Urist, Stodir and Ingish miners:" << endl
        << argv[0] << " -i 31,42,77 -al 0" << endl
        ;
        if (quiet == false) {
            cout << "Press any key to continue" << endl;
            cin.ignore();
        }
}

DFHack::Materials * Materials;
DFHack::VersionInfo *mem;
DFHack::Creatures * Creatures = NULL;

// Note that toCaps() changes the string itself and I'm using it a few times in
// an unsafe way below. Didn't crash yet however.
std::string toCaps(std::string s)
{
    const int length = s.length();
    bool caps=true;
    if (length == 0) {
        return s;
    }
    for(int i=0; i!=length ; ++i)
    {
        if (caps)
        {
            s[i] = std::toupper(s[i]);
            caps = false;
        }
        else if (s[i] == '_' || s[i] == ' ')
        {
            s[i] = ' ';
            caps = true;
        }
        else
        {
            s[i] = std::tolower(s[i]);
        }
    }
    return s;
}

int strtoint(const string &str)
{
    stringstream ss(str);
    int result;
    return ss >> result ? result : -1;
}


// A C++ standard library function should be used instead
bool is_in(int m, int set[], int set_size)
{
    for (int i=0; i<set_size; i++)
    {
        if (m == set[i])
            return true;
    }
    return false;
}

int * find_int(std::vector<int> v, int comp)
{
    for (int i=0; i<v.size(); i++)
    {
        //fprintf(stderr, "Comparing %d with %d and returning %x...\n", v[i], comp, &v[i]);
        if (v[i] == comp)
            return &v[i];
    }
    return NULL;
}



void printCreature(DFHack::Context * DF, const DFHack::t_creature & creature, int index)
{


    DFHack::Translation *Tran = DF->getTranslation();
    DFHack::VersionInfo *mem = DF->getMemoryInfo();

    string type="(no type)";
    if (Materials->raceEx[creature.race].rawname[0])
    {
        type = toCaps(Materials->raceEx[creature.race].rawname);
    }

    string name="(no name)";
    if(creature.name.nickname[0])
    {
        name = creature.name.nickname;
    }
    else
    {
        if(creature.name.first_name[0])
        {
            name = toCaps(creature.name.first_name);

            string transName = Tran->TranslateName(creature.name,false);
            if(!transName.empty())
            {
                name += " " + toCaps(transName);
            }
        }
    }

    string profession="";
    try {
        profession = mem->getProfession(creature.profession);
    }
    catch (exception& e)
    {
        cout << "Error retrieving creature profession: " << e.what() << endl;
    }
    if(creature.custom_profession[0])
    {
        profession = creature.custom_profession;
    }


    string job="No Job";
    if(creature.current_job.active)
    {
        job=mem->getJob(creature.current_job.jobId);
        int p=job.size();
        while (p>0 && (job[p]==' ' || job[p]=='\t'))
            p--;
        if (p <= 1)
        {
            stringstream ss;
            ss << creature.current_job.jobId;
            job = ss.str();
        }
    }

    if (showfirstlineonly)
    {
        printf("%3d", index);
        printf(" %-17s", type.c_str());
        printf(" %-32s", name.c_str());
        printf(" %-16s", toCaps(profession).c_str());
        printf(" %-30s", job.c_str());
        printf(" %5d", creature.happiness);
        if (showdead)
        {
            printf(" %-5s", creature.flags1.bits.dead ? "Dead" : "Alive");
        }

        printf("\n");

        return;
    }
    else
    {
        printf("ID: %d", index);
        printf(", %s", type.c_str());
        printf(", %s", name.c_str());
        printf(", %s", toCaps(profession).c_str());
        printf(", Job: %s", job.c_str());
        printf(", Happiness: %d", creature.happiness);
        printf("\n");
    }

    if((creature.mood != NO_MOOD) && (creature.mood<=MAX_MOOD))
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
            bool is_social = is_in(skillid, social_skills, sizeof(social_skills)/sizeof(social_skills[0]));
            if (!is_social || (is_social && showsocial))
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
                if (skillrating > 0 || skillexperience > 0) 
                {
                    cout << "(Skill " << int(skillid) << ") " << setw(16) << skillname << ": " 
                        << skillrating << "/" << skillexperience << endl;
                }
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
                laborname = "(Undefined)";
            }
            bool is_labor = is_in(i, hauler_labors, sizeof(hauler_labors)/sizeof(hauler_labors[0]));
            if (!is_labor || (is_labor && showhauler))
                cout << "(Labor " << i << ") " << setw(16) << laborname << endl;
        }
    }
    if (showallflags)
    {
        DFHack::t_creaturflags1 f1 = creature.flags1;
        DFHack::t_creaturflags2 f2 = creature.flags2;

        if(f1.bits.had_mood){cout<<toCaps("Flag: had_mood") << endl; }
        if(f1.bits.marauder){cout<<toCaps("Flag: marauder") << endl; }
        if(f1.bits.drowning){cout<<toCaps("Flag: drowning") << endl; }
        if(f1.bits.merchant){cout<<toCaps("Flag: merchant") << endl; }
        if(f1.bits.forest){cout<<toCaps("Flag: forest") << endl; }
        if(f1.bits.left){cout<<toCaps("Flag: left") << endl; }
        if(f1.bits.rider){cout<<toCaps("Flag: rider") << endl; }
        if(f1.bits.incoming){cout<<toCaps("Flag: incoming") << endl; }
        if(f1.bits.diplomat){cout<<toCaps("Flag: diplomat") << endl; }
        if(f1.bits.zombie){cout<<toCaps("Flag: zombie") << endl; }
        if(f1.bits.skeleton){cout<<toCaps("Flag: skeleton") << endl; }
        if(f1.bits.can_swap){cout<<toCaps("Flag: can_swap") << endl; }
        if(f1.bits.on_ground){cout<<toCaps("Flag: on_ground") << endl; }
        if(f1.bits.projectile){cout<<toCaps("Flag: projectile") << endl; }
        if(f1.bits.active_invader){cout<<toCaps("Flag: active_invader") << endl; }
        if(f1.bits.hidden_in_ambush){cout<<toCaps("Flag: hidden_in_ambush") << endl; }
        if(f1.bits.invader_origin){cout<<toCaps("Flag: invader_origin") << endl; }
        if(f1.bits.coward){cout<<toCaps("Flag: coward") << endl; }
        if(f1.bits.hidden_ambusher){cout<<toCaps("Flag: hidden_ambusher") << endl; }
        if(f1.bits.invades){cout<<toCaps("Flag: invades") << endl; }
        if(f1.bits.check_flows){cout<<toCaps("Flag: check_flows") << endl; }
        if(f1.bits.ridden){cout<<toCaps("Flag: ridden") << endl; }
        if(f1.bits.caged){cout<<toCaps("Flag: caged") << endl; }
        if(f1.bits.tame){cout<<toCaps("Flag: tame") << endl; }
        if(f1.bits.chained){cout<<toCaps("Flag: chained") << endl; }
        if(f1.bits.royal_guard){cout<<toCaps("Flag: royal_guard") << endl; }
        if(f1.bits.fortress_guard){cout<<toCaps("Flag: fortress_guard") << endl; }
        if(f1.bits.suppress_wield){cout<<toCaps("Flag: suppress_wield") << endl; }
        if(f1.bits.important_historical_figure){cout<<toCaps("Flag: important_historical_figure") << endl; }

        if(f2.bits.swimming){cout<<toCaps("Flag: swimming") << endl; }
        if(f2.bits.sparring){cout<<toCaps("Flag: sparring") << endl; }
        if(f2.bits.no_notify){cout<<toCaps("Flag: no_notify") << endl; }
        if(f2.bits.unused){cout<<toCaps("Flag: unused") << endl; }
        if(f2.bits.calculated_nerves){cout<<toCaps("Flag: calculated_nerves") << endl; }
        if(f2.bits.calculated_bodyparts){cout<<toCaps("Flag: calculated_bodyparts") << endl; }
        if(f2.bits.important_historical_figure){cout<<toCaps("Flag: important_historical_figure") << endl; }
        if(f2.bits.killed){cout<<toCaps("Flag: killed") << endl; }
        if(f2.bits.cleanup_1){cout<<toCaps("Flag: cleanup_1") << endl; }
        if(f2.bits.cleanup_2){cout<<toCaps("Flag: cleanup_2") << endl; }
        if(f2.bits.cleanup_3){cout<<toCaps("Flag: cleanup_3") << endl; }
        if(f2.bits.for_trade){cout<<toCaps("Flag: for_trade") << endl; }
        if(f2.bits.trade_resolved){cout<<toCaps("Flag: trade_resolved") << endl; }
        if(f2.bits.has_breaks){cout<<toCaps("Flag: has_breaks") << endl; }
        if(f2.bits.gutted){cout<<toCaps("Flag: gutted") << endl; }
        if(f2.bits.circulatory_spray){cout<<toCaps("Flag: circulatory_spray") << endl; }
        if(f2.bits.locked_in_for_trading){cout<<toCaps("Flag: locked_in_for_trading") << endl; }
        if(f2.bits.slaughter){cout<<toCaps("Flag: slaughter") << endl; }
        if(f2.bits.underworld){cout<<toCaps("Flag: underworld") << endl; }
        if(f2.bits.resident){cout<<toCaps("Flag: resident") << endl; }
        if(f2.bits.cleanup_4){cout<<toCaps("Flag: cleanup_4") << endl; }
        if(f2.bits.calculated_insulation){cout<<toCaps("Flag: calculated_insulation") << endl; }
        if(f2.bits.visitor_uninvited){cout<<toCaps("Flag: visitor_uninvited") << endl; }
        if(f2.bits.visitor){cout<<toCaps("Flag: visitor") << endl; }
        if(f2.bits.calculated_inventory){cout<<toCaps("Flag: calculated_inventory") << endl; }
        if(f2.bits.vision_good){cout<<toCaps("Flag: vision_good") << endl; }
        if(f2.bits.vision_damaged){cout<<toCaps("Flag: vision_damaged") << endl; }
        if(f2.bits.vision_missing){cout<<toCaps("Flag: vision_missing") << endl; }
        if(f2.bits.breathing_good){cout<<toCaps("Flag: breathing_good") << endl; }
        if(f2.bits.breathing_problem){cout<<toCaps("Flag: breathing_problem") << endl; }
        if(f2.bits.roaming_wilderness_population_source){cout<<toCaps("Flag: roaming_wilderness_population_source") << endl; }
        if(f2.bits.roaming_wilderness_population_source_not_a_map_feature){cout<<toCaps("Flag: roaming_wilderness_population_source_not_a_map_feature") << endl; }
    }
    else
    {
        /* FLAGS 1 */
        if(creature.flags1.bits.dead)       	{ cout << "Flag: Dead" << endl; }
        if(creature.flags1.bits.on_ground)  	{ cout << "Flag: On the ground" << endl; }
        if(creature.flags1.bits.tame)       	{ cout << "Flag: Tame" << endl; }
        if(creature.flags1.bits.royal_guard)	{ cout << "Flag: Royal guard" << endl; }
        if(creature.flags1.bits.fortress_guard)	{ cout << "Flag: Fortress guard" << endl; }

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
    cout << endl;
}

int main (int argc, const char* argv[])
{
    // let's be more useful when double-clicked on windows
#ifndef LINUX_BUILD
    quiet = false;
#endif

    string creature_type = "Dwarf";
    std::vector<int> creature_id;
    bool find_nonicks = false;
    bool find_nicks = false;
    bool remove_skills = false;
    bool remove_labors = false;
    bool kill_creature = false;
    bool make_hauler = false;
    bool remove_hauler = false;
    bool add_labor = false;
    int add_labor_n = NOT_SET;
    bool remove_labor = false;
    int remove_labor_n = NOT_SET;
    bool set_happiness = false;
    int set_happiness_n = NOT_SET;
    bool set_mood = false;
    int set_mood_n = NOT_SET;
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
        int arg_next_int = NOT_SET;
        /* Check if argv[i+1] is a number >= 0 */
        if (i < argc-1) {
            arg_next = argv[i+1];
            arg_next_int = strtoint(arg_next);
            if (arg_next != "0" && arg_next_int == 0) {
                arg_next_int = NOT_SET;
            }
        }

        if(arg_cur == "-q")
        {
            quiet = true;
        }
        else if(arg_cur == "+q")
        {
            quiet = false;
        }
        else if(arg_cur == "-v")
        {
            verbose = true;
        }
        else if(arg_cur == "-1" || arg_cur == "--summary")
        {
            showfirstlineonly = true;
        }
        else if(arg_cur == "-ss" || arg_cur == "--showsocial")
        {
            showsocial = true;
        }
        else if(arg_cur == "+sh" || arg_cur == "-nosh" || arg_cur == "--noshowhauler")
        {
            showhauler = false;
        }
        else if(arg_cur == "--showdead")
        {
            showdead = true;
        }
        else if(arg_cur == "--showallflags")
        {
            showallflags = true;
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
        else if(arg_cur == "-ll" || arg_cur == "--listlabors")
        {
            list_labors = true;
        }
        // add single labor
        else if(arg_cur == "-al" && i < argc-1)
        {
            if (arg_next_int == NOT_SET || arg_next_int >= NUM_CREATURE_LABORS) {
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
            if (arg_next_int == NOT_SET || arg_next_int >= NUM_CREATURE_LABORS) {
                usage(argc, argv);
                return 1;
            }
            remove_labor = true;
            remove_labor_n = arg_next_int;
            i++;
        }
        else if(arg_cur == "--setmood" && i < argc-1)
        {
            if (arg_next_int < NO_MOOD || arg_next_int > MAX_MOOD) {
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
        else if(arg_cur == "--kill")
        {
            kill_creature = true;
            showallflags = true;
            showdead = true;
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
        else if(arg_cur == "-nn" || arg_cur == "--nonicks")
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
            std::stringstream ss(argv[i+1]);
            int num;
            while (ss >> num) {
                creature_id.push_back(num);
                ss.ignore(1);
            }

            creature_type = ""; // if -i is given, match all creatures
            showdead = true;
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
                cout << "Labor " << int(i) << ": " << laborname << endl;
            }
            catch (exception& e) {
                if (verbose) 
                {
                    laborname = "Unknown";
                    cout << "Labor " << int(i) << ": " << laborname << endl;
                }
            }
        }
    }
    else
    {
        if (showfirstlineonly)
        {
            printf("ID  Type              Name/nickname                    Job title        Current job                    Happy%s\n", showdead?" Dead ":"");
            printf("--- ----------------- -------------------------------- ---------------- ------------------------------ -----%s\n", showdead?" -----":"");
        }

        vector<uint32_t> addrs;
        for(uint32_t creature_idx = 0; creature_idx < numCreatures; creature_idx++)
        {
            DFHack::t_creature creature;
            Creatures->ReadCreature(creature_idx,creature);
            /* Check if we want to display/change this creature or skip it */
            bool hasnick = (creature.name.nickname[0] != '\0');

            if (
                    // Check for -i <num> and -c <type>
                    (NULL != find_int(creature_id, creature_idx)
                     || toCaps(string(Materials->raceEx[creature.race].rawname)) == toCaps(creature_type)
                     || "All" == toCaps(creature_type))
                    // Check for -nn
                    && ((find_nonicks == true && hasnick == false)
                        || (find_nicks == true && hasnick == true)
                        || (find_nicks == false && find_nonicks == false))
                    && (find_nonicks == false || creature.name.nickname[0] == '\0')
                    && (showdead == true || !creature.flags1.bits.dead)
               )
            {
                printCreature(DF,creature,creature_idx);
                addrs.push_back(creature.origin);

                bool dochange = (
                        remove_skills
                        || remove_labors || add_labor || remove_labor
                        || make_hauler || remove_hauler 
                        || kill_creature
                        || set_happiness
                        || set_mood
                        );

                if (toCaps(creature_type) == "Dwarf" 
                        && (creature.profession == PROFESSION_CHILD || creature.profession == PROFESSION_BABY))
                {
                    dochange = false;
                }

                bool allow_massdesignation =
                    creature_id.size()==0 || toCaps(creature_type) != "Dwarf" || find_nonicks == true || force_massdesignation;
                if (dochange == true && allow_massdesignation == false)
                {
                    cout
                        << "Not changing creature because none of -c (other than dwarf), -i or -nn was" << endl
                        << "selected. Add -f (force) to override this safety measure." << endl;
                    dochange = false;
                }

                if (dochange) 
                {
                    if(creature.has_default_soul)
                    {
                        if (kill_creature)
                        {
                            /*
                               [quote author=Eldrick Tobin link=topic=58809.msg2178545#msg2178545 date=1302638055]

                               After extensive testing that just ate itself -.-;

                               Runesmith does not unset the following:
                               - Active Invader (sets if they are just about the invade, as Currently 
                               Invading removes this one)
                               - Hidden Ambusher (Just in Case, however it is still set when an Active Invader)
                               - Hidden in Ambush (Just in Case, however it is still set when an Active Invader,
                               until discovery)
                               - Incoming (Sets if something is here yet... wave X of a siege here)
                               - Invader -Fleeing/Leaving
                               - Currently Invading

                               When it nukes something it basically just sets them to 'dead'. It does not also 
                               set them to 'killed'. Show dead will show everything (short of 'vanished'/'deleted'
                               I'd suspect) so one CAN go through the intensive process to revive a broken siege. These
                               particular flags are not visible at the same exact time so multiple passes -even through
                               a narrow segment- are advised.

                               Problem I ran into (last thing before I mention something more DFHack related):
                               I set the Killed Flag (but not dead), and I got mortally wounded siegers that refused to
                               just pift in Magma. [color=purple]Likely missing upper torsoes on examination[/color].

                             */
                            DFHack::t_creaturflags1 f1 = creature.flags1;
                            DFHack::t_creaturflags2 f2 = creature.flags2;

                            f1.bits.dead = 1;
                            f2.bits.killed = 1;
                            f1.bits.active_invader = 0;   /*!< 17: Active invader (for organized ones) */
                            f1.bits.hidden_ambusher = 0;  /*!< 21: Active marauder/invader moving inward? */
                            f1.bits.hidden_in_ambush = 0;
                            f1.bits.invades = 0;          /*!< 22: Marauder resident/invader moving in all the way */

                            cout << "Setting f1.bits.dead = 1" << endl;
                            cout << "Setting f2.bits.killed = 1" << endl;
                            cout << "Setting f1.bits.active_invader = 0" << endl; 
                            cout << "Setting f1.bits.hidden_ambusher = 0" << endl; 
                            cout << "Setting f1.bits.hidden_in_ambush = 0" << endl;
                            cout << "Setting f1.bits.invades = 0" << endl;          
                            cout << "Writing flags..." << endl;
                            if (!Creatures->WriteFlags(creature_idx, f1.whole, f2.whole))
                            {
                                cout << "Error writing creature flags!" << endl;
                            }
                            // We want the flags to be shown after our modification, but they are not read back
                            creature.flags1 = f1;
                            creature.flags2 = f2;
                        }

                        if (set_mood) 
                        {
                            /* Doesn't really work to disable a mood */
                            cout << "Setting mood to " << set_mood_n << "..." << endl;
                            Creatures->WriteMood(creature_idx, set_mood_n);
                            DFHack::t_creaturflags1 f1 = creature.flags1;
                            DFHack::t_creaturflags2 f2 = creature.flags2;
                            f1.bits.has_mood = (set_mood_n == NO_MOOD ? 0 : 1);
                            if (!Creatures->WriteFlags(creature_idx, f1.whole, f2.whole))
                            {
                                cout << "Error writing creature flags!" << endl;
                            }
                            creature.flags1 = f1;
                            creature.flags2 = f2;
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

                        if (add_labor || remove_labor || remove_labors || make_hauler || remove_hauler)
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
                                for (int labs=0;
                                        labs < sizeof(hauler_labors)/sizeof(hauler_labors[0]);
                                        labs++)
                                {
                                    creature.labors[hauler_labors[labs]] = 0;
                                }
                            }

                            if (make_hauler) {
                                cout << "Setting hauler labors..." << endl;
                                for (int labs=0;
                                        labs < sizeof(hauler_labors)/sizeof(hauler_labors[0]);
                                        labs++)
                                {
                                    creature.labors[hauler_labors[labs]] = 1;
                                }
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
