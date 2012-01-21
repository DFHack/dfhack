bool isOre()
{
    return flags.is_set(df::inorganic_flags::METAL_ORE) || flags.is_set(df::inorganic_flags::THREAD_METAL);
}