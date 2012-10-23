-- A GUI front-end for the workflow plugin.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local guimat = require 'gui.materials'
local widgets = require 'gui.widgets'
local dlg = require 'gui.dialogs'

local workflow = require 'plugins.workflow'

function check_enabled(cb,...)
    if workflow.isEnabled() then
        return cb(...)
    else
        dlg.showYesNoPrompt(
            'Enable Plugin',
            { 'The workflow plugin is not enabled currently.', NEWLINE, NEWLINE
              'Press ', { key = 'MENU_CONFIRM' }, ' to enable it.' },
            COLOR_YELLOW,
            curry(function(...)
                workflow.setEnabled(true)
                return cb(...)
            end,...)
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

function JobConstraints:init(args)
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
                'Workflow Constraints'
            }
        },
        widgets.List{
            view_id = 'list',
            frame = { t = 2, b = 2 },
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

function JobConstraints:initListChoices(clist)
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
            order_pen = COLOR_RED
        end
        local itemstr
        if cons.is_craft then
            itemstr = 'any craft'
        else
            itemstr = describe_item_type(cons)
        end
        if cons.min_quality > 0 then
            itemstr = itemstr .. ' ('..df.item_quality[cons.min_quality]..')'
        end
        local matstr = describe_material(cons)
        local matflagstr = ''
        local matflags = {}
        list_flags(matflags, cons.mat_mask)
        if #matflags > 0 then
            matflagstr = 'class: '..table.concat(matflags, ', ')
        end

        table.insert(choices, {
            text = {
                goal, ' ', { text = '(now '..curval..')', pen = order_pen }, NEWLINE,
                '  ', itemstr, NEWLINE, '  ', matstr, NEWLINE, '  ', matflagstr
            },
            obj = cons
        })
    end

    self.subviews.list:setChoices(choices)
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

check_enabled(function()
    local job = dfhack.gui.getSelectedJob()
    if not job.flags['repeat'] then
        dlg.showMessage('Not Supported', 'Workflow only tracks repeat jobs.', COLOR_LIGHTRED)
        return
    end
    local clist = workflow.listConstraints(job)
    if not clist then
        dlg.showMessage('Not Supported', 'This type of job is not supported by workflow.', COLOR_LIGHTRED)
        return
    end
    JobConstraints{ job = job, clist = clist }:show()
end)

