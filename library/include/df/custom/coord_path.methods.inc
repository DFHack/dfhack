unsigned size() const { return x.size(); }

coord operator[] (unsigned idx) const {
    if (idx >= x.size() || idx >= y.size() || idx >= z.size())
        return coord();
    else
        return coord(x[idx], y[idx], z[idx]);
}
