#pragma once
#ifndef CL_MOD_VERMIN
#define CL_MOD_VERMIN
/**
 * \defgroup grp_vermin Wild vermin (ants, bees, etc)
 * @ingroup grp_vermin
 */
#include "dfhack/Export.h"
#include "dfhack/Module.h"

#ifdef __cplusplus
namespace DFHack
{
#endif
    /**
     * Structure for holding a read DF vermin spawn point object
     * \ingroup grp_vermin
     */
    struct t_spawnPoint
    {
        uint32_t origin;
        int16_t race;
        uint16_t type;
        uint16_t x;
        uint16_t y;
        uint16_t z;
        bool     in_use;
        uint8_t  unknown;
        uint32_t countdown;
    };

#ifdef __cplusplus
    class DFContextShared;
    class SpawnPoints;

    /**
     * The Vermin module - allows reading DF vermin
     * \ingroup grp_modules
     * \ingroup grp_vermin
     */
    class DFHACK_EXPORT Vermin : public Module
    {
        public:
        Vermin();
        ~Vermin();

        bool Finish();

        // NOTE: caller must call delete on result when done.
        SpawnPoints* getSpawnPoints();

        private:
        struct Private;
        Private *d;

        friend class SpawnPoints;
    };

    class DFHACK_EXPORT SpawnPoints
    {
    public:
        static const uint16_t TYPE_WILD_COLONY = 0xFFFF;

    protected:
        SpawnPoints(Vermin * v);

    public:
        ~SpawnPoints();

        size_t size();
        bool   Read (const uint32_t index, t_spawnPoint & point);
        bool   Write (const uint32_t index, t_spawnPoint & point);
        bool   isValid();

        static bool isWildColony(t_spawnPoint & point);

    private:
        Vermin* v;
        std::vector <void*> * p_sp;

        friend class Vermin;
    };
}
#endif // __cplusplus

#endif
