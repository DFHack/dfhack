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
using namespace DFHack;



bool API::ReadWoodMaterials (vector<t_matgloss> & woods)
{
/*
    int matgloss_address = d->offset_descriptor->getAddress ("matgloss");
    int matgloss_wood_name_offset = d->offset_descriptor->getOffset("matgloss_wood_name");
    // TODO: find flag for autumnal coloring?
    DfVector p_matgloss(d->p, matgloss_address, 4);

    woods.clear();

    t_matgloss mat;
    // TODO: use brown?
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    uint32_t size = p_matgloss.getSize();
    for (uint32_t i = 0; i < size ;i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        d->p->readSTLString (temp, mat.id, 128);
        d->p->readSTLString (temp+matgloss_wood_name_offset, mat.name, 128);
        woods.push_back (mat);
    }
    */
    return true;
}

bool API::ReadInorganicMaterials (vector<t_matgloss> & inorganic)
{
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress ("mat_inorganics");
    //int matgloss_colors = minfo->getOffset ("material_color");
    //int matgloss_stone_name_offset = minfo->getOffset("matgloss_stone_name");

    DfVector p_matgloss (d->p, matgloss_address, 4);

    uint32_t size = p_matgloss.getSize();
    inorganic.resize (0);
    inorganic.reserve (size);
    for (uint32_t i = 0; i < size;i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        t_matgloss mat;
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        d->p->readSTLString (temp, mat.id, 128);
        /*
        d->p->readSTLString (temp+matgloss_stone_name_offset, mat.name, 128);
        mat.fore = (uint8_t) g_pProcess->readWord (temp + matgloss_colors);
        mat.back = (uint8_t) g_pProcess->readWord (temp + matgloss_colors + 2);
        mat.bright = (uint8_t) g_pProcess->readWord (temp + matgloss_colors + 4);
        */
        inorganic.push_back (mat);
    }
    return true;
    
}

bool API::ReadPlantMaterials (vector<t_matgloss> & plants)
{
    /*
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress ("matgloss");
    int matgloss_offset = minfo->getHexValue ("matgloss_skip");
    int matgloss_plant_name_offset = minfo->getOffset("matgloss_plant_name");
    DfVector p_matgloss(d->p, matgloss_address + matgloss_offset * 2, 4);

    plants.clear();

    // TODO: use green?
    t_matgloss mat;
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    for (uint32_t i = 0; i < p_matgloss.getSize();i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        d->p->readSTLString (temp, mat.id, 128);
        d->p->readSTLString (temp+matgloss_plant_name_offset, mat.name, 128);
        plants.push_back (mat);
    }
    */
    return true;
}

bool API::ReadPlantMaterials (vector<t_matglossPlant> & plants)
{
    /*
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress ("matgloss");
    int matgloss_offset = minfo->getHexValue ("matgloss_skip");
    int matgloss_plant_name_offset = minfo->getOffset("matgloss_plant_name");
    int matgloss_plant_drink_offset = minfo->getOffset("matgloss_plant_drink");
    int matgloss_plant_food_offset = minfo->getOffset("matgloss_plant_food");
    int matgloss_plant_extract_offset = minfo->getOffset("matgloss_plant_extract");
    DfVector p_matgloss(d->p, matgloss_address + matgloss_offset * 2, 4);

    plants.clear();

    // TODO: use green?
    t_matglossPlant mat;
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    for (uint32_t i = 0; i < p_matgloss.getSize();i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        d->p->readSTLString (temp, mat.id, 128);
        d->p->readSTLString (temp+matgloss_plant_name_offset, mat.name, 128);
        d->p->readSTLString (temp+matgloss_plant_drink_offset, mat.drink_name, 128);
        d->p->readSTLString (temp+matgloss_plant_food_offset, mat.food_name, 128);
        d->p->readSTLString (temp+matgloss_plant_extract_offset, mat.extract_name, 128);
        
        //d->p->readSTLString (temp
        plants.push_back (mat);
    }
    */
    return true;
}

bool API::ReadCreatureTypes (vector<t_matgloss> & creatures)
{
    /*
    memory_info * minfo = d->offset_descriptor;
    int matgloss_address = minfo->getAddress ("matgloss");
    int matgloss_offset = minfo->getHexValue ("matgloss_skip");
    int matgloss_creature_name_offset = minfo->getOffset("matgloss_creature_name");
    DfVector p_matgloss (d->p, matgloss_address + matgloss_offset * 6, 4);

    creatures.clear();

    // TODO: use green?
    t_matgloss mat;
    mat.fore = 7;
    mat.back = 0;
    mat.bright = 0;
    for (uint32_t i = 0; i < p_matgloss.getSize();i++)
    {
        // read the matgloss pointer from the vector into temp
        uint32_t temp = * (uint32_t *) p_matgloss[i];
        // read the string pointed at by
        //fill_char_buf(mat.id, d->p->readSTLString(temp)); // reads a C string given an address
        d->p->readSTLString (temp, mat.id, 128);
        d->p->readSTLString (temp+matgloss_creature_name_offset, mat.name, 128);
        creatures.push_back (mat);
    }
    */
    return true;
}
