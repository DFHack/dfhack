-- Increase feline population by inducing pregnancies

-- script by Putnam, configured by PeridexisErrant
-- based on the plugin by Zhentar; modified by dark_rabite, peterix, belal

--[[=begin

catsplosion
===========
Makes cats just *multiply*, by inducing a pregnancy in every cat on the map.
It is not a good idea to run this more than once or twice.

=end]]

local function getSpecies(unit)
    return df.creature_raw.find(unit.race).creature_id
end

local function getFemaleCasteWithSameMaxAge(unit)
    local curCaste=df.creature_raw.find(unit.race).caste[unit.caste]
    for k,caste in ipairs(df.creature_raw.find(unit.race).caste) do
        if caste.gender==0 and caste.misc.maxage_min==curCaste.misc.maxage_min and caste.misc.maxage_max==curCaste.misc.maxage_max then return k end
    end
end

function impregnate(unit)
    local script=require('gui.script')
    script.start(function()
    unit=unit or dfhack.gui.getSelectedUnit(true)
    if not unit then
        error("Failed to impregnate. Unit not selected/valid")
    end
    if unit.curse then
        unit.curse.add_tags2.STERILE=false
    end
    local genes = unit.appearance.genes
    if unit.relations.pregnancy_genes == nil then
        unit.relations.pregnancy_genes = unit.appearance.genes:new()
    end
    local ngenes = unit.relations.pregnancy_genes
    local rng=dfhack.random.new()
    local timer=rng:random(100)+1
    unit.relations.pregnancy_timer=timer
    unit.relations.pregnancy_caste=1
    if unit.sex==1 then
        local normal_caste=unit.enemy.normal_caste
        unit.enemy.normal_caste=getFemaleCasteWithSameMaxAge(unit)
        script.sleep(timer+1,'ticks')
        unit.enemy.normal_caste=normal_caste
    end
    end)
end

for k,v in ipairs(df.global.world.units.active) do
    if getSpecies(v)=='CAT' then
        impregnate(v)
    end
end
