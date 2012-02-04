coord(const coord2d &c, uint16_t _z) : x(c.x), y(c.y), z(_z) {}
coord(uint16_t _x, uint16_t _y, uint16_t _z) : x(_x), y(_y), z(_z) {}

operator coord2d() const { return coord2d(x,y); }

bool isValid() const { return x != -30000; }
void clear() { x = y = z = -30000; }

bool operator==(const coord &other) const
{
    return (x == other.x) && (y == other.y) && (z == other.z);
}
bool operator!=(const coord &other) const
{
    return (x != other.x) || (y != other.y) || (z != other.z);
}

bool operator<(const coord &other) const
{
    if (x != other.x) return (x < other.x);
    if (y != other.y) return (y < other.y);
    return z < other.z;
}

coord operator+(const coord &other) const
{
    return coord(x + other.x, y + other.y, z + other.z);
}
coord operator-(const coord &other) const
{
    return coord(x - other.x, y - other.y, z - other.z);
}

coord operator/(int number) const
{
    return coord(x/number, y/number, z);
}
coord operator*(int number) const
{
    return coord(x*number, y*number, z);
}
coord operator%(int number) const
{
    return coord(x%number, y%number, z);
}

coord operator-(int number) const
{
    return coord(x,y,z-number);
}
coord operator+(int number) const
{
    return coord(x,y,z+number);
}
