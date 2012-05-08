inline bool getassignment( const df::coord2d &xy )
{
    return tile_bitmask.getassignment(xy);
}
inline bool getassignment( int x, int y )
{
    return tile_bitmask.getassignment(x,y);
}
inline void setassignment( const df::coord2d &xy, bool bit )
{
    return tile_bitmask.setassignment(xy, bit);
}
inline void setassignment( int x, int y, bool bit )
{
    return tile_bitmask.setassignment(x, y, bit);
}
bool has_assignments()
{
    return tile_bitmask.has_assignments();
}

