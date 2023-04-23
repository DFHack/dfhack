local _ENV = mkmodule('plugins.stockpiles')

local argparse = require('argparse')
local gui = require('gui')
local logistics = require('plugins.logistics')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local STOCKPILES_DIR = 'dfhack-config/stockpiles';
local STOCKPILES_LIBRARY_DIR = 'hack/data/stockpiles';

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
    if name:find('[^%w._]') then
        qerror('name can only contain numbers, letters, periods, and underscores')
    end
end

local function get_sp_id(opts)
    if opts.id then return opts.id end
    local sp = dfhack.gui.getSelectedStockpile()
    if sp then return sp.id end
    return nil
end

local included_elements = {containers=1, general=2, categories=4, types=8, features=16}

function export_stockpile(name, opts)
    assert_safe_name(name)
    name = STOCKPILES_DIR .. '/' .. name

    local includedElements = 0
    for _, inc in ipairs(opts.includes) do
        includedElements = includedElements | included_elements[inc]
    end

    if includedElements == 0 then
        for _, v in pairs(included_elements) do includedElements = includedElements | v end
    end

    stockpiles_export(name, get_sp_id(opts), includedElements)
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

function import_stockpile(name, opts)
    name = normalize_name(name)
    stockpiles_import(name, get_sp_id(opts), opts.mode, table.concat(opts.filters or {}, ','))
end

function import_route(name, route_id, stop_id, mode, filters)
    name = normalize_name(name)
    stockpiles_route_import(name, route_id, stop_id, mode, table.concat(filters or {}, ','))
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

local function process_args(opts, args)
    if args[1] == 'help' then
        opts.help = true
        return
    end

    opts.includes = {}
    opts.mode = 'set'
    opts.filters = {}

    return argparse.processArgsGetopt(args, {
        {
            'h',
            'help',
            handler=function()
                opts.help = true
            end,
        }, {
            'm',
            'mode',
            hasArg=true,
            handler=function(arg)
                opts.mode = parse_mode(arg)
            end,
        }, {
            'f',
            'filter',
            hasArg=true,
            handler=function(arg)
                opts.filters = argparse.stringList(arg)
            end,
        }, {
            'i',
            'include',
            hasArg=true,
            handler=function(arg)
                opts.includes = parse_include(arg)
            end,
        }, {
            's',
            'stockpile',
            hasArg=true,
            handler=function(arg)
                opts.id = argparse.nonnegativeInt(arg, 'stockpile')
            end,
        },
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
        export_stockpile(positionals[1], opts)
    elseif command == 'import' then
        import_stockpile(positionals[1], opts)
    else
        return false
    end

    return true
end

--------------------
-- dialogs
--------------------

StockpilesExport = defclass(StockpilesExport, widgets.Window)
StockpilesExport.ATTRS{
    frame_title='Export stockpile settings',
    frame={w=50, h=45},
    resizable=true,
    resize_min={w=50, h=20},
}

function StockpilesExport:init()
    self:addviews{
        widgets.EditField{}, widgets.Label{frame={}, text='Include which elements?'},
        widgets.ToggleHotkeyLabel{},
    }
end

function StockpilesExport:onInput(keys)
    -- if required
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
-- StockpilesOverlay
--------------------

MinimizeButton = defclass(MinimizeButton, widgets.Panel)
MinimizeButton.ATTRS{
    label_pos='left',
    get_minimized_fn=DEFAULT_NIL,
    on_click=DEFAULT_NIL,
}

function MinimizeButton:init()
    local show_label, hide_label = 'show', 'hide'
    local label_width = math.max(#show_label, #hide_label)

    self:addviews{
        widgets.Label{
            frame={t=0, l=0, w=1, h=1},
            text='['..string.char(30)..']',
            text_pen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_LIGHTRED},
            text_hpen=dfhack.pen.parse{fg=COLOR_WHITE, bg=COLOR_RED},
            visible=self.get_minimized_fn,
        }, widgets.Label{
            frame={t=0, l=1, w=1, h=1},
            text={'[',
                {
                    text=function()
                        return self.get_minimized_fn() and string.char(31) or string.char(30)
                    end,
                },']',
            },
            text_pen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_GREY},
            text_hpen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_WHITE},
            on_click=self:callback('toggleMinimized'),
        }, widgets.Label{
            frame={t=0, r=0, w=1, h=1},
            text=']',
            text_pen=COLOR_RED,
            visible=function()
                return self.minimized
            end,
        },
    }
end

StockpilesOverlay = defclass(StockpilesOverlay, overlay.OverlayWidget)
StockpilesOverlay.ATTRS{
    default_pos={x=53, y=-6},
    default_enabled=true,
    viewscreens='dwarfmode/Some/Stockpile',
    frame={w=27, h=11},
}

function StockpilesOverlay:init()
    self.minimized = false
    self.hovered = false

    local main_panel = widgets.ResizingPanel{
        view_id='main',
        frame={t=0, l=0, r=0},
        frame_style=gui.MEDIUM_FRAME,
        frame_background=gui.CLEAR_PEN,
        autoarrange_subviews=true,
        visible=function()
            return not self.minimized
        end,
        subviews={
            widgets.HotkeyLabel{
                frame={t=0, l=0},
                label='apply settings',
                key='CUSTOM_CTRL_I',
                on_activate=do_import,
            }, widgets.HotkeyLabel{
                frame={t=1, l=0},
                label='export settings',
                key='CUSTOM_CTRL_E',
                on_activate=do_export,
            }, widgets.Panel{
                frame={t=2, h=4},
                subviews={
                    widgets.Label{
                        frame={t=1, l=0, h=2},
                        auto_height=false,
                        text={'Designate items brought', NEWLINE, 'to this stockpile for:'},
                    },
                },
                visible=function()
                    return self.hovered or self.subviews.melt:getOptionValue() or
                                   self.subviews.trade:getOptionValue() or
                                   self.subviews.dump:getOptionValue()
                end,
            }, widgets.ToggleHotkeyLabel{
                view_id='melt',
                frame={t=6, l=2},
                label='melting',
                key='CUSTOM_CTRL_M',
                on_change=self:callback('toggleLogisticsFeature', 'melt'),
                visible=function()
                    return self.hovered or self.subviews.melt:getOptionValue()
                end,
            }, widgets.ToggleHotkeyLabel{
                view_id='trade',
                frame={t=7, l=2},
                label='trading',
                key='CUSTOM_CTRL_T',
                on_change=self:callback('toggleLogisticsFeature', 'trade'),
                visible=function()
                    return self.hovered or self.subviews.trade:getOptionValue()
                end,
            }, widgets.ToggleHotkeyLabel{
                view_id='dump',
                frame={t=8, l=2},
                label='dumping',
                key='CUSTOM_CTRL_D',
                on_change=self:callback('toggleLogisticsFeature', 'dump'),
                visible=function()
                    return self.hovered or self.subviews.dump:getOptionValue()
                end,
            },
        },
    }

    local minimized_panel = widgets.Panel{
        frame={t=0, r=0, w=3, h=1},
        subviews={
            widgets.Label{
                frame={t=0, l=0, w=1, h=1},
                text='[',
                text_pen=COLOR_RED,
                visible=function()
                    return self.minimized
                end,
            }, widgets.Label{
                frame={t=0, l=1, w=1, h=1},
                text={
                    {
                        text=function()
                            return self.minimized and string.char(31) or string.char(30)
                        end,
                    },
                },
                text_pen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_GREY},
                text_hpen=dfhack.pen.parse{fg=COLOR_BLACK, bg=COLOR_WHITE},
                on_click=self:callback('toggleMinimized'),
            }, widgets.Label{
                frame={t=0, r=0, w=1, h=1},
                text=']',
                text_pen=COLOR_RED,
                visible=function()
                    return self.minimized
                end,
            },
        },
    }

    self:addviews{main_panel, minimized_panel}
end

function StockpilesOverlay:overlay_onupdate()
    -- periodically pick up changes made from other interfaces
    self.cur_stockpile = nil
end

function StockpilesOverlay:onRenderFrame()
    local sp = dfhack.gui.getSelectedStockpile()
    local sp_updated = false
    if self.cur_stockpile ~= sp then
        local config = logistics.logistics_getStockpileConfigs(sp.stockpile_number)[1]
        self.subviews.melt:setOption(config.melt == 1)
        self.subviews.trade:setOption(config.trade == 1)
        self.subviews.dump:setOption(config.dump == 1)
        self.cur_stockpile = sp
        sp_updated = true
    end
    local prev_hovered = self.hovered
    self.hovered = not not (self.hovered and self:getMouseFramePos() or
                           self.subviews.main:getMousePos())
    if self.hovered ~= prev_hovered or sp_updated then
        self:updateLayout()
        df.global.gps.force_full_display_count = 1
    end
end

function StockpilesOverlay:toggleLogisticsFeature(feature)
    local sp = dfhack.gui.getSelectedStockpile()
    local config = logistics.logistics_getStockpileConfigs(sp.stockpile_number)[1]
    -- logical xor
    logistics.logistics_setStockpileConfig(config.stockpile_number,
            (feature == 'melt') ~= (config.melt == 1), (feature == 'trade') ~= (config.trade == 1),
            (feature == 'dump') ~= (config.dump == 1))
    self.cur_stockpile = nil
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
