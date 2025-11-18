auto operator== (const texture_fullid &other) const {
    return (
        this->texpos == other.texpos &&
        this->r == other.r &&
        this->g == other.g &&
        this->b == other.b &&
        this->br == other.br &&
        this->bg == other.bg &&
        this->bb == other.bb &&
        this->flag.whole == other.flag.whole
        );
}

auto operator< (const texture_fullid &other) const {
    if (this->texpos < other.texpos) return true;
    if (this->r < other.r) return true;
    if (this->g < other.g) return true;
    if (this->b < other.b) return true;
    if (this->br < other.br) return true;
    if (this->bg < other.bg) return true;
    if (this->bb < other.bb) return true;
    if (this->flag.whole < other.flag.whole) return true;
    return false;
}
