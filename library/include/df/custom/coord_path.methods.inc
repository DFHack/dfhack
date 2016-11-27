bool empty() const { return x.empty(); }
size_t size() const { return x.size(); }

void clear() {
    x.clear();
    y.clear();
    z.clear();
}

coord operator[] (size_t idx) const {
    if (idx >= x.size())
        return coord();
    else
        return coord(x[idx], y[idx], z[idx]);
}

void erase(size_t idx) {
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
