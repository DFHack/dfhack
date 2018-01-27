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
    struct art_image;
    struct art_image_ref;
    struct world;
}

namespace MapExtras
{
    class MapCache;
}

DFHack::command_result GetItemList(DFHack::color_ostream &stream, const DFHack::EmptyMessage *in, RemoteFortressReader::MaterialList *out);
void CopyItem(RemoteFortressReader::Item * NetItem, df::item * DfItem);
void ConvertDFColorDescriptor(int16_t index, RemoteFortressReader::ColorDefinition * out);

#if(defined(WIN32) && !defined(_WIN64))
typedef df::art_image * (__thiscall *GET_IMAGE)(df::world*, df::art_image_ref *, int16_t *);
#else
typedef df::art_image *(__fastcall *GET_IMAGE)(df::world *, df::art_image_ref *, int16_t *);
#endif


#endif // !ITEM_READER_H
