config.mode = 'fortress'
config.target = 'sort'

local info = require('plugins.sort.info')
local sort = require('plugins.sort')

local overlay_widgets = package.loaded['plugins.overlay']

local mocked_tabs

local function make_list()
    return {sort_flags={NEEDS_RESORTED=false}}
end

local function make_tab(visible)
    local lists = {
        Interrogate=make_list(),
        Convict=make_list(),
    }
    return {
        flag={VISIBILITY_VISIBLE=visible},
        children_by_name={
            ['Right panel']={children_by_name=lists},
        },
    }, lists
end

local function make_widget_tree(cold_visible)
    local open_tab, open_lists = make_tab(not cold_visible)
    local cold_tab, cold_lists = make_tab(cold_visible)
    mocked_tabs = {
        children={open_tab, cold_tab},
        children_by_name={
            ['Open cases']=open_tab,
            ['Cold cases']=cold_tab,
        },
    }
    return open_lists, cold_lists
end

local function get_widget(parent, ...)
    local widget = parent
    for _,key in ipairs{...} do
        if type(widget) == 'table' then
            widget = widget.children_by_name[key]
        else
            -- real df container: only the top-level justice Tabs lookup is mocked
            widget = key == 'Tabs' and mocked_tabs or nil
        end
        if widget == nil then return nil end
    end
    return widget
end

local function get_widget_children(container)
    return type(container) == 'table' and container.children or {}
end

local function with_mocks(fn, extra_patches)
    local patches = {
        {dfhack.gui, 'getWidget', get_widget},
        {dfhack.gui, 'getWidgetChildren', get_widget_children},
        {info, 'interrogate_instance', info.interrogate_instance},
        {info, 'convict_instance', info.convict_instance},
    }
    for _,patch in ipairs(extra_patches or {}) do
        table.insert(patches, patch)
    end
    mock.patch(patches, fn)
end

function test.justice_filter_pokes_visible_cold_cases_list()
    local open_lists, cold_lists = make_widget_tree(true)
    with_mocks(function()
        info.InterrogationOverlay{}.subviews.subset.on_change()
    end)
    expect.false_(open_lists.Interrogate.sort_flags.NEEDS_RESORTED)
    expect.true_(cold_lists.Interrogate.sort_flags.NEEDS_RESORTED)
end

function test.justice_filter_pokes_visible_open_cases_list()
    local open_lists, cold_lists = make_widget_tree(false)
    with_mocks(function()
        info.InterrogationOverlay{}.subviews.subset.on_change()
    end)
    expect.true_(open_lists.Interrogate.sort_flags.NEEDS_RESORTED)
    expect.false_(cold_lists.Interrogate.sort_flags.NEEDS_RESORTED)
end

function test.justice_conviction_pokes_visible_cold_cases_list()
    local open_lists, cold_lists = make_widget_tree(true)
    with_mocks(function()
        info.ConvictionOverlay{}.subviews.subset.on_change()
    end)
    expect.false_(open_lists.Convict.sort_flags.NEEDS_RESORTED)
    expect.true_(cold_lists.Convict.sort_flags.NEEDS_RESORTED)
end

function test.justice_filter_installed_on_visible_list()
    local _, cold_lists = make_widget_tree(true)
    local passed_list
    local set_filter = mock.observe_func(function(list) passed_list = list end)
    with_mocks(function()
        info.InterrogationOverlay{}:render()
    end, {
        {sort, 'sort_set_justice_filter_fn', set_filter},
        {overlay_widgets.OverlayWidget, 'render', mock.func()},
    })
    expect.eq(1, set_filter.call_count)
    expect.eq(cold_lists.Interrogate, passed_list)
end
