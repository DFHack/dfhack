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

#include "Internal.h"

#include <string>
#include <sstream>
#include <vector>
#include <cstdio>
#include <map>
using namespace std;

#include "ContextShared.h"
#include "dfhack/DFTypes.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFVector.h"
#include "dfhack/modules/Materials.h"
#include "dfhack/modules/Items.h"
#include "dfhack/modules/Creatures.h"
#include "ModuleFactory.h"

using namespace DFHack;

Module* DFHack::createItems(DFContextShared * d)
{
    return new Items(d);
}

enum accessor_type {ACCESSOR_CONSTANT, ACCESSOR_INDIRECT, ACCESSOR_DOUBLE_INDIRECT};

/* this is used to store data about the way accessors work */
class Accessor
{
public:
    enum DataWidth {
        Data32 = 0,
        DataSigned16,
        DataUnsigned16
    };
private:
    accessor_type type;
    int32_t constant;
    int32_t offset1;
    int32_t offset2;
    Process * p;
    DataWidth dataWidth;
    uint32_t method;
public:
    Accessor(uint32_t function, Process * p);
    Accessor(accessor_type type, int32_t constant, uint32_t offset1, uint32_t offset2, uint32_t dataWidth, Process * p);
    std::string dump();
    int32_t getValue(uint32_t objectPtr);
    bool isConstant();
};
class ItemImprovementDesc
{
private:
    Accessor * AType;
    Process * p;
public:
    ItemImprovementDesc(uint32_t VTable, Process * p);
    bool getImprovement(uint32_t descptr, t_improvement & imp);
    uint32_t vtable;
    uint32_t maintype;
};

class ItemDesc
{
private:
    Accessor * AMainType;
    Accessor * ASubType;
    Accessor * ASubIndex;
    Accessor * AIndex;
    Accessor * AQuality;
    Accessor * AQuantity;
    Accessor * AWear;
    Process * p;
    bool hasDecoration;
    int idFieldOffset;
public:
    ItemDesc(uint32_t VTable, Process * p);
    bool readItem(uint32_t itemptr, dfh_item & item);
    std::string dumpAccessors();
    std::string className;
    uint32_t vtable;
    uint32_t mainType;
    std::vector<ItemImprovementDesc> improvement;
};

inline bool do_match(uint32_t &ptr, uint64_t val, int size, uint64_t mask, uint64_t check)
{
    if ((val & mask) == check) {
        ptr += size;
        return true;
    }
    return false;
}

static bool match_MEM_ACCESS(uint32_t &ptr, uint64_t v, int isize, int in_reg, int &out_reg, int &offset)
{
    // ESP & EBP are hairy
    if (in_reg == 4 || in_reg == 5)
        return false;

    if ((v & 7) != in_reg)
        return false;

    out_reg = (v>>3) & 7;

    switch ((v>>6)&3) {
        case 0: // MOV REG2, [REG]
            offset = 0;
            ptr += isize+1;
            return true;
        case 1: // MOV REG2, [REG+offset8]
            offset = (signed char)(v >> 8);
            ptr += isize+2;
            return true;
        case 2: // MOV REG2, [REG+offset32]
            offset = (signed int)(v >> 8);
            ptr += isize+5;
            return true;
        default:
            return false;
    }
}

static bool match_MOV_MEM(uint32_t &ptr, uint64_t v, int in_reg, int &out_reg, int &offset, Accessor::DataWidth &size) 
{
    int prefix = 0;
    size = Accessor::Data32;
    if ((v & 0xFF) == 0x8B) { // MOV
        v >>= 8;
        prefix = 1;
    }
    else if ((v & 0xFFFF) == 0x8B66) { // MOV 16-bit
        v >>= 16;
        prefix = 2;
        size = Accessor::DataUnsigned16;
    }
    else if ((v & 0xFFFF) == 0xBF0F) { // MOVSX
        v >>= 16;
        prefix = 2;
        size = Accessor::DataSigned16;
    }
    else if ((v & 0xFFFF) == 0xB70F) { // MOVZ
        v >>= 16;
        prefix = 2;
        size = Accessor::DataUnsigned16;
    }
    else
        return false;

    return match_MEM_ACCESS(ptr, v, prefix, in_reg, out_reg, offset);
}

Accessor::Accessor(uint32_t function, Process *p)
{
    this->p = p;
    this->type = ACCESSOR_CONSTANT;
    if(!p)
    {
        this->constant = 0;
        return;
    }
    method = function;
    uint32_t temp = function;
    int data_reg = -1;
    uint64_t v = p->readQuad(temp);

    if (do_match(temp, v, 2, 0xFFFF, 0xC033) ||
        do_match(temp, v, 2, 0xFFFF, 0xC031)) // XOR EAX, EAX
    {
        data_reg = 0;
        this->constant = 0;
    }
    else if (do_match(temp, v, 3, 0xFFFFFF, 0xFFC883)) // OR EAX, -1
    {
        data_reg = 0;
        this->constant = -1;
    }
    else if (do_match(temp, v, 5, 0xFF, 0xB8)) // MOV EAX,imm
    {
        data_reg = 0;
        this->constant = (v>>8) & 0xFFFFFFFF;
    }
    else
    {
        DataWidth xsize;
        int ptr_reg = 1, tmp; // ECX

        // MOV REG,[ESP+4]
        if (do_match(temp, v, 4, 0xFFFFC7FFU, 0x0424448B))
        {
            ptr_reg = (v>>11)&7;
            v = p->readQuad(temp);
        }

        if (match_MOV_MEM(temp, v, ptr_reg, tmp, this->offset1, xsize)) {
            data_reg = tmp;
            this->type = ACCESSOR_INDIRECT;
            this->dataWidth = xsize;

            if (xsize == Data32)
            {
                v = p->readQuad(temp);

                if (match_MOV_MEM(temp, v, data_reg, tmp, this->offset2, xsize)) {
                    data_reg = tmp;
                    this->type = ACCESSOR_DOUBLE_INDIRECT;
                    this->dataWidth = xsize;
                }
            }
        }
    }

    v = p->readQuad(temp);

    if (data_reg == 0 && do_match(temp, v, 1, 0xFF, 0xC3)) // RET
        return;
    else
    {
        this->type = ACCESSOR_CONSTANT;
        this->constant = 0;
        printf("bad accessor @0x%x\n", function);
    }
}

bool Accessor::isConstant()
{
    if(this->type == ACCESSOR_CONSTANT)
        return true;
    else
        return false;
}

string Accessor::dump()
{
    stringstream sstr;
    sstr << hex << "method @0x" << method << dec << " ";
    switch(type)
    {
        case ACCESSOR_CONSTANT:
            sstr << "Constant: " << dec << constant;
            break;
        case ACCESSOR_INDIRECT:
            switch(dataWidth)
            {
                case Data32:
                    sstr << "int32_t ";
                    break;
                case DataSigned16:
                    sstr << "int16_t ";
                    break;
                case DataUnsigned16:
                    sstr << "uint16_t ";
                    break;
                default:
                    sstr << "unknown ";
                    break;
            }
            sstr << hex << "[obj + 0x" << offset1 << " ]";
            break;
        case ACCESSOR_DOUBLE_INDIRECT:
            switch(dataWidth)
            {
                case Data32:
                    sstr << "int32_t ";
                    break;
                case DataSigned16:
                    sstr << "int16_t ";
                    break;
                case DataUnsigned16:
                    sstr << "uint16_t ";
                    break;
                default:
                    sstr << "unknown ";
                    break;
            }
            sstr << hex << "[ [obj + 0x" << offset1 << " ] + 0x" << offset2 << " ]";
            break;
    }
    return sstr.str();
}

int32_t Accessor::getValue(uint32_t objectPtr)
{
    int32_t offset = this->offset1;

    switch(this->type)
    {
    case ACCESSOR_CONSTANT:
        return this->constant;
        break;
    case ACCESSOR_DOUBLE_INDIRECT:
        objectPtr = p->readDWord(objectPtr + this->offset1);
        offset = this->offset2;
        // fallthrough
    case ACCESSOR_INDIRECT:
        switch(this->dataWidth)
        {
        case Data32:
            return p->readDWord(objectPtr + offset);
        case DataSigned16:
            return (int16_t) p->readWord(objectPtr + offset);
        case DataUnsigned16:
            return (uint16_t) p->readWord(objectPtr + offset);
        default:
            return -1;
        }
        break;
    default:
        return -1;
    }
}

// FIXME: turn into a proper factory with caching
Accessor * buildAccessor (OffsetGroup * I, Process * p, const char * name, uint32_t vtable)
{
    int32_t offset;
    if(I->getSafeOffset(name,offset))
    {
        return new Accessor( p->readDWord( vtable + offset ), p);
    }
    else
    {
        fprintf(stderr,"Missing offset for item accessor \"%s\"\n", name);
        return new Accessor(-1,0); // dummy accessor. always returns -1
    }
}

ItemDesc::ItemDesc(uint32_t VTable, Process *p)
{
    int32_t funcOffsetA, funcOffsetB, funcOffsetC, funcOffsetD, funcOffsetQuality, funcOffsetWear;
    OffsetGroup * Items = p->getDescriptor()->getGroup("Items");

    /* 
     * FIXME: and what about types, different sets of methods depending on class?
     * what about more complex things than constants and integers?
     * If this is to be generally useful, it needs much more power.
     */ 
    AMainType = buildAccessor(Items, p, "item_type_accessor", VTable);
    ASubType = buildAccessor(Items, p, "item_subtype_accessor", VTable);
    ASubIndex = buildAccessor(Items, p, "item_subindex_accessor", VTable);
    AIndex = buildAccessor(Items, p, "item_index_accessor", VTable);
    AQuality = buildAccessor(Items, p, "item_quality_accessor", VTable);
    AWear = buildAccessor(Items, p, "item_wear_accessor", VTable);
    AQuantity = buildAccessor(Items, p, "item_quantity_accessor", VTable);

    idFieldOffset = Items->getOffset("id");

    this->vtable = VTable;
    this->p = p;
    this->className = p->readClassName(VTable).substr(5);
    this->className.resize(this->className.size()-2);

    this->hasDecoration = false;
    if(AMainType->isConstant())
        mainType = this->AMainType->getValue(0);
    else
    {
        fprintf(stderr, "Bad item main type at function %p\n", (void*) p->readDWord( VTable + funcOffsetA ));
        mainType = 0;
    }
}

string ItemDesc::dumpAccessors()
{
    std::stringstream outss;
    outss << "MainType  :" << AMainType->dump() << endl;
    outss << "ASubType  :" << ASubType->dump() << endl;
    outss << "ASubIndex :" << ASubIndex->dump() << endl;
    outss << "AIndex    :" << AIndex->dump() << endl;
    outss << "AQuality  :" << AQuality->dump() << endl;
    outss << "AQuantity :" << AQuantity->dump() << endl;
    outss << "AWear     :" << AWear->dump() << endl;
    return outss.str();
}


bool ItemDesc::readItem(uint32_t itemptr, DFHack::dfh_item &item)
{
    item.id = p->readDWord(itemptr+idFieldOffset);
    p->read(itemptr, sizeof(t_item), (uint8_t*)&item.base);
    item.matdesc.itemType = AMainType->getValue(itemptr);
    item.matdesc.subType = ASubType->getValue(itemptr);
    item.matdesc.subIndex = ASubIndex->getValue(itemptr);
    item.matdesc.index = AIndex->getValue(itemptr);
    item.quality = AQuality->getValue(itemptr);
    item.quantity = AQuantity->getValue(itemptr);
    item.origin = itemptr;
    // FIXME: use templates. seriously.
    // Note: this accessor returns a 32-bit value with the higher
    // half sometimes containing garbage, so the cast is essential:
    item.wear_level = (int16_t)this->AWear->getValue(itemptr);
    return true;
}

class Items::Private
{
    public:
        DFContextShared *d;
        Process * owner;
        std::map<int32_t, ItemDesc *> descType;
        std::map<uint32_t, ItemDesc *> descVTable;
        std::map<int32_t, uint32_t> idLookupTable;
        uint32_t refVectorOffset;
        uint32_t idFieldOffset;
        uint32_t itemVectorAddress;
        ClassNameCheck isOwnerRefClass;
        ClassNameCheck isContainerRefClass;
        ClassNameCheck isContainsRefClass;
};

Items::Items(DFContextShared * d_)
{
    d = new Private;
    d->d = d_;
    d->owner = d_->p;

    d->isOwnerRefClass = ClassNameCheck("general_ref_unit_itemownerst");
    d->isContainerRefClass = ClassNameCheck("general_ref_contained_in_itemst");
    d->isContainsRefClass = ClassNameCheck("general_ref_contains_itemst");

    DFHack::OffsetGroup* itemGroup = d_->offset_descriptor->getGroup("Items");
    d->itemVectorAddress = itemGroup->getAddress("items_vector");
    d->idFieldOffset = itemGroup->getOffset("id");
    d->refVectorOffset = itemGroup->getOffset("item_ref_vector");
}

bool Items::Start()
{
    d->idLookupTable.clear();
    return true;
}

bool Items::Finish()
{
    return true;
}

bool Items::readItemVector(std::vector<uint32_t> &items)
{
    DFHack::DfVector <uint32_t> p_items(d->owner, d->itemVectorAddress);

    d->idLookupTable.clear();
    items.resize(p_items.size());

    for (unsigned i = 0; i < p_items.size(); i++) {
        uint32_t ptr = p_items[i];
        items[i] = ptr;
        d->idLookupTable[d->owner->readDWord(ptr + d->idFieldOffset)] = ptr;
    }

    return true;
}

uint32_t Items::findItemByID(int32_t id)
{
    if (id < 0)
        return 0;

    if (d->idLookupTable.empty()) {
        std::vector<uint32_t> tmp;
        readItemVector(tmp);
    }

    return d->idLookupTable[id];
}

Items::~Items()
{
    Finish();
    std::map<uint32_t, ItemDesc *>::iterator it;
    it = d->descVTable.begin();
    while (it != d->descVTable.end())
    {
        delete (*it).second;
        ++it;
    }
    d->descType.clear();
    d->descVTable.clear();
    delete d;
}

bool Items::readItem(uint32_t itemptr, DFHack::dfh_item &item)
{
    std::map<uint32_t, ItemDesc *>::iterator it;
    Process * p = d->owner;
    ItemDesc * desc;

    uint32_t vtable = p->readDWord(itemptr);
    it = d->descVTable.find(vtable);
    if(it == d->descVTable.end())
    {
        desc = new ItemDesc(vtable, p);
        d->descVTable[vtable] = desc;
        d->descType[desc->mainType] = desc;
    }
    else
        desc = it->second;

    return desc->readItem(itemptr, item);
}

bool Items::writeItem(const DFHack::dfh_item &item)
{
    if(item.origin)
    {
        d->owner->write(item.origin, sizeof(t_item),(uint8_t *)&(item.base));
        return true;
    }
    return false;
}

/*
void Items::setItemFlags(uint32_t itemptr, t_itemflags new_flags)
{
    d->owner->writeDWord(itemptr + 0x0C, new_flags.whole);
}
*/
int32_t Items::getItemOwnerID(const DFHack::dfh_item &item)
{
    std::vector<int32_t> vals;
    if (readItemRefs(item, d->isOwnerRefClass, vals))
        return vals[0];
    else
        return -1;
}

int32_t Items::getItemContainerID(const DFHack::dfh_item &item)
{
    std::vector<int32_t> vals;
    if (readItemRefs(item, d->isContainerRefClass, vals))
        return vals[0];
    else
        return -1;
}

bool Items::getContainedItems(const DFHack::dfh_item &item, std::vector<int32_t> &items)
{
    return readItemRefs(item, d->isContainsRefClass, items);
}

bool Items::readItemRefs(const dfh_item &item, const ClassNameCheck &classname, std::vector<int32_t> &values)
{
    DFHack::DfVector<uint32_t> p_refs(d->owner, item.origin + d->refVectorOffset);

    values.clear();

    for (uint32_t i=0; i<p_refs.size(); i++)
    {
        uint32_t vtbl = d->owner->readDWord(p_refs[i]);
        if (classname(d->owner, vtbl))
            values.push_back(int32_t(d->owner->readDWord(p_refs[i] + 4)));
    }

    return !values.empty();
}

bool Items::removeItemOwner(dfh_item &item, Creatures *creatures)
{
    DFHack::DfVector<uint32_t> p_refs(d->owner, item.origin + d->refVectorOffset);

    for (uint32_t i=0; i<p_refs.size(); i++)
    {
        uint32_t vtbl = d->owner->readDWord(p_refs[i]);
        if (!d->isOwnerRefClass(d->owner, vtbl)) continue;

        int32_t oid = d->owner->readDWord(p_refs[i]+4);
        int32_t ix = creatures->FindIndexById(oid);

        if (ix < 0 || !creatures->RemoveOwnedItemIdx(ix, item.id))
            return false;

        if (!p_refs.remove(i--))
            return false;
    }

    item.base.flags.owned = 0;

    return true;
}

std::string Items::getItemClass(const dfh_item & item)
{
    return getItemClass(item.matdesc.itemType);
}

std::string Items::getItemClass(int32_t index)
{
    std::map<int32_t, ItemDesc *>::iterator it;
    std::string out;

    it = d->descType.find(index);
    if(it == d->descType.end())
    {
        /* these are dummy values for mood decoding */
        switch(index)
        {
            case 0: return "bar";
            case 1: return "cut gem";
            case 2: return "block";
            case 3: return "raw gem";
            case 4: return "raw stone";
            case 5: return "log";
            case 54: return "leather";
            case 57: return "cloth";
            case -1: return "probably bone or shell, but I really don't know";
            default: return "unknown";
        }
    }
    out = it->second->className;
    return out;
}

std::string Items::getItemDescription(const dfh_item & item, Materials * Materials)
{
    std::stringstream outss;
    switch(item.quality)
    {
        case 0:
            outss << "Ordinary ";
            break;
        case 1:
            outss << "Well crafted ";
            break;
        case 2:
            outss << "Finely crafted ";
            break;
        case 3:
            outss << "Superior quality ";
            break;
        case 4:
            outss << "Exceptionnal ";
            break;
        case 5:
            outss << "Masterful ";
            break;
        default: outss << "Crazy quality " << item.quality << " "; break;
    }
    outss << Materials->getDescription(item.matdesc) << " " << getItemClass(item.matdesc.itemType);
    return outss.str();
}

/// dump offsets used by accessors of a valid item to a string
std::string Items::dumpAccessors(const dfh_item & item)
{
    std::map< uint32_t, ItemDesc* >::const_iterator it = d->descVTable.find(item.base.vtable);
    if(it != d->descVTable.end())
        return it->second->dumpAccessors();
    return "crud";
}