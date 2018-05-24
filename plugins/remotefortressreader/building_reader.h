#ifndef BUILDING_READER_H
#define BUILDING_READER_H
#include <stdint.h>
#include "RemoteClient.h"
#include "RemoteFortressReader.pb.h"

DFHack::command_result GetBuildingDefList(DFHack::color_ostream &stream, const DFHack::EmptyMessage *in, RemoteFortressReader::BuildingList *out);
void CopyBuilding(int buildingIndex, RemoteFortressReader::BuildingInstance * remote_build);

#endif