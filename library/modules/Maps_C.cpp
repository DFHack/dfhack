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

#include "dfhack/DFPragma.h"
#include <vector>
#include <map>
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

uint16_t* Maps_ReadGeology(DFHackObject*  maps)
{
	if(maps != NULL)
	{
		std::vector < std::vector <uint16_t> > geology;
		
		if(((DFHack::Maps*)maps)->ReadGeology(geology))
		{
			uint16_t** buf = NULL;
			uint32_t geoLength = 0;
			
			for(unsigned int i = 0; i < geology.size(); i++)
			{
				for(unsigned int j = 0; j < geology[i].size(); j++)
				{
					geoLength += geology[i].size();
				}
			}
			
			((*alloc_ushort_buffer_callback)(buf, geoLength));
			
			if(buf != NULL)
			{
				uint16_t* bufCopyPtr = *buf;
				
				for(unsigned int i = 0; i < geology.size(); i++)
				{
					copy(geology[i].begin(), geology[i].end(), bufCopyPtr);
					
					bufCopyPtr += geology[i].size();
				}
				
				return *buf;
			}
			else
				return NULL;
		}
	}
	
	return NULL;
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
			
			t_feature** buf = NULL;
			
			((*alloc_feature_buffer_callback)(buf, featureVec.size()));
			
			if(buf != NULL)
			{
				copy(featureVec.begin(), featureVec.end(), *buf);
				
				return *buf;
			}
			else
				return NULL;
		}
		else
			return NULL;
	}
	
	return NULL;
}

c_featuremap_node* Maps_ReadLocalFeatures(DFHackObject* maps)
{
	if(maps != NULL)
	{
		std::map <DFCoord, std::vector<t_feature *> > local_features;
		std::map <DFCoord, std::vector<t_feature *> >::iterator iterate;
		uint32_t i;
		
		if(((DFHack::Maps*)maps)->ReadLocalFeatures(local_features))
		{
			if(local_features.empty() == true)
				return NULL;
			
			c_featuremap_node* featuremap;	
			uint32_t* featuremap_size = (uint32_t*)calloc(local_features.size(), sizeof(uint32_t));
			
			for(i = 0, iterate = local_features.begin(); iterate != local_features.end(); i++, iterate++)
				featuremap_size[i] = (*iterate).second.size();
			
			((*alloc_featuremap_buffer_callback)(&featuremap, featuremap_size, local_features.size()));
			
			free(featuremap_size);
			
			if(featuremap == NULL)
				return NULL;
			
			for(i = 0, iterate = local_features.begin(); iterate != local_features.end(); i++, iterate++)
			{
				uint32_t j;
				
				featuremap[i].coordinate.comparate = (*iterate).first.comparate;
				
				for(j = 0; j < (*iterate).second.size(); j++)
					featuremap[i].features[j] = *((*iterate).second[j]);
					
				//copy((*iterate).second.begin(), (*iterate).second.end(), featuremap[i].features);
				featuremap[i].feature_length = (*iterate).second.size();
			}
			
			return featuremap;
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

  return -1;
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

  return -1;
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
				((*alloc_vein_buffer_callback)(&v_buf, veins.size()));
				
				if(v_buf == NULL)
					return NULL;
				
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
				((*alloc_frozenliquidvein_buffer_callback)(&fv_buf, frozen_veins.size()));
				
				if(fv_buf == NULL)
					return NULL;
				
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
				((*alloc_spattervein_buffer_callback)(&sv_buf, spatter_veins.size()));
				
				if(sv_buf == NULL)
					return NULL;
				
				copy(spatter_veins.begin(), spatter_veins.end(), sv_buf);
			}
			
			return sv_buf;
		}
		else
			return NULL;
	}
	
	return NULL;
}

t_grassvein* Maps_ReadGrassVeins(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z)
{
	if(maps != NULL)
	{
        if(alloc_grassvein_buffer_callback == NULL)
			return NULL;
		
		vector<t_vein> veins;
		vector<t_grassvein> grass_veins;
		bool result = ((DFHack::Maps*)maps)->ReadVeins(x, y, z, &veins, 0, 0, &grass_veins);
		
		if(result)
		{
			t_grassvein* gs_buf = NULL;
			
			if(grass_veins.size() > 0)
			{
				((*alloc_grassvein_buffer_callback)(&gs_buf, grass_veins.size()));
				
				if(gs_buf == NULL)
					return NULL;
				
				copy(grass_veins.begin(), grass_veins.end(), gs_buf);
			}
			
			return gs_buf;
		}
		else
			return NULL;
	}
	
	return NULL;
}

t_worldconstruction* Maps_ReadWorldConstructions(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z)
{
	if(maps != NULL)
	{
		if(alloc_worldconstruction_buffer_callback == NULL)
			return NULL;
		
		vector<t_vein> veins;
		vector<t_worldconstruction> constructions;
		bool result = ((DFHack::Maps*)maps)->ReadVeins(x, y, z, &veins, 0, 0, 0, &constructions);
		
		if(result)
		{
			t_worldconstruction* ct_buf = NULL;
			
			if(constructions.size() > 0)
			{
				((*alloc_worldconstruction_buffer_callback)(&ct_buf, constructions.size()));
				
				if(ct_buf == NULL)
					return NULL;
				
				copy(constructions.begin(), constructions.end(), ct_buf);
			}
			
			return ct_buf;
		}
		else
			return NULL;
	}
	
	return NULL;
}

int Maps_ReadAllVeins(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z, c_allveins* vein_struct)
{
	if(maps != NULL)
	{
		if(alloc_vein_buffer_callback == NULL || alloc_frozenliquidvein_buffer_callback == NULL ||
			alloc_spattervein_buffer_callback == NULL || alloc_grassvein_buffer_callback == NULL ||
			alloc_worldconstruction_buffer_callback == NULL)
			return -1;
		
		vector<t_vein> veins;
		vector<t_frozenliquidvein> frozen_veins;
		vector<t_spattervein> spatter_veins;
		vector<t_grassvein> grass_veins;
		vector<t_worldconstruction> constructions;
		
		bool result = ((DFHack::Maps*)maps)->ReadVeins(x, y, z, &veins, &frozen_veins, &spatter_veins, &grass_veins, &constructions);
		
		if(result)
		{
			t_vein* v_buf = NULL;
			t_frozenliquidvein* fv_buf = NULL;
			t_spattervein* sv_buf = NULL;
			t_grassvein* gs_buf = NULL;
			t_worldconstruction* ct_buf = NULL;
			
			if(veins.size() > 0)
			{
				((*alloc_vein_buffer_callback)(&v_buf, veins.size()));
				
				if(v_buf == NULL)
					return -1;
				
				copy(veins.begin(), veins.end(), v_buf);
				
				vein_struct->veins = v_buf;
			}
			
			if(frozen_veins.size() > 0)
			{
				((*alloc_frozenliquidvein_buffer_callback)(&fv_buf, frozen_veins.size()));
				
				if(fv_buf == NULL)
					return -1;
				
				copy(frozen_veins.begin(), frozen_veins.end(), fv_buf);
				
				vein_struct->frozen_veins = fv_buf;
			}
			
			if(spatter_veins.size() > 0)
			{
				((*alloc_spattervein_buffer_callback)(&sv_buf, spatter_veins.size()));
				
				if(sv_buf == NULL)
					return -1;
				
				copy(spatter_veins.begin(), spatter_veins.end(), sv_buf);
				
				vein_struct->spatter_veins = sv_buf;
			}
			
			if(grass_veins.size() > 0)
			{
				((*alloc_grassvein_buffer_callback)(&gs_buf, grass_veins.size()));
				
				if(gs_buf == NULL)
					return -1;
				
				copy(grass_veins.begin(), grass_veins.end(), gs_buf);
				
				vein_struct->grass_veins = gs_buf;
			}
			
			if(constructions.size() > 0)
			{
				((*alloc_worldconstruction_buffer_callback)(&ct_buf, constructions.size()));
				
				if(ct_buf == NULL)
					return -1;
				
				copy(constructions.begin(), constructions.end(), ct_buf);
				
				vein_struct->world_constructions = ct_buf;
			}
			
			return 1;
		}
		else
			return 0;
	}
	
	return -1;
}

t_tree* Maps_ReadVegetation(DFHackObject* maps, uint32_t x, uint32_t y, uint32_t z)
{
	if(maps == NULL)
		return NULL;
	else
	{
		std::vector<t_tree> plants;
		bool result = ((DFHack::Maps*)maps)->ReadVegetation(x, y, z, &plants);
		t_tree* buf = NULL;
		
		if(!result || plants.size() <= 0)
			return NULL;
		else
		{
			((*alloc_tree_buffer_callback)(&buf, plants.size()));
			
			if(buf == NULL)
				return NULL;
			
			copy(plants.begin(), plants.end(), buf);
			
			return buf;
		}
	}
	
	return NULL;
}

#ifdef __cplusplus
}
#endif
