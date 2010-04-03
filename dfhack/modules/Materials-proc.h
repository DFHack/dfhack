/*
* Matgloss. next four methods look very similar. I could use two and move the processing one level up...
* I'll keep it like this, even with the code duplication as it will hopefully get more features and separate data types later.
* Yay for nebulous plans for a rock survey tool that tracks how much of which metal could be smelted from available resorces
*/
bool ReadInorganicMaterials (std::vector<t_matgloss> & output);
bool ReadOrganicMaterials (std::vector<t_matgloss> & output);
bool ReadWoodMaterials (std::vector<t_matgloss> & output);
bool ReadPlantMaterials (std::vector<t_matgloss> & output);
bool ReadPlantMaterials (std::vector<t_matglossPlant> & output);
bool ReadCreatureTypes (std::vector<t_matgloss> & output);