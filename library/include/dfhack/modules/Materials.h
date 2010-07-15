#ifndef CL_MOD_MATERIALS
#define CL_MOD_MATERIALS
/*
 * Materials
 */
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"
namespace DFHack
{
    class DFContextShared;

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

    struct t_bodypart
    {
        char id[128];
        char category[128];
        char single[128];
        char plural[128];
    };

    struct t_colormodifier
    {
        char part[128];
        std::vector<uint32_t> colorlist;
        uint32_t startdate; /* in days */
        uint32_t enddate; /* in days */
    };

    struct t_creaturecaste
    {
        char rawname[128];
        char singular[128];
        char plural[128];
        char adjective[128];
        std::vector<t_colormodifier> ColorModifier;
        std::vector<t_bodypart> bodypart;

        t_attrib strength;
        t_attrib agility;
        t_attrib toughness;
        t_attrib endurance;
        t_attrib recuperation;
        t_attrib disease_resistance;
        t_attrib analytical_ability;
        t_attrib focus;
        t_attrib willpower;
        t_attrib creativity;
        t_attrib intuition;
        t_attrib patience;
        t_attrib memory;
        t_attrib linguistic_ability;
        t_attrib spatial_sense;
        t_attrib musicality;
        t_attrib kinesthetic_sense;
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

    class DFHACK_EXPORT Materials : public Module
    {
    public:
        Materials(DFHack::DFContextShared * _d);
        ~Materials();
        bool Finish();

        std::vector<t_matgloss> inorganic;
        std::vector<t_matgloss> organic;
        std::vector<t_matgloss> tree;
        std::vector<t_matgloss> plant;
        std::vector<t_matgloss> race;
        std::vector<t_creaturetype> raceEx;
        std::vector<t_descriptor_color> color;
        std::vector<t_matglossOther> other;
        std::vector<t_matgloss> alldesc;

        bool ReadInorganicMaterials (void);
        bool ReadOrganicMaterials (void);
        bool ReadWoodMaterials (void);
        bool ReadPlantMaterials (void);
        bool ReadCreatureTypes (void);
        bool ReadCreatureTypesEx (void);
        bool ReadDescriptorColors(void);
        bool ReadOthers (void);

        void ReadAllMaterials(void);

		std::string getType(t_material & mat);
        std::string getDescription(t_material & mat);
    private:
        class Private;
        Private* d;
    };
}
#endif

