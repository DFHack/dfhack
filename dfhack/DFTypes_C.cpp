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

#include "integers.h"
#include <stdlib.h>
#include "string.h"
#include <vector>
#include <algorithm>

using namespace std;

#include "DFCommonInternal.h"
#include "DFTypes.h"
#include "modules/Materials.h"
#include "DFTypes_C.h"

using namespace DFHack;


/*
#ifdef __cplusplus
extern "C" {
#endif

c_colormodifier* ColorModifier_New()
{
	c_colormodifier* temp;
	
	temp = (c_colormodifier*)malloc(sizeof(c_colormodifier));
	
	if(temp == NULL)
		return NULL;
	
	temp->part[0] = '\0';
	temp->colorlist = NULL;
	temp->colorlistLength = 0;
	
	return temp;
}

void ColorModifier_Free(c_colormodifier* src)
{
	if(src != NULL)
	{
		if(src->colorlist != NULL)
			free(src->colorlist);
		
		free(src);
	}
}

c_creaturecaste* CreatureCaste_New()
{
	c_creaturecaste* temp;
	
	temp = (c_creaturecaste*)malloc(sizeof(c_creaturecaste));
	
	if(temp == NULL)
		return NULL;
	
	temp->rawname[0] = '\0';
	temp->singular[0] = '\0';
	temp->plural[0] = '\0';
	temp->adjective[0] = '\0';
	
	temp->ColorModifier = NULL;
	temp->colorModifierLength = 0;
	
	temp->bodypart = NULL;
	temp->bodypartLength = 0;
	
	return temp;
}

void CreatureCaste_Free(c_creaturecaste* src)
{
	if(src != NULL)
	{
		if(src->bodypart != NULL)
			free(src->bodypart);
		
		if(src->ColorModifier != NULL)
		{
			for(int i = 0; i < src->colorModifierLength; i++)
				ColorModifier_Free(&src->ColorModifier[i]);
			
			free(src->ColorModifier);
		}
		
		free(src);
	}
}

c_creaturetype* CreatureType_New()
{
	c_creaturetype* temp;
	
	temp = (c_creaturetype*)malloc(sizeof(c_creaturetype));
	
	if(temp == NULL)
		return NULL;
	
	temp->rawname[0] = '\0';
	
	temp->castes = NULL;
	temp->castesCount = 0;
	
	temp->extract = NULL;
	temp->extractCount = 0;
	
	temp->tile_character = 0;
	
	temp->tilecolor.fore = 0;
	temp->tilecolor.back = 0;
	temp->tilecolor.bright = 0;
	
	return temp;
}

void CreatureType_Free(c_creaturetype* src)
{
	if(src != NULL)
	{
		if(src->castes != NULL)
		{
			for(int i = 0; i < src->castesCount; i++)
				CreatureCaste_Free(&src->castes[i]);
			
			free(src->castes);
		}
		
		if(src->extract != NULL)
			free(src->extract);
		
		free(src);
	}
}

#ifdef __cplusplus
}
#endif

*/

int ColorListConvert(t_colormodifier* src, c_colormodifier* dest)
{
	if(src == NULL)
		return -1;
	
	dest = ((*alloc_colormodifier_callback)(src->part, src->colorlist.size()));
	
	copy(src->colorlist.begin(), src->colorlist.end(), dest->colorlist);
	
	return 1;
}

int CreatureCasteConvert(t_creaturecaste* src, c_creaturecaste* dest)
{
	if(src == NULL)
		return -1;
	
	dest = ((*alloc_creaturecaste_callback)(src->rawname, src->singular, src->plural, src->adjective, src->ColorModifier.size(), src->bodypart.size()));
	
	for(int i = 0; i < dest->colorModifierLength; i++)
		ColorListConvert(&src->ColorModifier[i], &dest->ColorModifier[i]);
	
	copy(src->bodypart.begin(), src->bodypart.end(), dest->bodypart);
	
	return 1;
}

int CreatureTypeConvert(t_creaturetype* src, c_creaturetype* dest)
{
	if(src == NULL)
		return -1;
	
	dest = ((*alloc_creaturetype_callback)(src->rawname, src->castes.size(), src->extract.size(), src->tile_character, src->tilecolor.fore, src->tilecolor.back, src->tilecolor.bright));
	
	for(int i = 0; i < dest->castesCount; i++)
		CreatureCasteConvert(&src->castes[i], &dest->castes[i]);
	
	copy(src->extract.begin(), src->extract.end(), dest->extract);
	
	return 1;
}
