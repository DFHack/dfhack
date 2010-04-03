/*
* Cursor and window coords
*/
bool InitViewAndCursor();
bool getViewCoords (int32_t &x, int32_t &y, int32_t &z);
bool setViewCoords (const int32_t x, const int32_t y, const int32_t z);

bool getCursorCoords (int32_t &x, int32_t &y, int32_t &z);
bool setCursorCoords (const int32_t x, const int32_t y, const int32_t z);

/*
* Window size in tiles
*/
bool InitViewSize();
bool getWindowSize(int32_t & width, int32_t & height);
/// get the creature vector index of the creature currently under DF' cursor
bool getCurrentCursorCreature (uint32_t & creature_index);
