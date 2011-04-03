#ifndef CL_MOD_ENGRAVINGS
#define CL_MOD_ENGRAVINGS
/*
* DF engravings
*/
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"

/**
 * \defgroup grp_engraving Engraving module parts
 * @ingroup grp_modules
 */
namespace DFHack
{
    /**
     * engraving flags
     * \ingroup grp_engraving
     */
    struct flg_engraving
    {
        unsigned int floor : 1; // engraved on a floor
        unsigned int west : 1; // engraved from west
        unsigned int east : 1; // engraved from east
        unsigned int north : 1; // engraved from north
        unsigned int south : 1; // engraved from south
        unsigned int hidden : 1; // hide the engraving
        unsigned int rest : 26; // probably unused
    };

    /**
     * type the engraving is made of
     * \ingroup grp_engraving
     */
    struct t_engraving
    {
        //0
        int32_t artistIdx; /*!< Index of the artist in some global vector */
        // 4
        int32_t unknownIdx; // likes to stay -1
        // 8
        uint32_t unknown1; // likes to stay 1
        // C
        uint16_t x; /*!< X coordinate */
        uint16_t y; /*!< Y coordinate */
        // 10
        uint16_t z; /*!< Z coordinate */
        uint16_t padding; /*!< Could be used for hiding values. */
        // 14
        flg_engraving flags; // 0x20 = hide symbol
        // 18
        uint32_t display_character; // really? 4 bytes for that?
        // 1C
        uint32_t type; // possibly an enum, decides what vectors to use for imagery
        // 20
        int16_t subtype_idx; // index in a vector kind of deal related to previous value
        uint16_t quality; // from 0 to 5
        // 24
        uint32_t unknown2;
        // 28 = length
    };
    /**
     * structure for holding a DF engraving
     * \ingroup grp_engraving
     */
    struct dfh_engraving
    {
        t_engraving s;
        uint32_t origin;
    };
    class DFContextShared;
    /**
     * The Engravings module - allows reading engravings :D
     * \ingroup grp_modules
     * \ingroup grp_engraving
     */
    class DFHACK_EXPORT Engravings : public Module
    {
        public:
        Engravings(DFContextShared * d);
        ~Engravings();
        bool Start(uint32_t & numEngravings);
        bool Read (const uint32_t index, dfh_engraving & engr);
        bool Write (const dfh_engraving & engr);
        bool Finish();

        private:
        struct Private;
        Private *d;
    };
}
#endif
