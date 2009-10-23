/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#ifndef BUILD_DFHACK_LIB
#   define BUILD_DFHACK_LIB
#endif

#include "DFCommon.h"
#include "DFHackAPI.h"
#include "DFHackAPIc.h"

#ifdef LINUX_BUILD
# ifndef secure_strcpy
#  define secure_strcpy(dst, size, buf) strcpy((dst), (buf))
# endif
#else
# if defined(_MSC_VER) && _MSC_VER >= 1400
#  ifndef secure_strcpy
#   define secure_strcpy(dst, size, buf) strcpy_s((dst), (size), (buf))
#  endif
# else
#  ifndef secure_strcpy
#   define secure_strcpy(dst, size, buf) strcpy((dst), (buf))
#  endif
# endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// The C interface for vector management
    DFHACKAPI void DFHackAPIVector_free (DFHackAPIVectorC *vector)
    {
        uint32_t i;
        switch (vector->type)
        {
            case DFHackAPIVectorTypeC_Normal:
                delete [] (vector->data);
                break;
            case DFHackAPIVectorTypeC_Matgloss:
                delete [] ( (t_matgloss *) vector->data);
                break;
            case DFHackAPIVectorTypeC_Uint16:
                delete [] ( (uint16_t *) vector->data);
                break;
            case DFHackAPIVectorTypeC_Vein:
                delete [] ( (t_vein *) vector->data);
                break;
            case DFHackAPIVectorTypeC_String:
                for (i = 0; i < vector->length; i++)
                    delete [] ( (char **) vector->data) [i];
                delete [] ( (char **) vector->data);
                break;
            case DFHackAPIVectorTypeC_Recursive:
                for (i = 0; i < vector->length; i++)
                    DFHackAPIVector_free (& ( (DFHackAPIVectorC *) vector->data) [i]);
                delete [] ( (DFHackAPIVectorC *) vector->data);
                break;
        }

        vector->type = DFHackAPIVectorTypeC_Normal;
        vector->length = 0;
        vector->data = 0;
    }

// The C interface to DFHackAPI (for multiple language support)
    DFHACKAPI DFHackAPIHandle CreateDFHackAPI (const char *path_to_xml)
    {
        return new DFHackAPIImpl (path_to_xml);
    }

    DFHACKAPI void DestroyDFHackAPI (DFHackAPIHandle self)
    {
        if (self != NULL)
            delete self;
    }

    DFHACKAPI bool DFHackAPI_Attach (DFHackAPIHandle self)
    {
        return self->Attach();
    }

    DFHACKAPI bool DFHackAPI_Detach (DFHackAPIHandle self)
    {
        return self->Detach();
    }

    DFHACKAPI bool DFHackAPI_isAttached (DFHackAPIHandle self)
    {
        return self->isAttached();
    }

    DFHACKAPI bool DFHackAPI_ReadStoneMatgloss (DFHackAPIHandle self, DFHackAPIVectorC *output)
    {
        vector<t_matgloss> result;
        uint32_t i;
        bool retn = self->ReadStoneMatgloss (result);

        output->type = DFHackAPIVectorTypeC_Matgloss;
        output->length = result.size();
        output->data = new t_matgloss[output->length];
        for (i = 0; i < output->length; i++)
            ( (t_matgloss *) output->data) [i] = result[i];

        return retn;
    }

    DFHACKAPI bool DFHackAPI_ReadWoodMatgloss (DFHackAPIHandle self, DFHackAPIVectorC *output)
    {
        vector<t_matgloss> result;
        uint32_t i;
        bool retn = self->ReadWoodMatgloss (result);

        output->type = DFHackAPIVectorTypeC_Matgloss;
        output->length = result.size();
        output->data = new t_matgloss[output->length];
        for (i = 0; i < output->length; i++)
            ( (t_matgloss *) output->data) [i] = result[i];

        return retn;
    }

    DFHACKAPI bool DFHackAPI_ReadMetalMatgloss (DFHackAPIHandle self, DFHackAPIVectorC *output)
    {
        vector<t_matgloss> result;
        uint32_t i;
        bool retn = self->ReadMetalMatgloss (result);

        output->type = DFHackAPIVectorTypeC_Matgloss;
        output->length = result.size();
        output->data = new t_matgloss[output->length];
        for (i = 0; i < output->length; i++)
            ( (t_matgloss *) output->data) [i] = result[i];

        return retn;
    }

    DFHACKAPI bool DFHackAPI_ReadPlantMatgloss (DFHackAPIHandle self, DFHackAPIVectorC *output)
    {
        vector<t_matgloss> result;
        uint32_t i;
        bool retn = self->ReadPlantMatgloss (result);

        output->type = DFHackAPIVectorTypeC_Matgloss;
        output->length = result.size();
        output->data = new t_matgloss[output->length];
        for (i = 0; i < output->length; i++)
            ( (t_matgloss *) output->data) [i] = result[i];

        return retn;
    }
    
    DFHACKAPI bool DFHackAPI_ReadCreatureMatgloss (DFHackAPIHandle self, DFHackAPIVectorC *output)
    {
        vector<t_matgloss> result;
        uint32_t i;
        bool retn = self->ReadCreatureMatgloss (result);
        
        output->type = DFHackAPIVectorTypeC_Matgloss;
        output->length = result.size();
        output->data = new t_matgloss[output->length];
        for (i = 0; i < output->length; i++)
            ( (t_matgloss *) output->data) [i] = result[i];
        
        return retn;
    }
    DFHACKAPI bool DFHackAPI_ReadGeology (DFHackAPIHandle self, DFHackAPIVectorC *assign)
    {
        vector< vector<uint16_t> > result;
        uint32_t i, j;
        bool retn = self->ReadGeology (result);

        assign->type = DFHackAPIVectorTypeC_Recursive;
        assign->length = result.size();
        assign->data = new DFHackAPIVectorC[assign->length];
        for (i = 0; i < assign->length; i++)
        {
            DFHackAPIVectorC &current = ( (DFHackAPIVectorC *) assign->data) [i];
            current.type = DFHackAPIVectorTypeC_Uint16;
            current.length = result[i].size();
            current.data = new uint16_t[current.length];
            for (j = 0; j < current.length; j++)
                ( (uint16_t *) current.data) [j] = result[i][j];
        }

        return retn;
    }

    DFHACKAPI bool DFHackAPI_InitMap (DFHackAPIHandle self)
    {
        return self->InitMap();
    }

    DFHACKAPI bool DFHackAPI_DestroyMap (DFHackAPIHandle self)
    {
        return self->DestroyMap();
    }

    DFHACKAPI void DFHackAPI_getSize (DFHackAPIHandle self, uint32_t* x, uint32_t* y, uint32_t* z)
    {
        return self->getSize (*x, *y, *z);
    }

    DFHACKAPI bool DFHackAPI_isValidBlock (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz)
    {
        return self->isValidBlock (blockx, blocky, blockz);
    }

    DFHACKAPI bool DFHackAPI_ReadTileTypes (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer)
    {
        return self->ReadTileTypes (blockx, blocky, blockz, buffer);
    }

    DFHACKAPI bool DFHackAPI_WriteTileTypes (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint16_t *buffer)
    {
        return self->WriteTileTypes (blockx, blocky, blockz, buffer);
    }

    DFHACKAPI bool DFHackAPI_ReadDesignations (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer)
    {
        return self->ReadDesignations (blockx, blocky, blockz, buffer);
    }

    DFHACKAPI bool DFHackAPI_WriteDesignations (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer)
    {
        return self->WriteDesignations (blockx, blocky, blockz, buffer);
    }

    DFHACKAPI bool DFHackAPI_ReadOccupancy (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer)
    {
        return self->ReadOccupancy (blockx, blocky, blockz, buffer);
    }

    DFHACKAPI bool DFHackAPI_WriteOccupancy (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint32_t *buffer)
    {
        return self->WriteOccupancy (blockx, blocky, blockz, buffer);
    }

    DFHACKAPI bool DFHackAPI_ReadRegionOffsets (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, uint8_t *buffer)
    {
        return self->ReadRegionOffsets (blockx, blocky, blockz, buffer);
    }

    DFHACKAPI bool DFHackAPI_ReadVeins (DFHackAPIHandle self, uint32_t blockx, uint32_t blocky, uint32_t blockz, DFHackAPIVectorC * veins)
    {
        vector<t_vein> result;
        uint32_t i;
        bool retn = self->ReadVeins (blockx, blocky, blockz, result);

        veins->type = DFHackAPIVectorTypeC_Vein;
        veins->length = result.size();
        veins->data = new t_vein[veins->length];
        for (i = 0; i < veins->length; i++)
            ( (t_vein *) veins->data) [i] = result[i];

        return retn;
    }

    DFHACKAPI uint32_t DFHackAPI_InitReadConstructions (DFHackAPIHandle self)
    {
        return self->InitReadConstructions();
    }

    DFHACKAPI bool DFHackAPI_ReadConstruction (DFHackAPIHandle self, const uint32_t *index, t_construction * construction)
    {
        return self->ReadConstruction (*index, *construction);
    }

    DFHACKAPI void DFHackAPI_FinishReadConstructions (DFHackAPIHandle self)
    {
        self->FinishReadConstructions();
    }

    DFHACKAPI uint32_t DFHackAPI_InitReadBuildings (DFHackAPIHandle self, DFHackAPIVectorC *v_buildingtypes)
    {
        vector<string> result;
        uint32_t i;
        uint32_t retn = self->InitReadBuildings (result);

        v_buildingtypes->type = DFHackAPIVectorTypeC_String;
        v_buildingtypes->length = result.size();
        v_buildingtypes->data = new char *[v_buildingtypes->length];
        for (i = 0; i < v_buildingtypes->length; i++)
        {
            char *str = new char[result[i].size() + 1];
            secure_strcpy (str, result[i].size() + 1, result[i].c_str());
            ( (char **) v_buildingtypes->data) [i] = str;
        }

        return retn;
    }

    DFHACKAPI bool DFHackAPI_ReadBuilding (DFHackAPIHandle self, const uint32_t *index, t_building * building)
    {
        return self->ReadBuilding (*index, *building);
    }

    DFHACKAPI void DFHackAPI_FinishReadBuildings (DFHackAPIHandle self)
    {
        self->FinishReadBuildings();
    }

    DFHACKAPI uint32_t DFHackAPI_InitReadVegetation (DFHackAPIHandle self)
    {
        return self->InitReadVegetation();
    }

    DFHACKAPI bool DFHackAPI_ReadVegetation (DFHackAPIHandle self, const uint32_t *index, t_tree_desc * shrubbery)
    {
        return self->ReadVegetation (*index, *shrubbery);
    }

    DFHACKAPI void DFHackAPI_FinishReadVegetation (DFHackAPIHandle self)
    {
        self->FinishReadVegetation();
    }

    DFHACKAPI uint32_t DFHackAPI_InitReadCreatures (DFHackAPIHandle self)
    {
        return self->InitReadCreatures();
    }

    DFHACKAPI bool DFHackAPI_ReadCreature (DFHackAPIHandle self, const uint32_t *index, t_creature * furball)
    {
        return self->ReadCreature (*index, *furball);
    }

    DFHACKAPI void DFHackAPI_FinishReadCreatures (DFHackAPIHandle self)
    {
        self->FinishReadCreatures();
    }

#ifdef __cplusplus
}
#endif
