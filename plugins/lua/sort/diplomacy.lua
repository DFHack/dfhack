local _ENV = mkmodule('plugins.sort.diplomacy')

local overlay = require('plugins.overlay')
local sortoverlay = require('plugins.sort.sortoverlay')
local widgets = require('gui.widgets')

local diplomacy = df.global.game.main_interface.diplomacy

-- ----------------------
-- DiplomacyOverlay
--

DiplomacyOverlay = defclass(DiplomacyOverlay, sortoverlay.SortOverlay)
DiplomacyOverlay.ATTRS{
    desc='Adds search and sort functionality to the elevate unit to barony screen.',
    default_pos={x=25, y=7},
    viewscreens='dwarfmode/Diplomacy',
    frame={w=57, h=1},
}

function DiplomacyOverlay:init()
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
                    text='',
                    on_change=function(text) self:do_search(text) end,
                },
            },
        },
    }

    self:register_handler('ELEVATE', diplomacy.land_holder_avail_hfid,
        curry(sortoverlay.single_vector_search,
            {
                get_search_key_fn=self:callback('get_search_key'),
                get_sort_fn=self:callback('get_sort'),
            }))
end

function DiplomacyOverlay:get_key()
    if diplomacy.selecting_land_holder_position then
        return 'ELEVATE'
    end
end

local function to_item_type_str(item_type)
    return string.lower(df.item_type[item_type]):gsub('_', ' ')
end

local function make_item_description(item_type, subtype)
    local itemdef = dfhack.items.getSubtypeDef(item_type, subtype)
    return itemdef and string.lower(itemdef.name) or to_item_type_str(item_type)
end

local function get_preferences(unit)
    if not unit then return {} end
    local preferences = {}
    for _, pref in ipairs(unit.status.current_soul.preferences) do
        if pref.type == df.unitpref_type.LikeItem and pref.active then
            table.insert(preferences, make_item_description(pref.item_type, pref.item_subtype))
        end
    end
    return preferences
end

local function get_unit(hfid)
    local hf = df.historical_figure.find(hfid)
    if not hf then return end
    return df.unit.find(hf.unit_id)
end

function DiplomacyOverlay:get_search_key(hfid)
    local unit = get_unit(hfid)
    if not unit then return end
    return ('%s %s'):format(sortoverlay.get_unit_search_key(unit),
        table.concat(get_preferences(unit), ' '))
end


function DiplomacyOverlay:get_sort()
    local _, sh = dfhack.screen.getWindowSize()
    local list_height = sh - 17
    local num_elems = list_height // 3
    diplomacy.scroll_position_land_holder_hf = math.max(0,
        math.min(diplomacy.scroll_position_land_holder_hf,
            #diplomacy.land_holder_avail_hfid - num_elems))
    return function(a, b)
        local unita = get_unit(a)
        local unitb = get_unit(b)
        return #get_preferences(unita) < #get_preferences(unitb)
    end
end

function DiplomacyOverlay:preUpdateLayout(parent_rect)
    local margin = (parent_rect.width - 114) // 3
    self.frame.w = 57 + margin
    self.subviews.panel.frame.l = margin
end

-- ----------------------
-- PreferenceOverlay
--

PreferenceOverlay = defclass(PreferenceOverlay, overlay.OverlayWidget)
PreferenceOverlay.ATTRS{
    desc='Adds information about unit preferences to the elevate unit to barony screen.',
    default_pos={x=-34, y=9},
    viewscreens='dwarfmode/Diplomacy/ElevateLandHolder',
    default_enabled=true,
    frame={w=29, h=1},
}

function PreferenceOverlay:init()
    self:addviews{
        widgets.Label{
            view_id='annotations',
            frame={t=0, l=0},
            text='',
            text_pen=COLOR_GRAY,
        }
    }
end

function PreferenceOverlay:preUpdateLayout(parent_rect)
    local list_height = parent_rect.height - 17
    self.frame.w = parent_rect.width - 85
    self.frame.h = list_height
    local margin = (parent_rect.width - 114) // 3
    self.subviews.annotations.frame.l = margin
end

function PreferenceOverlay:onRenderFrame(dc, rect)
    local margin = self.subviews.annotations.frame.l
    local num_elems = self.frame.h // 3
    local max_elem = math.min(#diplomacy.land_holder_avail_hfid-1,
        diplomacy.scroll_position_land_holder_hf+num_elems-1)

    local annotations = {}
    for idx=diplomacy.scroll_position_land_holder_hf,max_elem do
        table.insert(annotations, NEWLINE)
        table.insert(annotations, NEWLINE)
        local prefs = get_preferences(get_unit(diplomacy.land_holder_avail_hfid[idx]))
        table.insert(annotations, {text='[', pen=COLOR_RED})
        if #prefs == 0 then
            table.insert(annotations, 'no item preferences')
        else
            local pref_str = ('%d preference%s: %s'):format(#prefs,
                #prefs > 1 and 's' or '', table.concat(prefs, ', '))
            table.insert(annotations, pref_str:sub(1, self.frame.w-(margin+2)))
        end
        table.insert(annotations, {text=']', pen=COLOR_RED})
        table.insert(annotations, NEWLINE)
    end
    self.subviews.annotations:setText(annotations)
    self.subviews.annotations:updateLayout()

    PreferenceOverlay.super.onRenderFrame(self, dc, rect)
end

return _ENV
