-- Allows setting useful preferences for forcing moods
-- based on pref_adjust by vjek, version 4, 20141016, for DF(hack) 40.08, with the getselectedunit portion from a Putnam script (and the dfhack lua API but he's still a wizard)
-- turned into this hacked together version by some jerk named Max™ on the 8th of March 2015
-- updated with print all and clear all functions on the 11th of April 2015
-- Praise Armok!
-- ---------------------------------------------------------------------------
function axeplate(unit)
local unit = dfhack.gui.getSelectedUnit()

	if unit==nil then
		print ("No unit available!  Aborting with extreme prejudice.")
		return
	end

local pss_counter=31415926

local prefcount = #(unit.status.current_soul.preferences)
print ("Before, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

utils = require 'utils'

-- axes and breastplates
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.WEAPON , creature_id = df.item_type.WEAPON , color_id = df.item_type.WEAPON , shape_id = df.item_type.WEAPON , plant_id = df.item_type.WEAPON , item_subtype = 1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.ARMOR , creature_id = df.item_type.ARMOR , color_id = df.item_type.ARMOR , shape_id = df.item_type.ARMOR , plant_id = df.item_type.ARMOR , item_subtype = 0 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
-- likes steel (8)
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("STEEL").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')

prefcount = #(unit.status.current_soul.preferences)
print ("After, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

end
-- ---------------------------------------------------------------------------
function hammershirt(unit)
local unit = dfhack.gui.getSelectedUnit()

	if unit==nil then
		print ("No unit available!  Aborting with extreme prejudice.")
		return
	end

local pss_counter=31415926

local prefcount = #(unit.status.current_soul.preferences)
print ("Before, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

utils = require 'utils'

-- hammers and mail shirts
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.WEAPON , creature_id = df.item_type.WEAPON , color_id = df.item_type.WEAPON , shape_id = df.item_type.WEAPON , plant_id = df.item_type.WEAPON , item_subtype = 2 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.ARMOR , creature_id = df.item_type.ARMOR , color_id = df.item_type.ARMOR , shape_id = df.item_type.ARMOR , plant_id = df.item_type.ARMOR , item_subtype = 1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
-- likes steel (8)
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("STEEL").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')

prefcount = #(unit.status.current_soul.preferences)
print ("After, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

end
-- ---------------------------------------------------------------------------
function swordboot(unit)
local unit = dfhack.gui.getSelectedUnit()

	if unit==nil then
		print ("No unit available!  Aborting with extreme prejudice.")
		return
	end

local pss_counter=31415926

local prefcount = #(unit.status.current_soul.preferences)
print ("Before, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

utils = require 'utils'

-- short swords and high boots
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.WEAPON , creature_id = df.item_type.WEAPON , color_id = df.item_type.WEAPON , shape_id = df.item_type.WEAPON , plant_id = df.item_type.WEAPON , item_subtype = 3 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.SHOES , creature_id = df.item_type.SHOES , color_id = df.item_type.SHOES , shape_id = df.item_type.SHOES , plant_id = df.item_type.SHOES , item_subtype = 1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
-- likes steel (8)
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("STEEL").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')

prefcount = #(unit.status.current_soul.preferences)
print ("After, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

end
-- ---------------------------------------------------------------------------
function spearboot(unit)
local unit = dfhack.gui.getSelectedUnit()

	if unit==nil then
		print ("No unit available!  Aborting with extreme prejudice.")
		return
	end

local pss_counter=31415926

local prefcount = #(unit.status.current_soul.preferences)
print ("Before, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

utils = require 'utils'

-- spears and high boots
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.WEAPON , creature_id = df.item_type.WEAPON , color_id = df.item_type.WEAPON , shape_id = df.item_type.WEAPON , plant_id = df.item_type.WEAPON , item_subtype = 4 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.SHOES , creature_id = df.item_type.SHOES , color_id = df.item_type.SHOES , shape_id = df.item_type.SHOES , plant_id = df.item_type.SHOES , item_subtype = 1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
-- likes steel (8)
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("STEEL").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')

prefcount = #(unit.status.current_soul.preferences)
print ("After, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

end
-- ---------------------------------------------------------------------------
function maceshield(unit)
local unit = dfhack.gui.getSelectedUnit()

	if unit==nil then
		print ("No unit available!  Aborting with extreme prejudice.")
		return
	end

local pss_counter=31415926

local prefcount = #(unit.status.current_soul.preferences)
print ("Before, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

utils = require 'utils'

-- maces and shields
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.WEAPON , creature_id = df.item_type.WEAPON , color_id = df.item_type.WEAPON , shape_id = df.item_type.WEAPON , plant_id = df.item_type.WEAPON , item_subtype = 5 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.SHIELD , creature_id = df.item_type.SHIELD , color_id = df.item_type.SHIELD , shape_id = df.item_type.SHIELD , plant_id = df.item_type.SHIELD , item_subtype = 0 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
-- likes steel (8)
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("STEEL").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')

prefcount = #(unit.status.current_soul.preferences)
print ("After, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

end
-- ---------------------------------------------------------------------------
function xbowhelm(unit)
local unit = dfhack.gui.getSelectedUnit()

	if unit==nil then
		print ("No unit available!  Aborting with extreme prejudice.")
		return
	end

local pss_counter=31415926

local prefcount = #(unit.status.current_soul.preferences)
print ("Before, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

utils = require 'utils'

-- crossbows and helms
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.WEAPON , creature_id = df.item_type.WEAPON , color_id = df.item_type.WEAPON , shape_id = df.item_type.WEAPON , plant_id = df.item_type.WEAPON , item_subtype = 6 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.HELM , creature_id = df.item_type.HELM , color_id = df.item_type.HELM , shape_id = df.item_type.HELM , plant_id = df.item_type.HELM , item_subtype = 0 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
-- likes steel (8)
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("STEEL").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')

prefcount = #(unit.status.current_soul.preferences)
print ("After, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

end
-- ---------------------------------------------------------------------------
function pickglove(unit)
local unit = dfhack.gui.getSelectedUnit()

	if unit==nil then
		print ("No unit available!  Aborting with extreme prejudice.")
		return
	end

local pss_counter=31415926

local prefcount = #(unit.status.current_soul.preferences)
print ("Before, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

utils = require 'utils'

-- picks and gauntlets
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.WEAPON , creature_id = df.item_type.WEAPON , color_id = df.item_type.WEAPON , shape_id = df.item_type.WEAPON , plant_id = df.item_type.WEAPON , item_subtype = 7 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.GLOVES  , creature_id = df.item_type.GLOVES  , color_id = df.item_type.GLOVES  , shape_id = df.item_type.GLOVES  , plant_id = df.item_type.GLOVES  , item_subtype = 0 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
-- likes steel (8)
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("STEEL").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')

prefcount = #(unit.status.current_soul.preferences)
print ("After, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

end
-- ---------------------------------------------------------------------------
function longglove(unit)
local unit = dfhack.gui.getSelectedUnit()

	if unit==nil then
		print ("No unit available!  Aborting with extreme prejudice.")
		return
	end

local pss_counter=31415926

local prefcount = #(unit.status.current_soul.preferences)
print ("Before, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

utils = require 'utils'

-- long swords and gauntlets, skipped bows and whatnot
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.WEAPON , creature_id = df.item_type.WEAPON , color_id = df.item_type.WEAPON , shape_id = df.item_type.WEAPON , plant_id = df.item_type.WEAPON , item_subtype = 13 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.GLOVES  , creature_id = df.item_type.GLOVES  , color_id = df.item_type.GLOVES  , shape_id = df.item_type.GLOVES  , plant_id = df.item_type.GLOVES  , item_subtype = 0 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
-- likes steel (8)
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("STEEL").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')

prefcount = #(unit.status.current_soul.preferences)
print ("After, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

end
-- ---------------------------------------------------------------------------
function daggerpants(unit)
local unit = dfhack.gui.getSelectedUnit()

	if unit==nil then
		print ("No unit available!  Aborting with extreme prejudice.")
		return
	end

local pss_counter=31415926

local prefcount = #(unit.status.current_soul.preferences)
print ("Before, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

utils = require 'utils'

-- daggers and greaves, skipped the weapons which are too large for most dorfs
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.WEAPON , creature_id = df.item_type.WEAPON , color_id = df.item_type.WEAPON , shape_id = df.item_type.WEAPON , plant_id = df.item_type.WEAPON , item_subtype = 16 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 4 , item_type = df.item_type.PANTS  , creature_id = df.item_type.PANTS  , color_id = df.item_type.PANTS  , shape_id = df.item_type.PANTS  , plant_id = df.item_type.PANTS  , item_subtype = 1 , mattype = -1 , matindex = -1 , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')
pss_counter = pss_counter + 1
-- likes steel (8)
utils.insert_or_update(unit.status.current_soul.preferences, { new = true, type = 0 , item_type = -1 , creature_id = -1 , color_id = -1 , shape_id = -1 , plant_id = -1 , item_subtype = -1 , mattype = 0 , matindex = dfhack.matinfo.find("STEEL").index , active = true, prefstring_seed = pss_counter }, 'prefstring_seed')

prefcount = #(unit.status.current_soul.preferences)
print ("After, unit "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)).." has "..prefcount.." preferences")

end
-- ---------------------------------------------------------------------------
function print_all(v)
unit=v

local kk,vv

for kk,vv in ipairs(unit.status.current_soul.preferences) do
	printall(vv)
	end
end
-- ---------------------------------------------------------------------------
function clear_all(v)
unit=v

local kk,vv,unk_counter
unk_counter=31415926

for kk,vv in ipairs(unit.status.current_soul.preferences) do
	vv.type = -1
	vv.item_type = -1
	vv.creature_id = -1
	vv.color_id = -1
	vv.shape_id = -1
	vv.plant_id = -1
	vv.item_subtype = -1
	vv.mattype = -1
	vv.matindex = -1
	vv.active = false
	vv.prefstring_seed = unk_counter
	unk_counter = unk_counter + 1
	end
end
-- ---------------------------------------------------------------------------
function printpref_all_dwarves()
	for _,v in ipairs(df.global.world.units.active) do
		if v.race == df.global.ui.race_id then
			print("Showing Preferences for "..dfhack.TranslateName(dfhack.units.getVisibleName(v)))
			print_all(v)
			end
		end
end
-- ---------------------------------------------------------------------------
function clearpref_all_dwarves()
	for _,v in ipairs(df.global.world.units.active) do
		if v.race == df.global.ui.race_id then
			print("Clearing Preferences for "..dfhack.TranslateName(dfhack.units.getVisibleName(v)))
			clear_all(v)
			end
		end
end
-- ---------------------------------------------------------------------------
function clear_preferences(v)
local unit = dfhack.gui.getSelectedUnit()
	local prefs=unit.status.current_soul.preferences
		for index,pref in ipairs(prefs) do
   		pref:delete()
	end
prefs:resize(0)
end
-- ---------------------------------------------------------------------------
-- main script operation starts here
-- ---------------------------------------------------------------------------
local opt = ...
local profilename

if opt then
	if opt=="show" then
		printpref_all_dwarves()
		return
	end
	if opt=="c" then
		clear_preferences(v)
		return
	end
	if opt=="all" then
		clearpref_all_dwarves()
		return
	end
	if opt=="axp" then
		axeplate(unit)
		return
	end
	if opt=="has" then
		hammershirt(unit)
		return
	end
	if opt=="swb" then
		swordboot(unit)
		return
	end
	if opt=="spb" then
		spearboot(unit)
		return
	end
	if opt=="mas" then
		maceshield(unit)
		return
	end
	if opt=="xbh" then
		xbowhelm(unit)
		return
	end
	if opt=="pig" then
		pickglove(unit)
		return
	end
	if opt=="log" then
		longglove(unit)
		return
	end
	if opt=="dap" then
		daggerpants(unit)
		return
	end
else
	print ("Sets preferences for mooding to include a weapon type, equipment type, and material.")
	print ("Valid options:")
	print ("show		-- show preferences of all units")
	print ("c		-- clear preferences of selected unit")
	print ("all		-- clear preferences of all units")
	print ("axp		-- likes axes, breastplates, and steel")
	print ("has		-- likes hammers, mail shirts, and steel")
	print ("swb		-- likes short swords, high boots, and steel")
	print ("spb		-- likes spears, high boots, and steel")
	print ("mas		-- likes maces, shields, and steel")
	print ("xbh		-- likes crossbows, helms, and steel")
	print ("pig		-- likes picks, gauntlets, and steel")
	print ("log		-- likes long swords, gauntlets, and steel")
	print ("dap		-- likes daggers, greaves, and steel")
	print ("Feel free to adjust the values as you see fit, change the has steel to platinum, change the axp axes to great axes, whatnot.")
	end
