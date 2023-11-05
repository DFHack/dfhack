local _ENV = mkmodule('plugins.sort.info')

local gui = require('gui')
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

-- these sort functions attempt to match the vanilla info panel sort behavior, which
-- is not quite the same as the rest of DFHack. For example, in other DFHack sorts,
-- we'd always sort by name descending as a secondary sort. To match vanilla sorting,
-- if the primary sort is ascending, the secondary name sort will also be ascending.
--
-- also note that vanilla sorts are not stable, so there might still be some jitter
-- if the player clicks one of the vanilla sort widgets after searching
local function sort_by_name_desc(a, b)
    return a.sort_name < b.sort_name
end

local function sort_by_name_asc(a, b)
    return a.sort_name > b.sort_name
end

local function sort_by_prof_desc(a, b)
    if a.profession_list_order1 == b.profession_list_order1 then
        return sort_by_name_desc(a, b)
    end
    return a.profession_list_order1 < b.profession_list_order1
end

local function sort_by_prof_asc(a, b)
    if a.profession_list_order1 == b.profession_list_order1 then
        return sort_by_name_asc(a, b)
    end
    return a.profession_list_order1 > b.profession_list_order1
end

local function sort_by_job_name_desc(a, b)
    if a.job_sort_name == b.job_sort_name then
        return sort_by_name_desc(a, b)
    end
    return a.job_sort_name < b.job_sort_name
end

local function sort_by_job_name_asc(a, b)
    if a.job_sort_name == b.job_sort_name then
        -- use descending tertiary sort for visual stability
        return sort_by_name_desc(a, b)
    end
    return a.job_sort_name > b.job_sort_name
end

local function sort_by_job_desc(a, b)
    if not not a.jb == not not b.jb then
        return sort_by_job_name_desc(a, b)
    end
    return not not a.jb
end

local function sort_by_job_asc(a, b)
    if not not a.jb == not not b.jb then
        return sort_by_job_name_asc(a, b)
    end
    return not not b.jb
end

local function sort_by_stress_desc(a, b)
    if a.stress == b.stress then
        return sort_by_name_desc(a, b)
    end
    return a.stress > b.stress
end

local function sort_by_stress_asc(a, b)
    if a.stress == b.stress then
        return sort_by_name_asc(a, b)
    end
    return a.stress < b.stress
end

local function get_sort()
    if creatures.sorting_cit_job then
        return creatures.sorting_cit_job_is_ascending and sort_by_job_asc or sort_by_job_desc
    elseif creatures.sorting_cit_stress then
        return creatures.sorting_cit_stress_is_ascending and sort_by_stress_asc or sort_by_stress_desc
    elseif creatures.sorting_cit_nameprof_doing_prof then
        return creatures.sorting_cit_nameprof_is_ascending and sort_by_prof_asc or sort_by_prof_desc
    else
        return creatures.sorting_cit_nameprof_is_ascending and sort_by_name_asc or sort_by_name_desc
    end
end

local function get_cri_unit_search_key(cri_unit)
    return ('%s %s'):format(
        cri_unit.un and sortoverlay.get_unit_search_key(cri_unit.un) or '',
        cri_unit.job_sort_name)
end

local function get_race_name(raw_id)
    local raw = df.creature_raw.find(raw_id)
    if not raw then return end
    return raw.name[1]
end

-- get name in both dwarvish and English
local function get_artifact_search_key(artifact)
    return ('%s %s'):format(dfhack.TranslateName(artifact.name), dfhack.TranslateName(artifact.name, true))
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

local function restore_allocated_data(vec, data)
    if not data.saved_visible or not data.saved_original then return end
    for _,elem in ipairs(data.saved_original) do
        if not utils.linear_index(data.saved_visible, elem) then
            vec:insert('#', elem)
        end
    end
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
    default_pos={x=64, y=8},
    viewscreens='dwarfmode/Info',
    frame={w=40, h=6},
}

local function get_squad_options()
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

local function get_burrow_options()
    local options = {{label='Any', value='all', pen=COLOR_GREEN}}
    for _, burrow in ipairs(df.global.plotinfo.burrows.list) do
        table.insert(options, {
            label=#burrow.name > 0 and burrow.name or ('Burrow %d'):format(burrow.id + 1),
            value=burrow.id,
            pen=COLOR_YELLOW,
        })
    end
    return options
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
                    frame={l=1, t=0},
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
                    frame={l=1, t=0},
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
                    frame={l=1, t=0},
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

    local CRI_UNIT_VECS = {
        CITIZEN=creatures.cri_unit.CITIZEN,
        PET=creatures.cri_unit.PET,
        OTHER=creatures.cri_unit.OTHER,
        DECEASED=creatures.cri_unit.DECEASED,
    }
    for key,vec in pairs(CRI_UNIT_VECS) do
        self:register_handler(key, vec,
            curry(sortoverlay.single_vector_search,
                {
                    get_search_key_fn=get_cri_unit_search_key,
                    get_sort_fn=get_sort
                }),
            curry(restore_allocated_data, vec))
    end

    self:register_handler('JOBS', tasks.cri_job,
        curry(sortoverlay.single_vector_search, {get_search_key_fn=get_cri_unit_search_key}),
        curry(restore_allocated_data, tasks.cri_job))
    self:register_handler('PET_OT', creatures.atk_index,
        curry(sortoverlay.single_vector_search, {get_search_key_fn=get_race_name}))
    self:register_handler('PET_AT', creatures.trainer,
        curry(sortoverlay.single_vector_search, {get_search_key_fn=sortoverlay.get_unit_search_key}))
    self:register_handler('PET_WA', creatures.work_animal_recipient,
        curry(sortoverlay.single_vector_search, {
            get_search_key_fn=sortoverlay.get_unit_search_key,
            matches_filters_fn=self:callback('matches_filters'),
        }))
    self:register_handler('WORK_DETAILS', work_details.assignable_unit, work_details_search)

    for idx,name in ipairs(df.artifacts_mode_type) do
        if idx < 0 then goto continue end
        self:register_handler(name, objects.list[idx],
            curry(sortoverlay.single_vector_search, {get_search_key_fn=get_artifact_search_key}))
        ::continue::
    end
end

function InfoOverlay:get_key()
    if info.current_mode == df.info_interface_mode_type.CREATURES then
        if creatures.current_mode == df.unit_list_mode_type.PET then
            if creatures.showing_overall_training then
                return 'PET_OT'
            elseif creatures.adding_trainer then
                return 'PET_AT'
            elseif creatures.assign_work_animal then
                return 'PET_WA'
            end
        end
        return df.unit_list_mode_type[creatures.current_mode]
    elseif info.current_mode == df.info_interface_mode_type.JOBS then
        return 'JOBS'
    elseif info.current_mode == df.info_interface_mode_type.ARTIFACTS then
        return df.artifacts_mode_type[objects.mode]
    elseif info.current_mode == df.info_interface_mode_type.LABOR then
        if info.labor.mode == df.labor_mode_type.WORK_DETAILS then
            return 'WORK_DETAILS'
        end
    end
end

local function resize_overlay(self)
    local sw = dfhack.screen.getWindowSize()
    local overlay_width = math.min(40, sw-(self.frame_rect.x1 + 30))
    if overlay_width ~= self.frame.w then
        self.frame.w = overlay_width
        return true
    end
end

local function is_tabs_in_two_rows()
    return dfhack.screen.readTile(64, 6, false).ch == 0
end

local function get_panel_offsets()
    local tabs_in_two_rows = is_tabs_in_two_rows()
    local shift_right = info.current_mode == df.info_interface_mode_type.ARTIFACTS or
        info.current_mode == df.info_interface_mode_type.LABOR
    local l_offset = (not tabs_in_two_rows and shift_right) and 4 or 0
    local t_offset = 1
    if tabs_in_two_rows then
        t_offset = shift_right and 0 or 3
    end
    if info.current_mode == df.info_interface_mode_type.JOBS then
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

function InfoOverlay:onRenderBody(dc)
    InfoOverlay.super.onRenderBody(self, dc)
    if self:updateFrames() then
        self:updateLayout()
    end
    if self.refresh_search then
        self.refresh_search = nil
        self:do_search(self.subviews.search.text)
    end
end

function InfoOverlay:onInput(keys)
    if keys._MOUSE_L and self:get_key() == 'WORK_DETAILS' then
        self.refresh_search = true
    end
    return InfoOverlay.super.onInput(self, keys)
end

function InfoOverlay:matches_filters(unit)
    local subset = self.subviews.subset:getOptionValue()
    if subset == 'all' then
        return true
    elseif subset == 'civilian' then
        return unit.military.squad_id == -1
    elseif subset == 'military' then
        local squad_id = unit.military.squad_id
        if squad_id == -1 then return false end
        local target_id = self.subviews.squad:getOptionValue()
        if target_id == 'all' then return true end
        return target_id == squad_id
    elseif subset == 'burrow' then
        local target_id = self.subviews.burrow:getOptionValue()
        if target_id == 'all' then return true end
        return utils.binsearch(unit.burrows, target_id)
    end
    return true
end

-- ----------------------
-- CandidatesOverlay
--

CandidatesOverlay = defclass(CandidatesOverlay, sortoverlay.SortOverlay)
CandidatesOverlay.ATTRS{
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
        curry(restore_allocated_data, administrators.candidate))
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
-- InterrogationOverlay
--

InterrogationOverlay = defclass(InterrogationOverlay, sortoverlay.SortOverlay)
InterrogationOverlay.ATTRS{
    default_pos={x=47, y=10},
    viewscreens='dwarfmode/Info/JUSTICE',
    frame={w=27, h=9},
}

function InterrogationOverlay:init()
    self:addviews{
        widgets.Panel{
            view_id='panel',
            frame={l=0, t=4, h=5, r=0},
            frame_background=gui.CLEAR_PEN,
            frame_style=gui.FRAME_MEDIUM,
            visible=self:callback('get_key'),
            subviews={
                widgets.EditField{
                    view_id='search',
                    frame={l=0, t=0, r=0},
                    label_text="Search: ",
                    key='CUSTOM_ALT_S',
                    on_change=function(text) self:do_search(text) end,
                },
                widgets.ToggleHotkeyLabel{
                    view_id='include_interviewed',
                    frame={l=0, t=1, w=23},
                    key='CUSTOM_SHIFT_I',
                    label='Interviewed:',
                    options={
                        {label='Include', value=true, pen=COLOR_GREEN},
                        {label='Exclude', value=false, pen=COLOR_RED},
                    },
                    visible=function() return justice.interrogating end,
                    on_change=function() self:do_search(self.subviews.search.text, true) end,
                },
                widgets.CycleHotkeyLabel{
                    view_id='subset',
                    frame={l=0, t=2, w=28},
                    key='CUSTOM_SHIFT_F',
                    label='Show:',
                    options={
                        {label='All', value='all', pen=COLOR_GREEN},
                        {label='Risky visitors', value='risky', pen=COLOR_RED},
                        {label='Other visitors', value='visitors', pen=COLOR_LIGHTRED},
                        {label='Residents', value='residents', pen=COLOR_YELLOW},
                        {label='Citizens', value='citizens', pen=COLOR_CYAN},
                        {label='Animals', value='animals', pen=COLOR_BLUE},
                        {label='Deceased or missing', value='deceased', pen=COLOR_MAGENTA},
                        {label='Others', value='others', pen=COLOR_GRAY},
                    },
                    on_change=function() self:do_search(self.subviews.search.text, true) end,
                },
            },
        },
    }

    self:register_handler('INTERROGATING', justice.interrogation_list,
        curry(sortoverlay.flags_vector_search,
            {
                get_search_key_fn=sortoverlay.get_unit_search_key,
                get_elem_id_fn=function(unit) return unit.id end,
                matches_filters_fn=self:callback('matches_filters'),
            },
        justice.interrogation_list_flag))
    self:register_handler('CONVICTING', justice.conviction_list,
        curry(sortoverlay.single_vector_search,
            {
                get_search_key_fn=sortoverlay.get_unit_search_key,
                matches_filters_fn=self:callback('matches_filters'),
            }))
end

function InterrogationOverlay:reset()
    InterrogationOverlay.super.reset(self)
    self.subviews.include_interviewed:setOption(true, false)
    self.subviews.subset:setOption('all')
end

function InterrogationOverlay:get_key()
    if justice.interrogating then
        return 'INTERROGATING'
    elseif justice.convicting then
        return 'CONVICTING'
    end
end

local RISKY_PROFESSIONS = utils.invert{
    df.profession.THIEF,
    df.profession.MASTER_THIEF,
    df.profession.CRIMINAL,
}

local function is_risky(unit)
    if RISKY_PROFESSIONS[unit.profession] or RISKY_PROFESSIONS[unit.profession2] then
        return true
    end
    if dfhack.units.getReadableName(unit):endswith('necromancer') then return true end
    return not dfhack.units.isAlive(unit)  -- detect intelligent undead
end

function InterrogationOverlay:matches_filters(unit, flag)
    if justice.interrogating then
        local include_interviewed = self.subviews.include_interviewed:getOptionValue()
        if not include_interviewed and flag == 2 then return false end
    end
    local subset = self.subviews.subset:getOptionValue()
    if subset == 'all' then
        return true
    elseif dfhack.units.isDead(unit) or not dfhack.units.isActive(unit) then
        return subset == 'deceased'
    elseif dfhack.units.isInvader(unit) or dfhack.units.isOpposedToLife(unit)
        or unit.flags2.visitor_uninvited or unit.flags4.agitated_wilderness_creature
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

function InterrogationOverlay:render(dc)
    local sw = dfhack.screen.getWindowSize()
    local info_panel_border = 31 -- from edges of panel to screen edges
    local info_panel_width = sw - info_panel_border
    local info_panel_center = info_panel_width // 2
    local panel_x_offset = (info_panel_center + 5) - self.frame_rect.x1
    local frame_w = math.min(panel_x_offset + 37, info_panel_width - 56)
    local panel_l = panel_x_offset
    local panel_t = is_tabs_in_two_rows() and 4 or 0

    if self.frame.w ~= frame_w or
        self.subviews.panel.frame.l ~= panel_l or
        self.subviews.panel.frame.t ~= panel_t
    then
        self.frame.w = frame_w
        self.subviews.panel.frame.l = panel_l
        self.subviews.panel.frame.t = panel_t
        self:updateLayout()
    end

    InterrogationOverlay.super.render(self, dc)
end

return _ENV
