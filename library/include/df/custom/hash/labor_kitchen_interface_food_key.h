template<>
struct std::hash<labor_kitchen_interface_food_key> {
    auto operator()(const labor_kitchen_interface_food_key &a) const -> size_t {
        struct thing {
            int16_t t;
            int16_t st;
            int32_t x;
        } thing{
            .t = a.type;
            .st = a.subtype,
            .x = static_cast<int32_t>(a.mat) ^ (static_cast<int32_t>(a.matg))
        };

        return hash<int64_t>()(bit_cast<int64_t>(thing));
        }
    };
