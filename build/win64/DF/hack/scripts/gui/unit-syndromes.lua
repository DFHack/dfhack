-- GUI for exploring unit syndromes (and their effects).

local gui = require('gui')
local utils = require('utils')
local widgets = require('gui.widgets')

local function getEffectTarget(target)
    local values = {}
    local limit = #target.key - 1

    for i = 0, limit, 1 do
        table.insert(values, ("%s(%s)"):format(
            df.creature_interaction_effect_target_mode[target.mode[i]],
            target.tissue[i].value
        ))
    end

    return table.concat(values, "\n")
end

local function getEffectInteraction(effect)
    return ("%s"):format(effect.interaction.adv_name)
end

local function getEffectCreatureName(effect)
    if effect.race_str == "" then
        return "UNKNOWN"
    end

    local creature = df.global.world.raws.creatures.all[effect.race[0]]

    if effect.caste_str == "DEFAULT" then
        return ("%s%s"):format(string.upper(creature.name[0]), (", %s"):format(effect.caste_str))
    else
        -- TODO: Caste seems to be entirely unused.
        local caste = creature.caste[effect.caste[0]]

        return ("%s%s"):format(string.upper(creature.name[0]), (", %s"):format(string.upper(caste.name[0])))
    end
end

local function getAttributePairs(values, percentages)
    local attributes = {}

    for key, value in pairs(values) do
        if value ~= 0 then
            table.insert(attributes, ("%s +%s"):format(key, value))
        end
    end

    for key, value in pairs(percentages) do
        if value ~= 0 then
            table.insert(attributes, ("%s %s%%"):format(key, value))
        end
    end

    return attributes
end

local EffectFlagDescription = {
    [df.creature_interaction_effect_type.PAIN] = function(effect)
        return ([[POWER=%s %s]]):format(effect.sev, getEffectTarget(effect.target))
    end,
    [df.creature_interaction_effect_type.SWELLING] = function(effect)
        return ([[POWER=%s %s]]):format(effect.sev, getEffectTarget(effect.target))
    end,
    [df.creature_interaction_effect_type.OOZING] = function(effect)
        return ([[POWER=%s %s]]):format(effect.sev, getEffectTarget(effect.target))
    end,
    [df.creature_interaction_effect_type.BRUISING] = function(effect)
        return ([[POWER=%s %s]]):format(effect.sev, getEffectTarget(effect.target))
    end,
    [df.creature_interaction_effect_type.BLISTERS] = function(effect)
        return ([[POWER=%s %s]]):format(effect.sev, getEffectTarget(effect.target))
    end,
    [df.creature_interaction_effect_type.NUMBNESS] = function(effect)
        return ([[POWER=%s %s]]):format(effect.sev, getEffectTarget(effect.target))
    end,
    [df.creature_interaction_effect_type.PARALYSIS] = function(effect)
        return ([[POWER=%s %s]]):format(effect.sev, getEffectTarget(effect.target))
    end,
    [df.creature_interaction_effect_type.FEVER] = function(effect)
        return ([[POWER=%s]]):format(effect.sev)
    end,
    [df.creature_interaction_effect_type.BLEEDING] = function(effect)
        return ([[POWER=%s]]):format(effect.sev)
    end,
    [df.creature_interaction_effect_type.COUGH_BLOOD] = function(effect)
        return ([[POWER=%s]]):format(effect.sev)
    end,
    [df.creature_interaction_effect_type.VOMIT_BLOOD] = function(effect)
        return ([[POWER=%s]]):format(effect.sev)
    end,
    [df.creature_interaction_effect_type.NAUSEA] = function(effect)
        return ([[POWER=%s]]):format(effect.sev)
    end,
    [df.creature_interaction_effect_type.UNCONSCIOUSNESS] = function(effect)
        return ([[POWER=%s]]):format(effect.sev)
    end,
    [df.creature_interaction_effect_type.NECROSIS] = function(effect)
        return ([[POWER=%s %s]]):format(effect.sev, getEffectTarget(effect.target))
    end,
    [df.creature_interaction_effect_type.IMPAIR_FUNCTION] = function(effect)
        return ([[POWER=%s %s]]):format(effect.sev, getEffectTarget(effect.target))
    end,
    [df.creature_interaction_effect_type.DROWSINESS] = function(effect)
        return ([[POWER=%s]]):format(effect.sev)
    end,
    [df.creature_interaction_effect_type.DIZZINESS] = function(effect)
        return ([[POWER=%s]]):format(effect.sev)
    end,
    [df.creature_interaction_effect_type.FEEL_EMOTION] = function(effect)
        return ([[POWER=%s %s]]):format(effect.sev, df.emotion_type[effect.emotion])
    end,
    [df.creature_interaction_effect_type.ERRATIC_BEHAVIOR] = function(effect)
        return ([[POWER=%s]]):format(effect.sev)
    end,
    [df.creature_interaction_effect_type.CHANGE_PERSONALITY] = function(effect)
        local changed_facets = {}

        for facet, value in pairs(effect.facets) do
            if value ~= 0 then
                table.insert(changed_facets, ("%s %s"):format(facet, value))
            end
        end

        return table.concat(changed_facets, "\n")
    end,
    [df.creature_interaction_effect_type.ADD_TAG] = function(effect)
        local tags = {}

        for tag, value in pairs(effect.tags1) do
            if value then
                table.insert(tags, string.upper(tag))
            end
        end

        for tag, value in pairs(effect.tags2) do
            if value then
                table.insert(tags, string.upper(tag))
            end
        end

        return ("ADDS: \n%s"):format(table.concat(tags, "  \n"))
    end,
    [df.creature_interaction_effect_type.REMOVE_TAG] = function(effect)
        local tags = {}

        for tag, value in pairs(effect.tags1) do
            if value then
                table.insert(tags, string.upper(tag))
            end
        end

        for tag, value in pairs(effect.tags2) do
            if value then
                table.insert(tags, string.upper(tag))
            end
        end

        return ("REMOVES: \n%s"):format(table.concat(tags, "  \n"))
    end,
    [df.creature_interaction_effect_type.DISPLAY_TILE] = function(effect)
        return ("TILE: %s %s"):format(effect.color, effect.tile)
    end,
    [df.creature_interaction_effect_type.FLASH_TILE] = function(effect)
        return ("FLASH TILE: %s %s"):format(effect.sym_color[1], effect.sym_color[0])
    end,
    [df.creature_interaction_effect_type.SPEED_CHANGE] = function(effect)
        local is_negative = effect.bonus_add < 0

        if effect.bonus_add ~= 0 then
            return ("SPEED: %s%s"):format(is_negative and "-" or "+", effect.bonus_add)
        end

        return ("SPEED: %s%%"):format(effect.bonus_perc)
    end,
    [df.creature_interaction_effect_type.CAN_DO_INTERACTION] = function(effect)
        return ("ABILITY: %s"):format(getEffectInteraction(effect))
    end,
    [df.creature_interaction_effect_type.SKILL_ROLL_ADJUST] = function(effect)
        return ("MODIFIER=%s, CHANGE=%s"):format(effect.multiplier, effect.chance)
    end,
    [df.creature_interaction_effect_type.BODY_TRANSFORMATION] = function(effect)
        if effect.chance > 0 then
            return ("%s, CHANCE=%s%%"):format(getEffectCreatureName(effect), effect.chance)
        else
            return getEffectCreatureName(effect)
        end
    end,
    [df.creature_interaction_effect_type.PHYS_ATT_CHANGE] = function(effect)
        return ("%s"):format(table.concat(getAttributePairs(effect.phys_att_add, effect.phys_att_perc), "\n"))
    end,
    [df.creature_interaction_effect_type.MENT_ATT_CHANGE] = function(effect)
        return ("%s"):format(table.concat(getAttributePairs(effect.ment_att_add, effect.ment_att_perc), "\n"))
    end,
    -- TODO: Unfinished, materials need to be tested.
    [df.creature_interaction_effect_type.MATERIAL_FORCE_MULTIPLIER] = function(effect)
        local material = df.material[effect.mat_type]

        return ("RECIEVED DAMAGE SCALED BY %s%%%s"):format(
            (effect.fraction_mul * 100 / effect.fraction_div * 100) / 100,
            material and ("vs. %s"):format(material.stone_name)
        )
    end,
    -- TODO: Unfinished, unknown fields from previous script.
    [df.creature_interaction_effect_type.BODY_MAT_INTERACTION] = function(effect)
        return ("%s %s"):format(effect.interaction_name, effect.interaction_id)
    end,
    -- TODO: Unfinished.
    [df.creature_interaction_effect_type.BODY_APPEARANCE_MODIFIER] = function(effect)
        return ("TODO"):format(effect.interaction_name, effect.interaction_id)
    end,
    [df.creature_interaction_effect_type.BP_APPEARANCE_MODIFIER] = function(effect)
        return ("VALUE=%s CHANGE_TYPE_ENUM=%s%s"):format(effect.value, effect.unk_6c, getEffectTarget(effect.target))
    end,
    [df.creature_interaction_effect_type.DISPLAY_NAME] = function(effect)
        return ("SET NAME: %s"):format(effect.name)
    end,
}

local function getEffectFlags(effect)
    local flags = {}

    for name, value in pairs(effect.flags) do
        if value then
            table.insert(flags, name)
        end
    end

    return flags
end

local function getEffectDescription(effect)
    return EffectFlagDescription[effect:getType()] and EffectFlagDescription[effect:getType()](effect) or ""
end

local function getSyndromeName(syndrome_raw)
    local is_transformation = false

    for _, effect in pairs(syndrome_raw.ce) do
        if df.creature_interaction_effect_body_transformationst:is_instance(effect) then
            is_transformation = true
        end
    end

    if syndrome_raw.syn_name ~= "" then
        syndrome_raw.syn_name:gsub("^%l", string.upper)
    elseif is_transformation then
        return "Body transformation"
    end

    return "Mystery"
end

local function getSyndromeEffects(syndrome_type)
    local syndrome_raw = df.global.world.raws.syndromes.all[syndrome_type]
    local syndrome_effects = {}

    for _, effect in pairs(syndrome_raw.ce) do
        table.insert(syndrome_effects, effect)
    end

    return syndrome_effects
end

local function getSyndromeDescription(syndrome_raw, syndrome)
    local syndrome_duration = nil
    local syndrome_min_duration = nil
    local syndrome_max_duration = nil

    for _, effect in pairs(syndrome_raw.ce) do
        syndrome_min_duration = math.min(syndrome_min_duration or effect["end"], effect["end"])
        syndrome_max_duration = math.max(syndrome_max_duration or effect["end"], effect["end"])
    end

    if syndrome_min_duration == -1 then
        syndrome_duration = "[permanent]"
    elseif syndrome_min_duration == syndrome_max_duration then
        syndrome_duration = syndrome_min_duration
    else
        syndrome_duration = ("%s-%s"):format(syndrome_min_duration, syndrome_max_duration)
    end

    return ("%-22s %s%s \n%s effects"):format(
        getSyndromeName(syndrome_raw),
        syndrome and ("%s of "):format(syndrome.ticks) or "",
        syndrome_duration,
        #getSyndromeEffects(syndrome_raw.id)
    )
end

local function getUnitSyndromes(unit)
    local unit_syndromes = {}

    for _, syndrome in pairs(unit.syndromes.active) do
        table.insert(unit_syndromes, syndrome)
    end

    return unit_syndromes
end

local function getCitizens()
    local units = {}

    for _, unit in pairs(df.global.world.units.active) do
        if dfhack.units.isCitizen(unit) and dfhack.units.isDwarf(unit) then
            table.insert(units, unit)
        end
    end

    return units
end

local function getLivestock()
    local units = {}

    for _, unit in pairs(df.global.world.units.active) do
        local caste_flags = unit.caste and df.global.world.raws.creatures.all[unit.race].caste[unit.caste].flags

        if dfhack.units.isFortControlled(unit) and caste_flags and (caste_flags.PET or caste_flags.PET_EXOTIC) then
            table.insert(units, unit)
        end
    end

    return units
end

local function getWildAnimals()
    local units = {}

    for _, unit in pairs(df.global.world.units.active) do
        if unit.animal.population.feature_idx == -1 and unit.animal.population.cave_id == -1 then
            if dfhack.units.isAnimal(unit) and not dfhack.units.isFortControlled(unit) then
                table.insert(units, unit)
            end
        end
    end

    return units
end

local function getHostiles()
    local units = {}

    for _, unit in pairs(df.global.world.units.active) do
        if dfhack.units.isDanger(unit) or dfhack.units.isGreatDanger(unit) then
            table.insert(units, unit)
        end
    end

    return units
end

local function getAllUnits()
    local units = {}

    for _, unit in pairs(df.global.world.units.active) do
        table.insert(units, unit)
    end

    return units
end

UnitSyndromes = defclass(UnitSyndromes, widgets.Window)
UnitSyndromes.ATTRS {
    frame_title='Unit Syndromes',
    frame={w=50, h=30},
    resizable=true,
    resize_min={w=43, h=20},
}

function UnitSyndromes:init()
    self.stack = {}

    self:addviews{
        widgets.Pages{
            view_id = 'pages',
            frame = {t = 0, l = 0, b = 2},
            subviews = {
                widgets.List{
                    view_id = 'category',
                    frame = {t = 0, l = 0},
                    choices = {
                        { text = "Dwarves", get_choices = getCitizens },
                        { text = "Livestock", get_choices = getLivestock },
                        { text = "Wild animals", get_choices = getWildAnimals },
                        { text = "Hostile", get_choices = getHostiles },
                        { text = "All units", get_choices = getAllUnits },
                        { text = "All syndromes" },
                    },
                    on_submit = self:callback('showUnits'),
                },
                widgets.FilteredList{
                    view_id = 'units',
                    frame = {t = 0, l = 0},
                    row_height = 3,
                    on_submit = self:callback('showUnitSyndromes'),
                },
                widgets.FilteredList{
                    view_id = 'unit_syndromes',
                    frame = {t = 0, l = 0, r = 0},
                    on_submit = self:callback('showSyndromeEffects'),
                },
                widgets.WrappedLabel{
                    view_id = 'syndrome_effects',
                    frame = {t = 0, l = 0},
                    text_pen = COLOR_WHITE,
                    cursor_pen = COLOR_WHITE,
                    text_to_wrap = '',
                }
            }
        },
        widgets.HotkeyLabel{
            frame={l=0, b=0},
            label='Back',
            auto_width=true,
            key='LEAVESCREEN',
            on_activate=self:callback('previous_page'),
        },
    }
end

function UnitSyndromes:previous_page()
    local pages = self.subviews.pages
    local cur_page = pages:getSelected()
    if cur_page == 1 then
        view:dismiss()
        return
    end

    local state = table.remove(self.stack, #self.stack)
    pages:setSelected(state.page)
    if state.edit then
        state.edit:setFocus(true)
    end
end

function UnitSyndromes:push_state()
    table.insert(self.stack, {page=self.subviews.pages:getSelected(), edit=self.focus_group.cur})
end

function UnitSyndromes:onInput(keys)
    if keys._MOUSE_R_DOWN then
        self:previous_page()
        return true
    end
    return UnitSyndromes.super.onInput(self, keys)
end

function UnitSyndromes:showUnits(index, choice)
    local choices = {}

    if choice.text == "All syndromes" then
        for _, syndrome in pairs(df.global.world.raws.syndromes.all) do
            if #syndrome.ce == 0 then
                goto skipsyndrome
            end

            table.insert(choices, {
                syndrome_type = syndrome.id,
                text = getSyndromeDescription(syndrome)
            })

            :: skipsyndrome ::
        end

        self:push_state()
        self.subviews.pages:setSelected('unit_syndromes')
        self.subviews.unit_syndromes:setChoices(choices)
        self.subviews.unit_syndromes.edit:setFocus(true)

        return
    end

    for _, unit in pairs(choice.get_choices()) do
        local unit_syndromes = getUnitSyndromes(unit)

        table.insert(choices, {
            unit_id = unit.id,
            text = ("%s %s \n%s syndromes"):format(
                string.upper(df.global.world.raws.creatures.all[unit.race].name[0]),
                dfhack.TranslateName(unit.name),
                #unit_syndromes
            ),
        })
    end

    self:push_state()
    self.subviews.pages:setSelected('units')
    self.subviews.units:setChoices(choices)
    self.subviews.units.edit:setFocus(true)
end

function UnitSyndromes:showUnitSyndromes(index, choice)
    local unit = utils.binsearch(df.global.world.units.all, choice.unit_id, 'id')
    local unit_syndromes = getUnitSyndromes(unit)
    local choices = {}

    if #unit_syndromes == 0 then
        return
    end

    for _, syndrome in pairs(unit_syndromes) do
        local syndrome_raw = df.global.world.raws.syndromes.all[syndrome.type]

        if #syndrome_raw.ce == 0 then
            goto skipsyndrome
        end

        table.insert(choices, {
            syndrome_type = syndrome_raw.id,
            text = getSyndromeDescription(syndrome_raw, syndrome)
        })

        :: skipsyndrome ::
    end

    self:push_state()
    self.subviews.pages:setSelected('unit_syndromes')
    self.subviews.unit_syndromes:setChoices(choices)
    self.subviews.unit_syndromes.edit:setFocus(true)
end

function UnitSyndromes:showSyndromeEffects(index, choice)
    local choices = {'ID: '..tostring(choice.syndrome_type)}

    for _, effect in pairs(getSyndromeEffects(choice.syndrome_type)) do
        local effect_name = df.creature_interaction_effect_type[effect:getType()]
        local effect_duration = nil

        if effect["end"] == -1 then
            effect_duration = "permanent"
        elseif effect["start"] >= effect["peak"] or effect["peak"] <= 1 then
            effect_duration = ("%s-%s"):format(effect["start"], effect["end"])
        else
            effect_duration = ("%s-%s-%s"):format(effect["start"], effect["peak"], effect["end"])
        end

        table.insert(choices, ("%-22s %s \n%s\n%s"):format(
            effect_name,
            effect_duration,
            table.concat(getEffectFlags(effect), " "),
            getEffectDescription(effect)
        ))
    end

    self:push_state()
    self.subviews.pages:setSelected('syndrome_effects')
    self.subviews.syndrome_effects.text_to_wrap = table.concat(choices, "\n\n")
    self.subviews.syndrome_effects:updateLayout()
    self.subviews.pages:updateLayout()
end

SyndromesScreen = defclass(SyndromesScreen, gui.ZScreen)
SyndromesScreen.ATTRS {
    focus_path='showunitsyndromes',
}

function SyndromesScreen:init()
    self:addviews{UnitSyndromes{}}
end

function SyndromesScreen:onDismiss()
    view = nil
end

view = view and view:raise() or SyndromesScreen{}:show()
