local _ENV = mkmodule('plugins.stockpiles')

local argparse = require('argparse')
local gui = require('gui')
local logistics = require('plugins.logistics')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local STOCKPILES_DIR = 'dfhack-config/stockpiles'
local STOCKPILES_LIBRARY_DIR = 'hack/data/stockpiles'

local BAD_FILENAME_REGEX = '[^%w._]'

--------------------
-- plugin logic
--------------------

local function get_sp_name(name, num)
    if #name > 0 then return name end
    return ('Stockpile %d'):format(num)
end

local STATUS_FMT = '%6s %s'
local function print_status()
    local sps = df.global.world.buildings.other.STOCKPILE
    print(('Current stockpiles: %d'):format(#sps))
    if #sps > 0 then
        print()
        print(STATUS_FMT:format('ID', 'Name'))
        print(STATUS_FMT:format('------', '----------'))
    end
    for _, sp in ipairs(sps) do
        print(STATUS_FMT:format(sp.id, get_sp_name(sp.name, sp.stockpile_number)))
    end
end

local function list_dir(path, prefix, filters)
    local paths = dfhack.filesystem.listdir_recursive(path, 0, false)
    if not paths then
        dfhack.printerr(('Cannot find stockpile settings directory: "%s"'):format(path))
        return
    end
    local normalized_filters = {}
    for _, filter in ipairs(filters or {}) do table.insert(normalized_filters, filter:lower()) end
    for _, v in ipairs(paths) do
        local normalized_path = prefix .. v.path:lower()
        if v.isdir or not normalized_path:endswith('.dfstock') then goto continue end
        normalized_path = normalized_path:sub(1, -9)
        if #normalized_filters > 0 then
            local matched = false
            for _, filter in ipairs(normalized_filters) do
                if normalized_path:find(filter, 1, true) then
                    matched = true
                    break
                end
            end
            if not matched then goto continue end
        end
        print(('%s%s'):format(prefix, v.path:sub(1, -9)))
        ::continue::
    end
end

local function list_settings_files(filters)
    list_dir(STOCKPILES_DIR, '', filters)
    list_dir(STOCKPILES_LIBRARY_DIR, 'library/', filters)
end

local function assert_safe_name(name)
    if not name or #name == 0 then qerror('name missing or empty') end
    if name:find(BAD_FILENAME_REGEX) then
        qerror('name can only contain numbers, letters, periods, and underscores')
    end
end

local function get_sp_id(opts)
    if opts.id then return opts.id end
    local sp = dfhack.gui.getSelectedStockpile(true)
    if not sp then
        qerror('Please select a stockpile or specify the stockpile ID')
    end
    return sp.id
end

local included_elements = {containers=1, general=2, categories=4, types=8}

function export_settings(name, opts)
    opts = opts or {}

    assert_safe_name(name)
    local fname = STOCKPILES_DIR .. '/' .. name

    local includedElements = 0
    for _, inc in ipairs(opts.includes or {}) do
        includedElements = includedElements | included_elements[inc]
    end

    if includedElements == 0 then
        for _, v in pairs(included_elements) do includedElements = includedElements | v end
    end

    if opts.route_id then
        stockpiles_route_export(fname, opts.route_id, opts.stop_id, includedElements)
    else
        stockpiles_export(fname, get_sp_id(opts), includedElements)
    end
end

local function normalize_name(name)
    local is_library = false
    if name:startswith('library/') then
        name = name:sub(9)
        is_library = true
    end
    assert_safe_name(name)
    if not is_library and dfhack.filesystem.exists(STOCKPILES_DIR .. '/' .. name .. '.dfstock') then
        return STOCKPILES_DIR .. '/' .. name
    end
    return STOCKPILES_LIBRARY_DIR .. '/' .. name
end

function import_settings(name, opts)
    local fname = normalize_name(name)
    opts = opts or {}
    local mode = opts.mode or 'set'
    local filters = table.concat(opts.filters or {}, ',')
    if opts.route_id then
        stockpiles_route_import(fname, opts.route_id, opts.stop_id, mode, filters)
    else
        stockpiles_import(fname, get_sp_id(opts), mode, filters)
    end
end

local function parse_include(arg)
    local includes = argparse.stringList(arg, 'include')
    for _, v in ipairs(includes) do
        if not included_elements[v] then qerror(('invalid included element: "%s"'):format(v)) end
    end
    return includes
end

local valid_modes = {set=true, enable=true, disable=true}

local function parse_mode(arg)
    if not valid_modes[arg] then qerror(('invalid mode: "%s"'):format(arg)) end
    return arg
end

local function parse_stockpile_opt(id_or_name)
    if tonumber(id_or_name) then
        return argparse.nonnegativeInt(id_or_name, 'stockpile')
    end
    for _, sp in ipairs(df.global.world.buildings.other.STOCKPILE) do
        if sp.name == id_or_name then
            return sp.id
        end
    end
    qerror('could not find stockpile with name: ' .. tostring(id_or_name))
end

local function get_route(id_or_name)
    local id = tonumber(id_or_name)
    if id then
        return df.hauling_route.find(id)
    end
    for _, route in ipairs(df.global.plotinfo.hauling.routes) do
        if route.name == id_or_name then
            return route
        end
    end
    qerror('could not find route with name: ' .. tostring(id_or_name))
end

local function get_stop_id(route, id_or_name)
    local id = tonumber(id_or_name)
    if tonumber(id_or_name) then
        return argparse.nonnegativeInt(id_or_name, 'stop id')
    end
    if not id_or_name and #route.stops > 0 then
        return route.stops[0].id
    end
    for _, stop in ipairs(route.stops) do
        if stop.name == id_or_name then
            return stop.id
        end
    end
    qerror('could not find stop with name: ' .. tostring(id_or_name))
end

local function parse_route_opt(arg)
    local ids = argparse.stringList(arg, 'route') or {}
    local route = get_route(ids[1])
    return route.id, get_stop_id(route, ids[2])
end

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    opts.includes = {}
    opts.mode = 'set'
    opts.filters = {}

    return argparse.processArgsGetopt(args, {
        {'h', 'help', handler=function() opts.help = true end},
        {'m', 'mode', hasArg=true,
         handler=function(arg) opts.mode = parse_mode(arg) end},
        {'f', 'filter', hasArg=true,
         handler=function(arg) opts.filters = argparse.stringList(arg) end},
        {'i', 'include', hasArg=true,
         handler=function(arg) opts.includes = parse_include(arg) end},
        {'s', 'stockpile', hasArg=true,
         handler=function(arg) opts.id = parse_stockpile_opt(arg) end},
        {'r', 'route', hasArg=true,
         handler=function(arg) opts.route_id, opts.stop_id = parse_route_opt(arg) end},
    })
end

function parse_commandline(args)
    local opts = {}
    local positionals = process_args(opts, args)

    if opts.help or not positionals then return false end

    local command = table.remove(positionals, 1)
    if not command or command == 'status' then
        print_status()
    elseif command == 'list' then
        list_settings_files(positionals)
    elseif command == 'export' then
        export_settings(positionals[1], opts)
    elseif command == 'import' then
        import_settings(positionals[1], opts)
    else
        return false
    end

    return true
end

--------------------
-- dialogs
--------------------

StockpilesExport = defclass(StockpilesExport, widgets.Window)
StockpilesExport.ATTRS{frame_title='Export stockpile settings', frame={w=33, h=15}, resizable=true}

function StockpilesExport:init()
    self:addviews{
        widgets.EditField{
            view_id='edit',
            frame={t=0, l=0, r=0},
            label_text='name: ',
            on_char=function(ch)
                return not ch:match(BAD_FILENAME_REGEX)
            end,
        }, widgets.Label{frame={t=2, l=0}, text='Include which elements?'},
        widgets.ToggleHotkeyLabel{frame={t=4, l=0}, label='General settings', initial_option=false},
        widgets.ToggleHotkeyLabel{
            frame={t=5, l=0},
            label='Container settings',
            initial_option=false,
        }, widgets.ToggleHotkeyLabel{frame={t=6, l=0}, label='Categories', initial_option=true},
        widgets.ToggleHotkeyLabel{frame={t=7, l=0}, label='Subtypes', initial_option=true},
        widgets.HotkeyLabel{
            frame={t=10, l=0},
            label='export',
            key='SELECT',
            enabled=function()
                return #self.subviews.edit.text > 0
            end,
            on_activate=self:callback('on_submit'),
        },
    }
end

function StockpilesExport:on_submit(text)
    self:dismiss()
end

StockpilesExportScreen = defclass(StockpilesExportScreen, gui.ZScreenModal)
StockpilesExportScreen.ATTRS{focus_path='stockpiles/export'}

function StockpilesExportScreen:init()
    self:addviews{StockpilesExport{}}
end

function StockpilesExportScreen:onDismiss()
    export_view = nil
end

local function do_export()
    export_view = export_view and export_view:raise() or StockpilesExportScreen{}:show()
end

--------------------
-- ConfigModal
--------------------

ConfigModal = defclass(ConfigModal, gui.ZScreenModal)
ConfigModal.ATTRS{
    focus_path='stockpiles_config',
    on_close=DEFAULT_NIL,
}

function ConfigModal:init()
    local sp = dfhack.gui.getSelectedStockpile(true)
    local cur_setting = false
    if sp then
        local config = logistics.logistics_getStockpileConfigs(sp.stockpile_number)[1]
        cur_setting = config.melt_masterworks == 1
    end

    self:addviews{
        widgets.Window{
            frame={w=35, h=10},
            frame_title='Advanced logistics settings',
            subviews={
                widgets.ToggleHotkeyLabel{
                    view_id='melt_masterworks',
                    frame={l=0, t=0},
                    key='CUSTOM_M',
                    label='Melt masterworks',
                    initial_option=cur_setting,
                },
            },
        },
    }
end

function ConfigModal:onDismiss()
    self.on_close{melt_masterworks=self.subviews.melt_masterworks:getOptionValue()}
end

--------------------
-- MinimizeButton
--------------------

MinimizeButton = defclass(MinimizeButton, widgets.Widget)
MinimizeButton.ATTRS{
    label_unminimized='minimize',
    label_minimized='restore',
    label_pos='left',
    symbol_minimize=string.char(31),
    symbol_restore=string.char(30),
    get_minimized_fn=DEFAULT_NIL,
    on_click=DEFAULT_NIL,
}

function MinimizeButton:init()
    self.hovered = false

    ensure_key(self, 'frame').h = 1

    local is_hovered = function()
        return self.hovered
    end
    local is_not_hovered = function()
        return not self.hovered
    end
    local get_action_symbol = function()
        return self.get_minimized_fn() and self.symbol_minimize or self.symbol_restore
    end
    local get_label = function()
        local label = self.get_minimized_fn() and self.label_minimized or self.label_unminimized
        return (' %s '):format(label)
    end

    local hovered_text = {'[', {text=get_action_symbol}, ']'}
    table.insert(hovered_text, self.label_pos == 'left' and 1 or #hovered_text + 1,
            {text=get_label, hpen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_WHITE}})

    self:addviews{
        widgets.Label{
            view_id='unhovered_label',
            frame={t=0, w=3, h=1},
            text={'[', {text=get_action_symbol}, ']'},
            text_pen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_LIGHTRED},
            text_hpen=dfhack.pen.parse{fg=COLOR_WHITE, bg=COLOR_RED},
            on_click=function()
                self.on_click()
                self:updateLayout()
            end,
            visible=is_not_hovered,
        }, widgets.Label{
            view_id='hovered_label',
            frame={t=0, h=1},
            text=hovered_text,
            auto_width=true,
            text_pen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_LIGHTRED},
            text_hpen=dfhack.pen.parse{fg=COLOR_WHITE, bg=COLOR_RED},
            on_click=function()
                self.on_click()
                self:updateLayout()
            end,
            visible=is_hovered,
        },
    }

    if self.label_pos == 'left' then
        self.subviews.unhovered_label.frame.r = 0
        self.subviews.hovered_label.frame.r = 0
    else
        self.subviews.unhovered_label.frame.l = 0
        self.subviews.hovered_label.frame.l = 0
    end
end

function MinimizeButton:onRenderFrame()
    local prev_hovered = self.hovered
    if self.hovered then
        self.hovered = self.subviews.hovered_label:getMousePos()
    else
        self.hovered = self.subviews.unhovered_label:getMousePos()
    end
    if self.hovered ~= prev_hovered then
        self:updateLayout()
        df.global.gps.force_full_display_count = 1
    end
end

--------------------
-- StockpilesOverlay
--------------------

StockpilesOverlay = defclass(StockpilesOverlay, overlay.OverlayWidget)
StockpilesOverlay.ATTRS{
    desc='Shows a panel when a stockpile is selected for stockpile automation.',
    default_pos={x=5, y=44},
    default_enabled=true,
    viewscreens='dwarfmode/Stockpile/Some', -- hmm, how do I close it when 'dwarfmode/CustomStockpile' is also up?
    frame={w=49, h=5},
}

function StockpilesOverlay:init()
    self.minimized = false

    local function is_expanded()
        return not self.minimized
    end

    local main_panel = widgets.Panel{
        view_id='main',
        frame_style=gui.MEDIUM_FRAME,
        frame_background=gui.CLEAR_PEN,
        visible=is_expanded,
        subviews={
            -- widgets.HotkeyLabel{
            --     frame={t=0, l=0},
            --     label='import settings',
            --     auto_width=true,
            --     key='CUSTOM_CTRL_I',
            --     on_activate=do_import,
            -- }, widgets.HotkeyLabel{
            --     frame={t=1, l=0},
            --     label='export settings',
            --     auto_width=true,
            --     key='CUSTOM_CTRL_E',
            --     on_activate=do_export,
            -- },
            widgets.Panel{
                frame={t=0, l=0},
                subviews={
                    widgets.Label{
                        frame={t=0, l=0, h=1},
                        auto_height=false,
                        text={'Designate items/animals brought to this stockpile for:'},
                        text_pen=COLOR_DARKGREY,
                    }, widgets.ToggleHotkeyLabel{
                        view_id='melt',
                        frame={t=1, l=0},
                        auto_width=true,
                        key='CUSTOM_CTRL_M',
                        option_gap=-1,
                        options={{label='Melting', value=true, pen=COLOR_RED},
                                {label='Melting', value=false}},
                        initial_option=false,
                        on_change=self:callback('toggleLogisticsFeature', 'melt'),
                    }, widgets.ToggleHotkeyLabel{
                        view_id='trade',
                        frame={t=1, l=16},
                        auto_width=true,
                        key='CUSTOM_CTRL_T',
                        option_gap=-1,
                        options={{label='Trading', value=true, pen=COLOR_YELLOW},
                                {label='Trading', value=false}},
                        initial_option=false,
                        on_change=self:callback('toggleLogisticsFeature', 'trade'),
                    }, widgets.ToggleHotkeyLabel{
                        view_id='dump',
                        frame={t=1, l=32},
                        auto_width=true,
                        key='CUSTOM_CTRL_U',
                        option_gap=-1,
                        options={{label='Dumping', value=true, pen=COLOR_LIGHTMAGENTA},
                                {label='Dumping', value=false}},
                        initial_option=false,
                        on_change=self:callback('toggleLogisticsFeature', 'dump'),
                    }, widgets.ToggleHotkeyLabel{
                        view_id='train',
                        frame={t=2, l=32},
                        auto_width=true,
                        key='CUSTOM_CTRL_A',
                        option_gap=-1,
                        options={{label='Training', value=true, pen=COLOR_LIGHTBLUE},
                                {label='Training', value=false}},
                        initial_option=false,
                        on_change=self:callback('toggleLogisticsFeature', 'train'),
                    }, widgets.CycleHotkeyLabel{
                        view_id='forbid',
                        frame={t=2, l=0, w=16},
                        key='CUSTOM_CTRL_F',
                        option_gap=-1,
                        options={{label='Forbid', value=0},
                                {label='Forbid', value=1, pen=COLOR_LIGHTRED},
                                {label='Claim', value=2, pen=COLOR_LIGHTBLUE}},
                        initial_option=0,
                        on_change=self:callback('toggleLogisticsFeature', 'forbid'),
                    },
                },
            },
        },
    }

    self:addviews{
        main_panel,
        MinimizeButton{
            frame={t=0, r=9},
            get_minimized_fn=function() return self.minimized end,
            on_click=self:callback('toggleMinimized'),
        },
        widgets.ConfigureButton{
            frame={t=0, r=5},
            on_click=function() ConfigModal{on_close=self:callback('on_custom_config')}:show() end,
            visible=is_expanded,
        },
        widgets.HelpButton{
            command='stockpiles',
            visible=is_expanded,
        },
    }
end

function StockpilesOverlay:overlay_onupdate()
    -- periodically pick up changes made from other interfaces
    self.cur_stockpile = nil
end

function StockpilesOverlay:onRenderFrame()
    local sp = dfhack.gui.getSelectedStockpile(true)
    if sp and self.cur_stockpile ~= sp then
        local config = logistics.logistics_getStockpileConfigs(sp.stockpile_number)[1]
        self.subviews.melt:setOption(config.melt == 1)
        self.subviews.trade:setOption(config.trade == 1)
        self.subviews.dump:setOption(config.dump == 1)
        self.subviews.forbid:setOption(config.forbid)
        self.subviews.train:setOption(config.train == 1)
        self.cur_stockpile = sp
    end
end

function StockpilesOverlay:toggleLogisticsFeature(feature)
    self.cur_stockpile = nil
    local sp = dfhack.gui.getSelectedStockpile(true)
    if not sp then return end
    local config = logistics.logistics_getStockpileConfigs(sp.stockpile_number)[1]
    -- logical xor
    logistics.logistics_setStockpileConfig(config.stockpile_number,
            (feature == 'melt') ~= (config.melt == 1), (feature == 'trade') ~= (config.trade == 1),
            (feature == 'dump') ~= (config.dump == 1), (feature == 'train') ~= (config.train == 1),
            self.subviews.forbid:getOptionValue(), config.melt_masterworks == 1)
end

function StockpilesOverlay:on_custom_config(custom)
    local sp = dfhack.gui.getSelectedStockpile(true)
    if not sp then return end
    local config = logistics.logistics_getStockpileConfigs(sp.stockpile_number)[1]
    logistics.logistics_setStockpileConfig(config.stockpile_number,
            config.melt == 1, config.trade == 1,
            config.dump == 1, config.train == 1,
            config.forbid, custom.melt_masterworks)
end

function StockpilesOverlay:toggleMinimized()
    self.minimized = not self.minimized
    self.cur_stockpile = nil
end

function StockpilesOverlay:onInput(keys)
    if keys.CUSTOM_ALT_M then
        self:toggleMinimized()
        return true
    end
    return StockpilesOverlay.super.onInput(self, keys)
end

OVERLAY_WIDGETS = {overlay=StockpilesOverlay}

return _ENV
