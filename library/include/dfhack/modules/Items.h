#ifndef CL_MOD_ITEMS
#define CL_MOD_ITEMS

/*
* Creatures
*/
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
#include "dfhack/DFTypes.h"

namespace DFHack
{

class Context;
class DFContextShared;

struct t_item_header
{
    int16_t x;
    int16_t y;
    int16_t z;
    t_itemflags flags;
};

struct t_item
{
    t_item_header header;
    t_material matdesc;
    int32_t quantity;
    int32_t quality;
    int16_t wear_level;
};

struct t_improvement
{
    t_material matdesc;
    int32_t quality;
};

class DFHACK_EXPORT Items : public Module
{
public:
    Items(DFContextShared * _d);
    ~Items();
    bool Start();
    bool Finish();
    std::string getItemDescription(uint32_t itemptr, Materials * Materials);
    std::string getItemClass(int32_t index);
    bool getItemData(uint32_t itemptr, t_item & item);
    int32_t getItemOwnerID(uint32_t itemptr);
    void setItemFlags(uint32_t itemptr, t_itemflags new_flags);
private:
    class Private;
    Private* d;
};
}
#endif
