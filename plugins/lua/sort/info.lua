local _ENV = mkmodule('plugins.sort.info')

local gui   = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')
local utils = require('utils')

local info = df.global.game.main_interface.info
local creatures = info.creatures
local justice = info.justice
local objects = info.artifacts
local tasks = info.jobs
local work_details = info.labor.work_details

local state = {}

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

local function copy_to_lua_table(vec)
    local tab = {}
    for k,v in ipairs(vec) do
        tab[k+1] = v
    end
    return tab
end

local function general_search(vec, get_search_key_fn, get_sort_fn, matches_filters_fn, data, filter, incremental)
    if not data.saved_original then
        data.saved_original = copy_to_lua_table(vec)
    elseif not incremental then
        vec:assign(data.saved_original)
    end
    if matches_filters_fn ~= DEFAULT_NIL or filter ~= '' then
        local search_tokens = filter:split()
        for idx = #vec-1,0,-1 do
            local search_key = get_search_key_fn(vec[idx])
            if (search_key and not utils.search_text(search_key, search_tokens)) or
                (matches_filters_fn ~= DEFAULT_NIL and not matches_filters_fn(vec[idx]))
            then
                vec:erase(idx)
            end
        end
    end
    data.saved_visible = copy_to_lua_table(vec)
    if get_sort_fn then
        table.sort(data.saved_visible, get_sort_fn())
        vec:assign(data.saved_visible)
    end
end

-- add dynamically allocated elements that were not visible at the time of
-- closure back to the vector so they can be cleaned up when it is next initialized
local function cri_unitst_cleanup(vec, data)
    if not data.saved_visible or not data.saved_original then return end
    for _,elem in ipairs(data.saved_original) do
        if not utils.linear_index(data.saved_visible, elem) then
            vec:insert('#', elem)
        end
    end
end

local function get_unit_search_key(unit)
    return ('%s %s %s'):format(
        dfhack.units.getReadableName(unit),  -- last name is in english
        dfhack.units.getProfessionName(unit),
        dfhack.TranslateName(unit.name, false, true))  -- get untranslated last name
end

local function make_cri_unitst_handlers(vec, sort_fn)
    return {
        search_fn=curry(general_search, vec,
            function(elem)
                return ('%s %s'):format(
                    elem.un and get_unit_search_key(elem.un) or '',
                    elem.job_sort_name)
            end,
            sort_fn),
        cleanup_fn=curry(cri_unitst_cleanup, vec),
    }
end

local function overall_training_search(matches_filters_fn, data, filter, incremental)
    general_search(creatures.atk_index, function(elem)
            local raw = df.creature_raw.find(elem)
            if not raw then return end
            return raw.name[1]
        end, nil, matches_filters_fn, data, filter, incremental)
end

local function assign_trainer_search(matches_filters_fn, data, filter, incremental)
    general_search(creatures.trainer, function(elem)
            if not elem then return end
            return ('%s %s'):format(dfhack.TranslateName(elem.name), dfhack.units.getProfessionName(elem))
        end, nil, matches_filters_fn, data, filter, incremental)
end

local function work_details_search(matches_filters_fn, data, filter, incremental)
    if work_details.selected_work_detail_index ~= data.selected then
        data.saved_original = nil
        data.selected = work_details.selected_work_detail_index
    end
    general_search(work_details.assignable_unit, get_unit_search_key,
        nil, matches_filters_fn, data, filter, incremental)
end

-- independent implementation of search algorithm since we need to
-- keep two vectors in sync
local function interrogating_search(matches_filters_fn, data, filter, incremental)
    local vec, flags_vec = justice.interrogation_list, justice.interrogation_list_flag
    if not data.saved_original then
        data.saved_original = copy_to_lua_table(vec)
        data.saved_flags = copy_to_lua_table(flags_vec)
        data.saved_idx_map = {}
        for idx, unit in ipairs(data.saved_original) do
            data.saved_idx_map[unit.id] = idx  -- 1-based idx
        end
    else  -- sync flag changes to saved vector
        for idx, unit in ipairs(vec) do  -- 0-based idx
            data.saved_flags[data.saved_idx_map[unit.id]] = flags_vec[idx]
        end
    end

    if not incremental then
        vec:assign(data.saved_original)
        flags_vec:assign(data.saved_flags)
    end

    if matches_filters_fn or filter ~= '' then
        local search_tokens = filter:split()
        for idx = #vec-1,0,-1 do
            local search_key = get_unit_search_key(vec[idx])
            if (search_key and not utils.search_text(search_key, search_tokens)) or
                (matches_filters_fn and not matches_filters_fn(vec[idx], idx))
            then
                vec:erase(idx)
                flags_vec:erase(idx)
            end
        end
    end
end

local function convicting_search(matches_filters_fn, data, filter, incremental)
    general_search(justice.conviction_list, get_unit_search_key,
        nil, matches_filters_fn, data, filter, incremental)
end

local HANDLERS = {
    CITIZEN=make_cri_unitst_handlers(creatures.cri_unit.CITIZEN, get_sort),
    PET=make_cri_unitst_handlers(creatures.cri_unit.PET, get_sort),
    OTHER=make_cri_unitst_handlers(creatures.cri_unit.OTHER, get_sort),
    DECEASED=make_cri_unitst_handlers(creatures.cri_unit.DECEASED, get_sort),
    PET_OT={search_fn=overall_training_search},
    PET_AT={search_fn=assign_trainer_search},
    JOBS=make_cri_unitst_handlers(tasks.cri_job),
    WORK_DETAILS={search_fn=work_details_search},
    INTERROGATING={search_fn=interrogating_search},
    CONVICTING={search_fn=convicting_search},
}
for idx,name in ipairs(df.artifacts_mode_type) do
    if idx < 0 then goto continue end
    HANDLERS[name] = {
        search_fn=curry(general_search, objects.list[idx],
            function(elem)
                return ('%s %s'):format(dfhack.TranslateName(elem.name), dfhack.TranslateName(elem.name, true))
            end, nil)
    }
    ::continue::
end

local function get_key()
    if info.current_mode == df.info_interface_mode_type.JUSTICE then
        if justice.interrogating then
            return 'INTERROGATING'
        elseif justice.convicting then
            return 'CONVICTING'
        end
    elseif info.current_mode == df.info_interface_mode_type.CREATURES then
        if creatures.current_mode == df.unit_list_mode_type.PET then
            if creatures.showing_overall_training then
                return 'PET_OT'
            elseif creatures.adding_trainer then
                return 'PET_AT'
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

local function check_context(self)
    local key = get_key()
    if state.prev_key ~= key then
        state.prev_key = key
        local prev_text = key and ensure_key(state, key).prev_text or ''
        self.subviews.search:setText(prev_text)
    end
end

local function do_search(matches_filters_fn, text, force_full_search)
    if not force_full_search and not next(state) and text == '' then return end
    -- the EditField state is guaranteed to be consistent with the current
    -- context since when clicking to switch tabs, onRenderBody is always called
    -- before this text_input callback, even if a key is pressed before the next
    -- graphical frame would otherwise be printed. if this ever becomes untrue,
    -- then we can add an on_char handler to the EditField that also calls
    -- check_context.
    local key = get_key()
    if not key then return end
    local prev_text = ensure_key(state, key).prev_text
    -- some screens reset their contents between context switches; regardless
    -- a switch back to the context should results in an incremental search
    local incremental = not force_full_search and prev_text and text:startswith(prev_text)
    HANDLERS[key].search_fn(matches_filters_fn, state[key], text, incremental)
    state[key].prev_text = text
end

local function on_update(self)
    if self.overlay_onupdate_max_freq_seconds == 0 and
        not dfhack.gui.matchFocusString('dwarfmode/Info', dfhack.gui.getDFViewscreen(true))
    then
        for k,v in pairs(state) do
            local cleanup_fn = safe_index(HANDLERS, k, 'cleanup_fn')
            if cleanup_fn then cleanup_fn(v) end
        end
        state = {}
        self.subviews.search:setText('')
        self.subviews.search:setFocus(false)
        self.overlay_onupdate_max_freq_seconds = 60
    end
end

local function on_input(self, clazz, keys)
    if keys._MOUSE_R and self.subviews.search.focus then
        self.subviews.search:setFocus(false)
        return true
    end
    return clazz.super.onInput(self, keys)
end

local function is_interrogate_or_convict()
    local key = get_key()
    return key == 'INTERROGATING' or key == 'CONVICTING'
end

-- ----------------------
-- InfoOverlay
--

InfoOverlay = defclass(InfoOverlay, overlay.OverlayWidget)
InfoOverlay.ATTRS{
    default_pos={x=64, y=8},
    default_enabled=true,
    viewscreens='dwarfmode/Info',
    hotspot=true,
    overlay_onupdate_max_freq_seconds=0,
    frame={w=40, h=4},
}

function InfoOverlay:init()
    self:addviews{
        widgets.BannerPanel{
            view_id='panel',
            frame={l=0, t=0, r=0, h=1},
            visible=function() return get_key() and not is_interrogate_or_convict() end,
            subviews={
                widgets.EditField{
                    view_id='search',
                    frame={l=1, t=0, r=1},
                    label_text="Search: ",
                    key='CUSTOM_ALT_S',
                    on_change=function(text) do_search(DEFAULT_NIL, text) end,
                },
            },
        },
    }
end

function InfoOverlay:overlay_onupdate()
    on_update(self)
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
    if (frame.l == l and frame.t == t) then return ret end
    frame.l, frame.t = l, t
    return true
end

function InfoOverlay:onRenderBody(dc)
    if next(state) then
        check_context(self)
    end
    if self:updateFrames() then
        self:updateLayout()
    end
    if self.refresh_search then
        self.refresh_search = nil
        do_search(DEFAULT_NIL, self.subviews.search.text)
    end
    self.overlay_onupdate_max_freq_seconds = 0
    InfoOverlay.super.onRenderBody(self, dc)
end

function InfoOverlay:onInput(keys)
    if keys._MOUSE_L and get_key() == 'WORK_DETAILS' then
        self.refresh_search = true
    end
    return on_input(self, InfoOverlay, keys)
end

-- ----------------------
-- InterrogationOverlay
--

InterrogationOverlay = defclass(InterrogationOverlay, overlay.OverlayWidget)
InterrogationOverlay.ATTRS{
    default_pos={x=47, y=10},
    default_enabled=true,
    viewscreens='dwarfmode/Info/JUSTICE',
    frame={w=27, h=9},
    hotspot=true,
    overlay_onupdate_max_freq_seconds=0,
}

function InterrogationOverlay:overlay_onupdate()
    on_update(self)
end

function InterrogationOverlay:init()
    self:addviews{
        widgets.Panel{
            view_id='panel',
            frame={l=0, t=4, h=5, r=0},
            frame_background=gui.CLEAR_PEN,
            frame_style=gui.FRAME_MEDIUM,
            visible=is_interrogate_or_convict,
            subviews={
                widgets.EditField{
                    view_id='search',
                    frame={l=0, t=0, r=0},
                    label_text="Search: ",
                    key='CUSTOM_ALT_S',
                    on_change=function(text) do_search(self:callback('matches_filters'), text) end,
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
                    on_change=function()
                        do_search(self:callback('matches_filters'), self.subviews.search.text, true)
                    end,
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
                    on_change=function()
                        do_search(self:callback('matches_filters'), self.subviews.search.text, true)
                    end,
                },
            },
        },
    }
end

local function is_risky(unit)
    return false
end

function InterrogationOverlay:matches_filters(unit, idx)
    if justice.interrogating then
        local include_interviewed = self.subviews.include_interviewed:getOptionValue()
        if not include_interviewed and justice.interrogation_list_flag[idx] == 2 then
            return false
        end
    end
    local subset = self.subviews.subset:getOptionValue()
    if subset == 'all' then
        return true
    elseif dfhack.units.isDead(unit) or not dfhack.units.isActive(unit) then
        return subset == 'deceased'
    elseif dfhack.units.isVisiting(unit) then
        local risky = is_risky(unit)
        return (subset == 'risky' and risky) or (subset == 'visitors' and not risky)
    elseif dfhack.units.isAnimal(unit) then
        return subset == 'animals'
    elseif dfhack.units.isCitizen(unit) then
        return subset == 'citizens'
    elseif dfhack.units.isOwnGroup(unit) then
        return subset == 'residents'
    end
    return subset == 'others'
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

function InterrogationOverlay:onRenderBody(dc)
    if next(state) then
        check_context(self)
    else
        self.subviews.include_interviewed:setOption(true, false)
        self.subviews.subset:setOption('all')
    end
    self.overlay_onupdate_max_freq_seconds = 0
    InterrogationOverlay.super.onRenderBody(self, dc)
end

function InterrogationOverlay:onInput(keys)
    return on_input(self, InterrogationOverlay, keys)
end

return _ENV
