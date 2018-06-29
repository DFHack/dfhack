#ifndef DWARF_CONTROL_H
#define DWARF_CONTROL_H

#include "RemoteClient.h"
#include "RemoteFortressReader.pb.h"
#include "DwarfControl.pb.h"

DFHack::command_result SendDigCommand(DFHack::color_ostream &stream, const RemoteFortressReader::DigCommand *in);
DFHack::command_result SetPauseState(DFHack::color_ostream &stream, const RemoteFortressReader::SingleBool *in);
DFHack::command_result GetSideMenu(DFHack::color_ostream &stream, const dfproto::EmptyMessage *in, DwarfControl::SidebarState *out);
DFHack::command_result SetSideMenu(DFHack::color_ostream &stream, const DwarfControl::SidebarCommand *in);

#endif // !DWARF_CONTROL_H


