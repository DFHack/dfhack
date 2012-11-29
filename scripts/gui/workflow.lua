-- A GUI front-end for the workflow plugin.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local guimat = require 'gui.materials'
local widgets = require 'gui.widgets'
local dlg = require 'gui.dialogs'

local workflow = require 'plugins.workflow'

function check_enabled(cb)
    if workflow.isEnabled() then
        return cb()
    else
        dlg.showYesNoPrompt(
            'Enable Plugin',
            { 'The workflow plugin is not enabled currently.', NEWLINE, NEWLINE,
              'Press ', { key = 'MENU_CONFIRM' }, ' to enable it.' },
            COLOR_YELLOW,
            function()
                workflow.setEnabled(true)
                return cb()
            end
        )
    end
end

function check_repeat(job, cb)
    if job.flags['repeat'] then
        return cb()
    else
        dlg.showYesNoPrompt(
            'Not Repeat Job',
            { 'Workflow only tracks repeating jobs.', NEWLINE, NEWLINE,
              'Press ', { key = 'MENU_CONFIRM' }, ' to make this one repeat.' },
            COLOR_YELLOW,
            function()
                job.flags['repeat'] = true
                return cb()
            end
        )
    end
end

function describe_item_type(iobj)
    local itemline = 'any item'
    if iobj.is_craft then
        itemline = 'any craft'
    elseif iobj.item_type >= 0 then
        itemline = df.item_type.attrs[iobj.item_type].caption or iobj.item_type
        local subtype = iobj.item_subtype or -1
        local def = dfhack.items.getSubtypeDef(iobj.item_type, subtype)
        local count = dfhack.items.getSubtypeCount(iobj.item_type, subtype)
        if def then
            itemline = def.name
        elseif count >= 0 then
            itemline = 'any '..itemline
        end
    end
    return itemline
end

function is_caste_mat(iobj)
    return dfhack.items.isCasteMaterial(iobj.item_type or -1)
end

function describe_material(iobj)
    local matline = 'any material'
    if is_caste_mat(iobj) then
        matline = 'no material'
    elseif (iobj.mat_type or -1) >= 0 then
        local info = dfhack.matinfo.decode(iobj.mat_type, iobj.mat_index)
        if info then
            matline = info:toString()
        else
            matline = iobj.mat_type..':'..iobj.mat_index
        end
    end
    return matline
end

local null_cons = { goal_value = 0, goal_gap = 0, goal_by_count = false }

RangeEditor = defclass(RangeEditor, widgets.Label)

RangeEditor.ATTRS {
    get_cb = DEFAULT_NIL,
    save_cb = DEFAULT_NIL
}

function RangeEditor:init(args)
    self:setText{
        { key = 'BUILDING_TRIGGER_ENABLE_CREATURE',
            text = function()
            local cons = self.get_cb() or null_cons
            if cons.goal_by_count then
                return ': Count stacks  '
            else
                return ': Count items   '
            end
            end,
            on_activate = self:callback('onChangeUnit') },
        { key = 'BUILDING_TRIGGER_ENABLE_MAGMA', text = ': Modify',
            on_activate = self:callback('onEditRange') },
            NEWLINE, '  ',
        { key = 'BUILDING_TRIGGER_MIN_SIZE_DOWN',
            on_activate = self:callback('onIncRange', 'goal_gap', 5) },
        { key = 'BUILDING_TRIGGER_MIN_SIZE_UP',
            on_activate = self:callback('onIncRange', 'goal_gap', -1) },
        { text = function()
            local cons = self.get_cb() or null_cons
            return string.format(': Min %-4d ', cons.goal_value - cons.goal_gap)
            end },
        { key = 'BUILDING_TRIGGER_MAX_SIZE_DOWN',
            on_activate = self:callback('onIncRange', 'goal_value', -1) },
        { key = 'BUILDING_TRIGGER_MAX_SIZE_UP',
            on_activate = self:callback('onIncRange', 'goal_value', 5) },
        { text = function()
            local cons = self.get_cb() or null_cons
            return string.format(': Max %-4d', cons.goal_value)
            end },
    }
end

function RangeEditor:onChangeUnit()
    local cons = self.get_cb()
    cons.goal_by_count = not cons.goal_by_count
    self.save_cb(cons)
end

function RangeEditor:onEditRange()
    local cons = self.get_cb()
    dlg.showInputPrompt(
        'Input Range',
        'Enter the new constraint range:',
        COLOR_WHITE,
        (cons.goal_value-cons.goal_gap)..'-'..cons.goal_value,
        function(text)
            local maxv = string.match(text, '^%s*(%d+)%s*$')
            if maxv then
                cons.goal_value = maxv
                return self.save_cb(cons)
            end
            local minv,maxv = string.match(text, '^%s*(%d+)-(%d+)%s*$')
            if minv and maxv and minv ~= maxv then
                cons.goal_value = math.max(minv,maxv)
                cons.goal_gap = math.abs(maxv-minv)
                return self.save_cb(cons)
            end
            dlg.showMessage('Invalid Range', 'This range is invalid: '..text, COLOR_LIGHTRED)
        end
    )
end

function RangeEditor:onIncRange(field, delta)
    local cons = self.get_cb()
    if not cons.goal_by_count then
        delta = delta * 5
    end
    cons[field] = math.max(1, cons[field] + delta)
    self.save_cb(cons)
end

NewConstraint = defclass(NewConstraint, gui.FramedScreen)

NewConstraint.focus_path = 'workflow/new'

NewConstraint.ATTRS {
    frame_style = gui.GREY_LINE_FRAME,
    frame_title = 'New workflow constraint',
    frame_width = 39,
    frame_height = 20,
    frame_inset = 1,
    constraint = DEFAULT_NIL,
    on_submit = DEFAULT_NIL,
}

function NewConstraint:init(args)
    self.constraint = args.constraint or {}
    rawset_default(self.constraint, { goal_value = 10, goal_gap = 5, goal_by_count = false })

    local matlist = {}
    local matsel = 1
    local matmask = self.constraint.mat_mask

    for i,v in ipairs(df.dfhack_material_category) do
        if v and v ~= 'wood2' then
            table.insert(matlist, { icon = self:callback('isMatSelected', v), text = v })
            if matmask and matmask[v] and matsel == 1 then
                matsel = #matlist
            end
        end
    end

    self:addviews{
        widgets.Label{
            frame = { l = 0, t = 0 },
            text = 'Items matching:'
        },
        widgets.Label{
            frame = { l = 1, t = 2, w = 26 },
            text = {
                'Type: ',
                { pen = COLOR_LIGHTCYAN,
                  text = function() return describe_item_type(self.constraint) end },
                NEWLINE, '  ',
                { key = 'CUSTOM_T', text = ': Select, ',
                  on_activate = self:callback('chooseType') },
                { key = 'CUSTOM_SHIFT_C', text = ': Crafts',
                  on_activate = self:callback('chooseCrafts') },
                NEWLINE, NEWLINE,
                'Material: ',
                { pen = COLOR_LIGHTCYAN,
                  text = function() return describe_material(self.constraint) end },
                NEWLINE, '  ',
                { key = 'CUSTOM_P', text = ': Specific',
                  on_activate = self:callback('chooseMaterial') },
                NEWLINE, NEWLINE,
                'Other:',
                NEWLINE, ' ',
                { key = 'D_MILITARY_SUPPLIES_WATER_DOWN',
                  on_activate = self:callback('incQuality', -1) },
                { key = 'D_MILITARY_SUPPLIES_WATER_UP', key_sep = ': ',
                  text = function()
                    return df.item_quality[self.constraint.min_quality or 0]..' quality'
                  end,
                  on_activate = self:callback('incQuality', 1) },
                NEWLINE, '  ',
                { key = 'CUSTOM_L', key_sep = ': ',
                  text = function()
                    if self.constraint.is_local then
                        return 'Locally made only'
                    else
                        return 'Include foreign'
                    end
                  end,
                  on_activate = self:callback('toggleLocal') },
            }
        },
        widgets.Label{
            frame = { l = 0, t = 13 },
            text = {
                'Desired range: ',
                { pen = COLOR_LIGHTCYAN,
                  text = function()
                    local cons = self.constraint
                    local goal = (cons.goal_value-cons.goal_gap)..'-'..cons.goal_value
                    if cons.goal_by_count then
                        return goal .. ' stacks'
                    else
                        return goal .. ' items'
                    end
                  end },
            }
        },
        RangeEditor{
            frame = { l = 1, t = 15 },
            get_cb = self:cb_getfield('constraint'),
            save_cb = self:callback('onRangeChange'),
        },
        widgets.Label{
            frame = { l = 30, t = 0 },
            text = 'Mat class'
        },
        widgets.List{
            view_id = 'matlist',
            frame = { l = 30, t = 2, w = 9, h = 18 },
            scroll_keys = widgets.STANDARDSCROLL,
            choices = matlist,
            selected = matsel,
            on_submit = self:callback('onToggleMatclass')
        },
        widgets.Label{
            frame = { l = 0, b = 0, w = 29 },
            text = {
                { key = 'LEAVESCREEN', text = ': Cancel, ',
                  on_activate = self:callback('dismiss') },
                { key = 'MENU_CONFIRM', key_sep = ': ',
                  text = function()
                    if self.is_existing then return 'Update' else return 'Create new' end
                  end,
                  on_activate = function()
                    self:dismiss()
                    if self.on_submit then
                        self.on_submit(self.constraint)
                    end
                  end },
            }
        },
    }
end

function NewConstraint:postinit()
    self:onChange()
end

function NewConstraint:onChange()
    local token = workflow.constraintToToken(self.constraint)
    local out = workflow.findConstraint(token)

    if out then
        self.constraint = out
        self.is_existing = true
    else
        self.constraint.token = token
        self.is_existing = false
    end
end

function NewConstraint:chooseType()
    guimat.ItemTypeDialog{
        prompt = 'Please select a new item type',
        hide_none = true,
        on_select = function(itype,isub)
            local cons = self.constraint
            cons.item_type = itype
            cons.item_subtype = isub
            cons.is_craft = nil
            self:onChange()
        end
    }:show()
end

function NewConstraint:chooseCrafts()
    local cons = self.constraint
    cons.item_type = -1
    cons.item_subtype = -1
    cons.is_craft = true
    self:onChange()
end

function NewConstraint:chooseMaterial()
    local cons = self.constraint
    guimat.MaterialDialog{
        prompt = 'Please select a new material',
        none_caption = 'any material',
        frame_width = 37,
        on_select = function(mat_type, mat_index)
            local cons = self.constraint
            cons.mat_type = mat_type
            cons.mat_index = mat_index
            cons.mat_mask = nil
            self:onChange()
        end
    }:show()
end

function NewConstraint:incQuality(diff)
    local cons = self.constraint
    local nq = (cons.min_quality or 0) + diff
    if nq < 0 then
        nq = df.item_quality.Masterful
    elseif nq > df.item_quality.Masterful then
        nq = 0
    end
    cons.min_quality = nq
    self:onChange()
end

function NewConstraint:toggleLocal()
    local cons = self.constraint
    cons.is_local = not cons.is_local
    self:onChange()
end

function NewConstraint:isMatSelected(token)
    if self.constraint.mat_mask and self.constraint.mat_mask[token] then
        return { ch = '\xfb', fg = COLOR_LIGHTGREEN }
    else
        return nil
    end
end

function NewConstraint:onToggleMatclass(idx,obj)
    local cons = self.constraint
    if cons.mat_mask and cons.mat_mask[obj.text] then
        cons.mat_mask[obj.text] = false
    else
        cons.mat_mask = cons.mat_mask or {}
        cons.mat_mask[obj.text] = true
        cons.mat_type = -1
        cons.mat_index = -1
    end
    self:onChange()
end

function NewConstraint:onRangeChange()
    local cons = self.constraint
    cons.goal_gap = math.max(1, math.min(cons.goal_gap, cons.goal_value-1))
end

JobConstraints = defclass(JobConstraints, guidm.MenuOverlay)

JobConstraints.focus_path = 'workflow/job'

JobConstraints.ATTRS {
    job = DEFAULT_NIL,
    frame_inset = 1,
    frame_background = COLOR_BLACK,
}

function JobConstraints:init(args)
    self.building = dfhack.job.getHolder(self.job)

    self:addviews{
        widgets.Label{
            frame = { l = 0, t = 0 },
            text = {
                'Workflow Constraints'
            }
        },
        widgets.List{
            view_id = 'list',
            frame = { t = 2, b = 6 },
            row_height = 4,
            scroll_keys = widgets.SECONDSCROLL,
        },
        RangeEditor{
            frame = { l = 0, b = 3 },
            enabled = self:callback('isAnySelected'),
            get_cb = self:callback('getCurConstraint'),
            save_cb = self:callback('saveConstraint'),
        },
        widgets.Label{
            frame = { l = 0, b = 0 },
            text = {
                { key = 'CUSTOM_N', text = ': New limit, ',
                  on_activate = self:callback('onNewConstraint') },
                { key = 'CUSTOM_X', text = ': Delete',
                  enabled = self:callback('isAnySelected'),
                  on_activate = self:callback('onDeleteConstraint') },
                NEWLINE, NEWLINE,
                { key = 'LEAVESCREEN', text = ': Back',
                  on_activate = self:callback('dismiss') }
            }
        },
    }

    self:initListChoices(args.clist)
end

function JobConstraints:onGetSelectedBuilding()
    return self.building
end

function JobConstraints:onGetSelectedJob()
    return self.job
end

function JobConstraints:initListChoices(clist, sel_token)
    clist = clist or workflow.listConstraints(self.job)

    local choices = {}

    for i,cons in ipairs(clist) do
        local goal = (cons.goal_value-cons.goal_gap)..'-'..cons.goal_value
        local curval
        if cons.goal_by_count then
            goal = goal .. ' stacks'
            curval = cons.cur_count
        else
            goal = goal .. ' items'
            curval = cons.cur_amount
        end
        local order_pen = COLOR_GREY
        if cons.request == 'resume' then
            order_pen = COLOR_GREEN
        elseif cons.request == 'suspend' then
            order_pen = COLOR_BLUE
        end
        local itemstr = describe_item_type(cons)
        if cons.min_quality > 0 or cons.is_local then
            local lst = {}
            if cons.is_local then
                table.insert(lst, 'local')
            end
            if cons.min_quality > 0 then
                table.insert(lst, string.lower(df.item_quality[cons.min_quality]))
            end
            itemstr = itemstr .. ' ('..table.concat(lst,',')..')'
        end
        local matstr = describe_material(cons)
        local matflagstr = ''
        local matflags = utils.list_bitfield_flags(cons.mat_mask)
        if #matflags > 0 then
            matflags[1] = 'any '..matflags[1]
            if matstr == 'any material' then
                matstr = table.concat(matflags, ', ')
                matflags = {}
            end
        end
        if #matflags > 0 then
            matflagstr = table.concat(matflags, ', ')
        end

        table.insert(choices, {
            text = {
                goal, ' ', { text = '(now '..curval..')', pen = order_pen }, NEWLINE,
                '  ', itemstr, NEWLINE, '  ', matstr, NEWLINE, '  ', matflagstr
            },
            token = cons.token,
            obj = cons
        })
    end

    local selidx = nil
    if sel_token then
        selidx = utils.linear_index(choices, sel_token, 'token')
    end

    self.subviews.list:setChoices(choices, selidx)
end

function JobConstraints:isAnySelected()
    return self.subviews.list:getSelected() ~= nil
end

function JobConstraints:getCurConstraint()
    local i,v = self.subviews.list:getSelected()
    if v then return v.obj end
end

function JobConstraints:saveConstraint(cons)
    local out = workflow.setConstraint(cons.token, cons.goal_by_count, cons.goal_value, cons.goal_gap)
    self:initListChoices(nil, out.token)
end

function JobConstraints:onNewConstraint()
    local outputs = workflow.listJobOutputs(self.job)
    if #outputs == 0 then
        dlg.showMessage('Unsupported', 'Workflow cannot guess the outputs of this job.', COLOR_LIGHTRED)
        return
    end

    local variants = workflow.listWeakenedConstraints(outputs)

    local choices = {}
    for i,cons in ipairs(variants) do
        local itemstr = describe_item_type(cons)
        local matstr = describe_material(cons)
        local matflags = utils.list_bitfield_flags(cons.mat_mask)
        if #matflags > 0 then
            local fstr = table.concat(matflags, '/')
            if matstr == 'any material' then
                matstr = 'any '..fstr
            else
                matstr = 'any '..fstr..' '..matstr
            end
        end

        table.insert(choices, { text = itemstr..' of '..matstr, obj = cons })
    end

    dlg.showListPrompt(
        'New limit',
        'Select one of the possible outputs:',
        COLOR_WHITE,
        choices,
        function(idx,item)
            NewConstraint{
                constraint = item.obj,
                on_submit = self:callback('saveConstraint')
            }:show()
        end
    )
end

function JobConstraints:onDeleteConstraint()
    local cons = self:getCurConstraint()
    dlg.showYesNoPrompt(
        'Delete Constraint',
        'Really delete the current constraint?',
        COLOR_YELLOW,
        function()
            workflow.deleteConstraint(cons.token)
            self:initListChoices()
        end
    )
end

function JobConstraints:onInput(keys)
    if self:propagateMoveKeys(keys) then
        if df.global.world.selected_building ~= self.building then
            self:dismiss()
        end
    else
        JobConstraints.super.onInput(self, keys)
    end
end

if not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/QueryBuilding/Some/Workshop/Job') then
    qerror("This script requires a workshop job selected in the 'q' mode")
end

local job = dfhack.gui.getSelectedJob()

check_enabled(function()
    check_repeat(job, function()
        local clist = workflow.listConstraints(job)
        if not clist then
            dlg.showMessage('Not Supported', 'This type of job is not supported by workflow.', COLOR_LIGHTRED)
            return
        end
        JobConstraints{ job = job, clist = clist }:show()
    end)
end)

