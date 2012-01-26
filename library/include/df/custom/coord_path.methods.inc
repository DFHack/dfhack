unsigned size() const { return x.size(); }

coord operator[] (unsigned idx) const {
    if (idx >= x.size())
        return coord();
    else
        return coord(x[idx], y[idx], z[idx]);
}

void erase(unsigned idx) {
    if (idx < x.size()) {
        x.erase(x.begin()+idx);
        y.erase(y.begin()+idx);
        z.erase(z.begin()+idx);
    }
}

void push_back(const coord &crd) {
    x.push_back(crd.x);
    y.push_back(crd.y);
    z.push_back(crd.z);
}
