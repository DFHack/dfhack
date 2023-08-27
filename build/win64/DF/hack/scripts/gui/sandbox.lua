local gui = require('gui')
local materials = require('gui.materials')
local makeown = reqscript('makeown')
local syndrome_util = require('syndrome-util')
local widgets = require('gui.widgets')
local utils = require('utils')

local DISPOSITIONS = {
    HOSTILE = 1,
    HOSTILE_UNDEAD = 2,
    WILD = 3,
    FRIENDLY = 4,
    FORT = 5,
}

---------------------
-- Sandbox
--

Sandbox = defclass(Sandbox, widgets.Window)
Sandbox.ATTRS {
    frame_title='Armok\'s Sandbox',
    frame={r=2, t=18, w=26, h=20},
    frame_inset={b=1},
    interface_masks=DEFAULT_NIL,
}

local function is_sentient(unit)
    local caste_flags = unit.enemy.caste_flags
    return caste_flags.CAN_SPEAK or caste_flags.CAN_LEARN
end

local function finalize_sentient(unit, disposition)
    if disposition == DISPOSITIONS.HOSTILE or disposition == DISPOSITIONS.HOSTILE_UNDEAD then
        unit.flags1.active_invader = true;
        unit.flags1.marauder = true;
    elseif disposition == DISPOSITIONS.WILD then
        unit.flags2.visitor = true
        unit.flags3.guest = true
        unit.animal.leave_countdown = 20000
    elseif disposition == DISPOSITIONS.FRIENDLY then
        -- noop; units are created friendly by default
    elseif disposition == DISPOSITIONS.FORT then
        makeown.make_own(unit)
    end
end

local function finalize_animal(unit, disposition)
    if disposition == DISPOSITIONS.HOSTILE or disposition == DISPOSITIONS.HOSTILE_UNDEAD then
        unit.flags4.agitated_wilderness_creature = true
    elseif disposition == DISPOSITIONS.WILD then
        unit.flags2.roaming_wilderness_population_source = true
        unit.flags2.roaming_wilderness_population_source_not_a_map_feature = true
        unit.animal.leave_countdown = 20000
    elseif disposition == DISPOSITIONS.FRIENDLY then
        -- noop; units are created friendly by default
    elseif disposition == DISPOSITIONS.FORT then
        makeown.make_own(unit)
        unit.flags1.tame = true
        unit.training_level = df.animal_training_level.Domesticated
    end
end

local function is_arena_action_in_progress()
    return df.global.game.main_interface.arena_unit.open or
            df.global.game.main_interface.arena_tree.open or
            df.global.game.main_interface.bottom_mode_selected ~= -1
end

local function clear_arena_action()
    -- close any open arena UI elements
    df.global.game.main_interface.arena_unit.open = false
    df.global.game.main_interface.arena_tree.open = false
    df.global.game.main_interface.bottom_mode_selected = -1
end

local function finalize_units(first_created_unit_id, disposition, syndrome)
    for unit_id=first_created_unit_id,df.global.unit_next_id-1 do
        local unit = df.unit.find(unit_id)
        if not unit then goto continue end
        unit.profession = df.profession.STANDARD
        if syndrome then
            syndrome_util.infectWithSyndrome(unit, syndrome)
            unit.flags1.zombie = true;
        end
        unit.name.has_name = false
        if is_sentient(unit) then
            finalize_sentient(unit, disposition)
        else
            finalize_animal(unit, disposition)
        end
        ::continue::
    end
end

function Sandbox:init()
    self.spawn_group = 1
    self.first_unit_id = df.global.unit_next_id

    self:addviews{
        widgets.ResizingPanel{
            frame={t=0},
            frame_style=gui.FRAME_INTERIOR,
            autoarrange_subviews=1,
            subviews={
                widgets.Label{
                    frame={l=0},
                    text={
                        'Unit group #',
                        {text=function() return self.spawn_group end},
                        NEWLINE,
                        '  unit',
                        {text=function() return df.global.unit_next_id - self.first_unit_id == 1 and '' or 's' end},
                        ' in group: ',
                        {text=function() return df.global.unit_next_id - self.first_unit_id end},
                    },
                },
                widgets.Panel{frame={h=1}},
                widgets.HotkeyLabel{
                    frame={l=0},
                    key='CUSTOM_SHIFT_U',
                    label="Spawn unit",
                    on_activate=function()
                        df.global.enabler.mouse_lbut = 0
                        clear_arena_action()
                        view:sendInputToParent{ARENA_CREATE_CREATURE=true}
                        df.global.game.main_interface.arena_unit.editing_filter = true
                    end,
                },
                widgets.HotkeyLabel{
                    frame={l=0},
                    key='CUSTOM_SHIFT_G',
                    label='Start new group',
                    on_activate=self:callback('finalize_group'),
                    enabled=function() return df.global.unit_next_id ~= self.first_unit_id end,
                },
                widgets.Panel{frame={h=1}},
                widgets.CycleHotkeyLabel{
                    view_id='disposition',
                    frame={l=0},
                    key='CUSTOM_SHIFT_D',
                    key_back='CUSTOM_SHIFT_A',
                    label='Group disposition',
                    label_below=true,
                    options={
                        {label='hostile', value=DISPOSITIONS.HOSTILE, pen=COLOR_LIGHTRED},
                        {label='hostile (undead)', value=DISPOSITIONS.HOSTILE_UNDEAD, pen=COLOR_RED},
                        {label='independent/wild', value=DISPOSITIONS.WILD, pen=COLOR_YELLOW},
                        {label='friendly', value=DISPOSITIONS.FRIENDLY, pen=COLOR_GREEN},
                        {label='citizens/pets', value=DISPOSITIONS.FORT, pen=COLOR_BLUE},
                    },
                },
            },
        },
        widgets.ResizingPanel{
            frame={t=11},
            frame_style=gui.FRAME_INTERIOR,
            autoarrange_subviews=1,
            subviews={
                widgets.HotkeyLabel{
                    frame={l=0},
                    key='CUSTOM_SHIFT_T',
                    label="Spawn tree",
                    on_activate=function()
                        df.global.enabler.mouse_lbut = 0
                        clear_arena_action()
                        view:sendInputToParent{ARENA_CREATE_TREE=true}
                        df.global.game.main_interface.arena_tree.editing_filter = true
                    end,
                },
                widgets.HotkeyLabel{
                    frame={l=0},
                    key='CUSTOM_SHIFT_I',
                    label="Create item",
                    on_activate=function() dfhack.run_script('gui/create-item') end
                },
            },
        },
        widgets.HotkeyLabel{
            frame={l=1, b=0},
            key='LEAVESCREEN',
            label="Return to game",
            on_activate=function()
                repeat until not self:onInput{LEAVESCREEN=true}
                view:dismiss()
            end,
        },
    }
end

function Sandbox:onInput(keys)
    if keys._MOUSE_R_DOWN and self:getMouseFramePos() then
        clear_arena_action()
        return false
    end
    if keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        if is_arena_action_in_progress() then
            clear_arena_action()
            return true
        else
            return false
        end
    end
    if Sandbox.super.onInput(self, keys) then
        return true
    end
    if keys._MOUSE_L then
        if self:getMouseFramePos() then return true end
        for _,mask_panel in ipairs(self.interface_masks) do
            if mask_panel:getMousePos() then return true end
        end
    end
    view:sendInputToParent(keys)
end

function Sandbox:find_zombie_syndrome()
    if self.zombie_syndrome then return self.zombie_syndrome end
    for _,syn in ipairs(df.global.world.raws.syndromes.all) do
        for _,effect in ipairs(syn.ce) do
            if df.creature_interaction_effect_add_simple_flagst:is_instance(effect) and
                    effect.tags1.OPPOSED_TO_LIFE and effect['end'] == -1 then
                self.zombie_syndrome = syn
                return syn
            end
        end
        ::continue::
    end
    dfhack.printerr('permanent syndrome with OPPOSED_TO_LIFE not found; not marking as undead')
end

function Sandbox:finalize_group()
    local syndrome = nil
    if self.subviews.disposition:getOptionValue() == DISPOSITIONS.HOSTILE_UNDEAD then
        syndrome = self:find_zombie_syndrome()
    end

    finalize_units(self.first_unit_id,
            self.subviews.disposition:getOptionValue(),
            syndrome)

    self.spawn_group = self.spawn_group + 1
    self.first_unit_id = df.global.unit_next_id
end

---------------------
-- InterfaceMask
--

InterfaceMask = defclass(InterfaceMask, widgets.Panel)
InterfaceMask.ATTRS{
    frame_background=gui.TRANSPARENT_PEN,
}

function InterfaceMask:onInput(keys)
    return keys._MOUSE_L and self:getMousePos()
end

---------------------
-- SandboxScreen
--

SandboxScreen = defclass(SandboxScreen, gui.ZScreen)
SandboxScreen.ATTRS {
    focus_path='sandbox',
    force_pause=true,
    pass_pause=false,
    defocusable=false,
}

local RAWS = df.global.world.raws
local MAT_TABLE = RAWS.mat_table

-- elements of df.entity_sell_category
local EQUIPMENT_TYPES = {
    Weapons={itemdefs=RAWS.itemdefs.weapons,
        item_type=df.item_type.WEAPON,
        def_filter=function(def) return not def.flags.TRAINING end,
        mat_filter=function(mat) return mat.flags.ITEMS_WEAPON and mat.flags.ITEMS_METAL end},
    TrainingWeapons={itemdefs=RAWS.itemdefs.weapons,
        item_type=df.item_type.WEAPON,
        def_filter=function(def) return def.flags.TRAINING end,
        mat_filter=function(mat) return mat.flags.WOOD end},
    Ammo={itemdefs=RAWS.itemdefs.ammo,
        item_type=df.item_type.AMMO,
        mat_filter=function(mat) return mat.flags.ITEMS_AMMO end},
    Quivers={itemdefs={{subtype=-1}},
        item_type=df.item_type.QUIVER,
        want_leather=true,
        mat_filter=function(mat) return mat.flags.LEATHER end},
    Bodywear={itemdefs=RAWS.itemdefs.armor,
        item_type=df.item_type.ARMOR,
        want_leather=true,
        mat_filter=function(mat) return mat.flags.ITEMS_ARMOR or mat.flags.LEATHER end},
    Headwear={itemdefs=RAWS.itemdefs.helms,
        item_type=df.item_type.HELM,
        want_leather=true,
        mat_filter=function(mat) return mat.flags.ITEMS_ARMOR or mat.flags.LEATHER end},
    Handwear={itemdefs=RAWS.itemdefs.gloves,
        item_type=df.item_type.GLOVES,
        want_leather=true,
        mat_filter=function(mat) return mat.flags.ITEMS_ARMOR or mat.flags.LEATHER end},
    Footwear={itemdefs=RAWS.itemdefs.shoes,
        item_type=df.item_type.SHOES,
        want_leather=true,
        mat_filter=function(mat) return mat.flags.ITEMS_ARMOR or mat.flags.LEATHER end},
    Legwear={itemdefs=RAWS.itemdefs.pants,
        item_type=df.item_type.PANTS,
        want_leather=true,
        mat_filter=function(mat) return mat.flags.ITEMS_ARMOR or mat.flags.LEATHER end},
    Shields={itemdefs=RAWS.itemdefs.shields,
        item_type=df.item_type.SHIELD,
        want_leather=true,
        mat_filter=function(mat) return mat.flags.ITEMS_ARMOR or mat.flags.LEATHER end},
}

local function scan_organic(cat, vec, start_idx, base, do_insert)
    local indexes = MAT_TABLE.organic_indexes[cat]
    for idx = start_idx,#indexes-1 do
        local matindex = indexes[idx]
        local organic = vec[matindex]
        for offset, mat in ipairs(organic.material) do
            if do_insert(mat, base + offset, matindex) then
                return matindex
            end
        end
    end
    return 0
end

local function init_arena()
    local arena = df.global.world.arena
    local arena_unit = df.global.game.main_interface.arena_unit
    local arena_tree = df.global.game.main_interface.arena_tree
    local leather_index_hint, plant_index_hint = 0, 0

    -- races
    arena.race:resize(0)
    arena.caste:resize(0)
    arena.creature_cnt:resize(0)
    arena.type = -1
    arena_unit.race = 0
    arena_unit.caste = 0
    arena_unit.races_filtered:resize(0)
    arena_unit.races_all:resize(0)
    arena_unit.castes_filtered:resize(0)
    arena_unit.castes_all:resize(0)
    local arena_creatures = {}
    for i, cre in ipairs(RAWS.creatures.all) do
        if not cre.flags.VERMIN_GROUNDER and not cre.flags.VERMIN_SOIL
            and not cre.flags.DOES_NOT_EXIST and not cre.flags.EQUIPMENT_WAGON
        then
            table.insert(arena_creatures, {race=i, cre=cre})
        end
    end
    table.sort(arena_creatures,
            function(a, b) return string.lower(a.cre.name[0]) < string.lower(b.cre.name[0]) end)
    for _, cre_data in ipairs(arena_creatures) do
        arena.creature_cnt:insert('#', 0)
        for caste in ipairs(cre_data.cre.caste) do
            arena.race:insert('#', cre_data.race)
            arena.caste:insert('#', caste)
        end
    end

    -- interactions
    -- note this doesn't actually come up with anything in vanilla. normal arena
    -- mode reads from the files in data/vanilla/interaction examples/ where some
    -- usable insteractions exist
    arena.interactions:resize(0)
    arena.interaction = -1
    arena_unit.interactions:resize(0)
    arena_unit.interaction = -1
    for _, inter in ipairs(RAWS.interactions) do
        for _, effect in ipairs(inter.effects) do
            if #effect.arena_name > 0 then
                arena.interactions:insert('#', effect)
            end
        end
    end

    -- skills
    arena.skills:resize(0)
    arena.skill_levels:resize(0)
    arena_unit.skills:resize(0)
    arena_unit.skill_levels:resize(0)
    for i in ipairs(df.job_skill) do
        if i >= 0 then
            arena.skills:insert('#', i)
            arena.skill_levels:insert('#', 0)
        end
    end

    -- equipment
    -- this is slow, so optimize for speed:
    -- - use pre-allocated structures if possible
    -- - don't scan past the basic metals
    -- - only scan until we fine one kind of thing. we don't need 1000 types of leather
    --   or 40 types of wood
    -- - remember the last matched material and try that again for the next item type
    for idx, list in ipairs(arena.item_types.list) do
        local list_size = 0
        local data = EQUIPMENT_TYPES[df.entity_sell_category[idx]]
        if not data then goto continue end
        for _,itemdef in ipairs(data.itemdefs) do
            if data.def_filter and not data.def_filter(itemdef) then goto inner_continue end
            local do_insert = function(mat, mattype, matindex)
                if data.mat_filter and not data.mat_filter(mat) then return end
                local element = {
                    item_type=data.item_type,
                    item_subtype=itemdef.subtype,
                    mattype=mattype,
                    matindex=matindex,
                    unk_c=1}
                if #list > list_size then
                    utils.assign(list[list_size], element)
                else
                    element.new = df.embark_item_choice.T_list
                    list:insert('#', element)
                end
                list_size = list_size + 1
                return true
            end
            -- if there is call for glass tools, uncomment this
            -- for i in ipairs(df.builtin_mats) do
            --     do_insert(MAT_TABLE.builtin[i], i, -1)
            -- end
            for i, mat in ipairs(RAWS.inorganics) do
                do_insert(mat.material, 0, i)
                -- stop at the first "special" metal. we don't need more than that
                if mat.flags.DEEP_SPECIAL then break end
            end
            if data.want_leather then
                leather_index_hint = scan_organic(df.organic_mat_category.Leather, RAWS.creatures.all, leather_index_hint, materials.CREATURE_BASE, do_insert)
            end
            plant_index_hint = scan_organic(df.organic_mat_category.Wood, RAWS.plants.all, plant_index_hint, materials.PLANT_BASE, do_insert)
            ::inner_continue::
        end
        ::continue::
        for list_idx=list_size,#list-1 do
            df.delete(list[list_idx])
        end
        list:resize(list_size)
    end

    -- trees
    arena.tree_types:resize(0)
    arena.tree_age = 100
    arena_tree.tree_types_filtered:resize(0)
    arena_tree.tree_types_all:resize(0)
    arena_tree.age = 100
    for _, tree in ipairs(RAWS.plants.trees) do
        arena.tree_types:insert('#', tree)
    end
end

function SandboxScreen:init()
    init_arena()

    local mask_panel = widgets.Panel{
        subviews={
            InterfaceMask{frame={t=18, r=2, w=22, h=6}},
            InterfaceMask{frame={l=0, r=0, b=0, h=3}},
        },
    }

    self:addviews{
        mask_panel,
        Sandbox{
            view_id='sandbox',
            interface_masks=mask_panel.subviews,
        },
    }

    self.prev_gametype = df.global.gametype
    df.global.gametype = df.game_type.DWARF_ARENA
end

function SandboxScreen:onDismiss()
    df.global.gametype = self.prev_gametype
    view = nil
    self.subviews.sandbox:finalize_group()
end

if not dfhack.isWorldLoaded() then
    qerror('gui/sandbox must have a world loaded')
end

view = view and view:raise() or SandboxScreen{}:show()
