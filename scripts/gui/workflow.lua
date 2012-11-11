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

JobConstraints = defclass(JobConstraints, guidm.MenuOverlay)

JobConstraints.focus_path = 'workflow-job'

JobConstraints.ATTRS {
    job = DEFAULT_NIL,
    frame_inset = 1,
    frame_background = COLOR_BLACK,
}

local null_cons = { goal_value = 0, goal_gap = 0, goal_by_count = false }

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
        widgets.Label{
            frame = { l = 0, b = 3 },
            enabled = self:callback('isAnySelected'),
            text = {
                { key = 'BUILDING_TRIGGER_ENABLE_CREATURE',
                  text = function()
                    local cons = self:getCurConstraint() or null_cons
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
                    local cons = self:getCurConstraint() or null_cons
                    return string.format(': Min %-4d ', cons.goal_value - cons.goal_gap)
                  end },
                { key = 'BUILDING_TRIGGER_MAX_SIZE_DOWN',
                  on_activate = self:callback('onIncRange', 'goal_value', -1) },
                { key = 'BUILDING_TRIGGER_MAX_SIZE_UP',
                  on_activate = self:callback('onIncRange', 'goal_value', 5) },
                { text = function()
                    local cons = self:getCurConstraint() or null_cons
                    return string.format(': Max %-4d', cons.goal_value)
                  end },
            }
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
        if cons.min_quality > 0 then
            itemstr = itemstr .. ' ('..df.item_quality[cons.min_quality]..')'
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

function JobConstraints:onChangeUnit()
    local cons = self:getCurConstraint()
    cons.goal_by_count = not cons.goal_by_count
    self:saveConstraint(cons)
end

function JobConstraints:onEditRange()
    local cons = self:getCurConstraint()
    dlg.showInputPrompt(
        'Input Range',
        'Enter the new constraint range:',
        COLOR_WHITE,
        (cons.goal_value-cons.goal_gap)..'-'..cons.goal_value,
        function(text)
            local maxv = string.match(text, '^%s*(%d+)%s*$')
            if maxv then
                cons.goal_value = maxv
                return self:saveConstraint(cons)
            end
            local minv,maxv = string.match(text, '^%s*(%d+)-(%d+)%s*$')
            if minv and maxv and minv ~= maxv then
                cons.goal_value = math.max(minv,maxv)
                cons.goal_gap = math.abs(maxv-minv)
                return self:saveConstraint(cons)
            end
            dlg.showMessage('Invalid Range', 'This range is invalid: '..text, COLOR_LIGHTRED)
        end
    )
end

function JobConstraints:onIncRange(field, delta)
    local cons = self:getCurConstraint()
    if not cons.goal_by_count then
        delta = delta * 5
    end
    cons[field] = math.max(1, cons[field] + delta)
    self:saveConstraint(cons)
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
            self:saveConstraint(item.obj)
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

