/* Fixes bug 3708 (Ghosts that can't be engraved on a slab).

   Cause of the bug:

      In order to be engraved on a slab, the creature must be
    a historical figure, i.e. be in the historical figure list
    of the Legends mode. It seems that caravan guards are not
    added to that list until they do something notable, e.g.
    kill a goblin. Unfortunately, their own death doesn't
    trigger this sometimes.

   Solution:

      Steal a historical figure entry from a dead goblin, by
    replacing the IDs in the structures; also overwrite his
    name, race and profession to make the menus make slightly
    more sense.

   Downsides:

    - Obviously, this is an ugly hack.
    - The Legends mode still lists the guard as belonging to
      the goblin civilization, and killed by whoever killed the
      original goblin. There might be other inconsistencies.

   Positive sides:

    - Avoids messing with tricky creature control code,
      by allowing the ghost to be removed naturally.
 */

#include <iostream>
#include <climits>
#include <string.h>
#include <vector>
#include <list>
#include <stdio.h>
using namespace std;

#define DFHACK_WANT_MISCUTILS
#include <DFHack.h>

enum likeType
{
    FAIL = 0,
    MATERIAL = 1,
    ITEM = 2,
    FOOD = 3
};

DFHack::Materials * Materials;
DFHack::VersionInfo *mem;
DFHack::Creatures * Creatures = NULL;

void printCreature(DFHack::Context * DF, const DFHack::t_creature & creature)
{
    cout << "Address: " << hex <<  creature.origin << dec << ", creature race: " << Materials->raceEx[creature.race].rawname
         << ", position: " << creature.x << "x " << creature.y << "y "<< creature.z << "z" << endl
         << "Name: " << creature.name.first_name;

    if (creature.name.nickname[0])
        cout << " `" << creature.name.nickname << "'";

    DFHack::Translation * Tran = DF->getTranslation();

    cout << " " << Tran->TranslateName(creature.name,false)
         << " (" << Tran->TranslateName(creature.name,true) << ")" << endl;

    cout << "Profession: " << mem->getProfession(creature.profession);

    if(creature.custom_profession[0])
        cout << ", custom: " << creature.custom_profession;

    uint32_t dayoflife = creature.birth_year*12*28 + creature.birth_time/1200;
    cout << endl
         << "Born on the year " << creature.birth_year
         << ", month " << (creature.birth_time/1200/28)
         << ", day " << ((creature.birth_time/1200) % 28 + 1)
         << ", " << dayoflife << " days lived." << endl << endl;
}


int main (int numargs, char ** args)
{
    DFHack::World * World;
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
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    Creatures = DF->getCreatures();
    Materials = DF->getMaterials();
    World = DF->getWorld();
    DFHack::Translation * Tran = DF->getTranslation();

    uint32_t numCreatures;
    if(!Creatures->Start(numCreatures))
    {
        cerr << "Can't get creatures" << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    Materials->ReadCreatureTypes();
    Materials->ReadCreatureTypesEx();

    mem = DF->getMemoryInfo();
    DFHack::Process *p = DF->getProcess();

    if(!Tran->Start())
    {
        cerr << "Can't get name tables" << endl;
        return 1;
    }

    DFHack::OffsetGroup *ogc = mem->getGroup("Creatures")->getGroup("creature");
    uint32_t o_flags3 = ogc->getOffset("flags3");
    uint32_t o_c_hfid = ogc->getGroup("advanced")->getOffset("hist_figure_id");

    std::list<uint32_t> goblins;
    std::list<uint32_t> ghosts;

    for(uint32_t i = 0; i < numCreatures; i++)
    {
        DFHack::t_creature temp;
        Creatures->ReadCreature(i,temp);

        int32_t hfid = p->readDWord(temp.origin + o_c_hfid);

        if (hfid > 0) {
            if (temp.flags1.bits.dead) {
                std::string name = Materials->raceEx[temp.race].rawname;
                if (name == "GOBLIN")
                    goblins.push_back(i);
            }
        } else {
            uint32_t flags3 = p->readDWord(temp.origin + o_flags3);
            if (!(flags3 & 0x1000))
                continue;

            ghosts.push_back(i);
        }
    }

    if (goblins.size() >= ghosts.size() && ghosts.size() > 0)
    {
        DFHack::OffsetGroup *grp_figures = mem->getGroup("Legends")->getGroup("figures");
        uint32_t f_vector = p->readDWord(grp_figures->getAddress("vector"));
        uint32_t f_id = grp_figures->getOffset("figure_id");
        uint32_t f_unit = grp_figures->getOffset("unit_id");
        uint32_t f_name = grp_figures->getOffset("name");
        uint32_t f_race = grp_figures->getOffset("race");
        uint32_t f_profession = grp_figures->getOffset("profession");

        for (std::list<uint32_t>::iterator it = ghosts.begin(); it != ghosts.end(); ++it)
        {
            int i = *it;
            DFHack::t_creature ghost;
            Creatures->ReadCreature(i,ghost);

            printCreature(DF,ghost);

            int igoblin = goblins.front();
            goblins.pop_front();
            DFHack::t_creature goblin;
            Creatures->ReadCreature(igoblin,goblin);

            printCreature(DF,goblin);

            int32_t hfid = p->readDWord(goblin.origin + o_c_hfid);
            uint32_t fptr = p->readDWord(f_vector + 4*hfid);

            if (p->readDWord(fptr + f_id) != hfid ||
                p->readDWord(fptr + f_unit) != goblin.id ||
                p->readWord(fptr + f_race) != goblin.race)
            {
                cout << "Data structure inconsistency detected, aborting.";
                break;
            }

            if (1) {
                p->writeDWord(goblin.origin + o_c_hfid, -1);
                p->writeDWord(ghost.origin + o_c_hfid, hfid);
                p->writeDWord(fptr + f_unit, ghost.id);
                p->writeWord(fptr + f_race, ghost.race);
                p->writeWord(fptr + f_profession, ghost.profession);
                Creatures->CopyNameTo(ghost, fptr + f_name);
                cout << "Pair succesfully patched." << endl << endl;
            }
        }
    }
    else
    {
        cout << "No suitable ghosts, or not enough goblins." << endl;
    }

    Creatures->Finish();
    DF->Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
