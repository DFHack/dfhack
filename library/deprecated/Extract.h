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

/* Memory research
    http://dwarffortresswiki.net/index.php/User:Rick/memory.ini#A_table_of_available_settings
    http://dwarffortresswiki.net/index.php/User:Rick/Memory_research#Tile_Block
    http://dwarffortresswiki.net/index.php/User:AzureLightning/Memory_research
    http://dwarffortresswiki.net/index.php/User:Iluxan/Memory_research
 */
#ifndef EXTRACT_HEADER
#define EXTRACT_HEADER

class DfMap;

class Extractor
{
protected:
    DfMap *df_map;   // DF extracted map structure
    
public:
    bool Init();
    Extractor();
    ~Extractor();
    bool loadMap(string FileName);
    bool writeMap(string FileName);
    bool isMapLoaded();
    DfMap *getMap() {return df_map;};
    bool dumpMemory( string path_to_xml);
};
#endif // EXTRACT_HEADER
