/*
* Query the DF's GUI state
*/
bool InitReadGuiState();
///true if paused, false if not
bool ReadPauseState(); 
/// read the DF menu view state (stock screen, unit screen, other screens
bool ReadViewScreen(t_viewscreen &);
/// read the DF menu state (designation menu ect)
uint32_t ReadMenuState();