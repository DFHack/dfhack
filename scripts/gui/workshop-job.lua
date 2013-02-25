-- Show and modify properties of jobs in a workshop.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local guimat = require 'gui.materials'
local widgets = require 'gui.widgets'
local dlg = require 'gui.dialogs'

JobDetails = defclass(JobDetails, guidm.MenuOverlay)

JobDetails.focus_path = 'workshop-job'

JobDetails.ATTRS {
    job = DEFAULT_NIL,
    frame_inset = 1,
    frame_background = COLOR_BLACK,
}

function JobDetails:init(args)
    self.building = dfhack.job.getHolder(self.job)

    local status = { text = 'No worker', pen = COLOR_DARKGREY }
    local worker = dfhack.job.getWorker(self.job)
    if self.job.flags.suspend then
        status = { text = 'Suspended', pen = COLOR_RED }
    elseif worker then
        status = { text = dfhack.TranslateName(dfhack.units.getVisibleName(worker)), pen = COLOR_GREEN }
    end

    self:addviews{
        widgets.Label{
            frame = { l = 0, t = 0 },
            text = {
                { text = df.job_type.attrs[self.job.job_type].caption }, NEWLINE, NEWLINE,
                '  ', status
            }
        },
        widgets.Label{
            frame = { l = 0, t = 4 },
            text = {
                { key = 'CUSTOM_I', text = ': Input item, ',
                  enabled = self:callback('canChangeIType'),
                  on_activate = self:callback('onChangeIType') },
                { key = 'CUSTOM_M', text = ': Material',
                  enabled = self:callback('canChangeMat'),
                  on_activate = self:callback('onChangeMat') }
            }
        },
        widgets.List{
            view_id = 'list',
            frame = { t = 6, b = 2 },
            row_height = 4,
            scroll_keys = widgets.SECONDSCROLL,
        },
        widgets.Label{
            frame = { l = 0, b = 0 },
            text = {
                { key = 'LEAVESCREEN', text = ': Back',
                  on_activate = self:callback('dismiss') }
            }
        },
    }

    self:initListChoices()
end

function JobDetails:onGetSelectedBuilding()
    return self.building
end

function JobDetails:onGetSelectedJob()
    return self.job
end

function describe_item_type(iobj)
    local itemline = 'any item'
    if iobj.item_type >= 0 then
        itemline = df.item_type.attrs[iobj.item_type].caption or iobj.item_type
        local def = dfhack.items.getSubtypeDef(iobj.item_type, iobj.item_subtype)
        local count = dfhack.items.getSubtypeCount(iobj.item_type, iobj.item_subtype)
        if def then
            itemline = def.name
        elseif count >= 0 then
            itemline = 'any '..itemline
        end
    end
    return itemline
end

function is_caste_mat(iobj)
    return dfhack.items.isCasteMaterial(iobj.item_type)
end

function describe_material(iobj)
    local matline = 'any material'
    if is_caste_mat(iobj) then
        matline = 'material not applicable'
    elseif iobj.mat_type >= 0 then
        local info = dfhack.matinfo.decode(iobj.mat_type, iobj.mat_index)
        if info then
            matline = info:toString()
        else
            matline = iobj.mat_type..':'..iobj.mat_index
        end
    end
    return matline
end

function list_flags(list, bitfield)
    for name,val in pairs(bitfield) do
        if val then
            table.insert(list, name)
        end
    end
end

function JobDetails:initListChoices()
    local items = {}
    for i,ref in ipairs(self.job.items) do
        local idx = ref.job_item_idx
        if idx >= 0 then
            items[idx] = (items[idx] or 0) + 1
        end
    end

    local choices = {}
    for i,iobj in ipairs(self.job.job_items) do
        local head = 'Item '..(i+1)..': '..(items[i] or 0)..' of '..iobj.quantity
        if iobj.min_dimension > 0 then
            head = head .. '(size '..iobj.min_dimension..')'
        end

        local line1 = {}
        local reaction = df.reaction.find(iobj.reaction_id)
        if reaction and #iobj.contains > 0 then
            for _,ri in ipairs(iobj.contains) do
                table.insert(line1, 'has '..utils.call_with_string(
                    reaction.reagents[ri],'getDescription',iobj.reaction_id
                ))
            end
        end
        if iobj.metal_ore >= 0 then
            local ore = dfhack.matinfo.decode(0, iobj.metal_ore)
            if ore then
                table.insert(line1, 'ore of '..ore:toString())
            end
        end
        if iobj.has_material_reaction_product ~= '' then
            table.insert(line1, 'product '..iobj.has_material_reaction_product)
        end
        if iobj.reaction_class ~= '' then
            table.insert(line1, 'class '..iobj.reaction_class)
        end
        if iobj.has_tool_use >= 0 then
            table.insert(line1, 'has use '..df.tool_uses[iobj.has_tool_use])
        end
        list_flags(line1, iobj.flags1)
        list_flags(line1, iobj.flags2)
        list_flags(line1, iobj.flags3)
        if #line1 == 0 then
            table.insert(line1, 'no flags')
        end

        table.insert(choices, {
            index = i,
            iobj = iobj,
            text = {
                head, NEWLINE,
                '  ', { text = curry(describe_item_type, iobj) }, NEWLINE,
                '  ', { text = curry(describe_material, iobj) }, NEWLINE,
                '  ', table.concat(line1, ', '), NEWLINE
            }
        })
    end

    self.subviews.list:setChoices(choices)
end

function JobDetails:canChangeIType()
    local idx, obj = self.subviews.list:getSelected()
    return obj ~= nil
end

function JobDetails:setItemType(obj, item_type, item_subtype)
    obj.iobj.item_type = item_type
    obj.iobj.item_subtype = item_subtype

    if is_caste_mat(obj.iobj) then
        self:setMaterial(obj, -1, -1)
    end
end

function JobDetails:onChangeIType()
    local idx, obj = self.subviews.list:getSelected()
    guimat.ItemTypeDialog{
        prompt = 'Please select a new item type for input '..idx,
        none_caption = 'any item',
        item_filter = curry(dfhack.job.isSuitableItem, obj.iobj),
        on_select = self:callback('setItemType', obj)
    }:show()
end

function JobDetails:canChangeMat()
    local idx, obj = self.subviews.list:getSelected()
    return obj ~= nil and not is_caste_mat(obj.iobj)
end

function JobDetails:setMaterial(obj, mat_type, mat_index)
    if  obj.index == 0
    and self.job.mat_type == obj.iobj.mat_type
    and self.job.mat_index == obj.iobj.mat_index
    and self.job.job_type ~= df.job_type.PrepareMeal
    then
        self.job.mat_type = mat_type
        self.job.mat_index = mat_index
    end

    obj.iobj.mat_type = mat_type
    obj.iobj.mat_index = mat_index
end

function JobDetails:findUnambiguousItem(iobj)
    local count = 0
    local itype

    for i = 0,df.item_type._last_item do
        if dfhack.job.isSuitableItem(iobj, i, -1) then
            count = count + 1
            if count > 1 then return nil end
            itype = i
        end
    end

    return itype
end

function JobDetails:onChangeMat()
    local idx, obj = self.subviews.list:getSelected()

    if obj.iobj.item_type == -1 and obj.iobj.mat_type == -1 then
        -- If the job allows only one specific item type, use it
        local vitype = self:findUnambiguousItem(obj.iobj)

        if vitype then
            obj.iobj.item_type = vitype
        else
            dlg.showMessage(
                'Bug Alert',
                { 'Please set a specific item type first.\n\n',
                  'Otherwise the material will be matched\n',
                  'incorrectly due to a limitation in DF code.' },
                COLOR_YELLOW
            )
            return
        end
    end

    guimat.MaterialDialog{
        prompt = 'Please select a new material for input '..idx,
        none_caption = 'any material',
        mat_filter = function(mat,parent,mat_type,mat_index)
            return dfhack.job.isSuitableMaterial(obj.iobj, mat_type, mat_index)
        end,
        on_select = self:callback('setMaterial', obj)
    }:show()
end

function JobDetails:onInput(keys)
    if self:propagateMoveKeys(keys) then
        if df.global.world.selected_building ~= self.building then
            self:dismiss()
        end
    else
        JobDetails.super.onInput(self, keys)
    end
end

if not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/QueryBuilding/Some/Workshop/Job') then
    qerror("This script requires a workshop job selected in the 'q' mode")
end

local dlg = JobDetails{ job = dfhack.gui.getSelectedJob() }
dlg:show()
