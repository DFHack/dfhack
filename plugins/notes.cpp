#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>
#include <dfhack/modules/Notes.h>

using std::vector;
using std::string;
using namespace DFHack;

DFhackCExport command_result df_notes (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "notes";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("dumpnotes",
               "Dumps in-game notes",
               df_notes));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result df_notes (Core * c, vector <string> & parameters)
{
    Console & con = c->con;
    c->Suspend();

    DFHack::Notes * note_mod = c->getNotes();
    std::vector<t_note*>* note_list = note_mod->getNotes();

    if (note_list == NULL)
    {
        con << "No notes yet." << std::endl;
        c->Resume();
        return CR_OK;
    }

    if (note_list->empty())
    {
        con << "All notes deleted." << std::endl;
        c->Resume();
        return CR_OK;
    }


    for (size_t i = 0; i < note_list->size(); i++)
    {
        t_note* note = (*note_list)[i];

        con.print("Note at: %d/%d/%d\n", note->x, note->y, note->z);
        con.print("Note id: %d\n", note->id);
        con.print("Note symbol: '%c'\n", note->symbol);

        if (note->name.length() > 0)
            con << "Note name: " << (note->name) << std::endl;
        if (note->text.length() > 0)
            con << "Note text: " << (note->text) << std::endl;

        if (note->unk1 != 0)
            con.print("unk1: %x\n", note->unk1);
        if (note->unk2 != 0)
            con.print("unk2: %x\n", note->unk2);

        con << std::endl;
    }

    c->Resume();
    return CR_OK;
}
