local _ENV = mkmodule('plugins.eventful')
--[[
    
    
--]]
local function getShopName(btype,bsubtype,bcustom)
    local typenames_shop={[df.workshop_type.Carpenters]="CARPENTERS",[df.workshop_type.Farmers]="FARMERS",
        [df.workshop_type.Masons]="MASONS",[df.workshop_type.Craftsdwarfs]="CRAFTSDWARFS",
        [df.workshop_type.Jewelers]="JEWELERS",[df.workshop_type.MetalsmithsForge]="METALSMITHSFORGE",
        [df.workshop_type.MagmaForge]="MAGMAFORGE",[df.workshop_type.Bowyers]="BOWYERS",
        [df.workshop_type.Mechanics]="MECHANICS",[df.workshop_type.Siege]="SIEGE",
        [df.workshop_type.Butchers]="BUTCHERS",[df.workshop_type.Leatherworks]="LEATHERWORKS",
        [df.workshop_type.Tanners]="TANNERS",[df.workshop_type.Clothiers]="CLOTHIERS",
        [df.workshop_type.Fishery]="FISHERY",[df.workshop_type.Still]="STILL",
        [df.workshop_type.Loom]="LOOM",[df.workshop_type.Quern]="QUERN",
        [df.workshop_type.Kennels]="KENNELS",[df.workshop_type.Ashery]="ASHERY",
        [df.workshop_type.Kitchen]="KITCHEN",[df.workshop_type.Dyers]="DYERS",
        [df.workshop_type.Tool]="TOOL",[df.workshop_type.Millstone]="MILLSTONE",
        }
    local typenames_furnace={[df.furnace_type.WoodFurnace]="WOOD_FURNACE",[df.furnace_type.Smelter]="SMELTER",
        [df.furnace_type.GlassFurnace]="GLASS_FURNACE",[df.furnace_type.MagmaSmelter]="MAGMA_SMELTER",
        [df.furnace_type.MagmaGlassFurnace]="MAGMA_GLASS_FURNACE",[df.furnace_type.MagmaKiln]="MAGMA_KILN",
        [df.furnace_type.Kiln]="KILN"}
    if btype==df.building_type.Workshop then
        if typenames_shop[bsubtype]~=nil then 
            return typenames_shop[bsubtype]
        else
            return nil --todo add custom (not very useful)
        end
    elseif btype==df.building_type.Furnace then
        if typenames_furnace[bsubtype]~=nil then 
            return typenames_furnace[bsubtype]
        else
            return nil --todo add custom (not very useful)
        end
    end
end
_registeredStuff={}
local function unregall(state)
    if state==SC_WORLD_UNLOADED then
        onReactionComplete._library=nil
        postWorkshopFillSidebarMenu._library=nil
        dfhack.onStateChange.eventful= nil
        _registeredStuff={}
    end
end
local function onReact(reaction,unit,input_items,input_reagents,output_items,call_native)
    if _registeredStuff.reactionCallbacks and _registeredStuff.reactionCallbacks[reaction.code] then
        _registeredStuff.reactionCallbacks[reaction.code](reaction,unit,input_items,input_reagents,output_items,call_native)
    end
end
local function onPostSidebar(workshop)
    local shop_id=getShopName(workshop:getType(),workshop:getSubtype(),workshop:getCustomType())
    if shop_id then
        if _registeredStuff.shopNonNative and _registeredStuff.shopNonNative[shop_id]  then
            if _registeredStuff.shopNonNative[shop_id].all then
                --[[for _,button in ipairs(df.global.ui_sidebar_menus.workshop_job.choices_all) do
                    button.is_hidden=true
                end]]
                df.global.ui_sidebar_menus.workshop_job.choices_visible:resize(0)
            else
                --todo by name
            end
        end

        if _registeredStuff.reactionToShop and _registeredStuff.reactionToShop[shop_id]  then
            for _,reaction_name in ipairs(_registeredStuff.reactionToShop[shop_id]) do
                local new_button=df.interface_button_building_new_jobst:new()
                --new_button.hotkey_id=--todo get hotkey
                new_button.is_hidden=false
                new_button.building=workshop
                new_button.job_type=df.job_type.CustomReaction --could be used for other stuff too i guess...
                new_button.reaction_name=reaction_name
                new_button.is_custom=true
                local wjob=df.global.ui_sidebar_menus.workshop_job
                wjob.choices_all:insert("#",new_button)
                wjob.choices_visible:insert("#",new_button)
            end
        end
    end
end

function registerReaction(reaction_name,callback)
    _registeredStuff.reactionCallbacks=_registeredStuff.reactionCallbacks or {}
    _registeredStuff.reactionCallbacks[reaction_name]=callback
    onReactionComplete._library=onReact
    dfhack.onStateChange.eventful=unregall
end

function removeNative(shop_name,name)
    _registeredStuff.shopNonNative=_registeredStuff.shopNonNative or {}
    local shops=_registeredStuff.shopNonNative
    shops[shop_name]=shops[shop_name] or {}
    if name~=nil then
        table.insert(shops[shop_name],name)
    else
        shops[shop_name].all=true
    end
    postWorkshopFillSidebarMenu._library=onPostSidebar
    dfhack.onStateChange.eventful=unregall
end

function addReactionToShop(reaction_name,shop_name)
    _registeredStuff.reactionToShop=_registeredStuff.reactionToShop or {}
    local shops=_registeredStuff.reactionToShop
    shops[shop_name]=shops[shop_name] or {}
    table.insert(shops[shop_name],reaction_name)
    postWorkshopFillSidebarMenu._library=onPostSidebar
    dfhack.onStateChange.eventful=unregall
end
local function invertTable(tbl)
	local ret={}
	for k,v in pairs(tbl) do
		ret[v]=k
	end
	return ret
end
eventType=invertTable{[0]="TICK","JOB_INITIATED","JOB_COMPLETED","UNIT_DEATH","ITEM_CREATED",
	"BUILDING","CONSTRUCTION","SYNDROME","INVASION","INVENTORY_CHANGE","EVENT_MAX"}
return _ENV
