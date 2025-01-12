local _ENV = mkmodule('plugins.sort.info')

local gui = require('gui')
local overlay = require('plugins.overlay')
local sortoverlay = require('plugins.sort.sortoverlay')
local widgets = require('gui.widgets')
local utils = require('utils')

local info = df.global.game.main_interface.info
local administrators = info.administrators
local creatures = info.creatures
local justice = info.justice
local objects = info.artifacts
local tasks = info.jobs
local work_details = info.labor.work_details

local function get_cri_unit_search_key(cri_unit)
    return ('%s %s'):format(
        cri_unit.un and sortoverlay.get_unit_search_key(cri_unit.un) or '',
        cri_unit.job_sort_name)
end

local function get_task_search_key(cri_unit)
    local result = {get_cri_unit_search_key(cri_unit)}

    if cri_unit.jb then
        local bld = dfhack.job.getHolder(cri_unit.jb)
        if bld then
            table.insert(result, bld.name)
            local btype = bld:getType()
            if btype == df.building_type.Workshop then
                table.insert(result, df.workshop_type.attrs[bld.type].name or '')
                table.insert(result, df.workshop_type[bld.type])
            elseif btype == df.building_type.Furnace then
                table.insert(result, df.furnace_type[bld.type])
            elseif btype == df.building_type.Construction then
                table.insert(result, df.construction_type[bld.type])
            elseif btype == df.building_type.Trap then
                table.insert(result, df.trap_type[bld.trap_type])
            elseif btype == df.building_type.SiegeEngine then
                table.insert(result, df.siegeengine_type[bld.type])
            else
                table.insert(result, df.building_type.attrs[btype].name)
            end
        end
    end

    return table.concat(result, ' ')
end

local function get_race_name(raw_id)
    local raw = df.creature_raw.find(raw_id)
    if not raw then return end
    return raw.name[1]
end

-- get name in both dwarven and English
local function get_artifact_search_key(artifact)
    return ('%s %s'):format(dfhack.translation.translateName(artifact.name),
        dfhack.translation.translateName(artifact.name, true))
end

local function work_details_search(vec, data, text, incremental)
    if work_details.selected_work_detail_index ~= data.selected then
        data.saved_original = nil
        data.selected = work_details.selected_work_detail_index
    end
    sortoverlay.single_vector_search(
        {get_search_key_fn=sortoverlay.get_unit_search_key},
        vec, data, text, incremental)
end

local function free_allocated_data(data)
    if data.saved_visible and data.saved_original and #data.saved_visible ~= #data.saved_original then
        for _,elem in ipairs(data.saved_original) do
            if not utils.linear_index(data.saved_visible, elem) then
                elem:delete()
            end
        end
    end
    data.saved_original, data.saved_visible = nil, nil
end

local function serialize_skills(unit)
    if not unit or not unit.status or not unit.status.current_soul then
        return ''
    end
    local skills = {}
    for _, skill in ipairs(unit.status.current_soul.skills) do
        if skill.rating > 0 then -- ignore dabbling
            table.insert(skills, df.job_skill[skill.id])
        end
    end
    return table.concat(skills, ' ')
end

local function get_candidate_search_key(cand)
    if not cand.un then return end
    return ('%s %s'):format(
        sortoverlay.get_unit_search_key(cand.un),
        serialize_skills(cand.un))
end

-- ----------------------
-- InfoOverlay
--

InfoOverlay = defclass(InfoOverlay, sortoverlay.SortOverlay)
InfoOverlay.ATTRS{
    desc='Adds search and filter functionality to info screens.',
    default_pos={x=64, y=8},
    viewscreens={
        'dwarfmode/Info/JOBS',
        'dwarfmode/Info/ARTIFACTS/ARTIFACTS',
        'dwarfmode/Info/ARTIFACTS/SYMBOLS',
        'dwarfmode/Info/ARTIFACTS/NAMED_OBJECTS',
        'dwarfmode/Info/ARTIFACTS/WRITTEN_CONTENT',
    },
    frame={w=40, h=6},
}

function get_squad_options()
    local options = {{label='Any', value='all', pen=COLOR_GREEN}}
    local fort = df.historical_entity.find(df.global.plotinfo.group_id)
    if not fort then return options end
    for _, squad_id in ipairs(fort.squads) do
        table.insert(options, {
            label=dfhack.military.getSquadName(squad_id),
            value=squad_id,
            pen=COLOR_YELLOW,
        })
    end
    return options
end

function get_burrow_options()
    local options = {
        {label='Any', value='all', pen=COLOR_GREEN},
        {label='Unburrowed', value='none', pen=COLOR_LIGHTRED},
    }
    for _, burrow in ipairs(df.global.plotinfo.burrows.list) do
        table.insert(options, {
            label=#burrow.name > 0 and burrow.name or ('Burrow %d'):format(burrow.id + 1),
            value=burrow.id,
            pen=COLOR_YELLOW,
        })
    end
    return options
end

function matches_squad_burrow_filters(unit, subset, target_squad_id, target_burrow_id)
    if subset == 'all' then
        return true
    elseif subset == 'civilian' then
        return unit.military.squad_id == -1
    elseif subset == 'military' then
        local squad_id = unit.military.squad_id
        if squad_id == -1 then return false end
        if target_squad_id == 'all' then return true end
        return target_squad_id == squad_id
    elseif subset == 'burrow' then
        if target_burrow_id == 'all' then return #unit.burrows + #unit.inactive_burrows > 0 end
        if target_burrow_id == 'none' then return #unit.burrows + #unit.inactive_burrows == 0 end
        return utils.binsearch(unit.burrows, target_burrow_id) or
            utils.binsearch(unit.inactive_burrows, target_burrow_id)
    end
    return true
end

function InfoOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            view_id='panel',
            frame={l=0, t=0, r=0, h=1},
            visible=self:callback('get_key'),
            subviews={
                widgets.EditField{
                    view_id='search',
                    frame={l=1, t=0, r=1},
                    label_text="Search: ",
                    key='CUSTOM_ALT_S',
                    on_change=function(text) self:do_search(text) end,
                },
            },
        },
        widgets.BannerPanel{
            view_id='subset_panel',
            frame={l=0, t=1, r=0, h=1},
            visible=function() return self:get_key() == 'PET_WA' end,
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='subset',
                    frame={l=1, t=0, r=1},
                    key='CUSTOM_SHIFT_F',
                    label='Show:',
                    options={
                        {label='All', value='all', pen=COLOR_GREEN},
                        {label='Military', value='military', pen=COLOR_YELLOW},
                        {label='Civilians', value='civilian', pen=COLOR_CYAN},
                        {label='Burrowed', value='burrow', pen=COLOR_MAGENTA},
                    },
                    on_change=function(value)
                        local squad = self.subviews.squad
                        local burrow = self.subviews.burrow
                        squad.visible = false
                        burrow.visible = false
                        if value == 'military' then
                            squad.options = get_squad_options()
                            squad:setOption('all')
                            squad.visible = true
                        elseif value == 'burrow' then
                            burrow.options = get_burrow_options()
                            burrow:setOption('all')
                            burrow.visible = true
                        end
                        self:do_search(self.subviews.search.text, true)
                    end,
                },
            },
        },
        widgets.BannerPanel{
            view_id='subfilter_panel',
            frame={l=0, t=2, r=0, h=1},
            visible=function()
                local subset = self.subviews.subset:getOptionValue()
                return self:get_key() == 'PET_WA' and (subset == 'military' or subset == 'burrow')
            end,
            subviews={
                widgets.CycleHotkeyLabel{
                    view_id='squad',
                    frame={l=1, t=0, r=1},
                    key='CUSTOM_SHIFT_S',
                    label='Squad:',
                    options={
                        {label='Any', value='all', pen=COLOR_GREEN},
                    },
                    visible=false,
                    on_change=function() self:do_search(self.subviews.search.text, true) end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='burrow',
                    frame={l=1, t=0, r=1},
                    key='CUSTOM_SHIFT_B',
                    label='Burrow:',
                    options={
                        {label='Any', value='all', pen=COLOR_GREEN},
                    },
                    visible=false,
                    on_change=function() self:do_search(self.subviews.search.text, true) end,
                },
            },
        },
    }

    self:register_handler('JOBS', tasks.cri_job,
        curry(sortoverlay.single_vector_search, {get_search_key_fn=get_task_search_key}),
        free_allocated_data)

    for idx,name in ipairs(df.artifacts_mode_type) do
        if idx < 0 then goto continue end
        self:register_handler(name, objects.list[idx],
            curry(sortoverlay.single_vector_search, {get_search_key_fn=get_artifact_search_key}))
        ::continue::
    end
end

function InfoOverlay:reset()
    InfoOverlay.super.reset(self)
    self.subviews.subset:setOption('all')
end

function InfoOverlay:get_key()
    if info.current_mode == df.info_interface_mode_type.JOBS then
        return 'JOBS'
    elseif info.current_mode == df.info_interface_mode_type.ARTIFACTS then
        return df.artifacts_mode_type[objects.mode]
    end
end

function resize_overlay(self)
    local sw = dfhack.screen.getWindowSize()
    local overlay_width = math.min(40, sw-(self.frame_rect.x1 + 30))
    if overlay_width ~= self.frame.w then
        self.frame.w = overlay_width
        return true
    end
end

local function is_tabs_in_two_rows()
    return gui.get_interface_rect().width < 155
end

function get_panel_offsets()
    local tabs_in_two_rows = is_tabs_in_two_rows()
    local shift_right = info.current_mode == df.info_interface_mode_type.ARTIFACTS or
        info.current_mode == df.info_interface_mode_type.LABOR
    local l_offset = (not tabs_in_two_rows and shift_right) and 4 or 0
    local t_offset = 1
    if tabs_in_two_rows then
        t_offset = shift_right and 0 or 3
    end
    if info.current_mode == df.info_interface_mode_type.JOBS or
    info.current_mode == df.info_interface_mode_type.BUILDINGS then
        t_offset = t_offset - 1
    end
    return l_offset, t_offset
end

function InfoOverlay:updateFrames()
    local ret = resize_overlay(self)
    local l, t = get_panel_offsets()
    local frame = self.subviews.panel.frame
    if frame.l == l and frame.t == t then return ret end
    frame.l, frame.t = l, t
    local frame2 = self.subviews.subset_panel.frame
    frame2.l, frame2.t = l, t + 1
    local frame3 = self.subviews.subfilter_panel.frame
    frame3.l, frame3.t = l, t + 2
    return true
end

function InfoOverlay:do_refresh()
    self.refresh_search = nil
    if self:get_key() == 'JOBS' then
        local data = self.state.JOBS
        -- if any jobs have been canceled, fix up our data vectors
        if data and data.saved_visible and data.saved_original then
            local to_remove = {}
            for _,elem in ipairs(data.saved_visible) do
                if not utils.linear_index(tasks.cri_job, elem) then
                    table.insert(to_remove, elem)
                end
            end
            for _,elem in ipairs(to_remove) do
                table.remove(data.saved_visible, utils.linear_index(data.saved_visible, elem))
                data.saved_visible_size = data.saved_visible_size - 1
                table.remove(data.saved_original, utils.linear_index(data.saved_original, elem))
                data.saved_original_size = data.saved_original_size - 1
            end
        end
    end
    self:do_search(self.subviews.search.text, true)
end

function InfoOverlay:onRenderBody(dc)
    if self.refresh_search then
        self:do_refresh()
    end
    InfoOverlay.super.onRenderBody(self, dc)
    if self:updateFrames() then
        self:updateLayout()
    end
end

function InfoOverlay:onInput(keys)
    if self.refresh_search then
        self:do_refresh()
    end
    if keys._MOUSE_L then
        local key = self:get_key()
        if key == 'WORK_DETAILS' or key == 'JOBS' then
            self.refresh_search = true
        end
    end
    return InfoOverlay.super.onInput(self, keys)
end

function InfoOverlay:matches_filters(unit)
    return matches_squad_burrow_filters(unit, self.subviews.subset:getOptionValue(),
        self.subviews.squad:getOptionValue(), self.subviews.burrow:getOptionValue())
end

-- ----------------------
-- CandidatesOverlay
--

CandidatesOverlay = defclass(CandidatesOverlay, sortoverlay.SortOverlay)
CandidatesOverlay.ATTRS{
    desc='Adds search functionality to the noble assignment page.',
    default_pos={x=54, y=8},
    viewscreens='dwarfmode/Info/ADMINISTRATORS/Candidates',
    frame={w=27, h=3},
}

function CandidatesOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            view_id='panel',
            frame={l=0, t=0, r=0, h=1},
            subviews={
                widgets.EditField{
                    view_id='search',
                    frame={l=1, t=0, r=1},
                    label_text="Search: ",
                    key='CUSTOM_ALT_S',
                    on_change=function(text) self:do_search(text) end,
                },
            },
        },
    }

    self:register_handler('CANDIDATE', administrators.candidate,
        curry(sortoverlay.single_vector_search, {get_search_key_fn=get_candidate_search_key}),
        free_allocated_data)
end

function CandidatesOverlay:get_key()
    if administrators.choosing_candidate then
        return 'CANDIDATE'
    end
end

function CandidatesOverlay:updateFrames()
    local t = is_tabs_in_two_rows() and 2 or 0
    local frame = self.subviews.panel.frame
    if frame.t == t then return end
    frame.t = t
    return true
end

function CandidatesOverlay:onRenderBody(dc)
    CandidatesOverlay.super.onRenderBody(self, dc)
    if self:updateFrames() then
        self:updateLayout()
    end
end

-- ----------------------
-- WorkAnimalOverlay
--

WorkAnimalOverlay = defclass(WorkAnimalOverlay, overlay.OverlayWidget)
WorkAnimalOverlay.ATTRS{
    desc='Annotates units with how many work animals they have assigned on the assign work animal screen.',
    default_pos={x=79, y=11},
    viewscreens='dwarfmode/Info/CREATURES/AssignWorkAnimal',
    default_enabled=true,
    version=2,
    frame={w=29, h=1},
}

function WorkAnimalOverlay:init()
    self:addviews{
        widgets.Label{
            view_id='annotations',
            frame={t=0, l=0},
            text='',
        }
    }
end

local function get_work_animal_counts()
    local counts = {}
    for _,unit in ipairs(df.global.world.units.active) do
        if not dfhack.units.isOwnCiv(unit) or
            (not dfhack.units.isWar(unit) and not dfhack.units.isHunter(unit))
        then
            goto continue
        end
        local owner_id = unit.relationship_ids.PetOwner
        if owner_id == -1 then goto continue end
        counts[owner_id] = (counts[owner_id] or 0) + 1
        ::continue::
    end
    return counts
end

local function get_hunting_assignment()
    return dfhack.gui.getWidget(creatures, 'Tabs', 'Pets/Livestock', 'Hunting assignment')
end

local function get_scroll_rows()
    return dfhack.gui.getWidget(get_hunting_assignment(), 'Unit List', 1)
end

local function get_scroll_pos(scroll_rows)
    scroll_rows = scroll_rows or get_scroll_rows()
    return scroll_rows and scroll_rows.scroll or 0
end

function WorkAnimalOverlay:preUpdateLayout(parent_rect)
    local _, t = get_panel_offsets()
    local list_height = parent_rect.height - (17 + t)
    self.frame.h = list_height + t
    self.subviews.annotations.frame.t = t
end

function WorkAnimalOverlay:onRenderFrame(dc, rect)
    local scroll_rows = get_scroll_rows()
    if not scroll_rows then return end
    local rows = dfhack.gui.getWidgetChildren(scroll_rows)
    local scroll_pos = get_scroll_pos(scroll_rows)
    local max_elem = math.min(#rows, scroll_pos+scroll_rows.num_visible)

    local annotations = {}
    local counts = get_work_animal_counts()
    for idx=scroll_pos+1,max_elem do
        table.insert(annotations, NEWLINE)
        table.insert(annotations, NEWLINE)
        local unit = dfhack.gui.getWidget(rows[idx], 0).u
        local animal_count = counts[unit.id]
        if animal_count and animal_count > 0 then
            table.insert(annotations, {text='[', pen=COLOR_RED})
            table.insert(annotations, ('Assigned work animals: %d'):format(animal_count))
            table.insert(annotations, {text=']', pen=COLOR_RED})
        end
        table.insert(annotations, NEWLINE)
    end
    self.subviews.annotations:setText(annotations)
    self.subviews.annotations:updateLayout()

    WorkAnimalOverlay.super.onRenderFrame(self, dc, rect)
end

-- ------------------------
-- WorkAnimalFilterOverlay
--

WorkAnimalFilterOverlay = defclass(WorkAnimalFilterOverlay, overlay.OverlayWidget)
WorkAnimalFilterOverlay.ATTRS{
    desc='Adds filter capabilities to the work animal assignment screen.',
    default_pos={x=49, y=-6},
    default_enabled=true,
    viewscreens='dwarfmode/Info/CREATURES/AssignWorkAnimal',
    frame={w=35, h=5},
    frame_background=gui.CLEAR_PEN,
    frame_style=gui.FRAME_MEDIUM,
}

local function poke_list()
    get_hunting_assignment().sort_flags.NEEDS_RESORTED = true
end

filter_instance = nil

function WorkAnimalFilterOverlay:init()
    filter_instance = self

    self:addviews{
        widgets.CycleHotkeyLabel{
            view_id='subset',
            frame={l=0, t=0, r=0},
            key='CUSTOM_SHIFT_F',
            label='  Show:',
            options={
                {label='All', value='all', pen=COLOR_GREEN},
                {label='Military', value='military', pen=COLOR_YELLOW},
                {label='Civilians', value='civilian', pen=COLOR_CYAN},
                {label='Burrowed', value='burrow', pen=COLOR_MAGENTA},
            },
            on_change=function(value)
                local squad = self.subviews.squad
                local burrow = self.subviews.burrow
                squad.enabled = false
                burrow.enabled = false
                if value == 'military' then
                    squad.options = get_squad_options()
                    squad.enabled = true
                else
                    squad:setOption('all')
                end
                if value == 'burrow' then
                    burrow.options = get_burrow_options()
                    burrow.enabled = true
                else
                    burrow:setOption('all')
                end
                poke_list()
            end,
        },
        widgets.CycleHotkeyLabel{
            view_id='squad',
            frame={l=0, t=1, r=0},
            key='CUSTOM_SHIFT_S',
            label=' Squad:',
            options={
                {label='Any', value='all', pen=COLOR_GREEN},
            },
            enabled=false,
            on_change=poke_list,
        },
        widgets.CycleHotkeyLabel{
            view_id='burrow',
            frame={l=0, t=2, r=0},
            key='CUSTOM_SHIFT_B',
            label='Burrow:',
            options={
                {label='Any', value='all', pen=COLOR_GREEN},
            },
            enabled=false,
            on_change=poke_list,
        },
    }
end

function WorkAnimalFilterOverlay:render(dc)
    local unitlist = get_hunting_assignment()
    require('plugins.sort').sort_set_work_animal_assignment_filter_fn(unitlist)
    WorkAnimalFilterOverlay.super.render(self, dc)
end

function do_work_animal_assignment_filter(unit)
    local self = filter_instance
    return matches_squad_burrow_filters(unit,
        self.subviews.subset:getOptionValue(),
        self.subviews.squad:getOptionValue(),
        self.subviews.burrow:getOptionValue())
end

-- ----------------------
-- JusticeOverlay
--

local function get_unit_list(which)
    local tabs = dfhack.gui.getWidget(justice, 'Tabs')
    return dfhack.gui.getWidget(tabs, 'Open cases', 'Right panel', which) or
        dfhack.gui.getWidget(tabs, 'Cold cases', 'Right panel', which)
end

local function poke_list(which)
    get_unit_list(which).sort_flags.NEEDS_RESORTED = true
end

JusticeOverlay = defclass(JusticeOverlay, overlay.OverlayWidget)
JusticeOverlay.ATTRS{
    which=DEFAULT_NIL,
}

function JusticeOverlay:init()
    local panel = widgets.Panel{
        frame={t=0, b=0, r=0, w=30},
        frame_background=gui.CLEAR_PEN,
        frame_style=gui.FRAME_MEDIUM,
        autoarrange_subviews=true,
        subviews={
            widgets.CycleHotkeyLabel{
                view_id='subset',
                frame={l=0},
                key='CUSTOM_SHIFT_F',
                label='Show:',
                options={
                    {label='All', value='all', pen=COLOR_GREEN},
                    {label='Risky visitors', value='risky', pen=COLOR_RED},
                    {label='Other visitors', value='visitors', pen=COLOR_LIGHTRED},
                    {label='Residents', value='residents', pen=COLOR_YELLOW},
                    {label='Citizens', value='citizens', pen=COLOR_CYAN},
                    {label='Animals', value='animals', pen=COLOR_BLUE},
                    {label='Deceased or missing', value='deceased', pen=COLOR_LIGHTMAGENTA},
                    {label='Others', value='others', pen=COLOR_GRAY},
                },
                on_change=curry(poke_list, self.which),
            },
        },
    }
    self:add_widgets(panel)

    self:addviews{panel}
end

function JusticeOverlay:add_widgets(panel)
end

function JusticeOverlay:render(dc)
    require('plugins.sort').sort_set_justice_filter_fn(get_unit_list(self.which))
    JusticeOverlay.super.render(self, dc)
end

function JusticeOverlay:preUpdateLayout(parent_rect)
    self.frame.w = (parent_rect.width+1) // 2 + 60
end

-- ----------------------
-- InterrogationOverlay
--

interrogate_instance = nil

InterrogationOverlay = defclass(InterrogationOverlay, JusticeOverlay)
InterrogationOverlay.ATTRS{
    desc='Adds filter capabilities to the interrogation screen.',
    default_pos={x=1, y=-5},
    default_enabled=true,
    version=2,
    viewscreens='dwarfmode/Info/JUSTICE/Interrogating',
    frame={w=30, h=4},
    which='Interrogate'
}

function InterrogationOverlay:init()
    interrogate_instance = self
end

function InterrogationOverlay:add_widgets(panel)
    panel:addviews{
        widgets.ToggleHotkeyLabel{
            view_id='include_interviewed',
            frame={l=0},
            key='CUSTOM_SHIFT_I',
            label='Interviewed:',
            options={
                {label='Include', value=true, pen=COLOR_GREEN},
                {label='Exclude', value=false, pen=COLOR_LIGHTRED},
            },
            on_change=curry(poke_list, self.which),
        },
    }
end

-- ----------------------
-- ConvictionOverlay
--

convict_instance = nil

ConvictionOverlay = defclass(ConvictionOverlay, JusticeOverlay)
ConvictionOverlay.ATTRS{
    desc='Adds filter capabilities to the conviction screen.',
    default_pos={x=1, y=-6},
    default_enabled=true,
    viewscreens='dwarfmode/Info/JUSTICE/Convicting',
    frame={w=30, h=3},
    which='Convict'
}

function ConvictionOverlay:init()
    convict_instance = self
end

-- ----------------------
-- filtering logic
--

local RISKY_PROFESSIONS = utils.invert{
    df.profession.THIEF,
    df.profession.MASTER_THIEF,
    df.profession.CRIMINAL,
}

local function is_risky(unit)
    if RISKY_PROFESSIONS[unit.profession] or RISKY_PROFESSIONS[unit.profession2] then
        return true
    end
    if dfhack.units.getReadableName(unit):find('necromancer') then return true end
    return not dfhack.units.isAlive(unit)  -- detect intelligent undead
end

local function filter_matches(unit, subset)
    if subset == 'all' then
        return true
    elseif dfhack.units.isDead(unit) or not dfhack.units.isActive(unit) then
        return subset == 'deceased'
    elseif dfhack.units.isInvader(unit) or dfhack.units.isOpposedToLife(unit)
        or unit.flags2.visitor_uninvited or dfhack.units.isAgitated(unit)
    then
        return subset == 'others'
    elseif dfhack.units.isVisiting(unit) then
        local risky = is_risky(unit)
        return (subset == 'risky' and risky) or (subset == 'visitors' and not risky)
    elseif dfhack.units.isAnimal(unit) then
        return subset == 'animals'
    elseif dfhack.units.isCitizen(unit) then
        return subset == 'citizens'
    elseif unit.flags2.roaming_wilderness_population_source then
        return subset == 'others'
    end
    return subset == 'residents'
end

function do_justice_filter(unit)
    local self
    if dfhack.gui.matchFocusString('dwarfmode/Info/JUSTICE/Interrogating') then
        self = interrogate_instance
        if not self.subviews.include_interviewed:getOptionValue() and
            require('plugins.sort').sort_is_interviewed(unit)
        then
            return false
        end
    else
        self = convict_instance
    end
    return filter_matches(unit, self.subviews.subset:getOptionValue())
end

return _ENV
