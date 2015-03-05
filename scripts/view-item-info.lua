-- Extended Item Viewscreens
-- Shows information on material properties, weapon or armour stats, and more.
-- By PeridexisErrant, adapted from nb_item_info by Raidau

local help = [[Extended Item Viewscreen

A script to extend the item or unit viewscreen with additional information
including properties such as material info, weapon and attack properties,
armor thickness and coverage, and more.]]

function isInList(list, item)
    for k,v in pairs (list) do
        if item == v then
            return true
        end
    end
end

print ("view-item-info enabled")
local args = {...}
local lastframe = df.global.enabler.frame_last
if isInList(args, "help") or isInList(args, "?") then
    print(help) return
end

function append (list, str, indent)
    local str = str or " "
    local indent = indent or 0
    table.insert(list, {str, indent * 4})
end

function add_lines_to_list(t1,t2)
    for i=1,#t2 do
        t1[#t1+1] = t2[i]
    end
    return t1
end

function GetMatPlant (item)
    if dfhack.matinfo.decode(item).mode == "plant" then
        return dfhack.matinfo.decode(item).plant
    end
end

function GetMatPropertiesStringList (item)
    local mat = dfhack.matinfo.decode(item).material
    local list = {}
    local deg_U = item.temperature.whole
    local deg_C = math.floor((deg_U-10000)*5/9)
    append(list,"Temperature: "..deg_C.."\248C ("..deg_U.."U)")
    append(list,"Color: "..df.global.world.raws.language.colors[mat.state_color.Solid].name)
    local function GetStrainDescription (number)
        local str = "unknown"
        if tonumber(number) then str = " (very elastic)" end
        if number < 50000 then str = " (elastic)" end
        if number < 15001 then str = " (medium)" end
        if number < 5001 then str = " (stiff)" end
        if number < 1000 then str = " (very stiff)" end
        if number < 1 then str = " (crystalline)" end
        return str
    end
    local mat_properties_for = {
            "BAR", "SMALLGEM", "BOULDER", "ROUGH",
            "WOOD", "GEM", "ANVIL", "THREAD", "SHOES",
            "CLOTH", "ROCK", "WEAPON", "TRAPCOMP",
            "ORTHOPEDIC_CAST", "SIEGEAMMO", "SHIELD",
            "PANTS", "HELM", "GLOVES", "ARMOR", "AMMO"
            }
    if isInList(mat_properties_for, item) then
        append(list,"Material properties: ")
        append(list,"Solid density: "..mat.solid_density..'g/cm^3',1)
        local maxedge = mat.strength.max_edge
        append(list,"Maximum sharpness: "..maxedge.." ("..maxedge/standard.strength.max_edge*100 .."%)",1)
        if mat.molar_mass > 0 then append(list,"Molar mass: "..mat.molar_mass,1) end
        local s_yield = mat.strength.yield.SHEAR
        local s_fract = mat.strength.fracture.SHEAR
        local s_strain = mat.strength.strain_at_yield.SHEAR
        local s_str = "Shear yield: "..s_yield.."("..math.floor(s_yield/standard.strength.yield.SHEAR*100)..
            "%), fr.: "..s_fract.."("..math.floor(s_fract/standard.strength.fracture.SHEAR*100)..
            "%), el.: "..s_strain..GetStrainDescription(s_strain)
        append(list, s_str, 1)
        local i_yield = mat.strength.yield.IMPACT
        local i_fract = mat.strength.fracture.IMPACT
        local i_strain = mat.strength.strain_at_yield.IMPACT
        local i_str = "IMPACT yield: "..i_yield.."("..math.floor(i_yield/standard.strength.yield.IMPACT*100)..
            "%), fr.: "..i_fract.."("..math.floor(i_fract/standard.strength.fracture.IMPACT*100)..
            "%), el.: "..i_strain..GetStrainDescription(i_strain)
        append(list, i_str, 1)
    end
    append(list)
    return list
end

function GetArmorPropertiesStringList (item)
    local mat = dfhack.matinfo.decode(item).material
    local list = {}
    append(list,"Armor properties: ")
    append(list,"Thickness: "..item.subtype.props.layer_size,1)
    append(list,"Coverage: "..item.subtype.props.coverage.."%",1)
    append(list,"Fit for "..df.creature_raw.find(item.maker_race).name[0],1)
    append(list)
    return list
end

function GetShieldPropertiesStringList (item)
    local mat = dfhack.matinfo.decode(item).material
    local list = {}
    append(list,"Shield properties: ")
    append(list,"Base block chance: "..item.subtype.blockchance,1)
    append(list,"Fit for "..df.creature_raw.find(item.maker_race).name[0],1)
    append(list)
    return list
end

function GetWeaponPropertiesStringList (item)
    local mat = dfhack.matinfo.decode(item).material
    local list = {}
    append(list)
    if item._type == df.item_toolst and #item.subtype.attacks < 1 then
        return list
    end
    append(list,"Weapon properties: ")
    if item.sharpness > 0 then
        append(list,"Sharpness: "..item.sharpness.." ("..item.sharpness/standard.strength.max_edge*100 .."%)",1)
    else
        append(list,"Not edged",1)
    end
    if string.len(item.subtype.ranged_ammo) > 0 then
        append(list,"Ranged weapon",1)
        append(list,"Ammo: "..item.subtype.ranged_ammo:lower(),2)
        if item.subtype.shoot_force > 0 then
            append(list,"Shoot force: "..item.subtype.shoot_force,2)
        end
        if item.subtype.shoot_maxvel > 0 then
            append(list,"Maximum projectile velocity: "..item.subtype.shoot_maxvel,2)
        end
    end
    append(list,"Required size: "..item.subtype.minimum_size*10,1)
    if item.subtype.two_handed*10 > item.subtype.minimum_size*10 then
        append(list,"Used as 2-handed until unit size: "..item.subtype.two_handed*10,2)
    end
    append(list,"Attacks: ",1)
    for k,attack in pairs (item.subtype.attacks) do
        local name = attack.verb_2nd
        if attack.noun ~= "NO_SUB" then
            name = name.." with "..attack.noun
        end
        if attack.edged then
            name = name.." (edged)"
        else
            name = name.." (blunt)"
        end
        append(list,name,2)
        append(list,"Contact area: "..attack.contact,3)
        if attack.edged then
            append(list,"Penetration: "..attack.penetration,3)
        end
        append(list,"Velocity multiplier: "..attack.velocity_mult/1000,3)
        if attack.flags.bad_multiattack then
            append(list,"Bad multiattack",3)
        end
        append(list,"Prepare/recover: "..attack.prepare.."/"..attack.recover,3)
    end
    append(list)
    return list
end

function GetAmmoPropertiesStringList (item)
    local mat = dfhack.matinfo.decode(item).material
    local list = {}
    append(list)
    if item._type == df.item_toolst and #item.subtype.attacks < 1 then
        return list
    end
    append(list,"Ammo properties: ")
    if item.sharpness > 0 then
        append(list,"Sharpness: "..item.sharpness,1)
    else
        append(list,"Not edged",1)
    end
    append(list,"Attacks: ", 1)
    for k,attack in pairs (item.subtype.attacks) do
        local name = attack.verb_2nd
        if attack.noun ~= "NO_SUB" then
            name = name.." with "..attack.noun
        end
        if attack.edged then
            name = name.." (edged)"
        else
            name = name.." (blunt)"
        end
        append(list,name,2)
        append(list,"Contact area: "..attack.contact,3)
        if attack.edged then
            append(list,"Penetration: "..attack.penetration,3)
        end
        append(list,"Velocity multiplier: "..attack.velocity_mult/1000,3)
        if attack.flags.bad_multiattack then
            append(list,"Bad multiattack",3)
        end
        append(list,"Prepare "..attack.prepare.." / recover "..attack.recover,3)
    end
    append(list)
    return list
end

function edible_string (mat)
    local edible_string = "Edible"
    if mat.flags.EDIBLE_RAW or mat.flags.EDIBLE_COOKED then
        if mat.flags.EDIBLE_RAW then
            edible_string = edible_string.." raw"
            if mat.flags.EDIBLE_COOKED then
                edible_string = edible_string.." and cooked"
            end
        else
            if mat.flags.EDIBLE_COOKED then
                edible_string = edible_string.." only when cooked"
            end
        end
    else
        edible_string = "Not edible"
    end
    return edible_string
end

function GetReactionProduct (inmat, reaction)
    for k,v in pairs (inmat.reaction_product.id) do
        if v.value == reaction then
            return {inmat.reaction_product.material.mat_type[k],
                    inmat.reaction_product.material.mat_index[k]}
        end
    end
end

function add_react_prod (list, mat, product, str)
    local mat_type, mat_index = GetReactionProduct (mat, product)
    if mat_type then
        local result = dfhack.matinfo.decode(mat_type, mat_index).material.state_name.Liquid
        append(list, str..result)
    end
end

function get_plant_reaction_products (mat)
    local list = {}
    add_react_prod (list, mat, "DRINK_MAT", "Used to brew ")
    add_react_prod (list, mat, "GROWTH_JUICE_PROD", "Pressed into ")
    add_react_prod (list, mat, "PRESS_LIQUID_MAT", "Pressed into ")
    add_react_prod (list, mat, "LIQUID_EXTRACTABLE", "Extractable product: ")
    add_react_prod (list, mat, "WATER_SOLUTION_PROD", "Can be mixed with water to make ")
    add_react_prod (list, mat, "RENDER_MAT", "Rendered into ")
    add_react_prod (list, mat, "SOAP_MAT", "Used to make soap")
    if GetReactionProduct (mat, "SUGARABLE") then
        append(list,"Used to make sugar")
    end
    if  GetReactionProduct (mat, "MILLABLE") or
        GetReactionProduct (mat, "GRAIN_MILLABLE") or
        GetReactionProduct (mat, "GROWTH_MILLABLE") then
        append(list,"Can be milled")
    end
    if GetReactionProduct(mat, "GRAIN_THRESHABLE") then
        append(list,"Grain can be threshed")
    end
    if GetReactionProduct (mat, "CHEESE_MAT") then
        local mat_type, mat_index = GetReactionProduct (mat, "CHEESE_MAT")
        append(list,"Used to make "..dfhack.matinfo.decode(mat_type, mat_index).material.state_name.Solid)
    end
    return list
end

function GetFoodPropertiesStringList (item)
    local mat = dfhack.matinfo.decode(item).material
    local list = {{" ", 0}}
    append(list,edible_string(mat))
    if item._type == df.item_foodst then
        append(list,"This is prepared meal")
        append(list)
        return list
    end
    if mat == dfhack.matinfo.find ("WATER") then
        append(list,"Water is drinkable")
        append(list)
        return list
    end
    add_lines_to_list(list, get_plant_reaction_products(mat))
    if item._type == df.item_plantst and GetMatPlant (item) then
        local plant = GetMatPlant (item)
        for k,v in pairs (plant.material_defs) do
            if v ~= -1 and string.find (k,"type_") and not string.find (k,"type_basic")
                    or string.find (k,"type_seed") or string.find (k,"type_tree") then
                local targetmat = dfhack.matinfo.decode (v,
                    plant.material_defs["idx_"..string.match (k,"type_(.+)")])
                local state
                if string.find (k,"type_mill") then state = "Powder"
                elseif string.find (k,"type_thread") then state = "Solid"
                else state = "Liquid" end
                local st_name = targetmat.material.state_name[state]
                append(list,"Used to make "..targetmat.material.prefix..''..st_name)
            end
        end
    end
    append(list)
    return list
end

function get_all_uses_strings (scr)
    local all_lines = {}
    local itemtype = scr.item._type
    local FoodsAndPlants = {df.item_meatst, df.item_globst,
        df.item_plantst, df.item_plant_growthst, df.item_liquid_miscst,
         df.item_powder_miscst, df.item_cheesest, df.item_foodst}
    local ArmourTypes = {df.item_armorst, df.item_pantsst,
        df.item_helmst, df.item_glovesst, df.item_shoesst}
    if df.global.gamemode == 1 and scr.item ~= "COIN" then -- TODO: fix magic number
        add_lines_to_list(all_lines, {{"Value: "..dfhack.items.getValue(scr.item),0}})
    elseif isInList(ArmourTypes, itemtype) then
        add_lines_to_list(all_lines, GetArmorPropertiesStringList(scr.item))
    elseif itemtype == df.item_weaponst or itemtype == df.item_toolst then
        add_lines_to_list(all_lines, GetWeaponPropertiesStringList(scr.item))
    elseif itemtype == df.item_ammost then
        add_lines_to_list(all_lines, GetAmmoPropertiesStringList(scr.item))
    elseif itemtype == df.item_shieldst then
        add_lines_to_list(all_lines, GetShieldPropertiesStringList(scr.item))
    elseif itemtype == df.item_seedsst then
        local str = math.floor(GetMatPlant(scr.item).growdur/12).." days"
        add_lines_to_list(all_lines, {{"Growth time of this plant:  "..str, 0}})
    elseif isInList(FoodsAndPlants, itemtype) then
        add_lines_to_list(all_lines, GetFoodPropertiesStringList(scr.item))
    end
    return all_lines
end

function AddUsesString (viewscreen,line,indent)
    local str = df.new("string")
    str.value = tostring(line)
    local indent = indent or 0
    viewscreen.entry_ref:insert('#', nil)
    viewscreen.entry_indent:insert('#', indent)
    viewscreen.unk_34:insert('#', nil) -- TODO: fix magic number
    viewscreen.entry_string:insert('#', str)
    viewscreen.entry_reaction:insert('#', -1)
end

function dfhack.onStateChange.item_info (code)
    if code == SC_VIEWSCREEN_CHANGED and dfhack.isWorldLoaded() then
        standard = dfhack.matinfo.find("INORGANIC:IRON").material
        if not standard then return end
        local scr = dfhack.gui.getCurViewscreen()
        if scr._type == df.viewscreen_itemst then
            if #scr.entry_string > 0 and scr.entry_string[#scr.entry_string-1].value == " " then
                return
            end
            local all_lines = get_all_uses_strings (scr)
            for i = 1, #all_lines do
                AddUsesString(scr,all_lines[i][1],all_lines[i][2])
            end
            scr.caption_uses = true
        end
    end
end
