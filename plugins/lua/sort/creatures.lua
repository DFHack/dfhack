local _ENV = mkmodule('plugins.sort.creatures')

local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local creatures = df.global.game.main_interface.info.creatures

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

function InfoOverlay:overlay_onupdate(scr)
    if next(self.state) and not dfhack.gui.matchFocusString('dwarfmode/Info', scr) then
        -- TODO: add dynamically allocated elements that were not visible at the time of
        -- closure back to the list so they can be properly disposed of
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
    if text == prev_text then return end
    if prev_text and text:startswith(prev_text) then
        -- TODO: search
        print('performing incremental search; text:', text, 'key:', key)
    else
        -- TODO: save list if not already saved
        -- TODO: else restore list from saved list
        -- TODO: if text ~= '' then search
        -- TODO: sort according to vanilla sort widget state
        print('performing full search; text:', text, 'key:', key)
    end
    -- TODO: save visible list
    self.state[key].prev_text = text
end

return _ENV
