#ifndef CL_MOD_MATERIALS
#define CL_MOD_MATERIALS
/*
* Creatures
*/
#include "Export.h"
namespace DFHack
{
    struct APIPrivate;
    
    struct t_matgloss
    {
        char id[128]; //the id in the raws
        uint8_t fore; // Annoyingly the offset for this differs between types
        uint8_t back;
        uint8_t bright;
        char name[128]; //this is the name displayed ingame
    };

    struct t_matglossPlant
    {
        char id[128]; //the id in the raws
        uint8_t fore; // Annoyingly the offset for this differs between types
        uint8_t back;
        uint8_t bright;
        char name[128]; //this is the name displayed ingame
        char drink_name[128];  //the name this item becomes a drink
        char food_name[128];
        char extract_name[128];
    };
    
    class DFHACK_EXPORT Materials
    {
        public:
        
        Materials(DFHack::APIPrivate * _d);
        ~Materials();
        
        bool ReadInorganicMaterials (std::vector<t_matgloss> & output);
        bool ReadOrganicMaterials (std::vector<t_matgloss> & output);
        
        bool ReadWoodMaterials (std::vector<t_matgloss> & output);
        bool ReadPlantMaterials (std::vector<t_matgloss> & output);
        // bool ReadPlantMaterials (std::vector<t_matglossPlant> & output);
        
        // TODO: maybe move to creatures?
        bool ReadCreatureTypes (std::vector<t_matgloss> & output);
        private:
            APIPrivate* d;
    };
}
#endif

