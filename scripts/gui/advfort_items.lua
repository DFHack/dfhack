
--[[=begin

gui/advfort_items
=================
Does something with items in adventure mode jobs.

=end]]
local _ENV = mkmodule('hack.scripts.gui.advfort_items')

local gui=require('gui')
local wid=require('gui.widgets')
local gscript=require('gui.script')

jobitemEditor=defclass(jobitemEditor,gui.FramedScreen)
jobitemEditor.ATTRS{
    frame_style = gui.GREY_LINE_FRAME,
    frame_inset = 1,
    allow_add=false,
    allow_remove=false,
    allow_any_item=false,
    job=DEFAULT_NIL,
    job_items=DEFAULT_NIL,
    items=DEFAULT_NIL,
    on_okay=DEFAULT_NIL,
    autofill=true,
}
function update_slot_text(slot)
    local items=""
    for i,v in ipairs(slot.items) do
        items=items.." "..dfhack.items.getDescription(v,0)
        if i~=#slot.items then
            items=items..","
        end
    end

    slot.text=string.format("%02d. Filled(%d/%d):%s",slot.id+1,slot.filled_amount,slot.job_item.quantity,items)
end
--items-> table => key-> id of job.job_items, value-> table => key (num), value => item(ref)
function jobitemEditor:init(args)
    --self.job=args.job
    if self.job==nil and self.job_items==nil then qerror("This screen must have job target or job_items list") end
    if self.items==nil then qerror("This screen must have item list") end

    self:addviews{
            wid.Label{
                view_id = 'label',
                text = args.prompt,
                text_pen = args.text_pen,
                frame = { l = 0, t = 0 },
            },
            wid.List{
                view_id = 'itemList',
                frame = { l = 0, t = 2 ,b=2},
            },
            wid.Label{
                frame = { b=1,l=1},
                text ={{text= ": cancel",
                    key  = "LEAVESCREEN",
                    on_activate= self:callback("dismiss")
                    },
                    {
                    gap=3,
                    text= ": accept",
                    key  = "SEC_SELECT",
                    on_activate= self:callback("commit"),
                    enabled=self:callback("jobValid")
                    },
                     {
                    gap=3,
                    text= ": add",
                    key  = "CUSTOM_A",
                    enabled=self:callback("can_add"),
                    on_activate= self:callback("add_item")
                    },
                     {
                    gap=3,
                    text= ": remove",
                    key  = "CUSTOM_R",
                    enabled=self:callback("can_remove"),
                    on_activate= self:callback("remove_item")
                    },}
            },
    }
    self.assigned={}
    self:fill()
    if self.autofill then
        self:fill_slots()
    end
end
function jobitemEditor:get_slot()
    local idx,choice=self.subviews.itemList:getSelected()
    return choice
end
function jobitemEditor:can_add()
    local slot=self:get_slot()
    return slot.filled_amount<slot.job_item.quantity
end
function jobitemEditor:can_remove()
    local slot=self:get_slot()
    return #slot.items>0
end
function jobitemEditor:add_item()
    local cur_slot=self:get_slot()
    local choices={}
    table.insert(choices,{text="<no item>"})
    for k,v in pairs(cur_slot.choices) do
        if not self.assigned[v.id] then
            table.insert(choices,{text=dfhack.items.getDescription(v,0),item=v})
        end
    end
    gscript.start(function ()
        local _,_2,choice=gscript.showListPrompt("which item?", "Select item", COLOR_WHITE, choices)
        if choice ~= nil and choice.item~=nil then
            self:add_item_to_slot(cur_slot,choice.item)
        end
    end
    )
end
function jobitemEditor:fill_slots()
    for i,v in ipairs(self.slots) do
        while v.filled_amount<v.job_item.quantity do
            local added=false
            for _,it in ipairs(v.choices) do
                if not self.assigned[it.id] then
                    self:add_item_to_slot(v,it)
                    added=true
                    break
                end
            end
            if not added then
                break
            end
        end
    end
end
function jobitemEditor:add_item_to_slot(slot,item)
    table.insert(slot.items,item)
    slot.filled_amount=slot.filled_amount+item:getTotalDimension()
    self.assigned[item.id]=true
    update_slot_text(slot)
    self.subviews.itemList:setChoices(self.slots)
end
function jobitemEditor:remove_item()
    local slot=self:get_slot()
    for k,v in pairs(slot.items) do
        self.assigned[v.id]=nil
    end
    slot.items={}
    slot.filled_amount=0
    update_slot_text(slot)
    self.subviews.itemList:setChoices(self.slots)
end
function jobitemEditor:fill()
    self.slots={}
    for k,v in pairs(self.items) do
        local job_item

        if self.job then
            job_item=self.job.job_items[k]
        else
            job_item=self.job_items[k]
        end

        table.insert(self.slots,{job_item=job_item, id=k, items={},choices=v,filled_amount=0,slot_id=#self.slots})
        update_slot_text(self.slots[#self.slots])
    end
    self.subviews.itemList:setChoices(self.slots)
end

function jobitemEditor:jobValid()
    for k,v in pairs(self.slots) do
        if v.filled_amount<v.job_item.quantity then
            return false
        end
    end
    return true
end
function jobitemEditor:commit()
    if self.job then
        for _,slot in pairs(self.slots) do
            for _1,cur_item in pairs(slot.items) do
                self.job.items:insert("#",{new=true,item=cur_item,role=df.job_item_ref.T_role.Reagent,job_item_idx=slot.id})
            end
        end
    end
    self:dismiss()
    if self.on_okay then self.on_okay(self.slots) end
end
function jobitemEditor:onDestroy()
    if self.on_close then
        self.on_close()
    end
end
function showItemEditor(job,item_selections)
    jobitemEditor{
        job = job,
        items = item_selections,
        on_close = gscript.qresume(nil),
        on_okay = gscript.mkresume(true)
        --on_cancel=gscript.mkresume(false)
    }:show()

    return gscript.wait()
end

return _ENV