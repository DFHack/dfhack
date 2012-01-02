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
using namespace DFHack;

// our own, empty header.
#include "export.h"


// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
DFhackCExport command_result export_dwarves (Core * c, std::vector <std::string> & parameters);

// A plugins must be able to return its name. This must correspond to the filename - export.plug.so or export.plug.dll
DFhackCExport const char * plugin_name ( void )
{
    return "export";
}

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.clear();
    commands.push_back(PluginCommand("export",
                                     "Export dwarves to RuneSmith-compatible XML.",
                                     export_dwarves /*,
                                     true or false - true means that the command can't be used from non-interactive user interface'*/));
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

static char* physicals[] = {
    "Strength",
    "Agility",
    "Toughness",
    "Endurance",
    "Recuperation",
    "DiseaseResistance",
};

static char* mentals[] = {
    "Willpower",
    "Memory",
    "Focus",
    "Intuition",
    "Patience",
    "Empathy",
    "SocialAwareness",
    "Creatvity", //Speeling deliberate
    "Musicality",
    "AnalyticalAbility",
    "LinguisticAbility",
    "SpatialSense",
    "KinaestheticSense",
};

static void element(const char* name, const char* content, ostream& out, const char* extra_indent="") {
    out << extra_indent << "    <" << name << ">" << content << "</" << name << ">" << endl;
}

static void element(const char* name, const uint32_t content, ostream& out, const char* extra_indent="") {
    out << extra_indent << "    <" << name << ">" << content << "</" << name << ">" << endl;
}

static void printAttributes(Core* c, Translation* t, t_unit* cre, ostream& out) {
    out << "    <Attributes>" << endl;
    for (int i = 0; i < NUM_CREATURE_PHYSICAL_ATTRIBUTES; i++) {
        element(physicals[i], cre->origin->physical[i].unk_0, out, "  ");
    }

    df_soul * s = cre->origin->current_soul;
    if (s) {
        for (int i = 0; i < NUM_CREATURE_MENTAL_ATTRIBUTES; i++) {
            element(mentals[i], s->mental[i].unk_0, out, "  ");
        }
    }
    out << "    </Attributes>" << endl;
}

static void printTraits(Core* c, Translation* t, t_unit* cre, ostream& out) {
    out << "    <Traits>" << endl;
    df_soul * s = cre->origin->current_soul;
    if (s) {
        for (int i = 0; i < NUM_CREATURE_TRAITS; i++) {
            out << "      <Trait name='" << c->vinfo->getTraitName(i) <<
                "' value='" << s->traits[i] << "'>";
            string trait = c->vinfo->getTrait(i, s->traits[i]);
            if (!trait.empty()) {
                out << trait.c_str();
            }
            out << "</Trait>" << endl;
        }
    }
    out << "    </Traits>" << endl;
}

// GDC needs:
// Name
// Nickname
// Sex
// Attributes
// Traits
static void export_dwarf(Core* c, Translation* t, t_unit* cre, ostream& out) {
    string info = cre->name.first_name;
    info += " ";
    info += t->TranslateName(&cre->origin->name, false);
    info[0] = toupper(info[0]);
    c->con.print("Exporting %s\n", info.c_str());

    out << "  <Creature>" << endl;
    element("Name", info.c_str(), out);
    element("Nickname", cre->name.nickname, out);
    element("Sex", cre->sex == 0 ? "Female" : "Male", out);
    printAttributes(c, t, cre, out);
    printTraits(c, t, cre, out);

    out << "  </Creature>" << endl;
}

DFhackCExport command_result export_dwarves (Core * c, std::vector <std::string> & parameters)
{
    string filename;
    if (parameters.size() == 1) {
        filename = parameters[0];
    } else {
        c->con.print("export <filename>\n");
        return CR_OK;
    }

    ofstream outf(filename);
    if (!outf) {
        c->con.printerr("Failed to open file %s\n", filename.c_str());
        return CR_FAILURE;
    }

    c->Suspend();

    Translation* t = c->getTranslation();
    t->Start();

    DFHack::Units* cr = c->getUnits();
    uint32_t race = cr->GetDwarfRaceIndex();
    uint32_t civ = cr->GetDwarfCivId();

    uint32_t num_creatures;
    cr->Start(num_creatures);

    outf << "<?xml version='1.0' encoding='ibm850'?>" << endl << "<Creatures>" << endl;
    
    for (unsigned i = 0; i < num_creatures; ++i)
    {
        df_unit* cre = cr->GetCreature(i);
        if (cre->race == race && cre->civ == civ) {
            t_unit t_cre;
            cr->CopyCreature(cre, t_cre);
            export_dwarf(c, t, &t_cre, outf);
        }
    }
    outf << "</Creatures>" << endl;

    c->Resume();
    return CR_OK;
}
