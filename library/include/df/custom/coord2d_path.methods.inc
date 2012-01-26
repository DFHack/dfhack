unsigned size() const { return x.size(); }

coord2d operator[] (unsigned idx) const {
    if (idx >= x.size())
        return coord2d();
    else
        return coord2d(x[idx], y[idx]);
}

void erase(unsigned idx) {
    if (idx < x.size()) {
        x.erase(x.begin()+idx);
        y.erase(y.begin()+idx);
    }
}

void push_back(const coord2d &crd) {
    x.push_back(crd.x);
    y.push_back(crd.y);
}
