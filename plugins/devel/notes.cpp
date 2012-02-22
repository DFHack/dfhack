#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <vector>
#include <string>
#include <modules/Notes.h>

using std::vector;
using std::string;
using namespace DFHack;

command_result df_notes (Core * c, vector <string> & parameters);

DFHACK_PLUGIN("notes");

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("dumpnotes",
               "Dumps in-game notes",
               df_notes));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

command_result df_notes (Core * c, vector <string> & parameters)
{
    Console & con = c->con;
    c->Suspend();

    DFHack::Notes * note_mod = c->getNotes();
    std::vector<t_note*>* note_list = note_mod->notes;

    if (note_list == NULL)
    {
        con.printerr("Notes are not supported under this version of DF.\n");
        c->Resume();
        return CR_OK;
    }

    if (note_list->empty())
    {
        con << "There are no notes." << std::endl;
        c->Resume();
        return CR_OK;
    }


    for (size_t i = 0; i < note_list->size(); i++)
    {
        t_note* note = (*note_list)[i];

        con.print("Note %x at: %d/%d/%d\n",note, note->x, note->y, note->z);
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
