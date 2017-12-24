#ifndef ADVENTURE_CONTROL_H
#define ADVENTURE_CONTROL_H

#include "RemoteClient.h"
#include "AdventureControl.pb.h"

DFHack::command_result MoveCommand(DFHack::color_ostream &stream, const AdventureControl::MoveCommandParams *in);

#endif // !ADVENTURE_CONTROL_H
