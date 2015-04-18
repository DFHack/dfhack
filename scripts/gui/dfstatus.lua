-- dfstatus 1.5 - a quick access status screen.
-- originally written by enjia2000@gmail.com

local gui = require 'gui'

dfstatus = defclass(dfstatus, gui.FramedScreen)
dfstatus.ATTRS = {
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = 'dfstatus',
    frame_width = 16,
    frame_height = 17,
    frame_inset = 1,
    focus_path = 'dfstatus',
}

function dfstatus:onRenderBody(dc)
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

    for _, item in ipairs(df.global.world.items.all) do
        if not item.flags.rotten and not item.flags.dump and not item.flags.forbid then
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
    dc:pen(COLOR_LIGHTGREEN)
    dc:string("Drinks:  " .. drink)
    dc:newline(0)
    dc:string("Meals:   " .. prepared_meals)
    dc:newline(0)
    dc:newline(0)
    dc:string("Wood: " .. wood)
    dc:newline(0)
    dc:newline(0)
    dc:string("Hides: " .. tannedhides)
    dc:newline(0)
    dc:string("Cloth: " .. cloth)
    dc:newline(0)
    -- dc:string("Raw Fish: ".. raw_fish)
    -- dc:newline(0)
    -- dc:string("Plants: ".. plants)
    -- dc:newline(0)
    dc:newline(0)
    dc:string("Bars:")
    dc:newline(1)
    dc:string("Fuel:      " .. fuel)
    dc:newline(1)
    dc:string("Pig Iron:  " .. pigiron)
    dc:newline(1)
    dc:string("Steel:     " .. steel)
    dc:newline(1)
    dc:string("Iron:      " .. iron)
    dc:newline(1)
    dc:newline(1)
    dc:string("Copper:    " .. copper)
    dc:newline(1)
    dc:string("Silver:    " .. silver)
    dc:newline(1)
    dc:string("Gold:      " .. gold)
end

function dfstatus:onInput(keys)
    if keys.LEAVESCREEN or keys.SELECT then
        self:dismiss()
        scr = nil
    end
end

if not scr then
    scr = dfstatus()
    scr:show()
else
    scr:dismiss()
    scr = nil
end

