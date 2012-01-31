bool isOre()
{
    return flags.is_set(df::enums::inorganic_flags::METAL_ORE) || flags.is_set(df::enums::inorganic_flags::THREAD_METAL);
}