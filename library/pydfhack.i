%module pydfhack
%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"
%include "stdint.i"
%include "typemaps.i"

/* This goes to the header of the wrapper code */
%{
#define LINUX_BUILD
#define SWIG_WRAPPER
#include "DFTypes.h"
#include "DFHackAPI.h"
using namespace std;
using namespace DFHack;
%}

/* make swig not react to the macro */
%define DFHACK_EXPORT

/* Parse the header file to generate wrappers */
%include "DFTypes.h"

/* templates have to be instantiated for swig */
%template(MatglossVector) std::vector<DFHack::t_matgloss>;
%template(PlantMatglossVector) std::vector<DFHack::t_matglossPlant>;
%template(VeinVector) std::vector <DFHack::t_vein>;
%template(IceVeinVector) std::vector <DFHack::t_frozenliquidvein>;

/*
This causes swig to generate BS uncompilable code
%template(GeologyAssign) std::vector < std::vector <uint16_t> >;
*/

/*
SWIG typemaps recipe
%apply double *OUTPUT { double *result };  // 'double *result' is to be treated as OUTPUT
%clear double *result;                     // Remove all typemaps for double *result

INPUT
int *INPUT      
short *INPUT
long *INPUT
unsigned int *INPUT
unsigned short *INPUT
unsigned long *INPUT
double *INPUT
float *INPUT

OUTPUT
int *OUTPUT
short *OUTPUT
long *OUTPUT
unsigned int *OUTPUT
unsigned short *OUTPUT
unsigned long *OUTPUT
double *OUTPUT
float *OUTPUT

INOUT
int *INOUT
short *INOUT
long *INOUT
unsigned int *INOUT
unsigned short *INOUT
unsigned long *INOUT
double *INOUT
float *INOUT
*/

namespace DFHack
{
    class memory_info;
    class Process;
    class API
    {
        
        class Private;
        
        Private * const d;
    public:
        API(const std::string path_to_xml);
        ~API();
        bool Attach();
        bool Detach();
        bool isAttached();
        
        bool ReadPauseState(); 
        
        bool ReadViewScreen(t_viewscreen &);
        
        uint32_t ReadMenuState();
        
        bool Suspend();
        bool AsyncSuspend();
        bool Resume();
        bool ForceResume();
        bool isSuspended();
        bool ReadStoneMatgloss(std::vector<t_matgloss> & OUTPUT);
        bool ReadWoodMatgloss (std::vector<t_matgloss> & OUTPUT);
        bool ReadMetalMatgloss(std::vector<t_matgloss> & OUTPUT);
        bool ReadPlantMatgloss(std::vector<t_matgloss> & OUTPUT);
        /*
         * SWIG reports that it can't wrap the second method here
         */
        bool ReadPlantMatgloss (std::vector<t_matglossPlant> & OUTPUT);
        bool ReadCreatureMatgloss(std::vector<t_matgloss> & OUTPUT);

        bool ReadGeology( std::vector < std::vector <uint16_t> >& OUTPUT );

        bool InitMap();
        bool DestroyMap();
        
        %apply uint32_t& OUTPUT { uint32_t& x };
        %apply uint32_t& OUTPUT { uint32_t& y };
        %apply uint32_t& OUTPUT { uint32_t& z };
        void getSize(uint32_t& x, uint32_t& y, uint32_t& z);

        bool isValidBlock(uint32_t blockx, uint32_t blocky, uint32_t blockz);
        uint32_t getBlockPtr (uint32_t blockx, uint32_t blocky, uint32_t blockz);
        
        bool ReadTileTypes(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer);
        bool WriteTileTypes(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer);
        
        bool ReadDesignations(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer);
        bool WriteDesignations (uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer);
        
        bool ReadOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer);
        bool WriteOccupancy(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer);

        bool ReadDirtyBit(uint32_t blockx, uint32_t blocky, uint32_t blockz, bool &OUTPUT);
        bool WriteDirtyBit(uint32_t blockx, uint32_t blocky, uint32_t blockz, bool dirtybit);
        
        bool ReadRegionOffsets(uint32_t blockx, uint32_t blocky, uint32_t blockz, uint8_t *buffer);
        
        bool ReadVeins(uint32_t blockx, uint32_t blocky, uint32_t blockz, std::vector <t_vein> & veins, std::vector <t_frozenliquidvein>& ices);
        
        %apply uint32_t& OUTPUT { uint32_t & numObjs };
        
        bool InitReadConstructions( uint32_t & numObjs );
        bool ReadConstruction(const int32_t index, t_construction & OUTPUT);
        void FinishReadConstructions();

        bool InitReadBuildings ( uint32_t & numObjs );
        bool ReadBuilding(const int32_t index, t_building & OUTPUT);
        void FinishReadBuildings();

        bool InitReadVegetation( uint32_t & numObjs );
        bool ReadVegetation(const int32_t index, t_tree_desc & OUTPUT);
        void FinishReadVegetation();
        
        bool InitReadCreatures( uint32_t & numObjs );
        int32_t ReadCreatureInBox(int32_t index, t_creature & OUTPUT,
                                  const uint16_t x1, const uint16_t y1,const uint16_t z1,
                                  const uint16_t x2, const uint16_t y2,const uint16_t z2);

        bool ReadCreature(const int32_t index, t_creature & OUTPUT);
        void FinishReadCreatures();
        
        /*
        A conundrum really.
        */
        void ReadRaw (const uint32_t offset, const uint32_t size, uint8_t *target);
        void WriteRaw (const uint32_t offset, const uint32_t size, uint8_t *source);
        
        bool InitViewAndCursor();

        bool InitReadNotes( uint32_t & numnotes );
        bool ReadNote(const int32_t index, t_note & OUTPUT);
        void FinishReadNotes();

        bool InitReadSettlements( uint32_t & numObjs );
        bool ReadSettlement(const int32_t index, t_settlement & OUTPUT);
        bool ReadCurrentSettlement(t_settlement & settlement);
        void FinishReadSettlements();

        bool InitReadHotkeys( );
        bool ReadHotkeys(t_hotkey hotkeys[]);
        
        %apply int32_t& OUTPUT { int32_t& x };
        %apply int32_t& OUTPUT { int32_t& y };
        %apply int32_t& OUTPUT { int32_t& z };
        bool getViewCoords (int32_t &x, int32_t &y, int32_t &z);
        bool setViewCoords (const int32_t x, const int32_t y, const int32_t z);
        
        bool getCursorCoords (int32_t &x, int32_t &y, int32_t &z);
        bool setCursorCoords (const int32_t x, const int32_t y, const int32_t z);

        bool getCurrentCursorCreatures(std::vector<uint32_t> &OUTPUT); 

        bool InitViewSize();
        
        %apply int32_t& OUTPUT { int32_t& width };
        %apply int32_t& OUTPUT { int32_t& height };
        bool getWindowSize(int32_t & width, int32_t & height);
        bool getItemIndexesInBox(std::vector<uint32_t> &indexes,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2);
        bool InitReadNameTables (std::vector< std::vector<std::string> > & translations , std::vector< std::vector<std::string> > & foreign_languages);
        void FinishReadNameTables();
        
        std::string TranslateName(const t_name & name,const std::vector< std::vector<std::string> > & translations ,const std::vector< std::vector<std::string> > & foreign_languages, bool inEnglish=true);
        
        void WriteLabors(const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS]);
        
        bool InitReadItems(uint32_t & numitems);
        bool ReadItem(const uint32_t index, t_item & OUTPUT);
        void FinishReadItems();

        memory_info *getMemoryInfo();
        Process * getProcess();
        DFWindow * getWindow();
        bool ReadItemTypes(std::vector< std::vector< t_itemType > > & itemTypes);
    };
};

%enddef DFHACK_EXPORT