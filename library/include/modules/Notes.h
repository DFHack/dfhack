#pragma once
#ifndef CL_MOD_NOTES
#define CL_MOD_NOTES
/**
 * \defgroup grp_notes In game notes (and routes)
 * @ingroup grp_notes
 */
#include "Export.h"
#include "Module.h"

#include <vector>
#include <string>

#ifdef __cplusplus
namespace DFHack
{
#endif
    /**
     * Game's structure for a note.
     * \ingroup grp_notes
     */
    struct t_note
    {
        // First note created has id 0, second has id 1, etc.  Not affected
        // by lower id notes being deleted.
        uint32_t id; // 0
        uint8_t  symbol; // 4
        uint8_t  unk1; // alignment padding?
        uint16_t foreground; // 6
        uint16_t background; // 8
        uint16_t unk2; // alignment padding?

        std::string name; // C
        std::string text; // 10

        uint16_t x; // 14
        uint16_t y; // 16
        uint16_t z; // 18

        // Is there more?
    };

#ifdef __cplusplus

    /**
     * The notes module - allows reading DF in-game notes
     * \ingroup grp_modules
     * \ingroup grp_notes
     */
    class DFHACK_EXPORT Notes : public Module
    {
        public:
        Notes();
        ~Notes(){};
        bool Finish()
        {
            return true;
        }
        std::vector<t_note*>* notes;
    };

}
#endif // __cplusplus

#endif
