-- Maximizes adventurer skills/attributes on creation
--[====[

adv-max-skills
==============
When creating an adventurer, raises all changeable skills and
attributes to their maximum level.

]====]
if dfhack.gui.getCurFocus() ~= 'setupadventure' then
    qerror('Must be called on adventure mode setup screen')
end

local adv = dfhack.gui.getCurViewscreen().party_members[0] --hint:df.viewscreen_setupadventurest
for k in pairs(adv.skills) do
    adv.skills[k] = df.skill_rating.Legendary5
end
for k in pairs(adv.physical_levels) do
    adv.physical_levels[k] = df.adventurer_attribute_level.Superior
end
for k in pairs(adv.mental_levels) do
    adv.mental_levels[k] = df.adventurer_attribute_level.Superior
end
