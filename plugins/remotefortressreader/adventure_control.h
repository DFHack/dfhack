#ifndef ADVENTURE_CONTROL_H
#define ADVENTURE_CONTROL_H

#include "RemoteClient.h"
#include "AdventureControl.pb.h"

DFHack::command_result MoveCommand(DFHack::color_ostream &stream, const AdventureControl::MoveCommandParams *in);
DFHack::command_result JumpCommand(DFHack::color_ostream &stream, const AdventureControl::MoveCommandParams *in);
DFHack::command_result MenuQuery(DFHack::color_ostream &stream, const dfproto::EmptyMessage *in, AdventureControl::MenuContents *out);
void KeyUpdate();


#endif // !ADVENTURE_CONTROL_H
