--@ module = true

local dialogs = require('gui.dialogs')
local widgets = require('gui.widgets')

CH_UP = string.char(30)
CH_DN = string.char(31)
CH_MONEY = string.char(15)
CH_EXCEPTIONAL = string.char(240)

local to_pen = dfhack.pen.parse
SOME_PEN = to_pen{ch=':', fg=COLOR_YELLOW}
ALL_PEN = to_pen{ch='+', fg=COLOR_LIGHTGREEN}

function make_search_key(str)
    local out = ''
    for c in str:gmatch("[%w%s]") do
        out = out .. c:lower()
    end
    return out
end

local function get_broker_skill()
    local broker = dfhack.units.getUnitByNobleRole('broker')
    if not broker then return 0 end
    for _,skill in ipairs(broker.status.current_soul.skills) do
        if skill.id == df.job_skill.APPRAISAL then
            return skill.rating
        end
    end
    return 0
end

local function get_threshold(broker_skill)
    if broker_skill <= df.skill_rating.Dabbling then return 0 end
    if broker_skill <= df.skill_rating.Novice then return 10 end
    if broker_skill <= df.skill_rating.Adequate then return 25 end
    if broker_skill <= df.skill_rating.Competent then return 50 end
    if broker_skill <= df.skill_rating.Skilled then return 100 end
    if broker_skill <= df.skill_rating.Proficient then return 200 end
    if broker_skill <= df.skill_rating.Talented then return 500 end
    if broker_skill <= df.skill_rating.Adept then return 1000 end
    if broker_skill <= df.skill_rating.Expert then return 1500 end
    if broker_skill <= df.skill_rating.Professional then return 2000 end
    if broker_skill <= df.skill_rating.Accomplished then return 2500 end
    if broker_skill <= df.skill_rating.Great then return 3000 end
    if broker_skill <= df.skill_rating.Master then return 4000 end
    if broker_skill <= df.skill_rating.HighMaster then return 5000 end
    if broker_skill <= df.skill_rating.GrandMaster then return 10000 end
    return math.huge
end

local function estimate(value, round_base, granularity)
    local rounded = ((value+round_base)//granularity)*granularity
    local clamped = math.max(rounded, granularity)
    return clamped
end

-- If the item's value is below the threshold, it gets shown exactly as-is.
-- Otherwise, if it's less than or equal to [threshold + 50], it will round to the nearest multiple of 10 as an Estimate
-- Otherwise, if it's less than or equal to [threshold + 50] * 3, it will round to the nearest multiple of 100
-- Otherwise, if it's less than or equal to [threshold + 50] * 30, it will round to the nearest multiple of 1000
-- Otherwise, it will display a guess equal to [threshold + 50] * 30 rounded up to the nearest multiple of 1000.
function obfuscate_value(value)
    local threshold = get_threshold(get_broker_skill())
    if value < threshold then return tostring(value) end
    threshold = threshold + 50
    if value <= threshold then return ('~%d'):format(estimate(value, 5, 10)) end
    if value <= threshold*3 then return ('~%d'):format(estimate(value, 50, 100)) end
    if value <= threshold*30 then return ('~%d'):format(estimate(value, 500, 1000)) end
    return ('%d?'):format(estimate(threshold*30, 999, 1000))
end

local function to_title_case(str)
    str = str:gsub('(%a)([%w_]*)',
        function (first, rest) return first:upper()..rest:lower() end)
    str = str:gsub('_', ' ')
    return str
end

local function get_item_type_str(item)
    local str = to_title_case(df.item_type[item:getType()])
    if str == 'Trapparts' then
        str = 'Mechanism'
    end
    return str
end

local function get_artifact_name(item)
    local gref = dfhack.items.getGeneralRef(item, df.general_ref_type.IS_ARTIFACT)
    if not gref then return end
    local artifact = df.artifact_record.find(gref.artifact_id)
    if not artifact then return end
    local name = dfhack.TranslateName(artifact.name)
    return ('%s (%s)'):format(name, get_item_type_str(item))
end

function get_item_description(item)
    local desc = item.flags.artifact and get_artifact_name(item) or
        dfhack.items.getDescription(item, 0, true)
    local wear_level = item:getWear()
    if wear_level == 1 then desc = ('x%sx'):format(desc)
    elseif wear_level == 2 then desc = ('X%sX'):format(desc)
    elseif wear_level == 3 then desc = ('XX%sXX'):format(desc)
    end
    return desc
end

-- takes into account trade agreements
function get_perceived_value(item, caravan_state)
    local value = dfhack.items.getValue(item, caravan_state)
    for _,contained_item in ipairs(dfhack.items.getContainedItems(item)) do
        value = value + dfhack.items.getValue(contained_item, caravan_state)
        for _,contained_contained_item in ipairs(dfhack.items.getContainedItems(contained_item)) do
            value = value + dfhack.items.getValue(contained_contained_item, caravan_state)
        end
    end
    return value
end

function get_slider_widgets(self, suffix)
    suffix = suffix or ''
    return {
        widgets.Panel{
            frame={t=0, l=0, r=0, h=4},
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='min_condition'..suffix,
                    frame={l=0, t=0, w=18},
                    label='Min condition:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_C',
                    key='CUSTOM_SHIFT_V',
                    options={
                        {label='XXTatteredXX', value=3},
                        {label='XFrayedX', value=2},
                        {label='xWornx', value=1},
                        {label='Pristine', value=0},
                    },
                    initial_option=3,
                    on_change=function(val)
                        if self.subviews['max_condition'..suffix]:getOptionValue() > val then
                            self.subviews['max_condition'..suffix]:setOption(val)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='max_condition'..suffix,
                    frame={r=1, t=0, w=18},
                    label='Max condition:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_E',
                    key='CUSTOM_SHIFT_R',
                    options={
                        {label='XXTatteredXX', value=3},
                        {label='XFrayedX', value=2},
                        {label='xWornx', value=1},
                        {label='Pristine', value=0},
                    },
                    initial_option=0,
                    on_change=function(val)
                        if self.subviews['min_condition'..suffix]:getOptionValue() < val then
                            self.subviews['min_condition'..suffix]:setOption(val)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.RangeSlider{
                    frame={l=0, t=3},
                    num_stops=4,
                    get_left_idx_fn=function()
                        return 4 - self.subviews['min_condition'..suffix]:getOptionValue()
                    end,
                    get_right_idx_fn=function()
                        return 4 - self.subviews['max_condition'..suffix]:getOptionValue()
                    end,
                    on_left_change=function(idx) self.subviews['min_condition'..suffix]:setOption(4-idx, true) end,
                    on_right_change=function(idx) self.subviews['max_condition'..suffix]:setOption(4-idx, true) end,
                },
            },
        },
        widgets.Panel{
            frame={t=5, l=0, r=0, h=4},
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='min_quality'..suffix,
                    frame={l=0, t=0, w=18},
                    label='Min quality:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_Z',
                    key='CUSTOM_SHIFT_X',
                    options={
                        {label='Ordinary', value=0},
                        {label='-Well Crafted-', value=1},
                        {label='+Finely Crafted+', value=2},
                        {label='*Superior*', value=3},
                        {label=CH_EXCEPTIONAL..'Exceptional'..CH_EXCEPTIONAL, value=4},
                        {label=CH_MONEY..'Masterful'..CH_MONEY, value=5},
                        {label='Artifact', value=6},
                    },
                    initial_option=0,
                    on_change=function(val)
                        if self.subviews['max_quality'..suffix]:getOptionValue() < val then
                            self.subviews['max_quality'..suffix]:setOption(val)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='max_quality'..suffix,
                    frame={r=1, t=0, w=18},
                    label='Max quality:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_Q',
                    key='CUSTOM_SHIFT_W',
                    options={
                        {label='Ordinary', value=0},
                        {label='-Well Crafted-', value=1},
                        {label='+Finely Crafted+', value=2},
                        {label='*Superior*', value=3},
                        {label=CH_EXCEPTIONAL..'Exceptional'..CH_EXCEPTIONAL, value=4},
                        {label=CH_MONEY..'Masterful'..CH_MONEY, value=5},
                        {label='Artifact', value=6},
                    },
                    initial_option=6,
                    on_change=function(val)
                        if self.subviews['min_quality'..suffix]:getOptionValue() > val then
                            self.subviews['min_quality'..suffix]:setOption(val)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.RangeSlider{
                    frame={l=0, t=3},
                    num_stops=7,
                    get_left_idx_fn=function()
                        return self.subviews['min_quality'..suffix]:getOptionValue() + 1
                    end,
                    get_right_idx_fn=function()
                        return self.subviews['max_quality'..suffix]:getOptionValue() + 1
                    end,
                    on_left_change=function(idx) self.subviews['min_quality'..suffix]:setOption(idx-1, true) end,
                    on_right_change=function(idx) self.subviews['max_quality'..suffix]:setOption(idx-1, true) end,
                },
            },
        },
        widgets.Panel{
            frame={t=10, l=0, r=0, h=4},
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='min_value'..suffix,
                    frame={l=0, t=0, w=18},
                    label='Min value:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_B',
                    key='CUSTOM_SHIFT_N',
                    options={
                        {label='1'..CH_MONEY, value={index=1, value=1}},
                        {label='20'..CH_MONEY, value={index=2, value=20}},
                        {label='50'..CH_MONEY, value={index=3, value=50}},
                        {label='100'..CH_MONEY, value={index=4, value=100}},
                        {label='500'..CH_MONEY, value={index=5, value=500}},
                        {label='1000'..CH_MONEY, value={index=6, value=1000}},
                        -- max "min" value is less than max "max" value since the range of inf - inf is not useful
                        {label='5000'..CH_MONEY, value={index=7, value=5000}},
                    },
                    initial_option=1,
                    on_change=function(val)
                        if self.subviews['max_value'..suffix]:getOptionValue().value < val.value then
                            self.subviews['max_value'..suffix]:setOption(val.index)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='max_value'..suffix,
                    frame={r=1, t=0, w=18},
                    label='Max value:',
                    label_below=true,
                    key_back='CUSTOM_SHIFT_T',
                    key='CUSTOM_SHIFT_Y',
                    options={
                        {label='1'..CH_MONEY, value={index=1, value=1}},
                        {label='20'..CH_MONEY, value={index=2, value=20}},
                        {label='50'..CH_MONEY, value={index=3, value=50}},
                        {label='100'..CH_MONEY, value={index=4, value=100}},
                        {label='500'..CH_MONEY, value={index=5, value=500}},
                        {label='1000'..CH_MONEY, value={index=6, value=1000}},
                        {label='Max', value={index=7, value=math.huge}},
                    },
                    initial_option=7,
                    on_change=function(val)
                        if self.subviews['min_value'..suffix]:getOptionValue().value > val.value then
                            self.subviews['min_value'..suffix]:setOption(val.index)
                        end
                        self:refresh_list()
                    end,
                },
                widgets.RangeSlider{
                    frame={l=0, t=3},
                    num_stops=7,
                    get_left_idx_fn=function()
                        return self.subviews['min_value'..suffix]:getOptionValue().index
                    end,
                    get_right_idx_fn=function()
                        return self.subviews['max_value'..suffix]:getOptionValue().index
                    end,
                    on_left_change=function(idx) self.subviews['min_value'..suffix]:setOption(idx, true) end,
                    on_right_change=function(idx) self.subviews['max_value'..suffix]:setOption(idx, true) end,
                },
            },
        },
    }
end

function is_tree_lover_caravan(caravan)
    local caravan_he = df.historical_entity.find(caravan.entity);
    if not caravan_he then return false end
    local wood_ethic = caravan_he.entity_raw.ethic[df.ethic_type.KILL_PLANT]
    return wood_ethic == df.ethic_response.MISGUIDED or
        wood_ethic == df.ethic_response.SHUN or
        wood_ethic == df.ethic_response.APPALLING or
        wood_ethic == df.ethic_response.PUNISH_REPRIMAND or
        wood_ethic == df.ethic_response.PUNISH_SERIOUS or
        wood_ethic == df.ethic_response.PUNISH_EXILE or
        wood_ethic == df.ethic_response.PUNISH_CAPITAL or
        wood_ethic == df.ethic_response.UNTHINKABLE
end

function is_animal_lover_caravan(caravan)
    local caravan_he = df.historical_entity.find(caravan.entity);
    if not caravan_he then return false end
    local animal_ethic = caravan_he.entity_raw.ethic[df.ethic_type.KILL_ANIMAL]
    return animal_ethic == df.ethic_response.JUSTIFIED_IF_SELF_DEFENSE or
        animal_ethic == df.ethic_response.JUSTIFIED_IF_EXTREME_REASON or
        animal_ethic == df.ethic_response.MISGUIDED or
        animal_ethic == df.ethic_response.SHUN or
        animal_ethic == df.ethic_response.APPALLING or
        animal_ethic == df.ethic_response.PUNISH_REPRIMAND or
        animal_ethic == df.ethic_response.PUNISH_SERIOUS or
        animal_ethic == df.ethic_response.PUNISH_EXILE or
        animal_ethic == df.ethic_response.PUNISH_CAPITAL or
        animal_ethic == df.ethic_response.UNTHINKABLE
end

-- works for both mandates and unit preferences
-- adds spec to registry, but only if not in filter
local function register_item_type(registry, spec, filter)
    if not safe_index(filter, spec.item_type, spec.item_subtype) then
        ensure_keys(registry, spec.item_type)[spec.item_subtype] = true
    end
end

function get_banned_items()
    local banned_items = {}
    for _, mandate in ipairs(df.global.world.mandates) do
        if mandate.mode == df.mandate.T_mode.Export then
            register_item_type(banned_items, mandate)
        end
    end
    return banned_items
end

local function analyze_noble(unit, risky_items, banned_items)
    for _, preference in ipairs(unit.status.current_soul.preferences) do
        if preference.type == df.unit_preference.T_type.LikeItem and
            preference.active
        then
            register_item_type(risky_items, preference, banned_items)
        end
    end
end

local function get_mandate_noble_roles()
    local roles = {}
    for _, link in ipairs(df.global.world.world_data.active_site[0].entity_links) do
        local he = df.historical_entity.find(link.entity_id);
        if not he or
            (he.type ~= df.historical_entity_type.SiteGovernment and
             he.type ~= df.historical_entity_type.Civilization)
        then
            goto continue
        end
        for _, position in ipairs(he.positions.own) do
            if position.mandate_max > 0 then
                table.insert(roles, position.code)
            end
        end
        ::continue::
    end
    return roles
end

function get_risky_items(banned_items)
    local risky_items = {}
    for _, role in ipairs(get_mandate_noble_roles()) do
        for _, unit in ipairs(dfhack.units.getUnitsByNobleRole(role)) do
            analyze_noble(unit, risky_items, banned_items)
        end
    end
    return risky_items
end

local function to_item_type_str(item_type)
    return string.lower(df.item_type[item_type]):gsub('_', ' ')
end

local function make_item_description(item_type, subtype)
    local itemdef = dfhack.items.getSubtypeDef(item_type, subtype)
    return itemdef and string.lower(itemdef.name) or to_item_type_str(item_type)
end

local function get_banned_token(banned_items)
    if not next(banned_items) then
        return {
            gap=2,
            text='None',
            pen=COLOR_GREY,
        }
    end
    local strs = {}
    for item_type, subtypes in pairs(banned_items) do
        for subtype in pairs(subtypes) do
            table.insert(strs, make_item_description(item_type, subtype))
        end
    end
    return {
        gap=2,
        text=table.concat(strs, ', '),
        pen=COLOR_LIGHTRED,
    }
end

local function show_export_agreements(export_agreements)
    local strs = {}
    for _, agreement in ipairs(export_agreements) do
        for idx, price in ipairs(agreement.price) do
            local desc = make_item_description(agreement.items.item_type[idx], agreement.items.item_subtype[idx])
            local percent = (price * 100) // 128
            table.insert(strs, ('%20s %d%%'):format(desc..':', percent))
        end
    end
    dialogs.showMessage('Price agreement for exported items', table.concat(strs, '\n'))
end

local function get_ethics_token(animal_ethics, wood_ethics)
    local restrictions = {}
    if animal_ethics or wood_ethics then
        if animal_ethics then table.insert(restrictions, "Animals") end
        if wood_ethics then table.insert(restrictions, "Trees") end
    end
    return {
        gap=2,
        text=#restrictions == 0 and 'None' or table.concat(restrictions, ', '),
        pen=#restrictions ~= 0 and COLOR_LIGHTRED or COLOR_GREY,
    }
end

function get_info_widgets(self, export_agreements)
    return {
        widgets.Panel{
            frame={t=0, l=0, r=0, h=2},
            subviews={
                widgets.Label{
                    frame={t=0, l=0},
                    text={
                        'Merchant export agreements:',
                        {gap=1, text='None', pen=COLOR_GREY},
                    },
                },
                widgets.HotkeyLabel{
                    frame={t=0, l=28},
                    key='CUSTOM_SHIFT_H',
                    label='[details]',
                    text_pen=COLOR_LIGHTRED,
                    on_activate=function() show_export_agreements(export_agreements) end,
                    visible=#export_agreements > 0,
                },
                widgets.ToggleHotkeyLabel{
                    view_id='only_agreement',
                    frame={t=1, l=0},
                    label='Show only requested items:',
                    key='CUSTOM_SHIFT_A',
                    options={
                        {label='Yes', value=true, pen=COLOR_GREEN},
                        {label='No', value=false}
                    },
                    initial_option=false,
                    on_change=function() self:refresh_list() end,
                    visible=#export_agreements > 0,
                },
            },
        },
        widgets.Panel{
            frame={t=3, l=0, r=0, h=3},
            subviews={
                widgets.Label{
                    frame={t=0, l=0},
                    text={
                        'Merchant ethical restrictions:', NEWLINE,
                        get_ethics_token(self.animal_ethics, self.wood_ethics),
                    },
                },
                widgets.CycleHotkeyLabel{
                    view_id='ethical',
                    frame={t=2, l=0},
                    key='CUSTOM_SHIFT_G',
                    options={
                        {label='Show only ethically acceptable items', value='only', pen=COLOR_GREEN},
                        {label='Ignore ethical restrictions', value='show'},
                        {label='Show only ethically unacceptable items', value='hide', pen=COLOR_RED},
                    },
                    initial_option='only',
                    option_gap=0,
                    visible=self.animal_ethics or self.wood_ethics,
                    on_change=function() self:refresh_list() end,
                },
            },
        },
        widgets.Panel{
            frame={t=7, l=0, r=0, h=5},
            subviews={
                widgets.Label{
                    frame={t=0, l=0},
                    text={
                        'Items banned by export mandates:', NEWLINE,
                        get_banned_token(self.banned_items), NEWLINE,
                        'Additional items at risk of mandates:', NEWLINE,
                        get_banned_token(self.risky_items),
                    },
                },
                widgets.CycleHotkeyLabel{
                    view_id='banned',
                    frame={t=4, l=0},
                    key='CUSTOM_SHIFT_D',
                    options={
                        {label='Hide banned and risky items', value='both', pen=COLOR_GREEN},
                        {label='Hide banned items', value='banned_only', pen=COLOR_YELLOW},
                        {label='Ignore mandate restrictions', value='ignore', pen=COLOR_RED},
                    },
                    initial_option='both',
                    option_gap=0,
                    visible=next(self.banned_items) or next(self.risky_items),
                    on_change=function() self:refresh_list() end,
                },
            },
        },
    }
end

local function match_risky(item, risky_items)
    for item_type, subtypes in pairs(risky_items) do
        for subtype in pairs(subtypes) do
            if item_type == item:getType() and (subtype == -1 or subtype == item:getSubtype()) then
                return true
            end
        end
    end
    return false
end

-- returns is_banned, is_risky
function scan_banned(item, risky_items)
    if not dfhack.items.checkMandates(item) then return true, true end
    if match_risky(item, risky_items) then return false, true end
    for _,contained_item in ipairs(dfhack.items.getContainedItems(item)) do
        if not dfhack.items.checkMandates(contained_item) then return true, true end
        if match_risky(contained_item, risky_items) then return false, true end
    end
    return false, false
end

local function is_wood_based(mat_type, mat_index)
    if mat_type == df.builtin_mats.LYE or
        mat_type == df.builtin_mats.GLASS_CLEAR or
        mat_type == df.builtin_mats.GLASS_CRYSTAL or
        (mat_type == df.builtin_mats.COAL and mat_index == 1) or
        mat_type == df.builtin_mats.POTASH or
        mat_type == df.builtin_mats.ASH or
        mat_type == df.builtin_mats.PEARLASH
    then
        return true
    end

    local mi = dfhack.matinfo.decode(mat_type, mat_index)
    return mi and mi.material and
        (mi.material.flags.WOOD or
         mi.material.flags.STRUCTURAL_PLANT_MAT or
         mi.material.flags.SOAP)
end

function has_wood(item)
    if item.flags2.grown then return false end

    if is_wood_based(item:getMaterial(), item:getMaterialIndex()) then
        return true
    end

    if item:hasImprovements() then
        for _, imp in ipairs(item.improvements) do
            if is_wood_based(imp.mat_type, imp.mat_index) then
                return true
            end
        end
    end

    return false
end
