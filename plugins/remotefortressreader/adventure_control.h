#ifndef ADVENTURE_CONTROL_H
#define ADVENTURE_CONTROL_H

#include "RemoteClient.h"
#include "AdventureControl.pb.h"

DFHack::command_result MoveCommand(DFHack::color_ostream &stream, const AdventureControl::MoveCommandParams *in);
DFHack::command_result JumpCommand(DFHack::color_ostream &stream, const AdventureControl::MoveCommandParams *in);
DFHack::command_result MenuQuery(DFHack::color_ostream &stream, const dfproto::EmptyMessage *in, AdventureControl::MenuContents *out);
DFHack::command_result MovementSelectCommand(DFHack::color_ostream &stream, const dfproto::IntMessage *in);
DFHack::command_result MiscMoveCommand(DFHack::color_ostream &stream, const AdventureControl::MiscMoveParams *in);

void KeyUpdate();


#endif // !ADVENTURE_CONTROL_H
