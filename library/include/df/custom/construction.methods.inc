std::size_t operator()() const {
    size_t r = 17;
    const size_t m = 65537;
    r = m*(r+pos.x);
    r = m*(r+pos.y);
    r = m*(r+pos.z);
    return r;
}

bool operator==(const construction &other) const {
    return pos == other.pos;
}
