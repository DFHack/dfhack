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
#include "ModuleFactory.h"

using namespace DFHack;

Module* DFHack::createItems(DFContextShared * d)
{
    return new Items(d);
}

enum accessor_type {ACCESSOR_CONSTANT, ACCESSOR_INDIRECT, ACCESSOR_DOUBLE_INDIRECT};

/* this is used to store data about the way accessors work */
class DFHACK_EXPORT Accessor
{
private:
    accessor_type type;
    int32_t constant;
    int32_t offset1;
    int32_t offset2;
    Process * p;
    uint32_t dataWidth;
public:
    Accessor(uint32_t function, Process * p);
    Accessor(accessor_type type, int32_t constant, uint32_t offset1, uint32_t offset2, uint32_t dataWidth, Process * p);
    int32_t getValue(uint32_t objectPtr);
    bool isConstant();
};
class DFHACK_EXPORT ItemImprovementDesc
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

class DFHACK_EXPORT ItemDesc
{
private:
    Accessor * AMainType;
    Accessor * ASubType;
    Accessor * ASubIndex;
    Accessor * AIndex;
    Accessor * AQuality;
    Accessor * AWear;
    Process * p;
    bool hasDecoration;
public:
    ItemDesc(uint32_t VTable, Process * p);
    bool getItem(uint32_t itemptr, t_item & item);
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

static bool match_MOV_MEM(uint32_t &ptr, uint64_t v, int in_reg, int &out_reg, int &offset, bool &size16) 
{
    int prefix = 0;
    size16 = false;
    if ((v & 0xFF) == 0x8B) { // MOV
        v >>= 8;
        prefix = 1;
    }
    else if ((v & 0xFFFF) == 0x8B66) { // MOV 16-bit
        v >>= 16;
        prefix = 2;
        size16 = true;
    }
    else if ((v & 0xFFFF) == 0xBF0F) { // MOVSX
        v >>= 16;
        prefix = 2;
        size16 = true;
    }
    else if ((v & 0xFFFF) == 0xB70F) { // MOVZ
        v >>= 16;
        prefix = 2;
        size16 = true;
    }
    else
        return false;
    
    return match_MEM_ACCESS(ptr, v, prefix, in_reg, out_reg, offset);
}

// FIXME: this is crazy
Accessor::Accessor(uint32_t function, Process *p)
{
    this->p = p;
    this->type = ACCESSOR_CONSTANT;
    uint32_t ptr = function;
    uint64_t v = p->readQuad(ptr);
    int data_reg = -1;

    if (do_match(ptr, v, 2, 0xFFFF, 0xC033) ||
        do_match(ptr, v, 2, 0xFFFF, 0xC031)) // XOR EAX, EAX
    {
        data_reg = 0;
        this->constant = 0;
    }
    else if (do_match(ptr, v, 3, 0xFFFFFF, 0xFFC883)) // OR EAX, -1
    {
        data_reg = 0;
        this->constant = -1;
    }
    else if (do_match(ptr, v, 5, 0xFF, 0xB8)) // MOV EAX,imm
    {
        data_reg = 0;
        this->constant = (v>>8) & 0xFFFFFFFF;
    }
    else
    {
        bool size16;
        int ptr_reg = 1, tmp; // ECX

        // MOV REG,[ESP+4]
        if (do_match(ptr, v, 4, 0xFFFFC7FFU, 0x0424448B))
        {
            ptr_reg = (v>>11)&7;
            v = p->readQuad(ptr);
        }

        this->dataWidth = 4;

        if (match_MOV_MEM(ptr, v, ptr_reg, tmp, this->offset1, size16)) {
            data_reg = tmp;
            this->type = ACCESSOR_INDIRECT;

            if (size16)
                this->dataWidth = 2;
            else
            {
                v = p->readQuad(ptr);
            
                if (match_MOV_MEM(ptr, v, data_reg, tmp, this->offset2, size16)) {
                    data_reg = tmp;
                    this->type = ACCESSOR_DOUBLE_INDIRECT;
                    
                    if (size16)
                        this->dataWidth = 2;
                }
            }
        }
    }
    
    v = p->readQuad(ptr);
    
    if (data_reg == 0 && do_match(ptr, v, 1, 0xFF, 0xC3)) // RET
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
            return (int16_t) p->readWord(objectPtr + this->offset1);
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
            return (int16_t) p->readWord(p->readDWord(objectPtr + this->offset1) + this->offset2);
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
    OffsetGroup * Items = p->getDescriptor()->getGroup("Items");
    uint32_t funcOffsetA = Items->getOffset("item_type_accessor");
    uint32_t funcOffsetB = Items->getOffset("item_subtype_accessor");
    uint32_t funcOffsetC = Items->getOffset("item_subindex_accessor");
    uint32_t funcOffsetD = Items->getOffset("item_index_accessor");
    uint32_t funcOffsetQuality = Items->getOffset("item_quality_accessor");
    uint32_t funcOffsetWear = Items->getOffset("item_wear_accessor");
    this->vtable = VTable;
    this->p = p;
    this->className = p->readClassName(VTable).substr(5);
    this->className.resize(this->className.size()-2);
    this->AMainType = new Accessor( p->readDWord( VTable + funcOffsetA ), p);
    this->ASubType = new Accessor( p->readDWord( VTable + funcOffsetB ), p);
    this->ASubIndex = new Accessor( p->readDWord( VTable + funcOffsetC ), p);
    this->AIndex = new Accessor( p->readDWord( VTable + funcOffsetD ), p);
    this->AQuality = new Accessor( p->readDWord( VTable + funcOffsetQuality ), p);
    this->AWear = new Accessor( p->readDWord( VTable + funcOffsetWear ), p);
    this->hasDecoration = false;
    if(this->AMainType->isConstant())
        this->mainType = this->AMainType->getValue(0);
    else
    {
        fprintf(stderr, "Bad item main type at function %p\n", (void*) p->readDWord( VTable + funcOffsetA ));
        this->mainType = 0;
    }
}

bool ItemDesc::getItem(uint32_t itemptr, DFHack::t_item &item)
{
    this->p->read(itemptr+4, sizeof(t_item_header), (uint8_t*)&item.header);
    item.matdesc.itemType = this->AMainType->getValue(itemptr);
    item.matdesc.subType = this->ASubType->getValue(itemptr);
    item.matdesc.subIndex = this->ASubIndex->getValue(itemptr);
    item.matdesc.index = this->AIndex->getValue(itemptr);
    item.quality = this->AQuality->getValue(itemptr);
    item.quantity = 1; /* TODO */
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
};

Items::Items(DFContextShared * d_)
{
    d = new Private;
    d->d = d_;
    d->owner = d_->p;
}

bool Items::Start()
{
    return true;
}

bool Items::Finish()
{
    return true;
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

bool Items::getItemData(uint32_t itemptr, DFHack::t_item &item)
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

    return desc->getItem(itemptr, item);
}

void Items::setItemFlags(uint32_t itemptr, t_itemflags new_flags)
{
    d->owner->writeDWord(itemptr + 0x0C, new_flags.whole);
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

std::string Items::getItemDescription(uint32_t itemptr, Materials * Materials)
{
    DFHack::t_item item;
    std::string out;

    if(!this->getItemData(itemptr, item))
        return "??";
    switch(item.quality)
    {
        case 0: break;
        case 1: out.append("Well crafted "); break;
        case 2: out.append("Finely crafted "); break;
        case 3: out.append("Superior quality "); break;
        case 4: out.append("Exceptionnal "); break;
        case 5: out.append("Masterful "); break;
        default: out.append("Crazy quality "); break;
    }
    out.append(Materials->getDescription(item.matdesc));
    out.append(" ");
    out.append(this->getItemClass(item.matdesc.itemType));
    return out;
}

// The OLD items code follows (40d era)
// TODO: merge with the current Items module
/*
bool API::InitReadItems(uint32_t & numitems)
{
    try
    {
        int items = d->offset_descriptor->getAddress ("items");
        d->item_material_offset = d->offset_descriptor->getOffset ("item_materials");

        d->p_itm = new DfVector (d->p, items);
        d->itemsInited = true;
        numitems = d->p_itm->getSize();
        return true;
    }
    catch (Error::AllMemdef&)
    {
        d->itemsInited = false;
        numitems = 0;
        throw;
    }
}

bool API::getItemIndexesInBox(vector<uint32_t> &indexes,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2)
{
    if(!d->itemsInited) return false;
    indexes.clear();
    uint32_t size = d->p_itm->getSize();
    struct temp2{
        uint16_t coords[3];
        uint32_t flags;
    };
    temp2 temp2;
    for(uint32_t i =0;i<size;i++){
        uint32_t temp = d->p_itm->at(i);
        d->p->read(temp+sizeof(uint32_t),5 * sizeof(uint16_t), (uint8_t *) &temp2);
        if(temp2.flags & (1 << 0)){
            if (temp2.coords[0] >= x1 && temp2.coords[0] < x2)
            {
                if (temp2.coords[1] >= y1 && temp2.coords[1] < y2)
                {
                    if (temp2.coords[2] >= z1 && temp2.coords[2] < z2)
                    {
                        indexes.push_back(i);
                    }
                }
            }
        }
    }
    return true;
}

bool API::ReadItem (const uint32_t index, t_item & item)
{
    if (!d->itemsInited) return false;
    
    t_item_df40d item_40d;

    // read pointer from vector at position
    uint32_t temp = d->p_itm->at (index);

    //read building from memory
    d->p->read (temp, sizeof (t_item_df40d), (uint8_t *) &item_40d);

    // transform
    int32_t type = -1;
    d->offset_descriptor->resolveObjectToClassID (temp, type);
    item.origin = temp;
    item.vtable = item_40d.vtable;
    item.x = item_40d.x;
    item.y = item_40d.y;
    item.z = item_40d.z;
    item.type = type;
    item.ID = item_40d.ID;
    item.flags.whole = item_40d.flags;

    //TODO  certain item types (creature based, threads, seeds, bags do not have the first matType byte, instead they have the material index only located at 0x68
    d->p->read (temp + d->item_material_offset, sizeof (t_matglossPair), (uint8_t *) &item.material);
    //for(int i = 0; i < 0xCC; i++){  // used for item research
    //    uint8_t byte = MreadByte(temp+i);
    //    item.bytes.push_back(byte);
    //}
    return true;
}
void API::FinishReadItems()
{
    if(d->p_itm)
    {
        delete d->p_itm;
        d->p_itm = NULL;
    }
    d->itemsInited = false;
}
*/
/*
bool API::ReadItemTypes(vector< vector< t_itemType > > & itemTypes)
{
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress("matgloss");
    int matgloss_skip = minfo->getHexValue("matgloss_skip");
    int item_type_name_offset = minfo->getOffset("item_type_name");
    for(int i = 8;i<20;i++)
    {
        DfVector p_temp (d->p, matgloss_address + i*matgloss_skip);
        vector< t_itemType > typesForVec;
        for(uint32_t j =0; j<p_temp.getSize();j++)
        {
            t_itemType currType;
            uint32_t temp = *(uint32_t *) p_temp[j];
           // Mread(temp+40,sizeof(name),(uint8_t *) name);
            d->p->readSTLString(temp+4,currType.id,128);
            d->p->readSTLString(temp+item_type_name_offset,currType.name,128);
            //stringsForVec.push_back(string(name));
            typesForVec.push_back(currType);
        }
        itemTypes.push_back(typesForVec);
    }
    return true;
}
*/