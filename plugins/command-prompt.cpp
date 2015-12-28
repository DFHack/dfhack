//command-prompt a one line command entry at the top of the screen for quick commands

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <ColorText.h>

#include <modules/Screen.h>
#include <modules/Gui.h>

#include <set>
#include <list>
#include <utility>
#include <vector>

#include "df/interface_key.h"
#include "df/ui.h"
#include "df/graphic.h"
#include "df/enabler.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("command-prompt");
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(enabler);

std::vector<std::string> command_history;

class viewscreen_commandpromptst;
class prompt_ostream:public buffered_color_ostream
{
    viewscreen_commandpromptst *parent_;
    protected:
        void flush_proxy();
    public:
        prompt_ostream(viewscreen_commandpromptst* parent):parent_(parent){}
        bool empty(){return buffer.empty();}
};
class viewscreen_commandpromptst : public dfhack_viewscreen {
public:
    void feed(std::set<df::interface_key> *events);

    void logic() {
        dfhack_viewscreen::logic();
    }

    void render();
    void help() { }
    int8_t movies_okay() { return 0; }

    df::unit* getSelectedUnit() { return Gui::getAnyUnit(parent); }
    df::item* getSelectedItem() { return Gui::getAnyItem(parent); }
    df::building* getSelectedBuilding() { return Gui::getAnyBuilding(parent); }

    std::string getFocusString() { return "commandprompt"; }
    viewscreen_commandpromptst(std::string entry):is_response(false), submitted(false)
    {
        show_fps=gps->display_frames;
        gps->display_frames=0;
        cursor_pos = entry.size();
        frame = 0;
        history_idx = command_history.size();
        if (history_idx > 0)
        {
            if (command_history[history_idx - 1] == "")
            {
                command_history.pop_back();
                history_idx--;
            }
        }
        command_history.push_back(entry);
    }
    ~viewscreen_commandpromptst()
    {
        gps->display_frames=show_fps;
    }

    void add_response(color_value v, std::string s)
    {
        std::stringstream ss(s);
        std::string part;
        while (std::getline(ss, part))
        {
            responses.push_back(std::make_pair(v, part + '\n'));
        }
    }
    std::string get_entry()
    {
        return command_history[history_idx];
    }
    void set_entry(std::string entry)
    {
        command_history[history_idx] = entry;
    }
    void back_word()
    {
        std::string entry = get_entry();
        if (cursor_pos == 0)
            return;
        cursor_pos--;
        while (cursor_pos > 0 && !isalnum(entry[cursor_pos]))
            cursor_pos--;
        while (cursor_pos > 0 && isalnum(entry[cursor_pos]))
            cursor_pos--;
        if (!isalnum(entry[cursor_pos]) && cursor_pos != 0)
            cursor_pos++;
    }
    void forward_word()
    {
        std::string entry = get_entry();
        int len = entry.size();
        if (cursor_pos == len)
            return;
        cursor_pos++;
        while (cursor_pos <= len && !isalnum(entry[cursor_pos]))
            cursor_pos++;
        while (cursor_pos <= len && isalnum(entry[cursor_pos]))
            cursor_pos++;
        if (cursor_pos > len)
            cursor_pos = len;
    }

protected:
    std::list<std::pair<color_value,std::string> > responses;
    int cursor_pos;
    int history_idx;
    bool submitted;
    bool is_response;
    bool show_fps;
    int frame;
    void submit();
};
void prompt_ostream::flush_proxy()
{
    if (buffer.empty())
        return;
    for(auto it=buffer.begin();it!=buffer.end();it++)
        parent_->add_response(it->first,it->second);
    buffer.clear();
}
void viewscreen_commandpromptst::render()
{
    ++frame;
    if (frame >= enabler->gfps)
        frame = 0;
    if (Screen::isDismissed(this))
        return;

    dfhack_viewscreen::render();

    auto dim = Screen::getWindowSize();
    parent->render();
    if(is_response)
    {
        auto it=responses.begin();
        for(int i=0;i<dim.y && it!=responses.end();i++,it++)
        {
            Screen::fillRect(Screen::Pen(' ', 7, 0),0,i,dim.x,i);
            std::string cur_line=it->second;
            Screen::paintString(Screen::Pen(' ',it->first,0),0,i,cur_line.substr(0,cur_line.size()-1));
        }
    }
    else
    {
        std::string entry = get_entry();
        Screen::fillRect(Screen::Pen(' ', 7, 0),0,0,dim.x,0);
        Screen::paintString(Screen::Pen(' ', 7, 0), 0, 0,"[DFHack]#");
        std::string cursor = (frame < enabler->gfps / 2) ? "_" : " ";
        if(cursor_pos < (dim.x - 10))
        {
            Screen::paintString(Screen::Pen(' ', 7, 0), 10,0 , entry);
            if (entry.size() > dim.x - 10)
                Screen::paintTile(Screen::Pen('\032', 7, 0), dim.x - 1, 0);
            if (cursor != " ")
                Screen::paintString(Screen::Pen(' ', 10, 0), 10 + cursor_pos, 0, cursor);
        }
        else
        {
            size_t start = cursor_pos - dim.x + 10 + 1;
            Screen::paintTile(Screen::Pen('\033', 7, 0), 9, 0);
            Screen::paintString(Screen::Pen(' ', 7, 0), 10, 0, entry.substr(start));
            if (cursor != " ")
                Screen::paintString(Screen::Pen(' ', 10, 0), dim.x - 1, 0, cursor);
        }
    }
}
void viewscreen_commandpromptst::submit()
{
    CoreSuspendClaimer suspend;
    if(is_response)
    {
        Screen::dismiss(this);
        return;
    }
    if(submitted)
        return;
    submitted = true;
    prompt_ostream out(this);
    Core::getInstance().runCommand(out, get_entry());
    if(out.empty() && responses.empty())
        Screen::dismiss(this);
    else
    {
        is_response=true;
    }
}
void viewscreen_commandpromptst::feed(std::set<df::interface_key> *events)
{
    int old_pos = cursor_pos;
    std::string entry = get_entry();
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
        //if (command_history.size() && !entry.size())
        //    command_history.pop_back();
        return;
    }
    if (events->count(interface_key::SELECT))
    {
        submit();
        return;
    }
    if (is_response)
        return;
    for (auto it = events->begin(); it != events->end(); ++it)
    {
        auto key = *it;
        if (key==interface_key::STRING_A000) //delete?
        {
            if(entry.size() && cursor_pos > 0)
            {
                entry.erase(cursor_pos - 1, 1);
                cursor_pos--;
            }
            if(cursor_pos > entry.size())
                cursor_pos = entry.size();
            continue;
        }
        int charcode = Screen::keyToChar(key);
        if (charcode > 0)
        {
            entry.insert(cursor_pos, 1, char(charcode));
            cursor_pos++;
            set_entry(entry);
            return;
        }
    }
    // Prevent number keys from moving cursor
    if(events->count(interface_key::CURSOR_RIGHT))
    {
        cursor_pos++;
        if (cursor_pos > entry.size())
            cursor_pos = entry.size();
    }
    else if(events->count(interface_key::CURSOR_LEFT))
    {
        cursor_pos--;
        if (cursor_pos < 0) cursor_pos = 0;
    }
    else if(events->count(interface_key::CURSOR_RIGHT_FAST))
    {
        forward_word();
    }
    else if(events->count(interface_key::CURSOR_LEFT_FAST))
    {
        back_word();
    }
    else if(events->count(interface_key::CUSTOM_CTRL_A))
    {
        cursor_pos = 0;
    }
    else if(events->count(interface_key::CUSTOM_CTRL_E))
    {
        cursor_pos = entry.size();
    }
    else if(events->count(interface_key::CURSOR_UP))
    {
        history_idx--;
        if (history_idx < 0)
            history_idx = 0;
        entry = get_entry();
        cursor_pos = entry.size();
    }
    else if(events->count(interface_key::CURSOR_DOWN))
    {
        if (history_idx < command_history.size() - 1)
        {
            history_idx++;
            if (history_idx >= command_history.size())
                history_idx = command_history.size() - 1;
            entry = get_entry();
            cursor_pos = entry.size();
        }
    }
    set_entry(entry);
    if (old_pos != cursor_pos)
        frame = 0;

}

command_result show_prompt(color_ostream &out, std::vector <std::string> & parameters)
{
    if (Gui::getCurFocus() == "dfhack/commandprompt")
    {
        Screen::dismiss(Gui::getCurViewscreen(true));
    }
    std::string params;
    for(size_t i=0;i<parameters.size();i++)
        params+=parameters[i]+" ";
    Screen::show(new viewscreen_commandpromptst(params));
    return CR_OK;
}
bool hotkey_allow_all(df::viewscreen *top)
{
    return true;
}
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "command-prompt","Shows a command prompt on window.",show_prompt,hotkey_allow_all,
        "command-prompt [entry] - shows a cmd prompt in df window. Entry is used for default prefix (e.g. ':lua')"
        ));
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange (color_ostream &out, state_change_event e)
{
    if (e == SC_BEGIN_UNLOAD && Gui::getCurFocus() == "dfhack/commandprompt")
        return CR_FAILURE;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
