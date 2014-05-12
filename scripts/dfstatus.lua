-- dfstatus 1.5 - a quick access status screen.
-- written by enjia2000@gmail.com, fixed by Lethosor and PeridexisErrant


local gui = require 'gui'

function draw()
    screen2 = gui.FramedScreen{
        frame_style = gui.GREY_LINE_FRAME,
        frame_title = 'dfstatus',
        frame_width = 16,
        frame_height = 17,
        frame_inset = 1,
    }
end

if (not shown) then
    draw()
    screen2:show()
    shown = true
else
    shown = nil
    screen2:dismiss()
end

function screen2:onRenderBody(dc)
    local drink = 0
    local wood = 0
    --local meat = 0
    --local raw_fish = 0
    --local plants = 0
    local prepared_meals = 0
    
    local fuel = 0
    local pigiron = 0
    local iron = 0
    local steel = 0
    
    local silver = 0
    local copper = 0
    local gold = 0
    
    local tannedhides = 0
    local cloth = 0
    
    --print("------------------------------")
    for _,item in ipairs(df.global.world.items.all) do
    if(not item.flags.rotten and not item.flags.dump and not item.flags.forbid) then
        if(item:getType() ~= df.item_type.THREAD and item:getType() ~= df.item_type.REMAINS and item:getType() ~= df.item_type.ARMOR and item:getType() ~= df.item_type.SHOES and item:getType() ~= df.item_type.SHIELD and item:getType() ~= df.item_type.HELM and item:getType() ~= df.item_type.GOVES) then
            --print(item:getType() .. ":" .. dfhack.items.getDescription(item,0))
        end
        if(item:getType() == df.item_type.DRINK)then
            --print(item:getType() .. ":" .. dfhack.items.getDescription(item,0))
        end
        
        if (item:getType() == df.item_type.WOOD) then wood = wood + item:getStackSize()
        elseif (item:getType() == df.item_type.DRINK) then drink = drink + item:getStackSize()
             elseif (item:getType() == df.item_type.SKIN_TANNED) then tannedhides = tannedhides + item:getStackSize()
             elseif (item:getType() == df.item_type.CLOTH) then cloth = cloth + item:getStackSize()
        --elseif (item:getType() == df.item_type.MEAT) then meat = meat + item:getStackSize() 
        --elseif (item:getType() == df.item_type.FISH_RAW) then raw_fish = raw_fish + item:getStackSize() 
        --elseif (item:getType() == df.item_type.PLANT) then plants = plants + item:getStackSize() 
        elseif (item:getType() == df.item_type.FOOD) then prepared_meals = prepared_meals + item:getStackSize() 
        elseif (item:getType() == df.item_type.BAR) then
            for token in string.gmatch(dfhack.items.getDescription(item,0),"[^%s]+") do
                if (token == "silver") then silver = silver + item:getStackSize() 
                elseif (token == "charcoal" or token == "coke") then fuel = fuel + item:getStackSize()
                elseif (token == "iron") then iron = iron + item:getStackSize()
                elseif (token == "pig") then pigiron = pigiron + item:getStackSize()
                elseif (token == "copper") then copper = copper + item:getStackSize()
                elseif (token == "gold") then gold = gold + item:getStackSize()
                elseif (token == "steel") then steel = steel + item:getStackSize()
                end
                break -- only need to look at the 1st token of each item.
            end
        end
    end
end
    --print("------------------------------")
    dc:string("Drinks:  ".. drink, COLOR_LIGHTGREEN)
    dc:newline(0)
    dc:string("Meals:   ".. prepared_meals, COLOR_LIGHTGREEN)
    dc:newline(0)
    dc:newline(0)
    dc:string("Wood: ".. wood, COLOR_LIGHTGREEN)
    dc:newline(0)
    dc:newline(0)
    dc:string("Hides: ".. tannedhides, COLOR_LIGHTGREEN)
    dc:newline(0)
    dc:string("Cloth: ".. cloth, COLOR_LIGHTGREEN)
    dc:newline(0)
    -- dc:string("Raw Fish: ".. raw_fish, COLOR_LIGHTGREEN)
    -- dc:newline(0)
    -- dc:string("Plants: ".. plants, COLOR_LIGHTGREEN)
    -- dc:newline(0)
    dc:newline(0)
    dc:string("Bars:", COLOR_LIGHTGREEN)
    dc:newline(1)
    dc:string("Fuel:      ".. fuel, COLOR_LIGHTGREEN)
    dc:newline(1)
    dc:string("Pig Iron:  ".. pigiron, COLOR_LIGHTGREEN)
    dc:newline(1)
    dc:string("Steel:     ".. steel, COLOR_LIGHTGREEN)
    dc:newline(1)
    dc:string("Iron:      ".. iron, COLOR_LIGHTGREEN)
    dc:newline(1)
    dc:newline(1)
    dc:string("Copper:    ".. copper, COLOR_LIGHTGREEN)
    dc:newline(1)
    dc:string("Silver:    ".. silver, COLOR_LIGHTGREEN)
    dc:newline(1)
    dc:string("Gold:      ".. gold, COLOR_LIGHTGREEN)
end

function screen2:onInput(keys)
    if keys.LEAVESCREEN or keys.SELECT then
        shown = nil
        self:dismiss()
    end
end

