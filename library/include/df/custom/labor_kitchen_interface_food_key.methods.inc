    bool operator<(const df::labor_kitchen_interface_food_key &b) const {
        if (type<b.type) return true;
        if (subtype<b.subtype) return true;
        if (mat<b.mat) return true;
        if (matg<b.matg) return true;
        return false;
        }

    bool operator==(const df::labor_kitchen_interface_food_key &b) const {
        return type==b.type&&subtype==b.subtype&&mat==b.mat&&matg==b.matg;
        }
