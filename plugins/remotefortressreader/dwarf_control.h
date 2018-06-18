#ifndef DWARF_CONTROL_H
#define DWARF_CONTROL_H

#include "RemoteClient.h"
#include "RemoteFortressReader.pb.h"

DFHack::command_result SendDigCommand(DFHack::color_ostream &stream, const RemoteFortressReader::DigCommand *in);
DFHack::command_result SetPauseState(DFHack::color_ostream &stream, const RemoteFortressReader::SingleBool *in);

#endif // !DWARF_CONTROL_H


