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
    local matflags = utils.list_bitfield_flags(iobj.mat_mask)
    if #matflags > 0 then
        matflags = 'any '..table.concat(matflags, '/')
    else
        matflags = nil
    end

    if is_caste_mat(iobj) then
        return 'no material'
    elseif (iobj.mat_type or -1) >= 0 then
        local info = dfhack.matinfo.decode(iobj.mat_type, iobj.mat_index)
        local matline
        if info then
            matline = info:toString()
        else
            matline = iobj.mat_type..':'..iobj.mat_index
        end
        return matline, matflags
    else
        return matflags or 'any material'
    end
end

function current_stock(iobj)
    if iobj.goal_by_count then
        return iobj.cur_count
    else
        return iobj.cur_amount
    end
end

function if_by_count(iobj,bc,ba)
    if iobj.goal_by_count then
        return bc
    else
        return ba
    end
end

function compute_trend(history,field)
    local count = #history
    if count == 0 then
        return 0
    end
    local sumX,sumY,sumXY,sumXX = 0,0,0,0
    for i,v in ipairs(history) do
        sumX = sumX + i
        sumY = sumY + v[field]
        sumXY = sumXY + i*v[field]
        sumXX = sumXX + i*i
    end
    return (count * sumXY - sumX * sumY) / (count * sumXX - sumX * sumX)
end

------------------------
-- RANGE EDITOR GROUP --
------------------------

local null_cons = { goal_value = 0, goal_gap = 0, goal_by_count = false }

RangeEditor = defclass(RangeEditor, widgets.Label)

RangeEditor.ATTRS {
    get_cb = DEFAULT_NIL,
    save_cb = DEFAULT_NIL,
    keys = {
        count = 'CUSTOM_SHIFT_I',
        modify = 'CUSTOM_SHIFT_R',
        min_dec = 'BUILDING_TRIGGER_MIN_SIZE_DOWN',
        min_inc = 'BUILDING_TRIGGER_MIN_SIZE_UP',
        max_dec = 'BUILDING_TRIGGER_MAX_SIZE_DOWN',
        max_inc = 'BUILDING_TRIGGER_MAX_SIZE_UP',
    }
}

function RangeEditor:init(args)
    self:setText{
        { key = self.keys.count,
            text = function()
            local cons = self.get_cb() or null_cons
            if cons.goal_by_count then
                return ': Count stacks  '
            else
                return ': Count items   '
            end
            end,
            on_activate = self:callback('onChangeUnit') },
        { key = self.keys.modify, text = ': Range',
            on_activate = self:callback('onEditRange') },
            NEWLINE, '  ',
        { key = self.keys.min_dec,
            on_activate = self:callback('onIncRange', 'goal_gap', 2) },
        { key = self.keys.min_inc,
            on_activate = self:callback('onIncRange', 'goal_gap', -1) },
        { text = function()
            local cons = self.get_cb() or null_cons
            return string.format(': Min %-4d ', cons.goal_value - cons.goal_gap)
            end },
        { key = self.keys.max_dec,
            on_activate = self:callback('onIncRange', 'goal_value', -1) },
        { key = self.keys.max_inc,
            on_activate = self:callback('onIncRange', 'goal_value', 2) },
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
        delta = delta * 2
    end
    cons[field] = math.max(1, cons[field] + delta*5)
    self.save_cb(cons)
end

---------------------------
-- NEW CONSTRAINT DIALOG --
---------------------------

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
    self.constraint = args.constraint or { item_type = -1 }
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
                { pen = function()
                    if self:isValid() then return COLOR_LIGHTCYAN else return COLOR_LIGHTRED end
                  end,
                  text = function()
                    if self:isValid() then
                        return describe_item_type(self.constraint)
                    else
                        return 'item not set'
                    end
                  end },
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
            frame = { l = 0, t = 14 },
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
            frame = { l = 1, t = 16 },
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
                  enabled = self:callback('isValid'),
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

function NewConstraint:isValid()
    return self.constraint.item_type >= 0 or self.constraint.is_craft
end

function NewConstraint:onChange()
    local token = workflow.constraintToToken(self.constraint)
    local out

    if self:isValid() then
        out = workflow.findConstraint(token)
    end

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

------------------------------
-- CONSTRAINT HISTORY GRAPH --
------------------------------

HistoryGraph = defclass(HistoryGraph, widgets.Widget)

HistoryGraph.ATTRS {
    frame_inset = 1,
    history_pen = COLOR_CYAN,
}

function HistoryGraph:init(info)
end

function HistoryGraph:setData(history, bars)
    self.history = history or {}
    self.bars = bars or {}

    local maxval = 1
    for i,v in ipairs(self.history) do
        maxval = math.max(maxval, v)
    end
    for i,v in ipairs(self.bars) do
        maxval = math.max(maxval, v.value)
    end
    self.max_value = maxval
end

function HistoryGraph:onRenderFrame(dc,rect)
    dc:fill(rect.x1,rect.y1,rect.x1,rect.y2,{ch='\xb3', fg=COLOR_BROWN})
    dc:fill(rect.x1,rect.y2,rect.x2,rect.y2,{ch='\xc4', fg=COLOR_BROWN})
    dc:seek(rect.x1,rect.y1):char('\x1e', COLOR_BROWN)
    dc:seek(rect.x1,rect.y2):char('\xc5', COLOR_BROWN)
    dc:seek(rect.x2,rect.y2):char('\x10', COLOR_BROWN)
    dc:seek(rect.x1,rect.y2-1):char('0', COLOR_BROWN)
end

function HistoryGraph:onRenderBody(dc)
    local coeff = (dc.height-1)/self.max_value

    for i,v in ipairs(self.bars) do
        local y = dc.height-1-math.floor(0.5 + coeff*v.value)
        dc:fill(0,y,dc.width-1,y,v.pen or {ch='-', fg=COLOR_GREEN})
    end

    local xbase = dc.width-1-#self.history
    for i,v in ipairs(self.history) do
        local x = xbase + i
        local y = dc.height-1-math.floor(0.5 + coeff*v)
        dc:seek(x,y):char('*', self.history_pen)
    end
end

------------------------------
-- GLOBAL CONSTRAINT SCREEN --
------------------------------

ConstraintList = defclass(ConstraintList, gui.FramedScreen)

ConstraintList.focus_path = 'workflow/list'

ConstraintList.ATTRS {
    frame_title = 'Workflow Status',
    frame_inset = 0,
    frame_background = COLOR_BLACK,
    frame_style = gui.BOUNDARY_FRAME,
}

function ConstraintList:init(args)
    local fwidth_cb = self:cb_getfield('fwidth')

    self.fwidth = 20
    self.sort_by_severity = false

    self:addviews{
        widgets.Panel{
            frame = { l = 0, r = 31 },
            frame_inset = 1,
            on_layout = function(body)
                self.fwidth = body.width - (12+1+1+7+1+1+1+7)
            end,
            subviews = {
                widgets.Label{
                    frame = { l = 0, t = 0 },
                    text_pen = COLOR_CYAN,
                    text = {
                        { text = 'Item', width = 12 }, ' ',
                        { text = 'Material etc', width = fwidth_cb }, ' ',
                        { text = 'Stock / Limit' },
                    }
                },
                widgets.FilteredList{
                    view_id = 'list',
                    frame = { t = 2, b = 2 },
                    edit_below = true,
                    not_found_label = 'No matching constraints',
                    edit_pen = COLOR_LIGHTCYAN,
                    text_pen = { fg = COLOR_GREY, bg = COLOR_BLACK },
                    cursor_pen = { fg = COLOR_WHITE, bg = COLOR_GREEN },
                    on_select = self:callback('onSelectConstraint'),
                },
                widgets.Label{
                    frame = { b = 0, h = 1 },
                    text = {
                        { key = 'CUSTOM_SHIFT_A', text = ': Add',
                          on_activate = self:callback('onNewConstraint') }, ', ',
                        { key = 'CUSTOM_SHIFT_X', text = ': Delete',
                          on_activate = self:callback('onDeleteConstraint') }, ', ',
                        { key = 'CUSTOM_SHIFT_O', text = ': Severity Order',
                          on_activate = self:callback('onSwitchSort'),
                          pen = function()
                            if self.sort_by_severity then
                              return COLOR_LIGHTCYAN
                            else
                              return COLOR_WHITE
                            end
                          end },
                    }
                }
            }
        },
        widgets.Panel{
            frame = { w = 30, r = 0, h = 6, t = 0 },
            frame_inset = 1,
            subviews = {
                widgets.Label{
                    frame = { l = 0, t = 0 },
                    enabled = self:callback('isAnySelected'),
                    text = {
                        { text = function()
                            local cur = self:getCurConstraint()
                            if cur then
                                return string.format(
                                    'Currently %d (%d in use)',
                                    current_stock(cur),
                                    if_by_count(cur, cur.cur_in_use_count, cur.cur_in_use_amount)
                                )
                            else
                                return 'No constraint selected'
                            end
                          end }
                    }
                },
                RangeEditor{
                    frame = { l = 0, t = 2 },
                    enabled = self:callback('isAnySelected'),
                    get_cb = self:callback('getCurConstraint'),
                    save_cb = self:callback('saveConstraint'),
                    keys = {
                        count = 'CUSTOM_SHIFT_I',
                        modify = 'CUSTOM_SHIFT_R',
                        min_dec = 'SECONDSCROLL_PAGEUP',
                        min_inc = 'SECONDSCROLL_PAGEDOWN',
                        max_dec = 'SECONDSCROLL_UP',
                        max_inc = 'SECONDSCROLL_DOWN',
                    }
                },
            }
        },
        widgets.Widget{
            active = false,
            frame = { w = 1, r = 30 },
            frame_background = gui.BOUNDARY_FRAME.frame_pen,
        },
        widgets.Widget{
            active = false,
            frame = { w = 30, r = 0, h = 1, t = 6 },
            frame_background = gui.BOUNDARY_FRAME.frame_pen,
        },
        HistoryGraph{
            view_id = 'graph',
            frame = { w = 30, r = 0, t = 7, b = 0 },
        }
    }

    self:initListChoices(nil, args.select_token)
end

function stock_trend_color(cons)
    local stock = current_stock(cons)
    if stock >= cons.goal_value - cons.goal_gap then
        return COLOR_LIGHTGREEN, 0
    elseif stock <= cons.goal_gap then
        return COLOR_LIGHTRED, 4
    elseif stock >= cons.goal_value - 2*cons.goal_gap then
        return COLOR_GREEN, 1
    elseif stock <= 2*cons.goal_gap then
        return COLOR_RED, 3
    else
        local trend = if_by_count(cons, cons.trend_count, cons.trend_amount)
        if trend > 0.3 then
            return COLOR_GREEN, 1
        elseif trend < -0.3 then
            return COLOR_RED, 3
        else
            return COLOR_GREY, 2
        end
    end
end

function ConstraintList:initListChoices(clist, sel_token)
    clist = clist or workflow.listConstraints(nil, true)

    local fwidth_cb = self:cb_getfield('fwidth')
    local choices = {}

    for i,cons in ipairs(clist) do
        cons.trend_count = compute_trend(cons.history, 'cur_count')
        cons.trend_amount = compute_trend(cons.history, 'cur_amount')

        local itemstr = describe_item_type(cons)
        local matstr,matflagstr = describe_material(cons)
        if matflagstr then
            matstr = matflagstr .. ' ' .. matstr
        end

        if cons.min_quality > 0 or cons.is_local then
            local lst = {}
            if cons.is_local then
                table.insert(lst, 'local')
            end
            if cons.min_quality > 0 then
                table.insert(lst, string.lower(df.item_quality[cons.min_quality]))
            end
            matstr = matstr .. ' ('..table.concat(lst,',')..')'
        end

        local goal_color = COLOR_GREY
        if #cons.jobs == 0 then
            goal_color = COLOR_RED
        elseif cons.is_delayed then
            goal_color = COLOR_YELLOW
        end

        table.insert(choices, {
            text = {
                { text = itemstr, width = 12, pad_char = ' ' }, ' ',
                { text = matstr, width = fwidth_cb, pad_char = ' ' }, ' ',
                { text = curry(current_stock,cons), width = 7, rjustify = true,
                  pen = function() return { fg = stock_trend_color(cons) } end },
                { text = curry(if_by_count,cons,'S','I'), gap = 1,
                  pen = { fg = COLOR_GREY } },
                { text = function() return cons.goal_value end, gap = 1,
                  pen = { fg = goal_color } }
            },
            severity = select(2, stock_trend_color(cons)),
            search_key = itemstr .. ' | ' .. matstr,
            token = cons.token,
            obj = cons
        })
    end

    self:setChoices(choices, sel_token)
end

function ConstraintList:isAnySelected()
    return self.subviews.list:getSelected() ~= nil
end

function ConstraintList:getCurConstraint()
    local selidx,selobj = self.subviews.list:getSelected()
    if selobj then return selobj.obj end
end

function ConstraintList:onSwitchSort()
    self.sort_by_severity = not self.sort_by_severity
    self:setChoices(self.subviews.list:getChoices())
end

function ConstraintList:setChoices(choices, sel_token)
    if self.sort_by_severity then
        table.sort(choices, function(a,b)
            return a.severity > b.severity
                or (a.severity == b.severity and
                    current_stock(a.obj)/a.obj.goal_value < current_stock(b.obj)/b.obj.goal_value)
        end)
    else
        table.sort(choices, function(a,b) return a.search_key < b.search_key end)
    end

    local selidx = nil
    if sel_token then
        selidx = utils.linear_index(choices, sel_token, 'token')
    end

    local list = self.subviews.list
    local filter = list:getFilter()

    list:setChoices(choices, selidx)

    if filter ~= '' then
        list:setFilter(filter, selidx)

        if selidx and list:getSelected() ~= selidx then
            list:setFilter('', selidx)
        end
    end
end

function ConstraintList:onInput(keys)
    if keys.LEAVESCREEN then
        self:dismiss()
    else
        ConstraintList.super.onInput(self, keys)
    end
end

function ConstraintList:onNewConstraint()
    NewConstraint{
        on_submit = self:callback('saveConstraint')
    }:show()
end

function ConstraintList:saveConstraint(cons)
    local out = workflow.setConstraint(cons.token, cons.goal_by_count, cons.goal_value, cons.goal_gap)
    self:initListChoices(nil, out.token)
end

function ConstraintList:onDeleteConstraint()
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

function ConstraintList:onSelectConstraint(idx,item)
    local history, bars

    if item then
        local cons = item.obj
        local vfield = if_by_count(cons, 'cur_count', 'cur_amount')

        bars = {
            { value = cons.goal_value - cons.goal_gap, pen = {ch='-', fg=COLOR_GREEN} },
            { value = cons.goal_value, pen = {ch='-', fg=COLOR_LIGHTGREEN} },
        }

        history = {}
        for i,v in ipairs(cons.history or {}) do
            table.insert(history, v[vfield])
        end

        table.insert(history, cons[vfield])
    end

    self.subviews.graph:setData(history, bars)
end

-------------------------------
-- WORKSHOP JOB INFO OVERLAY --
-------------------------------

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
                { key = 'CUSTOM_SHIFT_A', text = ': Add limit, ',
                  on_activate = self:callback('onNewConstraint') },
                { key = 'CUSTOM_SHIFT_X', text = ': Delete',
                  enabled = self:callback('isAnySelected'),
                  on_activate = self:callback('onDeleteConstraint') },
                NEWLINE, NEWLINE,
                { key = 'LEAVESCREEN', text = ': Back',
                  on_activate = self:callback('dismiss') },
                '       ',
                { key = 'CUSTOM_SHIFT_S', text = ': Status',
                  on_activate = function()
                    local sel = self:getCurConstraint()
                    ConstraintList{ select_token = (sel or {}).token }:show()
                  end }
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
        local matstr,matflagstr = describe_material(cons)

        table.insert(choices, {
            text = {
                goal, ' ', { text = '(now '..curval..')', pen = order_pen }, NEWLINE,
                '  ', itemstr, NEWLINE, '  ', matstr, NEWLINE, '  ', (matflagstr or '')
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
        local matstr,matflags = describe_material(cons)
        if matflags then
            matstr = matflags..' '..matstr
        end

        table.insert(choices, { text = itemstr..' of '..matstr, obj = cons })
    end

    dlg.ListBox{
        frame_title = 'Add limit',
        text = 'Select one of the possible outputs:',
        text_pen = COLOR_WHITE,
        choices = choices,
        on_select = function(idx,item)
            self:saveConstraint(item.obj)
        end,
        select2_hint = 'Advanced',
        on_select2 = function(idx,item)
            NewConstraint{
                constraint = item.obj,
                on_submit = self:callback('saveConstraint')
            }:show()
        end,
    }:show()
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

local args = {...}

if args[1] == 'status' then
    check_enabled(function() ConstraintList{}:show() end)
else
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
end
