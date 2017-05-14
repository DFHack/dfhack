df::enums::dfhack_knowledge_scholar_flag::dfhack_knowledge_scholar_flag value() const
{
    int32_t value = category * 32;
    for (int32_t i = 0; i < 32; i++)
    {
        if (flags.whole & (1 << i))
        {
            value += i;
            break;
        }
    }
    return df::enums::dfhack_knowledge_scholar_flag::dfhack_knowledge_scholar_flag(value);
}
