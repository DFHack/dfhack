/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "DFCommonInternal.h"
#include "../private/APIPrivate.h"
#include "modules/Materials.h"
#include "modules/Items.h"
#include "DFMemInfo.h"
#include "DFProcess.h"
#include "DFVector.h"

using namespace DFHack;

class Items::Private
{
    public:
    APIPrivate *d;
    Process * owner;
    /*
    bool Inited;
    bool Started;
    */
};

Items::Items(APIPrivate * d_)
{
    d = new Private;
    d->d = d_;
    d->owner = d_->p;
}
Items::~Items()
{
    delete d;
	/* TODO : delete all item descs */
}

Accessor::Accessor(uint32_t function, Process *p)
{
	this->p = p;
	this->constant = 0;
	this->offset1 = 0;
	this->offset2 = 0;
	this->type = ACCESSOR_CONSTANT;
	this->dataWidth = 2;
	uint64_t funcText = p->readQuad(function);
	if( funcText == 0xCCCCCCCCCCC3C033LL )
	{
		return;
	}
	if( funcText == 0xCCCCCCCCC3FFC883LL )
	{
		/* or eax,-1; ret; */
		this->constant = -1;
		return;
	}
	if( (funcText&0xFFFFFFFFFF0000FFLL) == 0xCCCCC300000000B8LL )
	{
		/* mov eax, xx; ret; */
		this->constant = (funcText>>8) & 0xffff;
		return;
	}
	if( (funcText&0xFFFFFF0000FFFFFFLL) == 0xC300000000818B66LL )
	{
		/* mov ax, [ecx+xx]; ret; */
		this->type = ACCESSOR_INDIRECT;
		this->offset1 = (funcText>>24) & 0xffff;
		return;
	}
	if( (funcText&0xFFFFFFFF0000FFFFLL) == 0x8B6600000000818BLL )
	{
		uint64_t funcText2 = p->readQuad(function+8);
		if( (funcText2&0xFFFFFFFFFFFF00FFLL) == 0xCCCCCCCCCCC30040LL )
        {
			this->type = ACCESSOR_DOUBLE_INDIRECT;
			this->offset1 = (funcText>>16) & 0xffff;
			this->offset2 = (funcText2>>8) & 0xff;
			return;
		}
	}
	if( (funcText&0xFFFFFFFF0000FFFFLL) == 0xCCC300000000818BLL )
	{
		/* mov eax, [ecx+xx]; ret; */
		this->type = ACCESSOR_INDIRECT;
		this->offset1 = (funcText>>16) & 0xffff;
		this->dataWidth = 4;
		return;
	}
	printf("bad accessor @0x%x\n", function);
}

int32_t Accessor::getValue(uint32_t objectPtr)
{
	switch(this->type)
	{
	case ACCESSOR_CONSTANT:
		return this->constant;
		break;
	case ACCESSOR_INDIRECT:
		switch(this->dataWidth)
		{
		case 2:
			return p->readWord(objectPtr + this->offset1);
		case 4:
			return p->readDWord(objectPtr + this->offset1);
		default:
			return -1;
		}
		break;
	case ACCESSOR_DOUBLE_INDIRECT:
		switch(this->dataWidth)
		{
		case 2:
			return p->readWord(p->readDWord(objectPtr + this->offset1) + this->offset2);
		case 4:
			return p->readDWord(p->readDWord(objectPtr + this->offset1) + this->offset2);
		default:
			return -1;
		}
		break;
	default:
		return -1;
	}
}

ItemDesc::ItemDesc(uint32_t VTable, Process *p)
{
	uint32_t funcOffsetA = p->getDescriptor()->getOffset("item_type_accessor");
	uint32_t funcOffsetB = p->getDescriptor()->getOffset("item_subtype_accessor");
	uint32_t funcOffsetC = p->getDescriptor()->getOffset("item_subindex_accessor");
	uint32_t funcOffsetD = p->getDescriptor()->getOffset("item_index_accessor");
	uint32_t funcOffsetQuality = p->getDescriptor()->getOffset("item_quality_accessor");
	this->vtable = VTable;
	this->p = p;
	this->className = p->readClassName(VTable);
	this->AMainType = new Accessor( p->readDWord( VTable + funcOffsetA ), p);
	this->ASubType = new Accessor( p->readDWord( VTable + funcOffsetB ), p);
	this->ASubIndex = new Accessor( p->readDWord( VTable + funcOffsetC ), p);
	this->AIndex = new Accessor( p->readDWord( VTable + funcOffsetD ), p);
	this->AQuality = new Accessor( p->readDWord( VTable + funcOffsetQuality ), p);
	this->hasDecoration = false;
}

bool ItemDesc::getItem(uint32_t itemptr, DFHack::t_item &item)
{
	item.matdesc.itemType = this->AMainType->getValue(itemptr);
	item.matdesc.subType = this->ASubType->getValue(itemptr);
	item.matdesc.subIndex = this->ASubIndex->getValue(itemptr);
	item.matdesc.index = this->AIndex->getValue(itemptr);
	item.quality = this->AQuality->getValue(itemptr);
	item.quantity = 1; /* TODO */
	return true;
}

bool Items::getItemData(uint32_t itemptr, DFHack::t_item &item)
{
	std::map<uint32_t, ItemDesc *>::iterator it;
	Process * p = d->owner;
	ItemDesc * desc;

	it = this->descVTable.find(itemptr);
	if(it==descVTable.end())
	{
		uint32_t vtable = p->readDWord(itemptr);
		desc = new ItemDesc(vtable, p);
		this->descVTable[vtable] = desc;
	}
	else
		desc = it->second;

	return desc->getItem(itemptr, item);
}

std::string Items::getItemDescription(uint32_t itemptr)
{
	DFHack::t_item item;
	std::string out;

	if(!this->getItemData(itemptr, item))
		return "??";
	return "!!";
}