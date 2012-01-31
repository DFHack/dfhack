bool isGem()
{
    return flags.is_set(df::enums::material_flags::IS_GEM);
}
bool isStone()
{
    return flags.is_set(df::enums::material_flags::IS_STONE);
}
