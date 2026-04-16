coord2d(int16_t _x, int16_t _y) : x(_x), y(_y) {}

bool isValid() const { return x >= 0; }
void clear() { x = y = -30000; }

bool operator==(const coord2d &other) const
{
    return (x == other.x) && (y == other.y);
}
bool operator!=(const coord2d &other) const
{
    return (x != other.x) || (y != other.y);
}

bool operator<(const coord2d &other) const
{
    if (x != other.x) return (x < other.x);
    return y < other.y;
}

coord2d operator+(const coord2d &other) const
{
    return coord2d(x + other.x, y + other.y);
}
coord2d operator-(const coord2d &other) const
{
    return coord2d(x - other.x, y - other.y);
}

coord2d operator/(int number) const
{
    return coord2d((x < 0 ? x - number : x)/number, (y < 0 ? y - number : y)/number);
}
coord2d operator*(int number) const
{
    return coord2d(x*number, y*number);
}
coord2d operator%(int number) const
{
    return coord2d((x+number)%number, (y+number)%number);
}
coord2d operator&(int number) const
{
    return coord2d(x&number, y&number);
}
