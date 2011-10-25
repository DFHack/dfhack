/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

#pragma once
#ifndef CL_MOD_ITEMS
#define CL_MOD_ITEMS

/*
* Items!
*/
#include "dfhack/Export.h"
#include "dfhack/Module.h"
#include "dfhack/Types.h"
#include "dfhack/Virtual.h"
#include "dfhack/modules/Materials.h"
#include "dfhack/Process.h"
/**
 * \defgroup grp_items Items module and its types
 * @ingroup grp_modules
 */

namespace DFHack
{

class Context;
class DFContextShared;
class Creatures;

/**
 * Item flags. A bit fuzzy.
 * Mostly from http://dwarffortresswiki.net/index.php/User:Rick/Memory_research
 * \ingroup grp_items
 */
union t_itemflags
{
    uint32_t whole; ///< the whole struct. all 32 bits of it, as an integer
    struct
    {
        unsigned int on_ground : 1;      ///< 0000 0001 Item on ground
        unsigned int in_job : 1;         ///< 0000 0002 Item currently being used in a job
        unsigned int hostile : 1;        ///< 0000 0004 Item owned by hostile
        unsigned int in_inventory : 1;   ///< 0000 0008 Item in a creature or workshop inventory

        unsigned int unk1 : 1;           ///< 0000 0010 unknown, lost (artifact)?, unusable, unseen
        unsigned int in_building : 1;    ///< 0000 0020 Part of a building (including mechanisms, bodies in coffins)
        unsigned int unk2 : 1;           ///< 0000 0040 unknown, unseen
        unsigned int dead_dwarf : 1;     ///< 0000 0080 Dwarf's dead body or body part

        unsigned int rotten : 1;         ///< 0000 0100 Rotten food
        unsigned int spider_web : 1;     ///< 0000 0200 Thread in spider web
        unsigned int construction : 1;   ///< 0000 0400 Material used in construction
        unsigned int unk3 : 1;           ///< 0000 0800 unknown, unseen, unusable

        unsigned int unk4 : 1;           ///< 0000 1000 unknown, unseen
        unsigned int murder : 1;         ///< 0000 2000 Implies murder - used in fell moods
        unsigned int foreign : 1;        ///< 0000 4000 Item is imported
        unsigned int trader : 1;         ///< 0000 8000 Item ownwed by trader

        unsigned int owned : 1;          ///< 0001 0000 Item is owned by a dwarf
        unsigned int garbage_colect : 1; ///< 0002 0000 Marked for deallocation by DF it seems
        unsigned int artifact1 : 1;      ///< 0004 0000 Artifact ?
        unsigned int forbid : 1;         ///< 0008 0000 Forbidden item

        unsigned int unk6 : 1;           ///< 0010 0000 unknown, unseen
        unsigned int dump : 1;           ///< 0020 0000 Designated for dumping
        unsigned int on_fire: 1;         ///< 0040 0000 Indicates if item is on fire, Will Set Item On Fire if Set!
        unsigned int melt : 1;           ///< 0080 0000 Designated for melting, if applicable

        unsigned int hidden : 1;       ///< 0100 0000 Hidden item
        unsigned int in_chest : 1;     ///< 0200 0000 Stored in chest/part of well?
        unsigned int unk7 : 1;         ///< 0400 0000 unknown, unseen
        unsigned int artifact2 : 1;    ///< 0800 0000 Artifact ?

        unsigned int unk8 : 1;         ///< 1000 0000 unknown, unseen, common
        unsigned int unk9 : 1;         ///< 2000 0000 unknown, set when viewing details
        unsigned int unk10 : 1;        ///< 4000 0000 unknown, unseen
        unsigned int unk11 : 1;        ///< 8000 0000 unknown, unseen
    };
};

/**
 * Describes relationship of an item with other objects
 * \ingroup grp_items
 */
struct t_itemref : public t_virtual
{
    int32_t value;
};

/**
 * A partial mirror of a DF base type for items
 * \ingroup grp_items
 */
class df_item
{
public:
    // 4
    int16_t x;
    int16_t y;
    // 8
    int16_t z;
    // C
    t_itemflags flags;
    // 10
    uint32_t age;
    // 14
    uint32_t id;
    // 18
    std::vector<void *> unk1;
    std::vector<t_itemref *> itemrefs;
public:
    // 0x0
    virtual int32_t getType();
    virtual int16_t getSubtype();
    virtual int32_t getSubMaterial();
    virtual int16_t getMaterial();
    // 0x10
    /*
     hm, [4] looks complicated                    *
     takes a parameter
     looks like 0x017081A4 is a vector of something
     this one sets an item property at offset 0xA0
     (0.31.25 Windows SDL)
     */
    virtual void fn4(void);
    virtual void setMaterial(int16_t mat);
    virtual void setSubMaterial (int32_t submat);
    // another one? really? 
    virtual int16_t getMaterial2();
    // 0x20
    // more of the same?
    virtual int32_t getSubMaterial2();
    virtual void fn9(void);
    virtual void fn10(void);
    virtual void fn11(void);
    // 0x30
    virtual void fn12(void);
    virtual void fn13(void);
    virtual void fn14(void);
    virtual void fn15(void);
    // 0x40
    virtual void fn16(void);
    virtual void fn17(void);
    virtual void fn18(void);
    virtual void fn19(void);
    // 0x50
    virtual void fn20(void);
    virtual void fn21(void);
    virtual void fn22(void);
    virtual void fn23(void);
    // 0x60
    virtual void fn24(void);
    virtual void fn25(void);
    virtual void fn26(void);
    virtual void fn27(void);
    // 0x70
    virtual void fn28(void);
    virtual void fn29(void);
    virtual void fn30(void);
    virtual void fn31(void);
    // 0x80
    virtual void fn32(void);
    virtual void fn33(void);
    virtual void fn34(void);
    virtual void fn35(void);
    // 0x90
    virtual void fn36(void);
    virtual void fn37(void);
    virtual void fn38(void);
    virtual void fn39(void);
    // 0xA0
    virtual void fn40(void);
    virtual void fn41(void);
    virtual void fn42(void);
    virtual void fn43(void);
    // 0xB0
    virtual void fn44(void);
    virtual void fn45(void);
    virtual void fn46(void);
    virtual void fn47(void);
    // 0xC0
    virtual void fn48(void);
    virtual void fn49(void);
    virtual void fn50(void);
    virtual int16_t getWear(void); // 0 = normal, 1 = x, 2 = X, 3 = XX
    // 0xD0
    virtual void setWear(int16_t wear); // also zeroes wear timer?
    virtual void fn53(void);
    virtual void fn54(void);
    virtual void fn55(void);
    // 0xE0
    virtual void fn56(void);
    virtual void fn57(void);
    virtual void fn58(void);
    virtual void fn59(void);
    // 0xF0
    virtual void fn60(void);
    virtual void fn61(void);
    virtual void fn62(void);
    virtual void fn63(void);
    // 0x100
    virtual void fn64(void);
    virtual void fn65(void);
    virtual void fn66(void);
    virtual void fn67(void);
    // 0x110
    virtual void fn68(void);
    virtual void fn69(void);
    virtual void fn70(void);
    virtual void fn71(void);
    // 0x120
    virtual void fn72(void);
    virtual void fn73(void);
    virtual void fn74(void);
    virtual void fn75(void);
    // 0x130
    virtual void fn76(void);
    virtual void fn77(void);
    virtual void fn78(void);
    virtual void fn79(void);
    // 0x140
    virtual void fn80(void);
    virtual void fn81(void);
    virtual void fn82(void);
    virtual void fn83(void);
    // 0x150
    virtual void fn84(void);
    virtual void fn85(void);
    virtual void fn86(void);
    virtual void fn87(void);
    // 0x160
    virtual void fn88(void);
    virtual void fn89(void);
    virtual void fn90(void);
    virtual void fn91(void);
    // 0x170
    virtual void fn92(void);
    virtual void fn93(void);
    virtual void fn94(void);
    virtual void fn95(void);
    // 0x180
    virtual void fn96(void);
    virtual void fn97(void);
    virtual void fn98(void);
    virtual void fn99(void);
    // 0x190
    virtual void fn100(void);
    virtual void fn101(void);
    virtual void fn102(void);
    virtual void fn103(void);
    // 0x1A0
    virtual void fn104(void);
    virtual void fn105(void);
    virtual void fn106(void);
    virtual void fn107(void);
    // 0x1B0
    virtual void fn108(void);
    virtual void fn109(void);
    virtual void fn110(void);
    virtual void fn111(void);
    // 0x1C0
    virtual void fn112(void);
    virtual void fn113(void);
    virtual void fn114(void);
    virtual void fn115(void);
    // 0x1D0
    virtual void fn116(void);
    virtual void fn117(void);
    virtual void fn118(void);
    virtual void fn119(void);
    // 0x1E0
    virtual void fn120(void);
    virtual void fn121(void);
    virtual void fn122(void);
    virtual void fn123(void);
    // 0x1F0
    virtual void fn124(void);
    virtual void fn125(void);
    virtual void fn126(void);
    virtual void fn127(void);
    // 0x200
    virtual void fn128(void);
    virtual void fn129(void);
    virtual void fn130(void);
    virtual void fn131(void);
    // 0x210
    virtual void fn132(void);
    virtual int32_t getStackSize( void );
    virtual void fn134(void);
    virtual void fn135(void);
    // 0x220
    virtual void fn136(void);
    virtual void fn137(void);
    virtual void fn138(void);
    virtual void fn139(void);
    // 0x230
    virtual void fn140(void);
    virtual void fn141(void);
    virtual void fn142(void);
    virtual void fn143(void);
    // 0x240
    virtual void fn144(void);
    virtual void fn145(void);
    virtual void fn146(void);
    virtual void fn147(void);
    // 0x250
    virtual void fn148(void);
    virtual void fn149(void);
    virtual void fn150(void);
    virtual int16_t getQuality( void );
    // 0x260
    virtual void fn152(void);
    virtual void fn153(void);
    virtual void fn154(void);
    virtual void fn155(void);
    // 0x270
    virtual void fn156(void);
    virtual void fn157(void);
    virtual void fn158(void);
    virtual void fn159(void);
    // 0x280
    virtual void fn160(void);
    virtual void fn161(void);
    virtual void fn162(void);
    virtual void fn163(void);
    // 0x290
    virtual void fn164(void);
    virtual void fn165(void);
    virtual void fn166(void);
    virtual void fn167(void);
    // 0x2A0
    virtual void fn168(void); // value returned (int) = first param to descriprion construction function
    virtual void fn169(void);
    virtual void fn170(void);
    virtual void fn171(void);
    // 0x2B0
    virtual void fn172(void);
    virtual void fn173(void);
    virtual void fn174(void);
    virtual void fn175(void);
    // 0x2C0
    virtual void fn176(void);
    virtual void fn177(void);
    //virtual std::string *getItemDescription ( std::string * str, int sizes = 0); // 0 = stacked, 1 = singular, 2 = plural
    virtual std::string *getItemDescription ( std::string * str, int sizes = 0); // 0 = stacked, 1 = singular, 2 = plural
    virtual void fn179(void);
    // more follows for sure... bleh.
};

/**
 * Type for holding an item read from DF
 * \ingroup grp_items
 */
struct dfh_item
{
    df_item * base;
    t_material matdesc;
    int32_t quantity;
    int32_t quality;
    int16_t wear_level;
};


/**
 * Type for holding item improvements. broken/unused.
 * \ingroup grp_items
 */
struct t_improvement
{
    t_material matdesc;
    int32_t quality;
};

/**
 * The Items module
 * \ingroup grp_modules
 * \ingroup grp_items
 */
class DFHACK_EXPORT Items : public Module
{
public:
    Items();
    ~Items();
    bool Start();
    bool Finish();

    bool readItemVector(std::vector<df_item *> &items);
    df_item * findItemByID(int32_t id);

    /// get a string describing an item
    //std::string getItemDescription(const dfh_item & item, int type);
    /// get a short name for an item
    std::string getItemClass(const dfh_item & item);
    /// read an item, including the extra attributes
    bool readItem(df_item * itembase, dfh_item & item);
    /// write item base (position and flags only = t_item part of dfh_item)
    bool writeItem(const dfh_item & item);

    /// who owns this item we already read?
    int32_t getItemOwnerID(const dfh_item & item);
    /// which item is it contained in?
    int32_t getItemContainerID(const dfh_item & item);
    /// which items does it contain?
    bool getContainedItems(const dfh_item & item, std::vector<int32_t> &items);

    /// wipe out the owner records
    bool removeItemOwner(dfh_item &item, Creatures *creatures);

    bool readItemRefs(const dfh_item &item, const ClassNameCheck &classname,
                      std::vector<int32_t> &values);
    bool unknownRefs(const dfh_item &item, std::vector<std::string>& names,
                     std::vector<int32_t>& values);
private:
    class Private;
    Private* d;
};
}
#endif
