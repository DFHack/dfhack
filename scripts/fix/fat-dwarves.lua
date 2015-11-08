-- Makes fat dwarves non-fat.
--
-- See for more info:
--   http://www.bay12games.com/dwarves/mantisbt/view.php?id=5971
--[[=begin

fix/fat-dwarves
===============
Avoids 5-10% FPS loss due to constant recalculation of insulation for dwarves at
maximum fatness, by reducing the cap from 1,000,000 to 999,999.
Recalculation is triggered in steps of 250 units, and very fat dwarves
constantly bounce off the maximum value while eating.

=end]]
local num_fat = 0
local num_lagging = 0

for _,v in ipairs(df.global.world.units.all) do
    local fat = v.counters2.stored_fat
    if fat > 850000 then
        v.counters2.stored_fat = 500000
        if v.race == df.global.ui.race_id then
            print(fat,dfhack.TranslateName(dfhack.units.getVisibleName(v)))
            num_fat = num_fat + 1
            if fat > 999990 then
                num_lagging = num_lagging + 1
            end
        end
    end
end

print("Fat dwarves cured: "..num_fat)
print("Lag sources: "..num_lagging)
