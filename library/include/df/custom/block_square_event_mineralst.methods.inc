inline bool getassignment( const df::coord2d &xy )
{
    return getassignment(xy.x,xy.y);
}
inline bool getassignment( int x, int y )
{
    return (tile_bitmask[y] & (1 << x));
}
inline void setassignment( const df::coord2d &xy, bool bit )
{
    return setassignment(xy.x,xy.y, bit);
}
inline void setassignment( int x, int y, bool bit )
{
    if(bit)
        tile_bitmask[y] |= (1 << x);
    else
        tile_bitmask[y] &= 0xFFFF ^ (1 << x);
}