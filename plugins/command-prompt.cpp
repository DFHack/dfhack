//command-prompt a one line command entry at the top of the screen for quick commands

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <modules/Screen.h>

#include <set>

#include "df/interface_key.h"
#include "df/ui.h"
#include "df/graphic.h"
#include "df/enabler.h"
using namespace DFHack;
using namespace df::enums;

using df::global::ui;
using df::global::gps;
using df::global::enabler;

class viewscreen_commandpromptst : public dfhack_viewscreen {
public:
    void feed(std::set<df::interface_key> *events);

    void logic() {
        dfhack_viewscreen::logic();
    }

    void render();
    void help() { }

    std::string getFocusString() { return "commandprompt"; }
    viewscreen_commandpromptst()
    {
        show_fps=df::global::gps->display_frames;
        df::global::gps->display_frames=0;
    }
    ~viewscreen_commandpromptst()
    {
        df::global::gps->display_frames=show_fps;
    }
protected:
    bool show_fps;
    void submit();
    std::string entry;
};
void viewscreen_commandpromptst::render()
{
    if (Screen::isDismissed(this))
        return;

    dfhack_viewscreen::render();

    auto dim = Screen::getWindowSize();
    parent->render();
    Screen::fillRect(Screen::Pen(' ', 7, 0),0,0,dim.x,0);
    Screen::paintString(Screen::Pen(' ', 7, 0), 0, 0,"[DFHack]#");
    if(entry.size()<dim.x)
        Screen::paintString(Screen::Pen(' ', 7, 0), 10,0 , entry);
    else
    {
        Screen::paintTile(Screen::Pen('>', 7, 0), 9, 0);
        Screen::paintString(Screen::Pen(' ', 7, 0), 10, 0, entry.substr(entry.size()-dim.x));
    }
    
}
void viewscreen_commandpromptst::submit()
{
    CoreSuspendClaimer suspend;
    color_ostream_proxy out(Core::getInstance().getConsole());
    Core::getInstance().runCommand(out, entry);
    Screen::dismiss(this);
}
void viewscreen_commandpromptst::feed(std::set<df::interface_key> *events)
{

    bool leave_all = events->count(interface_key::LEAVESCREEN_ALL);
    if (leave_all || events->count(interface_key::LEAVESCREEN))
    {
        events->clear();
        Screen::dismiss(this);
        if (leave_all)
        {
            events->insert(interface_key::LEAVESCREEN);
            parent->feed(events);
            events->clear();
        }
        return;
    }
    if(events->count(interface_key::SELECT))
    {
        submit();
        return;
    }
    for (auto it = events->begin(); it != events->end(); ++it)
    {
        auto key = *it;
        if (key==interface_key::STRING_A000) //delete?
        {
            if(entry.size())
                entry.pop_back();
            continue;
        }
        if (key >= interface_key::STRING_A000 &&
            key <= interface_key::STRING_A255)
        {
            entry.push_back(char(key - interface_key::STRING_A000));
        }
    }
}
DFHACK_PLUGIN("command-prompt");
command_result show_prompt(color_ostream &out, std::vector <std::string> & parameters)
{
    Screen::show(new viewscreen_commandpromptst);
    return CR_OK;
}
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "command-prompt","Shows a command prompt on window.",show_prompt
        ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}