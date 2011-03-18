#ifndef CL_MOD_BUILDINGS
#define CL_MOD_BUILDINGS
/*
* Buildings - also includes zones and stockpiles
*/
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
/**
 * \defgroup grp_buildings Building module parts
 * @ingroup grp_modules
 */

namespace DFHack
{
    /**
     * Structure for holding a read DF building object
     * \ingroup grp_buildings
     */
    struct t_building
    {
        uint32_t origin;
        uint32_t vtable;
        
        uint32_t x1;
        uint32_t y1;
        
        uint32_t x2;
        uint32_t y2;
        
        uint32_t z;
        
        t_matglossPair material;
        
        uint32_t type;
        // FIXME: not complete, we need building presence bitmaps for stuff like farm plots and stockpiles, orientation (N,E,S,W) and state (open/closed)
    };
    
    class DFContextShared;
    /**
     * The Buildings module - allows reading DF buildings
     * \ingroup grp_modules
     * \ingroup grp_buildings
     */
    class DFHACK_EXPORT Buildings : public Module
    {
        public:
        Buildings(DFContextShared * d);
        ~Buildings();
        bool Start(uint32_t & numBuildings);
        // read one building at offset
        bool Read (const uint32_t index, t_building & building);
        bool Finish();
        
        // read a vector of names
        bool ReadCustomWorkshopTypes(std::map <uint32_t, std::string> & btypes);
        // returns -1 on error, >= 0 for real value
        int32_t GetCustomWorkshopType(t_building & building);
        
        private:
        struct Private;
        Private *d;
    };
}
#endif
