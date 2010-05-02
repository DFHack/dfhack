#ifndef CL_MOD_MATERIALS
#define CL_MOD_MATERIALS
/*
* Creatures
*/
#include "Export.h"
namespace DFHack
{
    class APIPrivate;
    
    struct t_matgloss
    {
        char id[128]; //the id in the raws
        uint8_t fore; // Annoyingly the offset for this differs between types
        uint8_t back;
        uint8_t bright;
        char name[128]; //this is the name displayed ingame
    };

    struct t_descriptor_color
    {
        char id[128]; // id in the raws
        float r;
        float v;
        float b;
        char name[128]; //displayed name
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
    
    struct t_creaturecaste
    {
        char rawname[128];
        char singular[128];
        char plural[128];
        char adjective[128];
    };

    struct t_matglossOther
    {
	    char rawname[128];
    };

    struct t_creatureextract
    {
	    char rawname[128];
    };
    // this doesn't transfer well across the shm gap...
    struct t_creaturetype
    {
        char rawname[128];
	std::vector <t_creaturecaste> castes;
	std::vector <t_creatureextract> extract;
	uint8_t tile_character;
	struct
	{
		uint16_t fore;
		uint16_t back;
		uint16_t bright;
	} tilecolor;
    };

    // this structure describes what are things made of in the DF world
    struct t_material
    {
	    int16_t itemType;
	    int16_t subType;
	    int16_t subIndex;
	    int32_t index;
	    uint32_t flags;
    };
    
    class DFHACK_EXPORT Materials
    {
        public:
        
        Materials(DFHack::APIPrivate * _d);
        ~Materials();

	std::vector<t_matgloss> inorganic;
	std::vector<t_matgloss> organic;
	std::vector<t_matgloss> tree;
	std::vector<t_matgloss> plant;
	std::vector<t_matgloss> race;
	std::vector<t_creaturetype> raceEx;
	std::vector<t_descriptor_color> color;
	std::vector<t_matglossOther> other;
        
        bool ReadInorganicMaterials (void);
        bool ReadOrganicMaterials (void);
        bool ReadWoodMaterials (void);
        bool ReadPlantMaterials (void);
        bool ReadCreatureTypes (void);
        bool ReadCreatureTypesEx (void);
	bool ReadDescriptorColors(void);
	bool ReadOthers (void);

	void ReadAllMaterials(void);

	std::string getDescription(t_material & mat);
	/*
        bool ReadInorganicMaterials (std::vector<t_matgloss> & output);
        bool ReadOrganicMaterials (std::vector<t_matgloss> & output);
        bool ReadWoodMaterials (std::vector<t_matgloss> & output);
        bool ReadPlantMaterials (std::vector<t_matgloss> & output);
        
        // TODO: maybe move to creatures?
        bool ReadCreatureTypes (std::vector<t_matgloss> & output);
        bool ReadCreatureTypesEx (vector<t_creaturetype> & creatures);
	bool ReadDescriptorColors(std::vector<t_descriptor_color> & output);
	*/
        private:
            class Private;
            Private* d;
    };
}
#endif

