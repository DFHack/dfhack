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

#include "DFCommonInternal.h"
#include "../private/APIPrivate.h"
#include "modules/Materials.h"
#include "DFMemInfo.h"
#include "DFProcess.h"
#include "DFVector.h"

using namespace DFHack;

Materials::Materials(APIPrivate * d_)
{
    d = d_;
}
Materials::~Materials(){}
/*
    {
LABEL_53:
      if ( a1
        || (signed int)a2 < 0
        || a2 >= (inorg_end - inorg_start) >> 2
        || (v13 = *(_DWORD *)(inorg_start + 4 * a2), !v13) )
      {
        switch ( a1 )
        {
          case 1:
            sub_40FDD0("AMBER");
            break;
          case 2:
            sub_40FDD0("CORAL");
            break;
          case 3:
            sub_40FDD0("GLASS_GREEN");
            break;
          case 4:
            sub_40FDD0("GLASS_CLEAR");
            break;
          case 5:
            sub_40FDD0("GLASS_CRYSTAL");
            break;
          case 6:
            sub_40FDD0("WATER");
            break;
          case 7:
            sub_40FDD0("COAL");
            break;
          case 8:
            sub_40FDD0("POTASH");
            break;
          case 9:
            sub_40FDD0("ASH");
            break;
          case 10:
            sub_40FDD0("PEARLASH");
            break;
          case 11:
            sub_40FDD0("LYE");
            break;
          case 12:
            sub_40FDD0("MUD");
            break;
          case 13:
            sub_40FDD0("VOMIT");
            break;
          case 14:
            sub_40FDD0("SALT");
            break;
          case 15:
            sub_40FDD0("FILTH_B");
            break;
          case 16:
            sub_40FDD0("FILTH_Y");
            break;
          case 17:
            sub_40FDD0("UNKNOWN_SUBSTANCE");
            break;
          case 18:
            sub_40FDD0("GRIME");
            break;
          default:
            sub_40A070("NONE", 4u);
            break;
        }
        result = sub_40A070("NONE", 4u);
        if ( a1 == 7 )
        {
          result = a2;
          if ( a2 )
          {
            if ( a2 == 1 )
              result = sub_40A070("CHARCOAL", 8u);
          }
          else
          {
            result = sub_40A070("COKE", 4u);
          }
        }
      }
      else
      {
        sub_40A070("INORGANIC", 9u);
        result = sub_409CA0(v13, 0, -1);
      }
    }

*/

/*
bool API::ReadInorganicMaterials (vector<t_matgloss> & inorganic)
{
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress ("mat_inorganics");
    int matgloss_colors = minfo->getOffset ("material_color");
    int matgloss_stone_name_offset = minfo->getOffset("matgloss_stone_name");

    DfVector <uint32_t> p_matgloss (d->p, matgloss_address);

    uint32_t size = p_matgloss.getSize();
    inorganic.resize (0);
    inorganic.reserve (size);
    for (uint32_t i = 0; i < size;i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = p_matgloss[i];
        // read the string pointed at by
        t_matgloss mat;
        //cout << temp << endl;
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        d->p->readSTLString (temp, mat.id, 128);
        
        d->p->readSTLString (temp+matgloss_stone_name_offset, mat.name, 128);
        mat.fore = (uint8_t) g_pProcess->readWord (temp + matgloss_colors);
        mat.back = (uint8_t) g_pProcess->readWord (temp + matgloss_colors + 2);
        mat.bright = (uint8_t) g_pProcess->readWord (temp + matgloss_colors + 4);
        
        inorganic.push_back (mat);
    }
    return true;
}
*/



// good for now
inline bool ReadNamesOnly(Process* p, uint32_t address, vector<t_matgloss> & names)
{
    DfVector <uint32_t> p_matgloss (p, address);
    uint32_t size = p_matgloss.size();
    names.clear();
    names.reserve (size);
    for (uint32_t i = 0; i < size;i++)
    {
        t_matgloss mat;
        p->readSTLString (p_matgloss[i], mat.id, 128);
        names.push_back(mat);
    }
    return true;
}

bool Materials::ReadInorganicMaterials (vector<t_matgloss> & inorganic)
{
    return ReadNamesOnly(d->p, d->offset_descriptor->getAddress ("mat_inorganics"), inorganic );
}

bool Materials::ReadOrganicMaterials (vector<t_matgloss> & organic)
{
    return ReadNamesOnly(d->p, d->offset_descriptor->getAddress ("mat_organics_all"), organic );
}

bool Materials::ReadWoodMaterials (vector<t_matgloss> & trees)
{
    return ReadNamesOnly(d->p, d->offset_descriptor->getAddress ("mat_organics_trees"), trees );
}

bool Materials::ReadPlantMaterials (vector<t_matgloss> & plants)
{
    return ReadNamesOnly(d->p, d->offset_descriptor->getAddress ("mat_organics_plants"), plants );
}
/*
Gives bad results combined with the creature race field!
bool Materials::ReadCreatureTypes (vector<t_matgloss> & creatures)
{
    return ReadNamesOnly(d->p, d->offset_descriptor->getAddress ("mat_creature_types"), creatures );
    return true;
}
*/
bool Materials::ReadCreatureTypes (vector<t_matgloss> & creatures)
{
    return ReadNamesOnly(d->p, d->offset_descriptor->getAddress ("creature_type_vector"), creatures );
    return true;
}

bool Materials::ReadCreatureTypesEx (vector<t_creaturetype> & creatures)
{
    DfVector <uint32_t> p_races (g_pProcess, d->offset_descriptor->getAddress ("creature_type_vector"));
    uint32_t castes_vector_offset = d->offset_descriptor->getOffset ("creature_type_caste_vector");
    uint32_t sizeof_string = d->offset_descriptor->getHexValue ("sizeof_string");
    uint32_t size = p_races.size();
    uint32_t sizecas = 0;
    creatures.clear();
    creatures.reserve (size);
    for (uint32_t i = 0; i < size;i++)
    {
        t_creaturetype mat;
        g_pProcess->readSTLString (p_races[i], mat.rawname, sizeof(mat.rawname));
        DfVector <uint32_t> p_castes(g_pProcess,p_races[i] + castes_vector_offset);
        sizecas = p_castes.size();
        for (uint32_t j = 0; j < sizecas;j++)
        {
            t_creaturecaste caste;
            uint32_t caste_start = p_castes[j];
            g_pProcess->readSTLString (caste_start, caste.rawname, sizeof(caste.rawname));
            g_pProcess->readSTLString (caste_start + sizeof_string, caste.singular, sizeof(caste.singular));
            g_pProcess->readSTLString (caste_start + 2 * sizeof_string, caste.plural, sizeof(caste.plural));
            g_pProcess->readSTLString (caste_start + 3 * sizeof_string, caste.adjective, sizeof(caste.adjective));
            mat.castes.push_back(caste);
        }
        creatures.push_back(mat);
    }
    return true;
}

