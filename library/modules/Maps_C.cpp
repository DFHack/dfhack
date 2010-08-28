/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf, doomchild

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

#include <vector>
#include <algorithm>

using namespace std;

#include "dfhack-c/DFTypes_C.h"
#include "dfhack-c/modules/Maps_C.h"

#ifdef __cplusplus
extern "C" {
#endif

int Maps_Start(DFHackObject* maps)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->Start();
	}
	
	return -1;
}

int Maps_Finish(DFHackObject* maps)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->Finish();
	}
	
	return -1;
}

t_feature* Maps_ReadGlobalFeatures(DFHackObject* maps)
{
	if(maps != NULL)
	{
		std::vector<t_feature> featureVec;
		
		if(((DFHack::Maps*)maps)->ReadGlobalFeatures(featureVec))
		{
			if(featureVec.size() <= 0)
				return NULL;
			
			t_feature* buf;
			
			(*alloc_t_feature_buffer_callback)(buf, featureVec.size());
			
			if(buf != NULL)
			{
				copy(featureVec.begin(), featureVec.end(), buf);
				
				return buf;
			}
			else
				return NULL;
		}
		else
			return NULL;
	}
	
	return NULL;
}

void Maps_getSize(DFHackObject* maps, uint32_t* x, uint32_t* y, uint32_t* z)
{
	if(maps != NULL)
	{
		((DFHack::Maps*)maps)->getSize(*x, *y, *z);
	}
}

int Maps_isValidBlock(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->isValidBlock(x, y, z);
	}
	
	return -1;
}

uint32_t Maps_getBlockPtr(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->getBlockPtr(x, y, z);
	}
	
	return 0;
}

int Maps_ReadBlock40d(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, mapblock40d* buffer)
{
	if(maps != NULL && buffer != NULL)
	{
		return ((DFHack::Maps*)maps)->ReadBlock40d(x, y, z, buffer);
	}
	
	return -1;
}

int Maps_ReadTileTypes(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, tiletypes40d* buffer)
{
	if(maps != NULL && buffer != NULL)
	{
		return ((DFHack::Maps*)maps)->ReadTileTypes(x, y, z, buffer);
	}
	
	return -1;
}

int Maps_WriteTileTypes(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, tiletypes40d* buffer)
{
	if(maps != NULL && buffer != NULL)
	{
		return ((DFHack::Maps*)maps)->WriteTileTypes(x, y, z, buffer);
	}
	
	return -1;
}

int Maps_ReadDesignations(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, designations40d* buffer)
{
	if(maps != NULL && buffer != NULL)
	{
		return ((DFHack::Maps*)maps)->ReadDesignations(x, y, z, buffer);
	}
	
	return -1;
}

int Maps_WriteDesignations(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, designations40d* buffer)
{
	if(maps != NULL && buffer != NULL)
	{
		return ((DFHack::Maps*)maps)->WriteDesignations(x, y, z, buffer);
	}
	
	return -1;
}

int Maps_ReadTemperatures(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, t_temperatures* temp1, t_temperatures* temp2)
{
	if(maps != NULL && temp1 != NULL && temp2 != NULL)
	{
		return ((DFHack::Maps*)maps)->ReadTemperatures(x, y, z, temp1, temp2);
	}
	
	return -1;
}

int Maps_WriteTemperatures(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, t_temperatures* temp1, t_temperatures* temp2)
{
	if(maps != NULL && temp1 != NULL && temp2 != NULL)
	{
		return ((DFHack::Maps*)maps)->WriteTemperatures(x, y, z, temp1, temp2);
	}
	
	return -1;
}

int Maps_ReadOccupancy(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, occupancies40d* buffer)
{
	if(maps != NULL && buffer != NULL)
	{
		return ((DFHack::Maps*)maps)->ReadOccupancy(x, y, z, buffer);
	}
	
	return -1;
}

int Maps_WriteOccupancy(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, occupancies40d* buffer)
{
	if(maps != NULL && buffer != NULL)
	{
		return ((DFHack::Maps*)maps)->WriteOccupancy(x, y, z, buffer);
	}
	
	return -1;
}

int Maps_ReadDirtyBit(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, int* dirtybit)
{
	if(maps != NULL)
	{
		bool bit;
		
		int result = ((DFHack::Maps*)maps)->ReadDirtyBit(x, y, z, bit);
		
		*dirtybit = bit;
		
		return result;
	}
	
	return -1;
}

int Maps_WriteDirtyBit(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, int dirtybit)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->WriteDirtyBit(x, y, z, (bool)dirtybit);
	}
	
	return -1;
}

int Maps_ReadFeatures(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, int16_t* local, int16_t* global)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->ReadFeatures(x, y, z, *local, *global);
	}
	
	return -1;
}

int Maps_WriteLocalFeature(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, int16_t local)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->WriteLocalFeature(x, y, z, local);
	}
	
	return -1;
}

int Maps_WriteEmptyLocalFeature(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->WriteLocalFeature(x, y, z, -1);
	}
}

int Maps_WriteGlobalFeature(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, int16_t local)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->WriteGlobalFeature(x, y, z, local);
	}
	
	return -1;
}

int Maps_WriteEmptyGlobalFeature(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->WriteGlobalFeature(x, y, z, -1);
	}
}

int Maps_ReadBlockFlags(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, t_blockflags* blockflags)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->ReadBlockFlags(x, y, z, *blockflags);
	}
	
	return -1;
}

int Maps_WriteBlockFlags(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, t_blockflags blockflags)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->WriteBlockFlags(x, y, z, blockflags);
	}
	
	return -1;
}

int Maps_ReadRegionOffsets(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, biome_indices40d* buffer)
{
	if(maps != NULL)
	{
		return ((DFHack::Maps*)maps)->ReadRegionOffsets(x, y, z, buffer);
	}
	
	return -1;
}

t_vein* Maps_ReadStandardVeins(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z)
{
	if(maps != NULL)
	{
		if(alloc_vein_buffer_callback == NULL)
			return NULL;
		
		vector<t_vein> veins;
		bool result = ((DFHack::Maps*)maps)->ReadVeins(x, y, z, &veins);
		
		if(result)
		{
			t_vein* v_buf = NULL;
			
			if(veins.size() > 0)
			{
				((*alloc_vein_buffer_callback)(v_buf, veins.size()));
				
				copy(veins.begin(), veins.end(), v_buf);
			}
			
			return v_buf;
		}
		else
			return NULL;
	}
	
	return NULL;
}

t_frozenliquidvein* Maps_ReadFrozenVeins(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z)
{
	if(maps != NULL)
	{
        if(alloc_frozenliquidvein_buffer_callback == NULL)
			return NULL;
		
		vector<t_vein> veins;
		vector<t_frozenliquidvein> frozen_veins;
		bool result = ((DFHack::Maps*)maps)->ReadVeins(x, y, z, &veins, &frozen_veins);
		
		if(result)
		{
			t_frozenliquidvein* fv_buf = NULL;
			
			if(frozen_veins.size() > 0)
			{
				((*alloc_frozenliquidvein_buffer_callback)(fv_buf, frozen_veins.size()));
				
				copy(frozen_veins.begin(), frozen_veins.end(), fv_buf);
			}
			
			return fv_buf;
		}
		else
			return NULL;
	}
	
	return NULL;
}

t_spattervein* Maps_ReadSpatterVeins(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z)
{
	if(maps != NULL)
	{
        if(alloc_spattervein_buffer_callback == NULL)
			return NULL;
		
		vector<t_vein> veins;
		vector<t_spattervein> spatter_veins;
		bool result = ((DFHack::Maps*)maps)->ReadVeins(x, y, z, &veins, 0, &spatter_veins);
		
		if(result)
		{
			t_spattervein* sv_buf = NULL;
			
			if(spatter_veins.size() > 0)
			{
				((*alloc_spattervein_buffer_callback)(sv_buf, spatter_veins.size()));
				
				copy(spatter_veins.begin(), spatter_veins.end(), sv_buf);
			}
			
			return sv_buf;
		}
		else
			return NULL;
	}
	
	return NULL;
}

#ifdef __cplusplus
}
#endif
