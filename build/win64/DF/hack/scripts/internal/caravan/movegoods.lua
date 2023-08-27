--@ module = true

local common = reqscript('internal/caravan/common')
local gui = require('gui')
local overlay = require('plugins.overlay')
local utils = require('utils')
local widgets = require('gui.widgets')

-- -------------------
-- MoveGoods
--

MoveGoods = defclass(MoveGoods, widgets.Window)
MoveGoods.ATTRS {
    frame_title='Move goods to/from depot',
    frame={w=84, h=46},
    resizable=true,
    resize_min={h=35},
    pending_item_ids=DEFAULT_NIL,
    depot=DEFAULT_NIL,
}

local STATUS_COL_WIDTH = 7
local VALUE_COL_WIDTH = 6
local QTY_COL_WIDTH = 5

local function sort_noop(a, b)
    -- this function is used as a marker and never actually gets called
    error('sort_noop should not be called')
end

local function sort_base(a, b)
    return a.data.desc < b.data.desc
end

local function sort_by_name_desc(a, b)
    if a.search_key == b.search_key then
        return sort_base(a, b)
    end
    return a.search_key < b.search_key
end

local function sort_by_name_asc(a, b)
    if a.search_key == b.search_key then
        return sort_base(a, b)
    end
    return a.search_key > b.search_key
end

local function sort_by_value_desc(a, b)
    local value_field = a.item_id and 'per_item_value' or 'total_value'
    if a.data[value_field] == b.data[value_field] then
        return sort_by_name_desc(a, b)
    end
    return a.data[value_field] > b.data[value_field]
end

local function sort_by_value_asc(a, b)
    local value_field = a.item_id and 'per_item_value' or 'total_value'
    if a.data[value_field] == b.data[value_field] then
        return sort_by_name_desc(a, b)
    end
    return a.data[value_field] < b.data[value_field]
end

local function sort_by_status_desc(a, b)
    local a_unselected = a.data.selected == 0 or (a.item_id and not a.data.items[a.item_id].pending)
    local b_unselected = b.data.selected == 0 or (b.item_id and not b.data.items[b.item_id].pending)
    if a_unselected == b_unselected then
        return sort_by_value_desc(a, b)
    end
    return not a_unselected
end

local function sort_by_status_asc(a, b)
    local a_unselected = a.data.selected == 0 or (a.item_id and not a.data.items[a.item_id].pending)
    local b_unselected = b.data.selected == 0 or (b.item_id and not b.data.items[b.item_id].pending)
    if a_unselected == b_unselected then
        return sort_by_value_desc(a, b)
    end
    return not b_unselected
end

local function sort_by_quantity_desc(a, b)
    if a.data.quantity == b.data.quantity then
        return sort_by_value_desc(a, b)
    end
    return a.data.quantity > b.data.quantity
end

local function sort_by_quantity_asc(a, b)
    if a.data.quantity == b.data.quantity then
        return sort_by_value_desc(a, b)
    end
    return a.data.quantity < b.data.quantity
end

local function is_active_caravan(caravan)
    if caravan.flags.tribute then return false end
    local trade_state = caravan.trade_state
    return caravan.time_remaining > 0 and
        (trade_state == df.caravan_state.T_trade_state.Approaching or
         trade_state == df.caravan_state.T_trade_state.AtDepot)
end

local function get_ethics_restrictions()
    local animal_ethics, wood_ethics = false, false
    for _,caravan in ipairs(df.global.plotinfo.caravans) do
        if is_active_caravan(caravan) then
            animal_ethics = animal_ethics or common.is_animal_lover_caravan(caravan)
            wood_ethics = wood_ethics or common.is_tree_lover_caravan(caravan)
        end
    end
    return animal_ethics, wood_ethics
end

local function get_export_agreements()
    local export_agreements = {}
    for _,caravan in ipairs(df.global.plotinfo.caravans) do
        if caravan.buy_prices and is_active_caravan(caravan) then
            table.insert(export_agreements, caravan.buy_prices)
        end
    end
    return export_agreements
end

function MoveGoods:init()
    self.value_pending = 0

    self.animal_ethics, self.wood_ethics = get_ethics_restrictions()
    self.banned_items = common.get_banned_items()
    self.risky_items = common.get_risky_items(self.banned_items)

    self:addviews{
        widgets.CycleHotkeyLabel{
            view_id='sort',
            frame={l=0, t=0, w=21},
            label='Sort by:',
            key='CUSTOM_SHIFT_S',
            options={
                {label='status'..common.CH_DN, value=sort_by_status_desc},
                {label='status'..common.CH_UP, value=sort_by_status_asc},
                {label='value'..common.CH_DN, value=sort_by_value_desc},
                {label='value'..common.CH_UP, value=sort_by_value_asc},
                {label='qty'..common.CH_DN, value=sort_by_quantity_desc},
                {label='qty'..common.CH_UP, value=sort_by_quantity_asc},
                {label='name'..common.CH_DN, value=sort_by_name_desc},
                {label='name'..common.CH_UP, value=sort_by_name_asc},
            },
            initial_option=sort_by_status_desc,
            on_change=self:callback('refresh_list', 'sort'),
        },
        widgets.EditField{
            view_id='search',
            frame={l=26, t=0},
            label_text='Search: ',
            on_char=function(ch) return ch:match('[%l -]') end,
        },
        widgets.Panel{
            frame={t=2, l=0, w=38, h=14},
            subviews=common.get_slider_widgets(self),
        },
        widgets.ToggleHotkeyLabel{
            view_id='hide_forbidden',
            frame={t=2, l=40, w=28},
            label='Hide forbidden items:',
            key='CUSTOM_SHIFT_F',
            options={
                {label='Yes', value=true, pen=COLOR_GREEN},
                {label='No', value=false}
            },
           initial_option=false,
            on_change=function() self:refresh_list() end,
        },
        widgets.Panel{
            frame={t=4, l=40, r=0, h=12},
            subviews=common.get_info_widgets(self, get_export_agreements()),
        },
        widgets.Panel{
            frame={t=17, l=0, r=0, b=6},
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='sort_status',
                    frame={t=0, l=STATUS_COL_WIDTH+1-7, w=7},
                    options={
                        {label='status', value=sort_noop},
                        {label='status'..common.CH_DN, value=sort_by_status_desc},
                        {label='status'..common.CH_UP, value=sort_by_status_asc},
                    },
                    initial_option=sort_by_status_desc,
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_status'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_value',
                    frame={t=0, l=STATUS_COL_WIDTH+2+VALUE_COL_WIDTH+1-6, w=6},
                    options={
                        {label='value', value=sort_noop},
                        {label='value'..common.CH_DN, value=sort_by_value_desc},
                        {label='value'..common.CH_UP, value=sort_by_value_asc},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_value'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_quantity',
                    frame={t=0, l=STATUS_COL_WIDTH+2+VALUE_COL_WIDTH+2+QTY_COL_WIDTH+1-4, w=4},
                    options={
                        {label='qty', value=sort_noop},
                        {label='qty'..common.CH_DN, value=sort_by_quantity_desc},
                        {label='qty'..common.CH_UP, value=sort_by_quantity_asc},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_quantity'),
                },
                widgets.CycleHotkeyLabel{
                    view_id='sort_name',
                    frame={t=0, l=STATUS_COL_WIDTH+2+VALUE_COL_WIDTH+2+QTY_COL_WIDTH+2, w=5},
                    options={
                        {label='name', value=sort_noop},
                        {label='name'..common.CH_DN, value=sort_by_name_desc},
                        {label='name'..common.CH_UP, value=sort_by_name_asc},
                    },
                    option_gap=0,
                    on_change=self:callback('refresh_list', 'sort_name'),
                },
                widgets.FilteredList{
                    view_id='list',
                    frame={l=0, t=2, r=0, b=0},
                    icon_width=2,
                    on_submit=self:callback('toggle_item'),
                    on_submit2=self:callback('toggle_range'),
                    on_select=self:callback('select_item'),
                },
            }
        },
        widgets.Label{
            frame={l=0, b=4, h=1, r=0},
            text={
                'Total value of trade items:',
                {gap=1,
                 text=function() return common.obfuscate_value(self.value_pending) end},
            },
        },
        widgets.HotkeyLabel{
            frame={l=0, b=2},
            label='Select all/none',
            key='CUSTOM_CTRL_A',
            on_activate=self:callback('toggle_visible'),
            auto_width=true,
        },
        widgets.ToggleHotkeyLabel{
            view_id='disable_buckets',
            frame={l=26, b=2},
            label='Show individual items:',
            key='CUSTOM_CTRL_I',
            options={
                {label='Yes', value=true, pen=COLOR_GREEN},
                {label='No', value=false}
            },
            initial_option=false,
            on_change=function() self:refresh_list() end,
        },
        widgets.WrappedLabel{
            frame={b=0, l=0, r=0},
            text_to_wrap='Click to mark/unmark for trade. Shift click to mark/unmark a range of items.',
        },
    }

    -- replace the FilteredList's built-in EditField with our own
    self.subviews.list.list.frame.t = 0
    self.subviews.list.edit.visible = false
    self.subviews.list.edit = self.subviews.search
    self.subviews.search.on_change = self.subviews.list:callback('onFilterChange')

    self.subviews.list:setChoices(self:get_choices())
end

function MoveGoods:refresh_list(sort_widget, sort_fn)
    sort_widget = sort_widget or 'sort'
    sort_fn = sort_fn or self.subviews.sort:getOptionValue()
    if sort_fn == sort_noop then
        self.subviews[sort_widget]:cycle()
        return
    end
    for _,widget_name in ipairs{'sort', 'sort_status', 'sort_value', 'sort_quantity', 'sort_name'} do
        self.subviews[widget_name]:setOption(sort_fn)
    end
    local list = self.subviews.list
    local saved_filter = list:getFilter()
    list:setFilter('')
    list:setChoices(self:get_choices(), list:getSelected())
    list:setFilter(saved_filter)
end

local function is_tradeable_item(item, depot)
    if item.flags.hostile or
        item.flags.in_inventory or
        item.flags.removed or
        item.flags.dead_dwarf or
        item.flags.spider_web or
        item.flags.construction or
        item.flags.encased or
        item.flags.unk12 or
        item.flags.murder or
        item.flags.trader or
        item.flags.owned or
        item.flags.garbage_collect or
        item.flags.on_fire or
        item.flags.in_chest
    then
        return false
    end
    if item.flags.in_job then
        local spec_ref = dfhack.items.getSpecificRef(item, df.specific_ref_type.JOB)
        if not spec_ref then return true end
        return spec_ref.data.job.job_type == df.job_type.BringItemToDepot
    end
    if item.flags.in_building then
        if dfhack.items.getHolderBuilding(item) ~= depot then return false end
        for _, contained_item in ipairs(depot.contained_items) do
            if contained_item.use_mode == 0 then return true end
            -- building construction materials
            if item == contained_item.item then return false end
        end
    end
    return dfhack.maps.canWalkBetween(xyz2pos(dfhack.items.getPosition(item)),
        xyz2pos(depot.centerx, depot.centery, depot.z))
end

local function get_entry_icon(data, item_id)
    if data.selected == 0 then return nil end
    if item_id then
        return data.items[item_id].pending and common.ALL_PEN or nil
    end
    if data.quantity == data.selected then return common.ALL_PEN end
    return common.SOME_PEN
end

local function make_choice_text(at_depot, value, quantity, desc)
    return {
        {width=STATUS_COL_WIDTH-2, text=at_depot and 'depot' or ''},
        {gap=2, width=VALUE_COL_WIDTH, rjustify=true, text=common.obfuscate_value(value)},
        {gap=2, width=QTY_COL_WIDTH, rjustify=true, text=quantity},
        {gap=2, text=desc},
    }
end

local function is_ethical_product(item, animal_ethics, wood_ethics)
    if not animal_ethics and not wood_ethics then return true end
    if item.flags.container then
        local contained_items = dfhack.items.getContainedItems(item)
        if df.item_binst:is_instance(item) then
            -- ignore the safety of the bin itself (unless the bin is empty)
            -- so items inside can still be traded
            local has_items = false
            for _, contained_item in ipairs(contained_items) do
                has_items = true
                if (not animal_ethics or not contained_item:isAnimalProduct()) and
                    (not wood_ethics or not common.has_wood(contained_item))
                then
                    -- bin passes if at least one contained item is safe
                    return true
                end
            end
            if has_items then
                -- no contained items are safe
                return false
            end
        else
            -- for other types of containers, any contamination makes it untradeable
            for _, contained_item in ipairs(contained_items) do
                if (animal_ethics and contained_item:isAnimalProduct()) or
                    (wood_ethics and common.has_wood(contained_item))
                then
                    return false
                end
            end
        end
    end

    return (not animal_ethics or not item:isAnimalProduct()) and
        (not wood_ethics or not common.has_wood(item))
end

function MoveGoods:cache_choices(disable_buckets)
    if self.choices then return self.choices[disable_buckets] end

    local pending = self.pending_item_ids
    local buckets = {}
    for _, item in ipairs(df.global.world.items.all) do
        local item_id = item.id
        if not item or not is_tradeable_item(item, self.depot) then goto continue end
        local value = common.get_perceived_value(item)
        if value <= 0 then goto continue end
        local is_pending = not not pending[item_id] or item.flags.in_building
        local is_forbidden = item.flags.forbid
        local is_banned, is_risky = common.scan_banned(item, self.risky_items)
        local is_requested = dfhack.items.isRequestedTradeGood(item)
        local wear_level = item:getWear()
        local desc = common.get_item_description(item)
        local key = ('%s/%d'):format(desc, value)
        if buckets[key] then
            local bucket = buckets[key]
            bucket.data.items[item_id] = {item=item, pending=is_pending, banned=is_banned, requested=is_requested}
            bucket.data.quantity = bucket.data.quantity + 1
            bucket.data.selected = bucket.data.selected + (is_pending and 1 or 0)
            bucket.data.num_at_depot = bucket.data.num_at_depot + (item.flags.in_building and 1 or 0)
            bucket.data.has_forbidden = bucket.data.has_forbidden or is_forbidden
            bucket.data.has_banned = bucket.data.has_banned or is_banned
            bucket.data.has_risky = bucket.data.has_risky or is_risky
            bucket.data.has_requested = bucket.data.has_requested or is_requested
        else
            local is_ethical = is_ethical_product(item, self.animal_ethics, self.wood_ethics)
            local data = {
                desc=desc,
                per_item_value=value,
                item=item, -- a representative item that we can use for filtering later
                items={[item_id]={item=item, pending=is_pending, banned=is_banned, risky=is_risky, requested=is_requested}},
                item_type=item:getType(),
                item_subtype=item:getSubtype(),
                quantity=1,
                quality=item.flags.artifact and 6 or item:getQuality(),
                wear=wear_level,
                selected=is_pending and 1 or 0,
                num_at_depot=item.flags.in_building and 1 or 0,
                has_forbidden=is_forbidden,
                has_banned=is_banned,
                has_risky=is_risky,
                has_requested=is_requested,
                ethical=is_ethical,
                dirty=false,
            }
            local entry = {
                search_key=common.make_search_key(desc),
                icon=curry(get_entry_icon, data),
                data=data,
            }
            buckets[key] = entry
        end
        ::continue::
    end

    local bucket_choices, nobucket_choices = {}, {}
    for _, bucket in pairs(buckets) do
        local data = bucket.data
        for item_id, item_data in pairs(data.items) do
            local nobucket_choice = copyall(bucket)
            nobucket_choice.icon = curry(get_entry_icon, data, item_id)
            nobucket_choice.text = make_choice_text(item_data.item.flags.in_building, data.per_item_value, 1, data.desc)
            nobucket_choice.item_id = item_id
            table.insert(nobucket_choices, nobucket_choice)
        end
        data.total_value = data.per_item_value * data.quantity
        bucket.text = make_choice_text(data.num_at_depot == data.quantity, data.total_value, data.quantity, data.desc)
        table.insert(bucket_choices, bucket)
        self.value_pending = self.value_pending + (data.per_item_value * data.selected)
    end

    self.choices = {}
    self.choices[false] = bucket_choices
    self.choices[true] = nobucket_choices
    return self:cache_choices(disable_buckets)
end

function MoveGoods:get_choices()
    local raw_choices = self:cache_choices(self.subviews.disable_buckets:getOptionValue())
    local choices = {}
    local include_forbidden = not self.subviews.hide_forbidden:getOptionValue()
    local banned = self.subviews.banned:getOptionValue()
    local only_agreement = self.subviews.only_agreement:getOptionValue()
    local ethical = self.subviews.ethical:getOptionValue()
    local min_condition = self.subviews.min_condition:getOptionValue()
    local max_condition = self.subviews.max_condition:getOptionValue()
    local min_quality = self.subviews.min_quality:getOptionValue()
    local max_quality = self.subviews.max_quality:getOptionValue()
    local min_value = self.subviews.min_value:getOptionValue().value
    local max_value = self.subviews.max_value:getOptionValue().value
    for _,choice in ipairs(raw_choices) do
        local data = choice.data
        if ethical ~= 'show' then
            if ethical == 'hide' and data.ethical then goto continue end
            if ethical == 'only' and not data.ethical then goto continue end
        end
        if not include_forbidden then
            if choice.item_id then
                if data.items[choice.item_id].item.flags.forbid then
                    goto continue
                end
            elseif data.has_forbidden then
                goto continue
            end
        end
        if min_condition < data.wear then goto continue end
        if max_condition > data.wear then goto continue end
        if min_quality > data.quality then goto continue end
        if max_quality < data.quality then goto continue end
        if min_value > data.per_item_value then goto continue end
        if max_value < data.per_item_value then goto continue end
        if only_agreement then
            if choice.item_id then
                if not data.items[choice.item_id].requested then
                    goto continue
                end
            elseif not data.has_requested then
                goto continue
            end
        end
        if banned ~= 'ignore' then
            if choice.item_id then
                if data.items[choice.item_id].banned or (banned ~= 'banned_only' and data.items[choice.item_id].risky) then
                    goto continue
                end
            elseif data.has_banned or (banned ~= 'banned_only' and data.has_risky) then
                goto continue
            end
        end
        table.insert(choices, choice)
        ::continue::
    end
    table.sort(choices, self.subviews.sort:getOptionValue())
    return choices
end

function MoveGoods:toggle_item_base(choice, target_value)
    if choice.item_id then
        local item_data = choice.data.items[choice.item_id]
        if item_data.pending then
            self.value_pending = self.value_pending - choice.data.per_item_value
            choice.data.selected = choice.data.selected - 1
        end
        if target_value == nil then target_value = not item_data.pending end
        item_data.pending = target_value
        if item_data.pending then
            self.value_pending = self.value_pending + choice.data.per_item_value
            choice.data.selected = choice.data.selected + 1
        end
    else
        self.value_pending = self.value_pending - (choice.data.selected * choice.data.per_item_value)
        if target_value == nil then target_value = (choice.data.selected ~= choice.data.quantity) end
        for _, item_data in pairs(choice.data.items) do
            item_data.pending = target_value
        end
        choice.data.selected = target_value and choice.data.quantity or 0
        self.value_pending = self.value_pending + (choice.data.selected * choice.data.per_item_value)
    end
    choice.data.dirty = true
    return target_value
end

function MoveGoods:select_item(idx, choice)
    if not dfhack.internal.getModifiers().shift then
        self.prev_list_idx = self.subviews.list.list:getSelected()
    end
end

function MoveGoods:toggle_item(idx, choice)
    self:toggle_item_base(choice)
end

function MoveGoods:toggle_range(idx, choice)
    if not self.prev_list_idx then
        self:toggle_item(idx, choice)
        return
    end
    local choices = self.subviews.list:getVisibleChoices()
    local list_idx = self.subviews.list.list:getSelected()
    local target_value
    for i = list_idx, self.prev_list_idx, list_idx < self.prev_list_idx and 1 or -1 do
        target_value = self:toggle_item_base(choices[i], target_value)
    end
    self.prev_list_idx = list_idx
end

function MoveGoods:toggle_visible()
    local target_value
    for _, choice in ipairs(self.subviews.list:getVisibleChoices()) do
        target_value = self:toggle_item_base(choice, target_value)
    end
end

-- -------------------
-- MoveGoodsModal
--

MoveGoodsModal = defclass(MoveGoodsModal, gui.ZScreenModal)
MoveGoodsModal.ATTRS {
    focus_path='caravan/movegoods',
    depot=DEFAULT_NIL,
    on_dismiss=DEFAULT_NIL,
}

local function get_pending_trade_item_ids()
    local item_ids = {}
    for _,job in utils.listpairs(df.global.world.jobs.list) do
        if job.job_type == df.job_type.BringItemToDepot and #job.items > 0 then
            item_ids[job.items[0].item.id] = true
        end
    end
    return item_ids
end

function MoveGoodsModal:init()
    self.pending_item_ids = get_pending_trade_item_ids()
    self.depot = self.depot or dfhack.gui.getSelectedBuilding(true)
    self:addviews{
        MoveGoods{
            pending_item_ids=self.pending_item_ids,
            depot=self.depot,
        },
    }
end

function MoveGoodsModal:onDismiss()
    -- mark/unmark selected goods for trade
    local depot = self.depot
    if not depot then return end
    local pending = self.pending_item_ids
    for _, choice in ipairs(self.subviews.list:getChoices()) do
        if not choice.data.dirty then goto continue end
        for item_id, item_data in pairs(choice.data.items) do
            local item = item_data.item
            if item_data.pending and not pending[item_id] then
                item.flags.forbid = false
                if dfhack.items.getHolderBuilding(item) == depot then
                    item.flags.in_building = true
                else
                    dfhack.items.markForTrade(item, depot)
                end
            elseif not item_data.pending and pending[item_id] then
                local spec_ref = dfhack.items.getSpecificRef(item, df.specific_ref_type.JOB)
                if spec_ref then
                    dfhack.job.removeJob(spec_ref.data.job)
                end
            elseif not item_data.pending and item.flags.in_building then
                item.flags.in_building = false
            end
        end
        ::continue::
    end
    if self.on_dismiss then
        self.on_dismiss(self)
    end
end

-- -------------------
-- MoveGoodsOverlay
--

MoveGoodsOverlay = defclass(MoveGoodsOverlay, overlay.OverlayWidget)
MoveGoodsOverlay.ATTRS{
    default_pos={x=-64, y=10},
    default_enabled=true,
    viewscreens='dwarfmode/ViewSheets/BUILDING/TradeDepot',
    frame={w=33, h=1},
    frame_background=gui.CLEAR_PEN,
}

local function has_trade_depot_and_caravan()
    local bld = dfhack.gui.getSelectedBuilding(true)
    if not bld or bld:getBuildStage() < bld:getMaxBuildStage() then
        return false
    end
    if #bld.jobs == 1 and bld.jobs[0].job_type == df.job_type.DestroyBuilding then
        return false
    end

    for _, caravan in ipairs(df.global.plotinfo.caravans) do
        if caravan.flags.tribute then goto continue end
        local trade_state = caravan.trade_state
        local time_remaining = caravan.time_remaining
        if time_remaining > 0 and
            (trade_state == df.caravan_state.T_trade_state.Approaching or
             trade_state == df.caravan_state.T_trade_state.AtDepot)
        then
            return true
        end
        ::continue::
    end
    return false
end

function MoveGoodsOverlay:init()
    self:addviews{
        widgets.TextButton{
            frame={t=0, l=0},
            label='DFHack move trade goods',
            key='CUSTOM_CTRL_T',
            on_activate=function() MoveGoodsModal{}:show() end,
            enabled=has_trade_depot_and_caravan,
        },
    }
end

-- -------------------
-- AssignTradeOverlay
--

AssignTradeOverlay = defclass(AssignTradeOverlay, overlay.OverlayWidget)
AssignTradeOverlay.ATTRS{
    default_pos={x=-41,y=-5},
    default_enabled=true,
    viewscreens='dwarfmode/AssignTrade',
    frame={w=33, h=1},
    frame_background=gui.CLEAR_PEN,
}

function AssignTradeOverlay:init()
    local on_dismiss = function(scr)
        scr:sendInputToParent('LEAVESCREEN')
    end
    self:addviews{
        widgets.TextButton{
            frame={t=0, l=0},
            label='DFHack move trade goods',
            key='CUSTOM_CTRL_T',
            on_activate=function()
                local depot = df.global.game.main_interface.assign_trade.trade_depot_bld
                MoveGoodsModal{depot=depot, on_dismiss=on_dismiss}:show()
            end,
        },
    }
end
