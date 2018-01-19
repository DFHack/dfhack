#ifndef ITEM_READER_H
#define ITEM_READER_H

#include <stdint.h>
#include "RemoteClient.h"
#include "RemoteFortressReader.pb.h"

#include "DataDefs.h"

namespace df
{
    struct item;
    struct map_block;
}

namespace MapExtras
{
    class MapCache;
}

DFHack::command_result GetItemList(DFHack::color_ostream &stream, const DFHack::EmptyMessage *in, RemoteFortressReader::MaterialList *out);
void CopyItem(RemoteFortressReader::Item * NetItem, df::item * DfItem);
void ConvertDFColorDescriptor(int16_t index, RemoteFortressReader::ColorDefinition * out);

#endif // !ITEM_READER_H
