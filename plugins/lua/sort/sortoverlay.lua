local _ENV = mkmodule('plugins.sort.sortoverlay')

local overlay = require('plugins.overlay')
local utils = require('utils')

function get_unit_search_key(unit)
    return ('%s %s %s'):format(
        dfhack.units.getReadableName(unit),
        dfhack.units.getProfessionName(unit),
        dfhack.TranslateName(unit.name, true, true))  -- get English last name
end

local function copy_to_lua_table(vec)
    local tab = {}
    for k,v in ipairs(vec) do
        tab[k+1] = v
    end
    return tab
end

-- ----------------------
-- SortOverlay
--

SortOverlay = defclass(SortOverlay, overlay.OverlayWidget)
SortOverlay.ATTRS{
    default_enabled=true,
    hotspot=true,
    overlay_onupdate_max_freq_seconds=0,
    -- subclasses expected to provide default_pos, viewscreens (single string), and frame
    -- viewscreens should be the top-level scope within which the search widget state is maintained
    -- once the player leaves that scope, widget state will be reset
}

function SortOverlay:init()
    self.state = {}
    self.handlers = {}
    -- subclasses expected to provide an EditField widget with view_id='search'
end

function SortOverlay:register_handler(key, vec, search_fn, cleanup_fn)
    self.handlers[key] = {
        vec=vec,
        search_fn=search_fn,
        cleanup_fn=cleanup_fn
    }
end

local function do_cleanup(handlers, key, data)
    if not key or not data then return end
    local cleanup_fn = safe_index(handlers, key, 'cleanup_fn')
    if cleanup_fn then
        cleanup_fn(data)
    end
    data.saved_original = nil
end

-- handles reset and clean up when the player exits the handled scope
function SortOverlay:overlay_onupdate()
    if self.overlay_onupdate_max_freq_seconds == 0 and
        not dfhack.gui.matchFocusString(self.viewscreens, dfhack.gui.getDFViewscreen(true))
    then
        for key,data in pairs(self.state) do
            if type(data) == 'table' then
                do_cleanup(self.handlers, key, data)
            end
        end
        self:reset()
        self.overlay_onupdate_max_freq_seconds = 300
    end
end

function SortOverlay:reset()
    self.state = {}
    self.subviews.search:setText('')
    self.subviews.search:setFocus(false)
end

-- returns the current context key for dereferencing the handler
-- subclasses must override
function SortOverlay:get_key()
    return nil
end

-- handles saving/restoring search strings when the player moves between different contexts
function SortOverlay:onRenderBody(dc)
    if next(self.state) then
        local key, group = self:get_key()
        if self.state.cur_group ~= group then
            self.state.cur_group = group
            do_cleanup(self.handlers, self.state.cur_key, self.state[self.state.cur_key])
        end
        if self.state.cur_key ~= key then
            self.state.cur_key = key
            local prev_text = key and ensure_key(self.state, key).prev_text or ''
            self.subviews.search:setText(prev_text)
            self:do_search(self.subviews.search.text, true)
        end
    end
    self.overlay_onupdate_max_freq_seconds = 0
    SortOverlay.super.onRenderBody(self, dc)
end

local function is_mouse_key(keys)
    return keys._MOUSE_L
        or keys._MOUSE_R
        or keys._MOUSE_M
        or keys.CONTEXT_SCROLL_UP
        or keys.CONTEXT_SCROLL_DOWN
        or keys.CONTEXT_SCROLL_PAGEUP
        or keys.CONTEXT_SCROLL_PAGEDOWN
end

function SortOverlay:onInput(keys)
    if keys._MOUSE_R and self.subviews.search.focus and self:get_key() then
        self.subviews.search:setFocus(false)
        return true
    end
    return SortOverlay.super.onInput(self, keys) or
        (self.subviews.search.focus and not is_mouse_key(keys))
end

function SortOverlay:do_search(text, force_full_search)
    if not force_full_search and not next(self.state) and text == '' then return end
    -- the EditField state is guaranteed to be consistent with the current
    -- context since when clicking to switch tabs, onRenderBody is always called
    -- before this text_input callback, even if a key is pressed before the next
    -- graphical frame would otherwise be printed. if this ever becomes untrue,
    -- then we can add an on_char handler to the EditField that also checks for
    -- context transitions.
    local key = self:get_key()
    if not key then return end
    local prev_text = ensure_key(self.state, key).prev_text
    -- some screens reset their contents between context switches; regardless,
    -- a switch back to the context should results in an incremental search
    local incremental = not force_full_search and prev_text and text:startswith(prev_text)
    local handler = self.handlers[key]
    handler.search_fn(handler.vec, self.state[key], text, incremental)
    self.state[key].prev_text = text
end

local function filter_vec(fns, flags_vec, vec, text, erase_fn)
    if fns.matches_filters_fn or text ~= '' then
        local search_tokens = text:split()
        for idx = #vec-1,0,-1 do
            local flag = flags_vec and flags_vec[idx] or nil
            local search_key = fns.get_search_key_fn and fns.get_search_key_fn(vec[idx], flag) or nil
            if (search_key and not utils.search_text(search_key, search_tokens)) or
                (fns.matches_filters_fn and not fns.matches_filters_fn(vec[idx], flag))
            then
                erase_fn(idx)
            end
        end
    end
end

function single_vector_search(fns, vec, data, text, incremental)
    vec = utils.getval(vec)
    if not data.saved_original then
        data.saved_original = copy_to_lua_table(vec)
        data.saved_original_size = #vec
    elseif not incremental then
        vec:assign(data.saved_original)
        vec:resize(data.saved_original_size)
    end
    filter_vec(fns, nil, vec, text, function(idx) vec:erase(idx) end)
    data.saved_visible = copy_to_lua_table(vec)
    data.saved_visible_size = #vec
    if fns.get_sort_fn then
        table.sort(data.saved_visible, fns.get_sort_fn())
        vec:assign(data.saved_visible)
        vec:resize(data.saved_visible_size)
    end
end

-- doesn't support sorting since nothing that uses this needs it yet
function flags_vector_search(fns, flags_vec, vec, data, text, incremental)
    local get_elem_id_fn = fns.get_elem_id_fn or function(elem) return elem end
    flags_vec, vec = utils.getval(flags_vec), utils.getval(vec)
    if not data.saved_original then
        -- we save the sizes since trailing nils get lost in the lua -> vec assignment
        data.saved_original = copy_to_lua_table(vec)
        data.saved_original_size = #vec
        data.saved_flags = copy_to_lua_table(flags_vec)
        data.saved_flags_size = #flags_vec
        data.saved_idx_map = {}
        for idx,elem in ipairs(data.saved_original) do
            data.saved_idx_map[get_elem_id_fn(elem)] = idx  -- 1-based idx
        end
    else  -- sync flag changes to saved vector
        for idx,elem in ipairs(vec) do  -- 0-based idx
            data.saved_flags[data.saved_idx_map[get_elem_id_fn(elem)]] = flags_vec[idx]
        end
    end

    if not incremental then
        vec:assign(data.saved_original)
        vec:resize(data.saved_original_size)
        flags_vec:assign(data.saved_flags)
        flags_vec:resize(data.saved_flags_size)
    end

    filter_vec(fns, flags_vec, vec, text, function(idx)
        vec:erase(idx)
        flags_vec:erase(idx)
    end)
end

return _ENV
