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

using namespace DFHack;
using namespace DFHack::Simple;
using namespace DFHack::Simple;

using df::global::ui;
using df::global::world;

// our own, empty header.
#include "dwarfexport.h"
#include <df/personality_facet_type.h>


// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result export_dwarves (Core * c, std::vector <std::string> & parameters);

// A plugins must be able to return its name. This must correspond to the filename - export.plug.so or export.plug.dll
DFhackCExport const char * plugin_name ( void )
{
    return "dwarfexport";
}

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.clear();
    commands.push_back(PluginCommand("dwarfexport",
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

static const char* physicals[] = {
    "Strength",
    "Agility",
    "Toughness",
    "Endurance",
    "Recuperation",
    "DiseaseResistance",
};

static const char* mentals[] = {
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

static void printAttributes(Core* c, df::unit* cre, ostream& out) {
    out << "    <Attributes>" << endl;
    for (int i = 0; i < NUM_CREATURE_PHYSICAL_ATTRIBUTES; i++) {
        element(physicals[i], cre->body.physical_attrs[i].unk1, out, "  ");
    }

    df::unit_soul * s = cre->status.current_soul;
    if (s) {
        for (int i = 0; i < NUM_CREATURE_MENTAL_ATTRIBUTES; i++) {
            element(mentals[i], s->mental_attrs[i].unk1, out, "  ");
        }
    }
    out << "    </Attributes>" << endl;
}

static void printTraits(Core* c, df::unit* cre, ostream& out)
{
    
    out << "    <Traits>" << endl;
    df::unit_soul * s = cre->status.current_soul;
    if (s)
    {
        FOR_ENUM_ITEMS(personality_facet_type,index)
        {
            out << "      <Trait name='" << df::enums::personality_facet_type::get_key(index) <<
                "' value='" << s->traits[index] << "'>";
            //FIXME: needs reimplementing trait string generation
            /*
            string trait = c->vinfo->getTrait(i, s->traits[i]);
            if (!trait.empty()) {
                out << trait.c_str();
            }
            */
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
static void export_dwarf(Core* c, df::unit* cre, ostream& out) {
    string info = cre->name.first_name;
    info += " ";
    info += Translation::TranslateName(&cre->name, false);
    info[0] = toupper(info[0]);
    c->con.print("Exporting %s\n", info.c_str());

    out << "  <Creature>" << endl;
    element("Name", info.c_str(), out);
    element("Nickname", cre->name.nickname.c_str(), out);
    element("Sex", cre->sex == 0 ? "Female" : "Male", out);
    printAttributes(c, cre, out);
    printTraits(c, cre, out);

    out << "  </Creature>" << endl;
}

command_result export_dwarves (Core * c, std::vector <std::string> & parameters)
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

    uint32_t race = ui->race_id;
    uint32_t civ = ui->civ_id;

    outf << "<?xml version='1.0' encoding='ibm850'?>" << endl << "<Creatures>" << endl;
    
    for (int i = 0; i < world->units.all.size(); ++i)
    {
        df::unit* cre = world->units.all[i];
        if (cre->race == race && cre->civ_id == civ) {
            export_dwarf(c, cre, outf);
        }
    }
    outf << "</Creatures>" << endl;

    c->Resume();
    return CR_OK;
}
