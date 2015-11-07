// some headers required for a plugin. Nothing special, just the basics.
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

#define DFHACK_WANT_MISCUTILS
#include <Core.h>
#include <VersionInfo.h>
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Units.h>
#include <modules/Translation.h>

#include <df/ui.h>
#include <df/world.h>
#include <df/unit.h>
#include <df/unit_soul.h>
#include <df/unit_labor.h>
#include <df/unit_skill.h>
/*
dwarfexport
===========
Export dwarves to RuneSmith-compatible XML; also unused by modern tools.
*/

using namespace DFHack;
using df::global::ui;
using df::global::world;

// our own, empty header.
#include "dwarfexport.h"
#include <df/personality_facet_type.h>


// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result export_dwarves (color_ostream &con, std::vector <std::string> & parameters);

DFHACK_PLUGIN("dwarfexport");

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init (color_ostream &con, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand("dwarfexport",
                                     "Export dwarves to RuneSmith-compatible XML.",
                                     export_dwarves /*,
                                     true or false - true means that the command can't be used from non-interactive user interface'*/));
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown (color_ostream &con)
{
    return CR_OK;
}

static const char* physicals[] = {
    "Strength",
    "Agility",
    "Toughness",
    "Endurance",
    "Recuperation",
    "DiseaseResistance",
};

static const char* mentals[] = {
    "AnalyticalAbility",
    "Focus",
    "Willpower",
    "Creatvity", //Speeling deliberate
    "Intuition",
    "Patience",
    "Memory",
    "LinguisticAbility",
    "SpatialSense",
    "Musicality",
    "KinaestheticSense",
    "Empathy",
    "SocialAwareness",
};

static void element(const char* name, const char* content, ostream& out, const char* extra_indent="") {
    out << extra_indent << "    <" << name << ">" << content << "</" << name << ">" << endl;
}

static void element(const char* name, const uint32_t content, ostream& out, const char* extra_indent="") {
    out << extra_indent << "    <" << name << ">" << content << "</" << name << ">" << endl;
}

static void printAttributes(color_ostream &con, df::unit* cre, ostream& out) {
    out << "    <Attributes>" << endl;
    for (int i = 0; i < NUM_CREATURE_PHYSICAL_ATTRIBUTES; i++) {
        element(physicals[i], cre->body.physical_attrs[i].value, out, "  ");
    }

    df::unit_soul * s = cre->status.current_soul;
    if (s) {
        for (int i = 0; i < NUM_CREATURE_MENTAL_ATTRIBUTES; i++) {
            element(mentals[i], s->mental_attrs[i].value, out, "  ");
        }
    }
    out << "    </Attributes>" << endl;
}

static void printTraits(color_ostream &con, df::unit* cre, ostream& out)
{

    out << "    <Traits>" << endl;
    df::unit_soul * s = cre->status.current_soul;
    if (s)
    {
        FOR_ENUM_ITEMS(personality_facet_type,index)
        {
            out << "      <Trait name='" << ENUM_KEY_STR(personality_facet_type, index) <<
                "' value='" << s->traits[index] << "'>";
            //FIXME: needs reimplementing trait string generation
            /*
            string trait = con->vinfo->getTrait(i, s->traits[i]);
            if (!trait.empty()) {
                out << trait.c_str();
            }
            */
            out << "</Trait>" << endl;

        }
    }
    out << "    </Traits>" << endl;
}

static int32_t getCreatureAge(df::unit* cre)
{
    int32_t yearDifference = *df::global::cur_year - cre->relations.birth_year;

    // If the birthday this year has not yet passed, subtract one year.
    // ASSUMPTION: birth_time is on the same scale as cur_year_tick
    if (cre->relations.birth_time >= *df::global::cur_year_tick) {
        yearDifference--;
    }

    return yearDifference;
}

static void printLabors(color_ostream &con, df::unit* cre, ostream& out)
{
    // Using British spelling here, consistent with Runesmith
    out << "  <Labours>" << endl;
    FOR_ENUM_ITEMS(unit_labor, iCount)
    {
        if (cre->status.labors[iCount]) {
            // Get the caption for the labor index.
            element("Labour", ENUM_ATTR_STR(unit_labor, caption, iCount), out);
        }
    }
    out << "  </Labours>" << endl;
}

static void printSkill(color_ostream &con, df::unit_skill* skill, ostream& out)
{
    out << "    <Skill>" << endl;

    element("Name", ENUM_ATTR_STR(job_skill, caption, skill->id), out);
    element("Level", skill->rating, out);

    out << "    </Skill>" << endl;
}

static void printSkills(color_ostream &con, df::unit* cre, ostream& out)
{

    std::vector<df::unit_skill* > vSkills = cre->status.current_soul->skills;

    out << "  <Skills>" << endl;
    for (int iCount = 0; iCount < vSkills.size(); iCount++)
    {
        printSkill(con, vSkills.at(iCount), out);
    }

    out << "  </Skills>" << endl;
}

// GDC needs:
// Name
// Nickname
// Sex
// Attributes
// Traits
static void export_dwarf(color_ostream &con, df::unit* cre, ostream& out) {
    string info = cre->name.first_name;
    info += " ";
    info += Translation::TranslateName(&cre->name, false);
    info[0] = toupper(info[0]);
    con.print("Exporting %s\n", info.c_str());

    out << "  <Creature>" << endl;
    element("Name", info.c_str(), out);
    element("Nickname", cre->name.nickname.c_str(), out);
    element("Sex", cre->sex == 0 ? "Female" : "Male", out);
    element("Age", getCreatureAge(cre), out);       // Added age, active labors, and skills March 9, 2012
    printAttributes(con, cre, out);
    printTraits(con, cre, out);
    printLabors(con, cre, out);
    printSkills(con, cre, out);

    out << "  </Creature>" << endl;
}

command_result export_dwarves (color_ostream &con, std::vector <std::string> & parameters)
{
    string filename;
    if (parameters.size() == 1) {
        filename = parameters[0];
    } else {
        con.print("export <filename>\n");
        return CR_OK;
    }

    ofstream outf(filename.c_str());
    if (!outf) {
        con.printerr("Failed to open file %s\n", filename.c_str());
        return CR_FAILURE;
    }

    CoreSuspender suspend;

    uint32_t race = ui->race_id;
    uint32_t civ = ui->civ_id;

    outf << "<?xml version='1.0' encoding='ibm850'?>" << endl << "<Creatures>" << endl;

    for (int i = 0; i < world->units.all.size(); ++i)
    {
        df::unit* cre = world->units.all[i];
        if (cre->race == race && cre->civ_id == civ) {
            export_dwarf(con, cre, outf);
        }
    }
    outf << "</Creatures>" << endl;

    return CR_OK;
}
