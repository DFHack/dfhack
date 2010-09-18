#ifndef CL_MOD_ITEMS
#define CL_MOD_ITEMS

/*
 * DEPRECATED, DO NOT USE UNTIL FURTHER NOTICE!
 **/

/*
* Creatures
*/
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
namespace DFHack
{

class Context;
class DFContextShared;

struct t_item
{
    t_material matdesc;
    int32_t quantity;
    int32_t quality;
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
private:
    class Private;
    Private* d;
};
}
#endif
