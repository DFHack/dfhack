#pragma once
#ifndef CL_MOD_NOTES
#define CL_MOD_NOTES
/**
 * \defgroup grp_notes In game notes (and routes)
 * @ingroup grp_notes
 */
#include "dfhack/Export.h"
#include "dfhack/Module.h"

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
        uint32_t id;

        uint8_t  symbol;
        uint8_t  unk1;
        uint16_t foreground;
        uint16_t background;
        uint16_t unk2;

        std::string name;
        std::string text;

        uint16_t x;
        uint16_t y;
        uint16_t z;

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
        ~Notes();

        bool Finish();

        // Returns NULL if there's no notes yet.
        std::vector<t_note*>* getNotes();

        private:
        struct Private;
        Private *d;

    };

}
#endif // __cplusplus

#endif
