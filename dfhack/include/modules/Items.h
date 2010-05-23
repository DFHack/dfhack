#ifndef CL_MOD_ITEMS
#define CL_MOD_ITEMS
/*
* Creatures
*/
#include "Export.h"
namespace DFHack
{

class Context;
class DFContextPrivate;


enum accessor_type {ACCESSOR_CONSTANT, ACCESSOR_INDIRECT, ACCESSOR_DOUBLE_INDIRECT};

/* this is used to store data about the way accessors work */
class DFHACK_EXPORT Accessor
{
private:
	accessor_type type;
	int32_t constant;
	uint32_t offset1;
	uint32_t offset2;
	Process * p;
	uint32_t dataWidth;
public:
	Accessor(uint32_t function, Process * p);
	Accessor(accessor_type type, int32_t constant, uint32_t offset1, uint32_t offset2, uint32_t dataWidth, Process * p);
	int32_t getValue(uint32_t objectPtr);
	bool isConstant();
};

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

class DFHACK_EXPORT Items
{
public:
	Items(DFContextPrivate * _d);
	~Items();
	std::string getItemDescription(uint32_t itemptr, Materials * Materials);
	std::string getItemClass(int32_t index);
	bool getItemData(uint32_t itemptr, t_item & item);
private:
	class Private;
	Private* d;
	std::map<int32_t, ItemDesc *> descType;
	std::map<uint32_t, ItemDesc *> descVTable;
};
}
#endif
