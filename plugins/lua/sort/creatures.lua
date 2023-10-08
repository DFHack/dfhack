local _ENV = mkmodule('plugins.sort.creatures')

local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')
local utils = require('utils')

local creatures = df.global.game.main_interface.info.creatures

-- these sort functions attempt to match the vanilla sort behavior, which is not
-- quite the same as the rest of DFHack. For example, in other DFHack sorts,
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

local function general_search(vec, get_search_key_fn, get_sort_fn, data, filter, incremental)
    if not data.saved_original then
        data.saved_original = copy_to_lua_table(vec)
    elseif not incremental then
        vec:assign(data.saved_original)
    end
    if filter ~= '' then
        local search_tokens = filter:split()
        for idx = #vec-1,0,-1 do
            local search_key = get_search_key_fn(vec[idx])
            if search_key and not utils.search_text(search_key, search_tokens) then
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

local function make_cri_unitst_handlers(vec)
    return {
        search_fn=curry(general_search, vec,
            function(elem)
                return ('%s %s'):format(elem.sort_name, elem.job_sort_name)
            end,
            get_sort),
        cleanup_fn=curry(cri_unitst_cleanup, vec),
    }
end

local function overall_training_search(data, filter, incremental)
    general_search(creatures.atk_index, function(elem)
            local raw = df.creature_raw.find(elem)
            if not raw then return end
            return raw.name[1]
        end, nil, data, filter, incremental)
end

local function assign_trainer_search(data, filter, incremental)
    general_search(creatures.trainer, function(elem)
            if not elem then return end
            return ('%s %s'):format(dfhack.TranslateName(elem.name), dfhack.units.getProfessionName(elem))
        end, nil, data, filter, incremental)
end

local HANDLERS = {
    CITIZEN=make_cri_unitst_handlers(creatures.cri_unit.CITIZEN),
    PET=make_cri_unitst_handlers(creatures.cri_unit.PET),
    OTHER=make_cri_unitst_handlers(creatures.cri_unit.OTHER),
    DECEASED=make_cri_unitst_handlers(creatures.cri_unit.DECEASED),
    PET_OT={search_fn=overall_training_search},
    PET_AT={search_fn=assign_trainer_search},
}

-- ----------------------
-- InfoOverlay
--

InfoOverlay = defclass(InfoOverlay, overlay.OverlayWidget)
InfoOverlay.ATTRS{
    default_pos={x=64, y=9},
    default_enabled=true,
    viewscreens={
        'dwarfmode/Info/CREATURES/CITIZEN',
        'dwarfmode/Info/CREATURES/PET',
        'dwarfmode/Info/CREATURES/OverallTraining',
        'dwarfmode/Info/CREATURES/AddingTrainer',
        'dwarfmode/Info/CREATURES/OTHER',
        'dwarfmode/Info/CREATURES/DECEASED',
    },
    hotspot=true,
    overlay_onupdate_max_freq_seconds=0,
    frame={w=40, h=3},
}

function InfoOverlay:init()
    self.state = {}

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
                    on_change=self:callback('text_input'),
                },
            },
        },
    }
end

local function cleanup(state)
    for k,v in pairs(state) do
        local cleanup_fn = safe_index(HANDLERS, k, 'cleanup_fn')
        if cleanup_fn then cleanup_fn(v) end
    end
end

function InfoOverlay:overlay_onupdate()
    if next(self.state) and
        not dfhack.gui.matchFocusString('dwarfmode/Info', dfhack.gui.getDFViewscreen(true))
    then
        cleanup(self.state)
        self.state = {}
        self.subviews.search:setText('')
        self.subviews.search:setFocus(false)
        self.overlay_onupdate_max_freq_seconds = 60
    end
end

local function are_tabs_in_two_rows()
    local pen = dfhack.screen.readTile(64, 6, false) -- tile is occupied iff tabs are in one row
    return pen.ch == 0
end

local function resize_overlay(self)
    local sw = dfhack.screen.getWindowSize()
    local overlay_width = math.min(40, sw-(self.frame_rect.x1 + 30))
    if overlay_width ~= self.frame.w then
        self.frame.w = overlay_width
        return true
    end
end

function InfoOverlay:updateFrames()
    local ret = resize_overlay(self)
    local two_rows = are_tabs_in_two_rows()
    if (self.two_rows == two_rows) then return ret end
    self.two_rows = two_rows
    self.subviews.panel.frame.t = two_rows and 2 or 0
    return true
end

local function get_key()
    if creatures.current_mode == df.unit_list_mode_type.PET then
        if creatures.showing_overall_training then
            return 'PET_OT'
        elseif creatures.adding_trainer then
            return 'PET_AT'
        end
    end
    return df.unit_list_mode_type[creatures.current_mode]
end

local function check_context(self)
    local key = get_key()
    if self.state.prev_key ~= key then
        self.state.prev_key = key
        local prev_text = ensure_key(self.state, key).prev_text
        self.subviews.search:setText(prev_text or '')
    end
end

function InfoOverlay:onRenderBody(dc)
    if next(self.state) then
        check_context(self)
    end
    if self:updateFrames() then
        self:updateLayout()
    end
    self.overlay_onupdate_max_freq_seconds = 0
    InfoOverlay.super.onRenderBody(self, dc)
end

function InfoOverlay:text_input(text)
    if not next(self.state) and text == '' then return end
    -- the EditField state is guaranteed to be consistent with the current
    -- context since when clicking to switch tabs, onRenderBody is always called
    -- before this text_input callback, even if a key is pressed before the next
    -- graphical frame would otherwise be printed. if this ever becomes untrue,
    -- then we can add an on_char handler to the EditField that also calls
    -- check_context.
    local key = get_key()
    local prev_text = ensure_key(self.state, key).prev_text
    -- some screens reset their contents between context switches; regardless
    -- a switch back to the context should results in an incremental search
    local incremental = prev_text and text:startswith(prev_text)
    HANDLERS[key].search_fn(self.state[key], text, incremental)
    self.state[key].prev_text = text
end

function InfoOverlay:onInput(keys)
    if keys._MOUSE_R and self.subviews.search.focus then
        self.subviews.search:setFocus(false)
        return true
    end
    return InfoOverlay.super.onInput(self, keys)
end

return _ENV
