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
    struct art_image_chunk;
    struct world;
}

namespace MapExtras
{
    class MapCache;
}

DFHack::command_result GetItemList(DFHack::color_ostream &stream, const DFHack::EmptyMessage *in, RemoteFortressReader::MaterialList *out);
void CopyItem(RemoteFortressReader::Item * NetItem, df::item * DfItem);
void ConvertDFColorDescriptor(int16_t index, RemoteFortressReader::ColorDefinition * out);

typedef df::art_image_chunk * (*GET_ART_IMAGE_CHUNK)(std::vector<df::art_image_chunk* > *, int);

void CopyImage(const df::art_image * image, RemoteFortressReader::ArtImage * netImage);

#endif // !ITEM_READER_H
